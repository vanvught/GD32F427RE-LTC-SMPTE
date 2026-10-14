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

#include <utility>

#include "json/ltcparams.h"
#include "configstore.h"
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

void LtcParams::Store(const char* buffer, uint32_t buffer_size) {
    ParseJsonWithTable(buffer, buffer_size, kLtcKeys);
    ConfigStore::Instance().Store(&store_ltc, &ConfigurationStore::ltc);

    Dump();
}

void LtcParams::Dump() {
    printf("%s::%s \'%s\':\n", __FILE__, __FUNCTION__, LtcParamsConst::kFileName);
    printf(" %s=%s\n", LtcParamsConst::kSource.name, ::ltc::InputToName(static_cast<::ltc::Input>(store_ltc.source)));
}
} // namespace json::ltc
