/**
 * @file ltc_input_midi.cpp
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include <cstdint>
#include <utility>

#include "gd32f4xx.h"
#include "ltc_midi.h"
#include "gd32_uart.h"
#include "output/ltc_output.h"
#include "ltc_gpio_config.h"
#include "ltc_debug.h"

namespace {
constexpr uint32_t kBaudrateDefault = 31250;
constexpr auto kUart = MIDI_UARTx;

enum class State { kIdle = 0, kMtcQf = 1, kSysex = 3 };
State state{State::kIdle};

uint8_t mtc_assembly[8];

struct ltc::TimeCode timecode;

uint32_t sysex_count{0};
constexpr uint32_t kSysexBufferSize = 8;
uint8_t sysex[kSysexBufferSize];
} // namespace

namespace ltc {
namespace {

void Init() {
    gd32::UartBegin(kUart, kBaudrateDefault, gd32::kUartBits8, gd32::kUartParityNone, gd32::kUartStop1Bit);
}

void TransmitRaw(const uint8_t* data, uint32_t length) {
    gd32::UartTransmit(kUart, data, length);
}
} // namespace

// Input
namespace input::midi {
void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    output::Destination::Instance().SetType(::ltc::Type::kUnknown);
    Init();

    sysex_count = 0;
    state = State::kIdle;

    usart_interrupt_flag_clear(kUart, USART_INT_FLAG_RBNE);
    usart_interrupt_enable(kUart, USART_INT_RBNE);

    NVIC_EnableIRQ(MIDI_UARTx_IRQn);

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    NVIC_DisableIRQ(MIDI_UARTx_IRQn);

    usart_interrupt_disable(kUart, USART_INT_RBNE);

    LTC_INPUT_DEBUG_EXIT();
}
} // namespace input::midi

// Output
namespace output::midi {
void Start() {
    LTC_OUTPUT_DEBUG_ENTRY();

    Init();

    LTC_OUTPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_OUTPUT_DEBUG_ENTRY();
    LTC_OUTPUT_DEBUG_EXIT();
}

void OutputTimeCode(const struct ::midi::Timecode* timecode) {
    uint8_t data[10] = {0xF0, 0x7F, 0x7F, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7};

    data[5] = static_cast<uint8_t>((((timecode->type) & 0x03) << 5) | (timecode->hours & 0x1F));
    data[6] = timecode->minutes & 0x3F;
    data[7] = timecode->seconds & 0x3F;
    data[8] = timecode->frames & 0x1F;

    TransmitRaw(data, 10);
}

void OutputQf(uint8_t value) {
    uint8_t data[2];

    data[0] = 0xF1;
    data[1] = value;

    TransmitRaw(data, 2);
}
} // namespace output::midi
} // namespace ltc

void HanleMtcQf(uint8_t byte) {
    // TODO (AvV) direction
    const uint8_t kPiece = (byte >> 4) & 0x07; // High Nibble (0-7 indexing time segment)
    const uint8_t kValue = byte & 0x0F;        // Low Nibble (data payload)

    mtc_assembly[kPiece] = kValue;

    switch (kPiece) {
        case 0:
        case 1:
            timecode.frames = (mtc_assembly[1] << 4) | mtc_assembly[0];
            break;

        case 2:
        case 3:
            timecode.seconds = (mtc_assembly[3] << 4) | mtc_assembly[2];
            break;

        case 4:
        case 5:
            timecode.minutes = (mtc_assembly[5] << 4) | mtc_assembly[4];
            break;

        case 6:
        case 7:
            timecode.hours = ((mtc_assembly[7] & 0x01) << 4) | mtc_assembly[6];
            timecode.type = (mtc_assembly[7] >> 1) & 0x03;

            if (kPiece == 7) {
                // TODO (AvV) Handle full MTC
            }
            break;

        default:
            break;
    }
}

void HandleMtc() {
    if (sysex_count != kSysexBufferSize) [[unlikely]] {
        return;
    }

    if ((sysex[0] == 0x7F) && (sysex[1] == 0x7F) && (sysex[2] == 0x01)) {
        timecode.hours = sysex[4] & 0x1F;
        timecode.minutes = sysex[5];
        timecode.seconds = sysex[6];
        timecode.frames = sysex[7];
        timecode.type = static_cast<uint8_t>(sysex[4] >> 5);

        ltc::output::Destination::Instance().Distribute(&timecode);
    }
}

extern "C" void MIDI_UARTx_IRQHandler() {
    if (RESET != usart_interrupt_flag_get(kUart, USART_INT_FLAG_RBNE)) {
        const uint8_t kByte = gd32::UartGetRxData(kUart);

        // System Real-Time Override (0xF8 - 0xFF)
        if (kByte >= std::to_underlying(midi::Type::kClock)) {
            if (kByte == std::to_underlying(midi::Type::kSystemReset)) {
                state = State::kIdle;
            }
            // Handle
            return;
        }

        // Process Status Bytes (0x80 - 0xF7)
        if (kByte >= std::to_underlying(midi::Type::kNoteOff)) {
            if (kByte == std::to_underlying(midi::Type::kTimeCodeQuarterFrame)) {
                state = State::kMtcQf;
            } else if (kByte == std::to_underlying(midi::Type::kSystemExclusive)) {
                state = State::kSysex;
                sysex_count = 0;
            } else if ((kByte == 0xF7) && (state == State::kSysex)) {
                HandleMtc();
                state = State::kIdle;
            } else {
                // Ignore Voice Channel messages (0x80-0xEF) or unsupported System Common strings
                state = State::kIdle;
            }
            return;
        }

        // Process Stream Data Bytes (0x00 - 0x7F)

        switch (state) {
            case State::kIdle:
                break;
            case State::kMtcQf:
                HanleMtcQf(kByte);
                state = State::kIdle; // QF is a single data byte message; clear state
                break;

            case State::kSysex:
                if (sysex_count < kSysexBufferSize) {
                    sysex[sysex_count++] = kByte;
                }

                break;

            default:
                // Discard data payload bytes that belong to channel messages (0x80 - 0xEF)
                break;
        }
    }
}