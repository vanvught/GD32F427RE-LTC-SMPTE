/**
 * @file ltc_node.h
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

#ifndef LTC_NODE_H_
#define LTC_NODE_H_

#include <cstdint>

#include "display.h"
#include "ltc_debug.h"
#include "input/ltc_input.h"
#include "ltc_ntpserver.h"
#include "output/ltc_output.h"
#include "input/ltc_input_internal.h"
#include "gnss.h"

namespace ltc {
class Node : input::Source {
   public:
    Node() {
        LTC_DEBUG_ENTRY();

        Display::Get()->Cls();
        // NTP Server
        ntpserver::Init(static_cast<uint32_t>(_TIME_STAMP_YEAR_), static_cast<uint32_t>(_TIME_STAMP_MONTH_), static_cast<uint32_t>(_TIME_STAMP_DAY_));
        // GPS
        gnss::Receiver::Instance().SetDate(static_cast<uint32_t>(_TIME_STAMP_YEAR_), static_cast<uint32_t>(_TIME_STAMP_MONTH_), static_cast<uint32_t>(_TIME_STAMP_DAY_));
        gnss::Receiver::Instance().SetModule(gnss::Module::kUbloxNeo);
        // Internal
        input::internal::SetTimecodeStart(::ltc::TimeCode{.frames = 0, .seconds = 0, .minutes = 0, .hours = 0, .type = 0});
        input::internal::SetTimecodeStop(::ltc::TimeCode{.frames = 0, .seconds = 0, .minutes = 0, .hours = 1, .type = 0});
        // Input
        input::Source::Select(::ltc::Input::kLtc);
        // Output
        output::Destination::StartEnabled();

        LTC_DEBUG_EXIT();
    }

    void SetUtcOffset(int32_t hours, uint32_t minutes) {
        ltc::output::Destination::Instance().SetUtcOffset(hours, minutes);
        gnss::Receiver::Instance().SetUtcOffset(hours, minutes);
    }

    void Run() {
        input::Source::RunInput();
        output::Destination::RunOutput();
        gnss::Receiver::Instance().Run();
    }
};
} // namespace ltc

#endif // LTC_NODE_H_
