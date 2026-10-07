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

#include <cstdint>
#include <string_view>

#include "ltc.h"
#include "ltc_timecode.h"
#include "output/ltc_output.h"
#include "ltc_debug.h"
#include "input/ltc_input_internal.h"
#include "input/ltc_input.h"
#include "common/utils/utils_print.h"

namespace ltc::global {
extern volatile bool timecode_available;
} // namespace ltc::global

namespace ltc::input::internal {
namespace {
TimeCode timecode_start{};
TimeCode timecode_stop{};

auto direction{input::internal::Direction::kForward};
auto pitch_type{Pitch::kNormal};
uint32_t pitch_ticker{0};
uint32_t pitch_previous{0};
float pitch_control{0};
auto is_started{false};

constexpr std::string_view kErrorInput{"kInput != Input::kInternal"};

bool PitchControl() {
    const auto kPitch = static_cast<uint32_t>(pitch_control * static_cast<float>(pitch_ticker)); // / 100;
    const auto kRemaining = (kPitch - pitch_previous);

    pitch_previous = kPitch;
    pitch_ticker++;

    return (kRemaining != 0);
}

void Forward() {
    if (pitch_type == Pitch::kNormal) {
        timecode::Increment();
    } else {
        if (pitch_type == Pitch::kFaster) {
            timecode::Increment();
            if (PitchControl()) {
                timecode::Increment();
            }
        } else {
            if (!PitchControl()) {
                timecode::Increment();
            }
        }
    }
}

void Backward() {
    if (pitch_type == Pitch::kNormal) {
        timecode::Decrement();
    } else {
        if (pitch_type == Pitch::kFaster) {
            timecode::Decrement();
            if (PitchControl()) {
                timecode::Decrement();
            }
        } else {
            if (!PitchControl()) {
                timecode::Decrement();
            }
        }
    }
}

void Copy(::ltc::TimeCode& timecode_to, const ::ltc::TimeCode& timecode_from) {
    const auto kFps = TimeCodeConst::kFps[static_cast<uint32_t>(global::timecode_running.type)];

    if (timecode_from.frames >= kFps) {
        timecode_to.frames = kFps - 1;
    } else {
        timecode_to.frames = timecode_from.frames;
    }

    timecode_to.seconds = timecode_from.seconds;
    timecode_to.minutes = timecode_from.minutes;
    timecode_to.hours = timecode_from.hours;
}
} // namespace

void SetStart(const ::ltc::TimeCode& timecode) {
    Copy(timecode_start, timecode);

    if (!is_started) {
        StartInit();
    }
}
void SetStop(const ::ltc::TimeCode& timecode) {
    Copy(timecode_stop, timecode);
}

void SetRunning(const ::ltc::TimeCode& timecode) {
    const auto kInput = ::ltc::input::Source::Instance().Input();

    if (kInput != Input::kInternal) {
        ERROR(kErrorInput);
        return;
    }

    Copy(global::timecode_running, timecode);
}

void SetGoto(const ::ltc::TimeCode& timecode) {
    is_started = false;
    SetStart(timecode);
}

void SetDirection(input::internal::Direction dir) {
    direction = dir;
}

void SetForward(uint32_t seconds) {
    const auto kInput = ::ltc::input::Source::Instance().Input();

    if (kInput != Input::kInternal) {
        ERROR(kErrorInput);
        return;
    }

    const auto kSecondsCurrent = timecode::ToSeconds(global::timecode_running);
    const auto kSecondsNew = kSecondsCurrent + seconds;

    timecode::Set(kSecondsNew % timecode::kSecondsPerDay, 0, output::Destination::Instance().Type());

    if (!is_started) {
        output::Destination::Instance().Distribute(&global::timecode_running);
    }
}

void SetBackward(uint32_t seconds) {
    const auto kInput = ::ltc::input::Source::Instance().Input();

    if (kInput != Input::kInternal) {
        ERROR(kErrorInput);
        return;
    }

    const auto kSecondsCurrent = timecode::ToSeconds(global::timecode_running);

    if (kSecondsCurrent >= seconds) {
        timecode::Set(kSecondsCurrent - seconds, 0, output::Destination::Instance().Type());
    } else {
        timecode::Set(timecode::kSecondsPerDay - (seconds - kSecondsCurrent), 0, output::Destination::Instance().Type());
    }
    if (!is_started) {
        output::Destination::Instance().Distribute(&global::timecode_running);
    }
}

void SetPitch(float pitch) {
    if (pitch < 0) {
        pitch_type = Pitch::kSlower;
        pitch_control = -pitch;
    } else if (pitch == 0) {
        pitch_type = Pitch::kNormal;
        return;
    } else {
        pitch_type = Pitch::kFaster;
        pitch_control = pitch;
    }

    pitch_previous = 0;
    pitch_ticker = 1;
}

void StartInit() {
    LTC_INPUT_DEBUG_ENTRY();

    SetRunning(timecode_start);

    output::Destination::Instance().Distribute(&global::timecode_running);

    LTC_INPUT_DEBUG_EXIT();
}

void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    StartInit();
    is_started = true;

    LTC_INPUT_DEBUG_EXIT();
}

void Resume() {
    LTC_INPUT_DEBUG_ENTRY();

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

    if (!global::timecode_available) {
        return;
    }

    global::timecode_available = false;

    output::Destination::Instance().Distribute(&global::timecode_running);

    if (direction == Direction::kForward) {
        Forward();
    } else {
        Backward();
    }
}
} // namespace ltc::input::internal