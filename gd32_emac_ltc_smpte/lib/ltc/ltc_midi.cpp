/**
 * @file ltc_input_midi.cpp
 * @brief Both DIN MIDI and Apple MIDI
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

#include "core/netif.h"
#include "gd32f4xx.h"
#include "midi.h"
#include "net/rtpmidi.h"
#include "gd32_uart.h"
#include "ltc_timecode.h"
#include "output/ltc_output.h"
#include "ltc_gpio_config.h"
#include "ltc_debug.h"

namespace ltc::global {
extern volatile bool timecode_available;
} // namespace ltc::global

namespace {
RtpMidi apple_midi;

constexpr uint32_t kBaudrateDefault = midi::defaults::kBaudrate;
constexpr auto kUart = MIDI_UARTx;

enum class State { kIdle = 0, kMtcQf = 1, kSysex = 3 };
State state{State::kIdle};

uint8_t mtc_assembly[8];

struct ltc::TimeCode timecode_input;

uint32_t sysex_count{0};
constexpr uint32_t kSysexBufferSize = 8;
uint8_t sysex[kSysexBufferSize];

void Timer9Config() {
    LTC_INPUT_DEBUG_ENTRY();

    rcu_periph_clock_enable(RCU_TIMER9);
    timer_deinit(TIMER9);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PRESCALER;
    timer_initpara.period = UINT32_MAX;
    timer_init(TIMER9, &timer_initpara);

    timer_counter_value_config(TIMER9, 0);
    timer_interrupt_flag_clear(TIMER9, UINT32_MAX);
    timer_interrupt_enable(TIMER9, TIMER_INT_UP);

    NVIC_SetPriority(TIMER0_UP_TIMER9_IRQn, 2);
    NVIC_EnableIRQ(TIMER0_UP_TIMER9_IRQn);

    LTC_INPUT_DEBUG_EXIT();
}

void Timer9Set(uint32_t type) {
    timer_interrupt_flag_clear(TIMER9, UINT32_MAX);
    TIMER_CTL0(TIMER9) &= ~TIMER_CTL0_CEN;

    if (type < static_cast<uint8_t>(::ltc::Type::kUnknown)) {
        TIMER_CAR(TIMER9) = TimeCodeConst::kTmrIntv[type];
        TIMER_CNT(TIMER9) = 0;
        TIMER_CTL0(TIMER9) |= TIMER_CTL0_CEN;

        timer_interrupt_enable(TIMER9, TIMER_INT_UP);
    }
}
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

    Timer9Config();

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    NVIC_DisableIRQ(TIMER0_UP_TIMER9_IRQn);

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

    data[0] = std::to_underlying(::midi::Type::kTimeCodeQuarterFrame);
    data[1] = value;

    TransmitRaw(data, 2);
}
} // namespace output::midi

// Input
namespace input::applemidi {
void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    output::Destination::Instance().SetType(::ltc::Type::kUnknown);

    Timer9Config();

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    NVIC_DisableIRQ(TIMER0_UP_TIMER9_IRQn);

    LTC_INPUT_DEBUG_EXIT();
}
} // namespace input::applemidi

// Output
namespace output::applemidi {
namespace {
auto is_started{false};
} // namespace
void Start() {
    LTC_OUTPUT_DEBUG_ENTRY();

    if (is_started) {
        LTC_OUTPUT_DEBUG_EXIT();
        return;
    }

    if (netif::IpAddr() != 0) {
        apple_midi.Start();
        apple_midi.Print();
        is_started = true;
    }

    LTC_OUTPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_OUTPUT_DEBUG_ENTRY();

    is_started = false;
    apple_midi.Stop();

    LTC_OUTPUT_DEBUG_EXIT();
}

void Restart() {
    LTC_OUTPUT_DEBUG_ENTRY();

    if (::ltc::output::Destination::Instance().IsEnabled(::ltc::Output::kApplemidi)) {
        Stop();
        Start();
    }

    LTC_OUTPUT_DEBUG_EXIT();
}
} // namespace output::applemidi
} // namespace ltc

void HanleMtcQf(uint8_t byte) {
    // TODO (AvV) direction
    const uint8_t kPiece = (byte >> 4) & 0x07; // High Nibble (0-7 indexing time segment)
    const uint8_t kValue = byte & 0x0F;        // Low Nibble (data payload)

    mtc_assembly[kPiece] = kValue;

    switch (kPiece) {
        case 0:
        case 1:
            timecode_input.frames = (mtc_assembly[1] << 4) | mtc_assembly[0];
            break;

        case 2:
        case 3:
            timecode_input.seconds = (mtc_assembly[3] << 4) | mtc_assembly[2];
            break;

        case 4:
        case 5:
            timecode_input.minutes = (mtc_assembly[5] << 4) | mtc_assembly[4];
            break;

        case 6:
        case 7:
            timecode_input.hours = ((mtc_assembly[7] & 0x01) << 4) | mtc_assembly[6];
            timecode_input.type = (mtc_assembly[7] >> 1) & 0x03;

            if (kPiece == 7) {
                memcpy(&ltc::global::timecode_running, &::timecode_input, sizeof(midi::Timecode));
                ltc::output::Destination::Instance().Distribute(&::timecode_input);
                Timer9Set(timecode_input.type);
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
        timecode_input.frames = sysex[7];
        timecode_input.seconds = sysex[6];
        timecode_input.minutes = sysex[5];
        timecode_input.hours = sysex[4] & 0x1F;
        timecode_input.type = static_cast<uint8_t>(sysex[4] >> 5);

        ltc::output::Destination::Instance().Distribute(&timecode_input);
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

extern "C" {
void TIMER0_UP_TIMER9_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(TIMER9);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
        ltc::timecode::Increment();
        ltc::output::Destination::Instance().Distribute(&ltc::global::timecode_running);
        timer_interrupt_disable(TIMER9, TIMER_INT_UP);
    }

    TIMER_INTF(TIMER9) = ~kIntFlag;
}
}

namespace rtpmidi {
namespace {
void HandleMtc(const struct midi::Message* message) {
    const auto* const kSystemExclusive = message->system_exclusive;

    if ((kSystemExclusive[1] == 0x7F) && (kSystemExclusive[2] == 0x7F) && (kSystemExclusive[3] == 0x01)) {
        timecode_input.frames = kSystemExclusive[8];
        timecode_input.seconds = kSystemExclusive[7];
        timecode_input.minutes = kSystemExclusive[6];
        timecode_input.hours = kSystemExclusive[5] & 0x1F;
        timecode_input.type = static_cast<uint8_t>(kSystemExclusive[5] >> 5);

        ltc::output::Destination::Instance().Distribute(&timecode_input);
    }
}
} // namespace

void MidiMessage(const struct midi::Message* message) {
    switch (static_cast<midi::Type>(message->type)) {
        case midi::Type::kTimeCodeQuarterFrame:
            break;

        case midi::Type::kSystemExclusive:
            HandleMtc(message);
            break;

        case midi::Type::kClock:
        default:
            break;
    }
}
} // namespace rtpmidi