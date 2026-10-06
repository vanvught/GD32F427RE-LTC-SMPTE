/**
 * @file shell_hwclock.cpp
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

#include <span>
#include <string_view>

#include "uart0.h"
#include "hwclock.h"

namespace shell {
namespace {
// Read the Hardware Clock and print the time.
constexpr char kShow[] = "show";
// Set the System Time from the Hardware Clock.
constexpr char kHcToSys[] = "hctosys";
// systohc
// Set the Hardware Clock to the current System Time.
constexpr char kSysToHc[] = "systohc";
} // namespace

static void Usage() {
    uart0::Puts("hwclock show|hctosys|systohc");
}

void HwClock(std::span<const std::string_view> args) {
    if (args.size() != 2) {
        Usage();
        return;
    }

    if (args[1] == kShow) {
        HwClock::Get()->Print();
        uart0::PutChar('\n');
        return;
    }

    if (args[1] == kHcToSys) {
        HwClock::Get()->HcToSys();
        uart0::PutChar('\n');
        return;
    }

    if (args[1] == kSysToHc) {
        HwClock::Get()->SysToHc();
        uart0::PutChar('\n');
        return;
    }

    Usage();
}
} // namespace shell