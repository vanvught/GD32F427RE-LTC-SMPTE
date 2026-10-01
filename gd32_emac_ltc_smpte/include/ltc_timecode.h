/**
 * @file ltc_timecode.h
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

#ifndef LTC_TIMECODE_H_
#define LTC_TIMECODE_H_

#include <cstdint>

#include "ltc.h"
#include "output/ltc_output.h"
#include "timecodeconst.h"

namespace ltc::timecode {
constexpr int32_t kHardSyncThreshold = 2;

inline void Set(uint32_t seconds, uint32_t usec, Type type) {
    global::timecode_running.type = static_cast<uint8_t>(type);
    global::timecode_running.seconds = static_cast<uint8_t>(seconds % 60U);
    seconds /= 60U;
    global::timecode_running.minutes = static_cast<uint8_t>(seconds % 60U);
    seconds /= 60U;
    global::timecode_running.hours = static_cast<uint8_t>(seconds % 24U);
    const auto kFps = TimeCodeConst::kFps[global::timecode_running.type];
    global::timecode_running.frames = static_cast<uint8_t>((usec * kFps) / 1000000U);
}

inline void Increment() {
    global::timecode_running.frames++;

    // Drop-frame timecode handling BEFORE rolling to the next second
    if (Type() == Type::kDf) {
        // Skip frames 00 and 01 except every 10th minute
        if ((global::timecode_running.minutes % 10 != 0) && (global::timecode_running.seconds == 0) && (global::timecode_running.frames < 2)) {
            global::timecode_running.frames = 2;
        }
    }

    // Handle frame rollover
    if (global::timecode_running.frames >= TimeCodeConst::kFps[static_cast<uint32_t>(global::timecode_running.type)]) {
        global::timecode_running.frames = 0;

        if (++global::timecode_running.seconds >= 60) {
            global::timecode_running.seconds = 0;

            if (++global::timecode_running.minutes >= 60) {
                global::timecode_running.minutes = 0;

                if (++global::timecode_running.hours >= 24) {
                    global::timecode_running.hours = 0;
                }
            }
        }
    }
}

inline void Decrement() {
    const auto kLastFrame = static_cast<uint8_t>(TimeCodeConst::kFps[static_cast<uint32_t>(global::timecode_running.type)] - 1);

    if (global::timecode_running.frames > 0) {
        global::timecode_running.frames--;
    } else {
        global::timecode_running.frames = kLastFrame;
    }

    // Handle drop-frame logic after frames decrement
    if (Type() == Type::kDf) {
        if ((global::timecode_running.minutes % 10 != 0) && (global::timecode_running.seconds == 0) && (global::timecode_running.frames == kLastFrame)) {
            global::timecode_running.frames = 1; // Skip to frame 01
        }
    }

    // Handle seconds rollover
    if (global::timecode_running.frames == kLastFrame) {
        if (global::timecode_running.seconds > 0) {
            global::timecode_running.seconds--;
        } else {
            global::timecode_running.seconds = 59;
        }

        if (global::timecode_running.minutes > 0) {
            global::timecode_running.minutes--;
        } else {
            global::timecode_running.minutes = 59;
        }

        if (global::timecode_running.hours > 0) {
            global::timecode_running.hours--;
        } else {
            global::timecode_running.hours = 23;
        }
    }
}

inline uint32_t ToFrames(const TimeCode& timecode) {
    const auto kFps = TimeCodeConst::kFps[timecode.type];
    const auto kSeconds = (static_cast<uint32_t>(timecode.hours) * 60U * 60U) + (static_cast<uint32_t>(timecode.minutes) * 60U) + static_cast<uint32_t>(timecode.seconds);
    return (kSeconds * kFps) + timecode.frames;
}

inline int32_t Difference(uint32_t reference, uint32_t running, uint32_t fps) {
    const auto kFramesPerDay = 24U * 60U * 60U * fps;
    const auto kHalfDay = kFramesPerDay / 2U;

    auto difference = static_cast<int32_t>(reference) - static_cast<int32_t>(running);

    if (difference > static_cast<int32_t>(kHalfDay)) {
        difference -= static_cast<int32_t>(kFramesPerDay);
    } else if (difference < -static_cast<int32_t>(kHalfDay)) {
        difference += static_cast<int32_t>(kFramesPerDay);
    }

    return difference;
}

inline void Sync(const timeval& time_val) {
    const auto kType = ltc::global::timecode_running.type;
    const auto kFps = TimeCodeConst::kFps[kType];

    const auto kSeconds = static_cast<uint32_t>(time_val.tv_sec + ltc::output::Destination::Instance().UtcOffset());
    const auto kSecondsOfDay = kSeconds % (24U * 60U * 60U);

    const auto kFrames = static_cast<uint32_t>(time_val.tv_usec) * kFps / 1000000U;

    const auto kReference = (kSecondsOfDay * kFps) + kFrames;

    const auto kRunning = ToFrames(ltc::global::timecode_running);

    const auto kDifference = Difference(kReference, kRunning, kFps);

    if ((kDifference > kHardSyncThreshold) || (kDifference < -kHardSyncThreshold)) {
        Set(kSeconds, static_cast<uint32_t>(time_val.tv_usec), static_cast<ltc::Type>(kType));
    }

    printf("reference=%u running=%u difference=%d, TIMER_CNT(TIMER3)=%u, TIMER_CNT(TIMER0)=%u, TIMER_CNT(TIMER10)=%u\n", static_cast<unsigned>(kReference), static_cast<unsigned>(kRunning), static_cast<unsigned>(kDifference),
           static_cast<unsigned>(TIMER_CNT(TIMER3)), static_cast<unsigned>(TIMER_CNT(TIMER0)), static_cast<unsigned>(TIMER_CNT(TIMER10)));
}

inline void LocalTime(uint32_t& seconds, uint32_t& microseconds) {
    struct timeval time_val;
    gettimeofday(&time_val, nullptr);

    seconds = static_cast<uint32_t>(time_val.tv_sec + output::Destination::Instance().UtcOffset());
    microseconds = static_cast<uint32_t>(time_val.tv_usec);
}
} // namespace ltc::timecode

#endif // LTC_TIMECODE_H_
