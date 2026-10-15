/**
 * @file ltcparamsconst.h
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

#ifndef JSON_SOURCEPARAMSCONST_H_
#define JSON_SOURCEPARAMSCONST_H_

#include <array>
#include <string_view>

#include "json/json_key.h"
#include "output/ltc_output.h"

namespace json::ltc {
inline constexpr std::size_t kOutputKeySize = 32;

consteval auto MakeOutputKeyName(std::string_view name) {
    std::array<char, kOutputKeySize> result{};

    constexpr std::string_view kPrefix = "output_";

    std::size_t index = 0;

    for (const char c : kPrefix) {
        result[index++] = c;
    }

    for (const char c : name) {
        result[index++] = c;
    }

    return result;
}

consteval auto MakeOutputKeyNames() {
    constexpr auto kCount = ::ltc::output::kSupportedOutputs.second;

    std::array<std::array<char, kOutputKeySize>, kCount> result{};

    for (std::size_t i = 0; i < kCount; ++i) {
        const auto kOutput = ::ltc::output::kSupportedOutputs.first[i];
        result[i] = MakeOutputKeyName(::ltc::OutputToName(kOutput));
    }

    return result;
}

inline constexpr auto kOutputKeyNames = MakeOutputKeyNames();

consteval auto MakeOutputKeyDefinitions() {
    constexpr auto kCount = kOutputKeyNames.size();

    std::array<json::KeyDefinition, kCount> result{};

    for (std::size_t i = 0; i < kCount; ++i) {
        const auto& name = kOutputKeyNames[i];

        std::size_t length = 0;

        while (name[length] != '\0') {
            ++length;
        }

        const auto kLength = static_cast<uint8_t>(length);

        result[i] = {.name = name.data(), .length = static_cast<uint8_t>(length), .hash = Fnv1a32(name.data(), kLength)};
    }

    return result;
}

inline constexpr auto kOutputKeyDefinitions = MakeOutputKeyDefinitions();

struct LtcParamsConst {
    static constexpr char kFileName[] = "ltc.json";

    static constexpr auto kSource = json::MakeKey("source");

    static constexpr auto kOutputs = kOutputKeyDefinitions;
};
} // namespace json::ltc

#endif // JSON_SOURCEPARAMSCONST_H_
