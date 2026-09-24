/**
 * @file ltc_etc.h
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

#ifndef LTC_ETC_H_
#define LTC_ETC_H_

#include <cstdint>
#include <strings.h>

#include "common/utils/utils_string.h"

namespace ltc::etc {
enum class UdpTerminators : uint8_t { kNone, kCr, kLf, kCrlf, kUndefined };

constexpr uint32_t kMaxNameLength = 5; // Including '\0'
constexpr const char kUdpTerminator[][kMaxNameLength] = {
    "None", //
    "CR",   //
    "LF",   //
    "CRLF", //
};

[[nodiscard]] constexpr const char* UdpTerminatorToName(UdpTerminators udp_terminiator) {
    if (udp_terminiator < UdpTerminators::kUndefined) {
        return kUdpTerminator[static_cast<uint32_t>(udp_terminiator)];
    }

    return common::kUndefined;
}

inline UdpTerminators UdpTerminatorFromName(const char* name) {
    uint32_t index = 0;

    for (const auto& input : kUdpTerminator) {
        if (strcasecmp(name, input) == 0) {
            return static_cast<UdpTerminators>(index);
        }

        ++index;
    }

    return UdpTerminators::kUndefined;
}

void SetDestinationIp(uint32_t destination_ip);
uint32_t DestinationIp();

void SetDestinationPort(uint16_t destination_port);
uint16_t DestinationPort();

void SetSourceMulticastIp(uint32_t source_multicast_ip);
uint32_t SourceMulticastIp();

void SetSourcePort(uint16_t source_port);
uint16_t SourcePort();

void SetUdpTerminator(UdpTerminators terminator);
UdpTerminators UdpTerminator();
} // namespace ltc::etc

#endif // LTC_ETC_H_
