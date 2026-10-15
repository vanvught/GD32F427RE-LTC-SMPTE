/**
 * @file ltcparams.cpp
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
#include <utility>

#include "json/ltcparams.h"
#include "configstore.h"
#include "input/ltc_input.h"
#include "json/json_debug.h"
#include "json/json_parser.h"
#include "ltc.h"

namespace json::ltc {
LtcParams::LtcParams() {
    ConfigStore::Instance().Copy(&store_ltc, &ConfigurationStore::ltc);
}

void LtcParams::SetSource(std::string_view value) {
    if (value.size() >= ::ltc::kInputMaxNameLength) {
        return;
    }

    store_ltc.source = std::to_underlying(::ltc::InputFromName(value));
}

void LtcParams::SetOutput(std::string_view key, std::string_view value) {
    for (const auto& entry : LtcParamsConst::kOutputs) {
        if (key != std::string_view{entry.name, entry.length}) {
            continue;
        }

        key.remove_prefix(7); // output_

        const auto kOutput = ::ltc::OutputFromName(key);

        if (value.starts_with('D')) {
            store_ltc.disabled_outputs |= std::to_underlying(kOutput);
            return;
        }

        store_ltc.disabled_outputs &= static_cast<uint16_t>(~std::to_underlying(kOutput));
        return;
    }
}

void LtcParams::Store(const char* buffer, uint32_t buffer_size) {
    ParseJsonWithTable(std::string_view{buffer, buffer_size}, kLtcKeys.data(), kLtcKeys.size());
    ConfigStore::Instance().Store(&store_ltc, &ConfigurationStore::ltc);

    Dump();
}

void LtcParams::Set() {
    // DO NOT CHANGE ORDER
    for (const auto& output : ::ltc::kOutputs) {
        const auto kOutput = std::to_underlying(output);
        if ((kOutput & store_ltc.disabled_outputs) == kOutput) {
            ::ltc::output::Destination::Instance().Disable(output);
        } else {
            ::ltc::output::Destination::Instance().Enable(output);
        }
    }
    // DO NOT CHANGE ORDER
    ::ltc::input::Source::Instance().Select(static_cast<::ltc::Input>(store_ltc.source));
}

void LtcParams::Dump() {
    if constexpr (kDebugJsonEnabled) {
        printf("%s::%s '%s':\n", __FILE__, __FUNCTION__, LtcParamsConst::kFileName);
        printf(" %s=%s\n", LtcParamsConst::kSource.name, ::ltc::InputToName(static_cast<::ltc::Input>(store_ltc.source)));

        constexpr const auto& supported = ::ltc::output::kSupportedOutputs;

        for (std::size_t i = 0; i < supported.second; ++i) {
            const auto& entry = LtcParamsConst::kOutputs[i];
            const auto kOutput = std::to_underlying(supported.first[i]);

            const auto kDisabled = (store_ltc.disabled_outputs & kOutput) != 0;

            printf(" %s=%s\n", entry.name, kDisabled ? "Disabled" : "Enabled");
        }
    }
}
} // namespace json::ltc
