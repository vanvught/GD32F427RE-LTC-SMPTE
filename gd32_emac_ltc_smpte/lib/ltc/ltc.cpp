/**
 * @file ltc.cpp
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

#ifdef CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER
#error
#endif // CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER

#include <cstdint>

#include "ltc.h"
#include "gd32.h" // IWYU pragma: keep
#include "common/utils/utils_string.h"

namespace ltc::global {
volatile uint32_t updates_per_second;
volatile uint32_t updates;
volatile uint32_t updates_previous;
} // namespace ltc::global

extern "C" {
void TIMER6_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(TIMER6);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
        ltc::global::updates_per_second = ltc::global::updates - ltc::global::updates_previous;
        ltc::global::updates_previous = ltc::global::updates;
        gv_seconds.uptime = gv_seconds.uptime + 1;
    }

    TIMER_INTF(TIMER6) = ~kIntFlag;
}
}

namespace ltc {
void ConvertToString(const TimeCode* ltc_timecode, char* timecode) {
    using common::Append2;

    Append2(&timecode[timecode::index::kHours], ltc_timecode->hours);
    Append2(&timecode[timecode::index::kMinutes], ltc_timecode->minutes);
    Append2(&timecode[timecode::index::kSeconds], ltc_timecode->seconds);
    Append2(&timecode[timecode::index::kFrames], ltc_timecode->frames);
}
} // namespace ltc