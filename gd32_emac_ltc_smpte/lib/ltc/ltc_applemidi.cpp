/**
 * @file ltc_output_applemidi.cpp
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

#include "output/ltc_output.h"
#include "core/netif.h"
#include "net/rtpmidi.h"
#include "ltc_debug.h"

namespace ltc::global {
extern volatile bool timecode_available;
} // namespace ltc::global

namespace ltc {
namespace {
RtpMidi apple_midi;
}
// Input
namespace input::applemidi {
void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    output::Destination::Instance().SetType(::ltc::Type::kUnknown);

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();
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

namespace rtpmidi {
namespace {
struct ltc::TimeCode timecode;

void HandleMtc(const struct midi::Message* message) {
    const auto* const kSystemExclusive = message->system_exclusive;

    timecode.frames = kSystemExclusive[8];
    timecode.seconds = kSystemExclusive[7];
    timecode.minutes = kSystemExclusive[6];
    timecode.hours = kSystemExclusive[5] & 0x1F;
    timecode.type = static_cast<uint8_t>(kSystemExclusive[5] >> 5);

    ltc::output::Destination::Instance().Distribute(&timecode);

    ltc::global::timecode_available = false;
}
} // namespace

void MidiMessage(const struct midi::Message* message) {
    switch (static_cast<midi::Type>(message->type)) {
        case midi::Type::kTimeCodeQuarterFrame:
            break;
        case midi::Type::kSystemExclusive: {
            const auto* const kSystemExclusive = message->system_exclusive;
            if ((kSystemExclusive[1] == 0x7F) && (kSystemExclusive[2] == 0x7F) && (kSystemExclusive[3] == 0x01)) {
                HandleMtc(message);
            }
        } break;
        case midi::Type::kClock:
        default:
            break;
    }
}
} // namespace rtpmidi