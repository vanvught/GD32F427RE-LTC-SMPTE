/**
 * @file ltc_input_systime.cpp
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

#include <sys/time.h>
#include <cstdint>

#include "ltc.h"
#include "ltc_timecode.h"
#include "ltc_debug.h"
#include "output/ltc_output.h"

namespace ltc::global {
extern volatile bool timecode_available;
extern volatile uint32_t timecode_counter;
} // namespace ltc::global

namespace ltc::input::systime {
namespace {
auto is_started{false};

} // namespace
void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    if (ltc::output::Destination::Instance().Type() == ltc::Type::kUnknown) {
        ltc::output::Destination::Instance().SetType(::ltc::Type::kSmpte);
    }

    uint32_t seconds;
    uint32_t microseconds;

    ltc::timecode::LocalTime(seconds, microseconds);
    ltc::timecode::Set(seconds, microseconds, ltc::output::Destination::Instance().Type());

    is_started = true;

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    is_started = false;

    LTC_INPUT_DEBUG_EXIT();
}

#if 0
void Run() {
    if (!is_started) {
        return;
    }

    if (!ltc::global::timecode_available) {
        return;
    }

    ltc::global::timecode_available = false;

    ltc::output::Destination::Instance().Distribute(&ltc::global::timecode_running);

    ltc::timecode::Increment();
}
#else
void Run() {
    if (!is_started) {
        return;
    }

    static uint32_t previous_counter{};

    const auto kCounter = ltc::global::timecode_counter;
    const auto kElapsed = kCounter - previous_counter;

    if (kElapsed == 0) {
        return;
    }

    previous_counter = kCounter;
    const auto kTimer3 = TIMER_CNT(TIMER3);

    ltc::global::timecode_available = false;

    ltc::output::Destination::Instance().Distribute(&ltc::global::timecode_running);

    ltc::timecode::Increment();

    if (kElapsed != 1) {
        printf("LTC MISSED: elapsed=%u counter=%u TIMER3=%u\n", static_cast<unsigned>(kElapsed), static_cast<unsigned>(kCounter), static_cast<unsigned>(kTimer3));
    }
}
#endif
} // namespace ltc::input::systime