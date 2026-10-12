/**
 * @file shell_date.cpp
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
#include <ctime>
#include <sys/time.h>

#include "firmware/global.h"
#include "uart0.h"
#include "shell.h"

namespace shell {
void Date(Arguments args) {
    if (args.size() != 1) {
        uart0::Puts(kTooMany);
        return;
    }

    timeval time_val{};

    if (gettimeofday(&time_val, nullptr) < 0) {
        uart0::Puts("Date not available.");
        return;
    }

    const auto* local_time = localtime(&time_val.tv_sec);

    int32_t hours;
    uint32_t minutes;
    global::GetUtcOffset(hours, minutes);

    uart0::Printf("%s %s%.2d:%.2u\n\n", asctime(local_time), hours > 0 ? "+" : "", static_cast<int>(hours), static_cast<unsigned int>(minutes));
}
} // namespace shell