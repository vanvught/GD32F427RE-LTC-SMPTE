/**
 * @file ltc_input_artnet.cpp
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

#include <cassert>
#include <utility>

#include "artnet.h"
#include "core/netif.h"
#include "ltc_debug.h"
#include "network_udp.h"
#include "output/ltc_output.h"
#include "ltc.h"

namespace ltc {
// Input
namespace input::artnet {
namespace {
int32_t handle{-1};
bool is_started{false};

void InputUdp(const uint8_t* buffer, uint32_t size, [[maybe_unused]] uint32_t from_ip, [[maybe_unused]] uint16_t from_port) {
    if (size != sizeof(::artnet::ArtTimeCode)) [[unlikely]] {
        return;
    }

    if ((buffer[10] != 0) || (buffer[11] != ::artnet::kProtocolRevision)) [[unlikely]] {
        return;
    }

    const auto kOpCode = static_cast<::artnet::OpCodes>((static_cast<uint16_t>(buffer[9] << 8)) + buffer[8]);

    if (kOpCode == ::artnet::OpCodes::kOpTimecode) [[likely]] {
        const auto* const kArtTimeCode = reinterpret_cast<const ::artnet::ArtTimeCode*>(buffer);
        ltc::output::Destination::Instance().Distribute(reinterpret_cast<const struct ltc::TimeCode*>(&kArtTimeCode->frames));
    }
}
} // namespace

void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    if (is_started) {
        LTC_INPUT_DEBUG_EXIT();
        return;
    }

    if (handle != -1) {
        network::udp::End(::artnet::kUdpPort);
    }

    artnet::handle = network::udp::Begin(::artnet::kUdpPort, InputUdp);
    assert(artnet::handle != -1);

    output::Destination::Instance().SetType(::ltc::Type::kUnknown);

    is_started = true;

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    if (!is_started) {
        LTC_INPUT_DEBUG_EXIT();
        return;
    }

    handle = network::udp::End(::artnet::kUdpPort);
    handle = -1;

    is_started = false;

    LTC_INPUT_DEBUG_EXIT();
}
} // namespace input::artnet

// Output
namespace output::artnet {
namespace {
int32_t handle = -1;
uint32_t output_ip;
::artnet::ArtTimeCode art_timecode;
} // namespace
struct TimeCode {
    uint8_t frames;  ///< Frames time. 0 – 29 depending on mode.
    uint8_t seconds; ///< Seconds. 0 - 59.
    uint8_t minutes; ///< Minutes. 0 - 59.
    uint8_t hours;   ///< Hours. 0 - 59.
    uint8_t type;    ///< 0 = Film (24fps) , 1 = EBU (25fps), 2 = DF (29.97fps), 3 = SMPTE (30fps)
} __attribute__((packed));

void Start() {
    LTC_OUTPUT_DEBUG_ENTRY();

    if (output_ip == 0) {
        output_ip = netif::BroadcastIpAddr();
    }

    if (handle != -1) {
        network::udp::End(::artnet::kUdpPort);
    }

    artnet::handle = network::udp::Begin(::artnet::kUdpPort, nullptr);
    assert(artnet::handle != -1);

    memcpy(artnet::art_timecode.id, ::artnet::kNodeId, sizeof(artnet::art_timecode.id));
    artnet::art_timecode.op_code = std::to_underlying(::artnet::OpCodes::kOpTimecode);
    artnet::art_timecode.prot_ver_hi = 0;
    artnet::art_timecode.prot_ver_lo = ::artnet::kProtocolRevision;
    artnet::art_timecode.filler1 = 0;
    artnet::art_timecode.filler2 = 0;

    LTC_DEBUG_PRINTF("output ip=" IPSTR, IP2STR(output_ip));
    LTC_OUTPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_OUTPUT_DEBUG_ENTRY();

    assert(handle != -1);
    handle = network::udp::End(::artnet::kUdpPort);
    handle = -1;

    LTC_OUTPUT_DEBUG_EXIT();
}

void SetDestinationIp(uint32_t destination_ip) {
    LTC_OUTPUT_DEBUG_ENTRY();

    output_ip = destination_ip;

    LTC_OUTPUT_DEBUG_EXIT();
}

uint32_t DestinationIp() {
    LTC_OUTPUT_DEBUG_ENTRY();

    return output_ip;

    LTC_OUTPUT_DEBUG_EXIT();
}

void Output(const ::ltc::TimeCode* timecode) {
    memcpy(&art_timecode.frames, timecode, sizeof(struct TimeCode));
    network::udp::Send(handle, reinterpret_cast<const uint8_t*>(&art_timecode), sizeof(struct ::artnet::ArtTimeCode), output_ip, ::artnet::kUdpPort);
}
} // namespace output::artnet
} // namespace ltc