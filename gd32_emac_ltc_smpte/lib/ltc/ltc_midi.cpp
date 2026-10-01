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
#include "usb/usbd/midi/usbd_midi.h"
#include "gd32_uart.h"
#include "ltc_timecode.h"
#include "output/ltc_output.h"
#include "ltc_gpio_config.h"
#include "ltc_debug.h"
#include "firmware/debug/debug_dump.h"

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

void Output(const struct ::midi::Timecode* timecode) {
    uint8_t data[10] = {0xF0, 0x7F, 0x7F, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7};

    data[5] = static_cast<uint8_t>((((timecode->type) & 0x03) << 5) | (timecode->hours & 0x1F));
    data[6] = timecode->minutes & 0x3F;
    data[7] = timecode->seconds & 0x3F;
    data[8] = timecode->frames & 0x1F;

    TransmitRaw(data, 10);
}

void Output(uint8_t value) {
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

// Input
namespace input::usbmidi {
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
} // namespace input::usbmidi

// Input
namespace output::usbmidi {
void Start() {
    LTC_OUTPUT_DEBUG_ENTRY();

    LTC_OUTPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_OUTPUT_DEBUG_ENTRY();

    LTC_OUTPUT_DEBUG_EXIT();
}
} // namespace output::usbmidi
} // namespace ltc

namespace ltc::input::mtc {
void HandleQuarterFrame(uint8_t value) {
    const uint8_t kPiece = (value >> 4) & 0x07;
    const uint8_t kValue = value & 0x0F;

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
                memcpy(&ltc::global::timecode_running, &timecode_input, sizeof(::midi::Timecode));

                ltc::output::Destination::Instance().Distribute(&timecode_input);
                Timer9Set(timecode_input.type);
            }
            break;

        default:
            break;
    }
}

void HandleFullFrame(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t frames) {
    timecode_input.frames = frames;
    timecode_input.seconds = seconds;
    timecode_input.minutes = minutes;
    timecode_input.hours = hours & 0x1F;
    timecode_input.type = static_cast<uint8_t>(hours >> 5);

    memcpy(&ltc::global::timecode_running, &timecode_input, sizeof(::midi::Timecode));

    ltc::output::Destination::Instance().Distribute(&timecode_input);
}

} // namespace ltc::input::mtc

void HandleDinMtc() {
    if (sysex_count != kSysexBufferSize) [[unlikely]] {
        return;
    }

    if ((sysex[0] == 0x7F) && (sysex[1] == 0x7F) && (sysex[2] == 0x01) && (sysex[3] == 0x01)) {
        ltc::input::mtc::HandleFullFrame(sysex[4], sysex[5], sysex[6], sysex[7]);
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
                HandleDinMtc();
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
                ltc::input::mtc::HandleQuarterFrame(kByte);
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
void HandleAppleMtc(const struct midi::Message* message) {
    const auto* const kData = message->system_exclusive;

    if ((kData[1] == 0x7F) && (kData[2] == 0x7F) && (kData[3] == 0x01) && (kData[4] == 0x01)) {
        ltc::input::mtc::HandleFullFrame(kData[5], kData[6], kData[7], kData[8]);
    }
}
} // namespace

void MidiMessage(const struct midi::Message* message) {
    switch (static_cast<midi::Type>(message->type)) {
        case midi::Type::kTimeCodeQuarterFrame:
            ltc::input::mtc::HandleQuarterFrame(message->data1);
            break;

        case midi::Type::kSystemExclusive:
            HandleAppleMtc(message);
            break;

        case midi::Type::kClock:
        default:
            break;
    }
}
} // namespace rtpmidi

namespace ltc::input::usbmidi {
namespace {

constexpr uint32_t kMtcSysExSize = 10;

uint8_t sysex[kMtcSysExSize];
uint32_t sysex_count;

void HandleUsbMtc(const ::usbmidi::MidiEvent& event) {
    const auto kCin = event.cable_cin & 0x0F;

    uint32_t length;

    switch (kCin) {
        case 0x04:
            length = 3;
            break;

        case 0x05:
            length = 1;
            break;

        case 0x06:
            length = 2;
            break;

        case 0x07:
            length = 3;
            break;

        default:
            return;
    }

    for (uint32_t i = 0; i < length; ++i) {
        const auto kByte = event.midi[i];

        if (kByte == 0xF0) {
            sysex_count = 0;
        }

        if (sysex_count < kMtcSysExSize) {
            sysex[sysex_count++] = kByte;
        }

        if (kByte == 0xF7) {
            if ((sysex_count == kMtcSysExSize) && (sysex[0] == 0xF0) && (sysex[1] == 0x7F) && (sysex[2] == 0x7F) && (sysex[3] == 0x01) && (sysex[4] == 0x01)) {
                ltc::input::mtc::HandleFullFrame(sysex[5], sysex[6], sysex[7], sysex[8]);
            }

            sysex_count = 0;
        }
    }
}
} // namespace

void Run() {
    ::usbmidi::MidiEvent event;

    while (::usbmidi::Message(event)) {
        const auto kCin = event.cable_cin & 0x0F;

        switch (kCin) {
            case 0x02: // two-byte System Common
                if (event.midi[0] == std::to_underlying(::midi::Type::kTimeCodeQuarterFrame)) {
                    ltc::input::mtc::HandleQuarterFrame(event.midi[1]);
                }
                break;

            case 0x04: // SysEx start/continue
            case 0x05: // SysEx end, 1 byte
            case 0x06: // SysEx end, 2 bytes
            case 0x07: // SysEx end, 3 bytes
                HandleUsbMtc(event);
                break;

            default:
                break;
        }
    }
}
} // namespace ltc::input::usbmidi