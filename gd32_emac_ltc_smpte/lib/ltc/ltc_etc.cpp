/**
 * @file ltc_input_etc.cpp
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

#include "ltc_etc.h"
#include "common/utils/utils_hex.h"
#include "ip4/ip4_address.h"
#include "network_udp.h"
#include "network_igmp.h"
#include "output/ltc_output.h"
#include "ltc.h"
#include "ltc_debug.h"
#include "firmware/debug/debug_dump.h"

namespace ltc {
namespace etc {
namespace {
struct Config {
    uint32_t destination_ip;
    uint32_t source_multicast_ip;
    uint16_t destination_port;
    uint16_t source_port;
    ltc::etc::UdpTerminators terminator;
};

struct Handle {
    int destination;
    int source;
};

constexpr char kPrefix[] = {'M', 'I', 'D', 'I', ' '};
constexpr char kHeader[] = {'F', '0', ' ', '7', 'F', ' ', '7', 'F', ' ', '0', '1', ' ', '0', '1', ' '};
constexpr char kTimecode[] = {'H', 'H', ' ', 'M', 'M', ' ', 'S', 'S', ' ', 'F', 'F', ' '};
constexpr char kEnd[] = {'F', '7'};
constexpr auto kMinMsgLength = sizeof(kPrefix) + sizeof(kHeader) + sizeof(kTimecode) + sizeof(kEnd);

Config config{};
Handle handle{.destination = -1, .source = -1};

char buffer[::ltc::etc::kMinMsgLength + 2];

auto is_init{false};
} // namespace

void SetDestinationIp(uint32_t destination_ip) {
    if ((network::IsPrivateIp(destination_ip) || network::IsMulticastIp(destination_ip))) {
        config.destination_ip = destination_ip;
    } else {
        config.destination_ip = 0;
    }
}

uint32_t DestinationIp() {
    return config.destination_ip;
}

void SetDestinationPort(uint16_t destination_port) {
    if (destination_port > 1023) {
        config.destination_port = destination_port;
    } else {
        config.destination_port = 0;
    }
}

uint16_t DestinationPort() {
    return config.destination_port;
}

void SetSourceMulticastIp(uint32_t source_multicast_ip) {
    if (network::IsMulticastIp(source_multicast_ip)) {
        config.source_multicast_ip = source_multicast_ip;
    } else {
        config.source_multicast_ip = 0;
    }
}

uint32_t SourceMulticastIp() {
    return config.source_multicast_ip;
}

void SetSourcePort(uint16_t source_port) {
    if (source_port > 1023) {
        config.source_port = source_port;
    } else {
        config.source_port = 0;
    }
}

uint16_t SourcePort() {
    return config.source_port;
}

void SetUdpTerminator(ltc::etc::UdpTerminators terminator) {
    if (terminator < UdpTerminators::kUndefined) {
        config.terminator = terminator;
    }
}

ltc::etc::UdpTerminators UdpTerminator() {
    return config.terminator;
}

void Print() {
    puts("ETC gateway");

    if ((config.destination_ip != 0) && (config.destination_port != 0)) {
        printf(" Destination: " IPSTR ":%u\n", IP2STR(config.destination_ip), static_cast<unsigned>(config.destination_port));
    } else {
        puts(" No output");
    }

    if (config.source_port != 0) {
        printf("Source port: %u\n", static_cast<unsigned>(config.source_port));
        if (config.source_multicast_ip != 0) {
            printf(" Multicast ip: " IPSTR, IP2STR(config.source_multicast_ip));
        }
    } else {
        puts(" No input");
    }

    printf(" UDP Termination: %s\n", UdpTerminatorToName(config.terminator));
}

void Init() {
    auto* pointer = buffer;
    memcpy(pointer, kPrefix, sizeof(kPrefix));
    pointer += sizeof(kPrefix);
    memcpy(pointer, kHeader, sizeof(kHeader));
    pointer += sizeof(kHeader);
    pointer[2] = ' ';
    pointer[5] = ' ';
    pointer[8] = ' ';
    pointer[11] = ' ';
    pointer += sizeof(kTimecode);
    memcpy(pointer, kEnd, sizeof(kEnd));

    is_init = true;
}
} // namespace etc

namespace input::etc {
namespace {
uint8_t FromHex(const char* hex) {
    const auto kLow = (hex[1] > '9' ? (hex[1] | 0x20) - 'a' + 10 : hex[1] - '0');
    const auto kHigh = (hex[0] > '9' ? (hex[0] | 0x20) - 'a' + 10 : hex[0] - '0');
    return static_cast<uint8_t>((kHigh << 4) | kLow);
}

void ParseTimeCode() {
    TimeCode timecode;

    const auto* pointer = &::ltc::etc::buffer[sizeof(::ltc::etc::kPrefix) + sizeof(::ltc::etc::kHeader)];
    auto data = FromHex(pointer);

    timecode.hours = data & 0x1F;
    timecode.type = static_cast<uint8_t>(data >> 5);
    pointer += 3;
    timecode.minutes = FromHex(pointer);
    pointer += 3;
    timecode.seconds = FromHex(pointer);
    pointer += 3;
    timecode.frames = FromHex(pointer);

    ltc::output::Destination::Instance().Distribute(&timecode);
}

void Input(const uint8_t* buffer, uint32_t size, [[maybe_unused]] uint32_t from_ip, [[maybe_unused]] uint16_t from_port) {
    const auto* const kUdpBuffer = reinterpret_cast<const char*>(buffer);
    debug::Dump(kUdpBuffer, size);

    if (size == ::ltc::etc::kMinMsgLength) {
        if (::ltc::etc::config.terminator != ::ltc::etc::UdpTerminators::kNone) {
            return;
        }
    }

    if (size == 1 + ::ltc::etc::kMinMsgLength) {
        if (::ltc::etc::config.terminator == ::ltc::etc::UdpTerminators::kCrlf) {
            return;
        }

        if ((::ltc::etc::config.terminator == ::ltc::etc::UdpTerminators::kCr) && (kUdpBuffer[::ltc::etc::kMinMsgLength] != 0x0D)) {
            return;
        }

        if ((::ltc::etc::config.terminator == ::ltc::etc::UdpTerminators::kLf) && (kUdpBuffer[::ltc::etc::kMinMsgLength] != 0x0A)) {
            return;
        }
    }

    if (size == 2 + ::ltc::etc::kMinMsgLength) {
        if (::ltc::etc::config.terminator != ::ltc::etc::UdpTerminators::kCrlf) {
            return;
        }

        if ((kUdpBuffer[::ltc::etc::kMinMsgLength] != 0x0D) || (kUdpBuffer[1 + ::ltc::etc::kMinMsgLength] != 0x0A)) {
            return;
        }
    }

    if (memcmp(kUdpBuffer, ::ltc::etc::buffer, sizeof(::ltc::etc::kPrefix) + sizeof(::ltc::etc::kHeader)) != 0) {
        return;
    }

    ParseTimeCode();
}
} // namespace

void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    if (::ltc::etc::config.source_port != 0) {
        ::ltc::etc::handle.source = network::udp::Begin(::ltc::etc::config.source_port, Input);

        if ((::ltc::etc::handle.source >= 0) && (::ltc::etc::config.source_multicast_ip != 0)) {
            network::igmp::JoinGroup(::ltc::etc::handle.source, ::ltc::etc::config.source_multicast_ip);
        }
    }

    output::Destination::Instance().SetType(::ltc::Type::kUnknown);

    if (!::ltc::etc::is_init) {
        ::ltc::etc::Init();
    }

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    network::igmp::LeaveGroup(::ltc::etc::handle.source, ::ltc::etc::config.source_multicast_ip);

    ::ltc::etc::handle.source = network::udp::End(::ltc::etc::config.source_port);
    ::ltc::etc::handle.source = -1;

    LTC_INPUT_DEBUG_EXIT();
}
} // namespace input::etc

namespace output::etc {
void Start() {
    LTC_OUTPUT_DEBUG_ENTRY();

    if ((::ltc::etc::config.destination_ip != 0) && (::ltc::etc::config.destination_port != 0)) {
        ::ltc::etc::handle.destination = network::udp::Begin(::ltc::etc::config.destination_port, nullptr);
        assert(::ltc::etc::handle.destination != 1);
    }

    if (!::ltc::etc::is_init) {
        ::ltc::etc::Init();
    }

    LTC_OUTPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_OUTPUT_DEBUG_ENTRY();

    ::ltc::etc::handle.destination = network::udp::End(::ltc::etc::config.destination_port);
    ::ltc::etc::handle.destination = -1;

    LTC_OUTPUT_DEBUG_EXIT();
}

void Output(const ::ltc::TimeCode* timecode) {
    if (::ltc::etc::handle.destination < 0) {
        return;
    }

    auto* pointer = &::ltc::etc::buffer[sizeof(::ltc::etc::kPrefix) + sizeof(::ltc::etc::kHeader)];

    const auto kData5 = static_cast<uint8_t>(((timecode->type) & 0x03) << 5) | (timecode->hours & 0x1F);

    *pointer++ = common::hex::ToCharUppercase((kData5 >> 4) & 0xFF);
    *pointer = common::hex::ToCharUppercase(kData5 & 0x0F);
    pointer += 2;
    *pointer++ = common::hex::ToCharUppercase((timecode->minutes >> 4) & 0xFF);
    *pointer = common::hex::ToCharUppercase(timecode->minutes & 0x0F);
    pointer += 2;
    *pointer++ = common::hex::ToCharUppercase((timecode->seconds >> 4) & 0xFF);
    *pointer = common::hex::ToCharUppercase(timecode->seconds & 0x0F);
    pointer += 2;
    *pointer++ = common::hex::ToCharUppercase((timecode->frames >> 4) & 0xFF);
    *pointer = common::hex::ToCharUppercase(timecode->frames & 0x0F);

    auto length = ::ltc::etc::kMinMsgLength;

    switch (::ltc::etc::config.terminator) {
        case ::ltc::etc::UdpTerminators::kCr:
            ::ltc::etc::buffer[::ltc::etc::kMinMsgLength] = 0x0D;
            length++;
            break;
        case ::ltc::etc::UdpTerminators::kLf:
            ::ltc::etc::buffer[::ltc::etc::kMinMsgLength] = 0x0A;
            length++;
            break;
        case ::ltc::etc::UdpTerminators::kCrlf:
            ::ltc::etc::buffer[::ltc::etc::kMinMsgLength] = 0x0D;
            ::ltc::etc::buffer[::ltc::etc::kMinMsgLength + 1] = 0x0A;
            length = static_cast<uint16_t>(length + 2);
            break;
        default:
            break;
    }

    network::udp::Send(::ltc::etc::handle.destination, reinterpret_cast<const uint8_t*>(::ltc::etc::buffer), length, ::ltc::etc::config.destination_ip, ::ltc::etc::config.destination_port);
    debug::Dump(::ltc::etc::buffer, length);
}
} // namespace output::etc
} // namespace ltc