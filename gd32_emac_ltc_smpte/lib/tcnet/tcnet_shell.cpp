/**
 * @file shell_tcnet.cpp
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

#include "common/utils/utils_string.h"
#include "shell.h"
#include "tcnet.h"

namespace shell::ltc {
namespace {
constexpr int32_t kMaxType = 30;
}

void TCNet(Arguments args) {
    if (args.size() == 1) {
        ::tcnet::Print();
        return;
    }

    if (args.size() == 3) {
        if (args[1] == "layer") {
            if (args[2].size() == 1) {
                const auto kLayer = tcnet::LayerFromChar(args[2].front());
                if (kLayer != tcnet::Layer::kLayerUndefined) {
                    tcnet::SetLayer(kLayer);
                    return;
                }
            }

            uart0::Puts(common::kUnknown);
            return;
        }

        if (args[1] == "type") {
            const auto kValue = shell::GetValue(args[2], kMaxType);

            if (!kValue) {
                uart0::Puts(common::kUnknown);
                return;
            }

            switch (*kValue) {
                case 24:
                    tcnet::SetTimeCodeType(tcnet::TimeCodeType::kFilm);
                    return;

                case 25:
                    tcnet::SetTimeCodeType(tcnet::TimeCodeType::kEbu25Fps);
                    return;

                case 29:
                    tcnet::SetTimeCodeType(tcnet::TimeCodeType::kDf);
                    return;

                case 30:
                    tcnet::SetTimeCodeType(tcnet::TimeCodeType::kSmpte30Fps);
                    return;

                default:
                    uart0::Puts(common::kUnknown);
                    return;
            }

            uart0::Puts(common::kUnknown);
            return;
        }

        if (args[1] == "use_timecode") {
            if (args[2].size() == 1) {
                const auto kUse = args[2].front() == 'y';
                tcnet::SetUseTimeCode(kUse);
                return;
            }
        }
    }

    uart0::Puts(common::kUnknown);
}
} // namespace shell::ltc
