/**
 * @file software_version.h
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

#ifndef SOFTWARE_VERSION_H_
#define SOFTWARE_VERSION_H_

#include <cstdint>
#include <string_view>

constexpr char kSoftwareVersion[] = "0.1";

constexpr uint8_t ParseVersion(std::string_view version) {
    uint32_t value = 0;

    for (const auto ch : version) {
        if (ch < '0' || ch > '9') {
            break;
        }

        value = (value * 10U) + static_cast<uint32_t>(ch - '0');
    }

    return static_cast<uint8_t>(value);
}

constexpr std::string_view kVersion{kSoftwareVersion};

constexpr auto kDot = kVersion.find('.');

static_assert(kDot != std::string_view::npos);

constexpr uint8_t kVersionMajor = ParseVersion(kVersion.substr(0, kDot));

constexpr uint8_t kVersionMinor = ParseVersion(kVersion.substr(kDot + 1));

#endif /* SOFTWARE_VERSION_H_ */
