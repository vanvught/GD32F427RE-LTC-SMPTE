/**
 * @file ltc_input_tcnet.cpp
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
#include "output/ltc_output.h"
#include "ltc_debug.h"
#include "tcnet.h"
#include "tcnet_timecode.h"

namespace ltc::input::tcnet {
namespace {
auto is_started{false};
}
void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    if (is_started) {
        LTC_INPUT_DEBUG_EXIT();
        return;
    }

    is_started = true;

    output::Destination::Instance().SetType(::ltc::Type::kUnknown);

    ::tcnet::Start();

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    if (!is_started) {
        LTC_INPUT_DEBUG_EXIT();
        return;
    }

    is_started = false;

    ::tcnet::Stop();

    LTC_INPUT_DEBUG_EXIT();
}
} // namespace ltc::input::tcnet

namespace tcnet {
void Handle([[maybe_unused]] const tcnet::Timecode* timecode) {
		ltc::output::Destination::Instance().Distribute(reinterpret_cast<const ::ltc::TimeCode*>(timecode));
}
} // namespace tcnet