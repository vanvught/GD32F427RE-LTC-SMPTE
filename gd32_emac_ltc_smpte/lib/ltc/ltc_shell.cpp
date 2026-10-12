/**
 * @file shell.cpp
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
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "shell.h"
#include "common/utils/utils_string.h"
#include "input/ltc_input.h"
#include "output/ltc_output.h"
#include "ltc_actions.h"
#include "uart0.h"

namespace shell::ltc {
using ::ltc::input::Source;
using ::ltc::output::Destination;

void Ltc(Arguments args) {
    if (args.size() != 2) {
        uart0::Puts(common::kUnknown);
        return;
    }

    ::ltc::actions::HandleAction(args[1]);
}

void Midi(Arguments args) {
    if (args.size() != 2) {
        uart0::Puts(common::kUnknown);
        return;
    }

    ::ltc::actions::midi::HandleAction(args[1]);
}

void Gps(Arguments args) {
    if (args.size() != 2) {
        uart0::Puts(common::kUnknown);
        return;
    }

    ::ltc::actions::gps::HandleAction(args[1]);
}

void TCNet(Arguments args) {
    if (args.size() != 2) {
        uart0::Puts(common::kUnknown);
        return;
    }

    ::ltc::actions::tcnet::HandleAction(args[1]);
}
} // namespace shell::ltc