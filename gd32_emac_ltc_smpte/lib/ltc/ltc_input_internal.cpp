/**
 * @file ltc_input_internal.cpp
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

#include "ltc.h"
#include "ltc_timecode.h"
#include "output/ltc_output.h"
#include "ltc_debug.h"

namespace ltc::global {
extern volatile bool timecode_available;
} // namespace ltc::global

namespace ltc::input::internal {
namespace {
ltc::TimeCode timecode_start{};
ltc::TimeCode timecode_stop{};
auto direction{ltc::output::Direction::kForward};
auto pitch{ltc::output::Pitch::kNormal};
uint32_t pitch_ticker{0};
uint32_t pitch_previous{0};
float pitch_control{0};
auto is_started{false};

bool PitchControl() {
    const auto kPitch = static_cast<uint32_t>(pitch_control * static_cast<float>(pitch_ticker)); // / 100;
    const auto kRemaining = (kPitch - pitch_previous);

    pitch_previous = kPitch;
    pitch_ticker++;

    return (kRemaining != 0);
}

void Forward() {
    if (pitch == ltc::output::Pitch::kNormal) {
        ltc::timecode::Increment();
    } else {
        if (pitch == ltc::output::Pitch::kFaster) {
            ltc::timecode::Increment();
            if (PitchControl()) {
                ltc::timecode::Increment();
            }
        } else {
            if (!PitchControl()) {
                ltc::timecode::Increment();
            }
        }
    }
}

void Backward() {
    if (pitch == ltc::output::Pitch::kNormal) {
        ltc::timecode::Decrement();
    } else {
        if (pitch == ltc::output::Pitch::kFaster) {
            ltc::timecode::Decrement();
            if (PitchControl()) {
                ltc::timecode::Decrement();
            }
        } else {
            if (!PitchControl()) {
                ltc::timecode::Decrement();
            }
        }
    }
}
} // namespace

void SetTimecodeStart(const ::ltc::TimeCode& timecode) {
    // TODO (AvV) Validation
    memcpy(&timecode_start, &timecode, sizeof(struct ltc::TimeCode));
}
void SetTimecodeStop(const ::ltc::TimeCode& timecode) {
    // TODO (AvV) Validation
    memcpy(&timecode_stop, &timecode, sizeof(struct ltc::TimeCode));
}

void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    memcpy(&global::timecode_running, &timecode_start, sizeof(struct ltc::TimeCode));

    const auto kFps = TimeCodeConst::kFps[static_cast<uint32_t>(global::timecode_running.type)];

    if (timecode_start.frames >= kFps) {
        timecode_start.frames = kFps - 1;
    }

    if (timecode_stop.frames >= kFps) {
        timecode_stop.frames = kFps - 1;
    }

    ltc::output::Destination::Instance().SetType(static_cast<::ltc::Type>(timecode_start.type));

    is_started = true;

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    is_started = false;

    LTC_INPUT_DEBUG_EXIT();
}

void Run() {
    if (!is_started) {
        return;
    }

    if (!ltc::global::timecode_available) {
        return;
    }

    ltc::global::timecode_available = false;

    ltc::output::Destination::Instance().Distribute(&ltc::global::timecode_running);

    if (direction == ltc::output::Direction::kForward) {
        Forward();
    } else {
        Backward();
    }
}
} // namespace ltc::input::internal