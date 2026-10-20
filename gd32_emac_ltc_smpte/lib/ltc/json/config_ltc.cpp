/**
 * @file config_ltc.cpp
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

#include "json/json_helpers.h"
#include "json/ltcparams.h"
#include "json/ltcparamsconst.h"
#include "input/ltc_input.h"
#include "ltc.h"
#include "output/ltc_output.h"

namespace json::config {
uint32_t GetLtc(char* buffer, uint32_t length) {
    const auto kSource = ::ltc::input::Source::Instance().Input();
    const auto& destination = ::ltc::output::Destination::Instance();

    return json::helpers::Serialize(buffer, length, [&](JsonDoc& doc) {
        doc[ltc::LtcParamsConst::kSource.name] = ::ltc::InputToName(kSource);

        constexpr const auto& supported = ::ltc::output::kSupportedOutputs;

        for (std::size_t i = 0; i < supported.second; ++i) {
            const auto kOutput = supported.first[i];
            const auto& key = ltc::LtcParamsConst::kOutputs[i];

            doc[key.name] = destination.IsEnabled(kOutput) ? "Enabled" : "Disabled";
        }
    });
}

void SetLtc(const char* buffer, uint32_t size) {
    ::json::ltc::LtcParams ltc_params;
    ltc_params.Store(std::string_view{buffer, size});
    ltc_params.Set();
}
} // namespace json::config