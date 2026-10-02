/**
 * @file tcnet.cpp
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
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <utility>

#include "tcnet.h"
#include "network_config.h"
#include "network_udp.h"
#include "softwaretimers.h"
#include "tcnet_packet.h"
#include "tcnet_timecode.h"
#include "timing.h"

namespace tcnet {
struct Broadcast {
    static constexpr uint16_t kPort0 = 60000;
    static constexpr uint16_t kPort1 = 60001;
    static constexpr uint16_t kPort2 = 60002;
};

struct Unicast {
    static constexpr uint16_t kPort = 65023;
};

namespace {

packet::OptIn packet_opt_in;
int32_t handles[2];
bool use_time_code{false};
uint32_t lx_time_offset{0};
uint32_t lx_time_code_offset{0};
enum Layer layer_ { tcnet::Layer::kLayerM };
enum TimeCodeType timecode_type_ { tcnet::TimeCodeType::kInvalid };
Timecode timecode_previous = {.frames = 0, .seconds = 0, .minutes = 0, .hours = 0, .type = UINT8_MAX};
float type_divider{1000.0F / 30};
TimerHandle_t timer_id = kTimerIdNone;

#ifdef DEBUG_TCNET
void DumpManagementHeader(const uint8_t* buffer) {
    static uint32_t s_timestamp_previous{0};
    const auto& management_header = reinterpret_cast<const struct packet::ManagementHeader&>(*buffer);

    printf("ManagementHeader\n");
    printf(" %.3s V%u.%u %.8s\n", management_header.header, static_cast<unsigned>(management_header.protocol_version_major), static_cast<unsigned>(management_header.protocol_version_minor), management_header.node_name);
    printf(" %s\n", management_header.node_type == std::to_underlying(packet::NodeType::kSlave) ? "SLAVE" : (std::to_underlying(packet::NodeType::kMaster) == management_header.node_type ? "MASTER" : "AUTO"));
    printf(" %u [%u] %u\n", static_cast<unsigned>(management_header.time_stamp), static_cast<unsigned>(management_header.time_stamp - s_timestamp_previous), static_cast<unsigned>(management_header.seq));

    s_timestamp_previous = management_header.time_stamp;
}
#endif

void InputPort60000(const uint8_t* buffer, [[maybe_unused]] uint32_t size, [[maybe_unused]] uint32_t from_ip, [[maybe_unused]] uint16_t from_port) {
    const auto* packet = reinterpret_cast<const packet::ManagementHeader*>(buffer);
    const auto kMessageType = static_cast<packet::MessageType>(packet->message_type);

    TCNET_DEBUG_PRINTF("kMessageType=%u", static_cast<unsigned>(kMessageType));

    if (kMessageType == packet::MessageType::kMessageTypeOptin) {
#ifdef DEBUG_TCNET
        DumpManagementHeader(buffer);

        const auto& opt_in = reinterpret_cast<const struct packet::OptIn&>(*buffer);

        printf(" OptIn\n");
        printf("  %u %u\n", static_cast<unsigned>(opt_in.node_count), static_cast<unsigned>(opt_in.node_listener_port));
        printf("  %u %u\n", static_cast<unsigned>(opt_in.node_count), static_cast<unsigned>(opt_in.up_time));
        printf("  %.16s %.16s %d.%d.%d [%d]\n", opt_in.vendor_name, opt_in.device_name, opt_in.device_major_version, opt_in.device_minor_version, opt_in.device_bug_version, opt_in.node_count);
#endif
    }
}

void InputPort60001(const uint8_t* buffer, [[maybe_unused]] uint32_t size, [[maybe_unused]] uint32_t from_ip, [[maybe_unused]] uint16_t from_port) {
    const auto& packet_time = reinterpret_cast<const struct packet::Time&>(*buffer);

    if (static_cast<packet::MessageType>(packet_time.management_header.message_type) == packet::MessageType::kMessageTypeTime) {
        tcnet::Timecode timecode;

        if (use_time_code) {
            const auto* lx_tc = reinterpret_cast<const packet::TimeCode*>(buffer + lx_time_code_offset);
            timecode.frames = lx_tc->frames;
            timecode.seconds = lx_tc->seconds;
            timecode.minutes = lx_tc->minutes;
            timecode.hours = lx_tc->hours;

            auto smpte_mode = lx_tc->smpte_mode;

            if (smpte_mode < 24) {
                smpte_mode = packet_time.SMPTEMode;
            }

            switch (smpte_mode) {
                case 24:
                    timecode.type = static_cast<uint8_t>(tcnet::TimeCodeType::kFilm);
                    break;
                case 25:
                    timecode.type = static_cast<uint8_t>(tcnet::TimeCodeType::kEbu25Fps);
                    break;
                case 29:
                    timecode.type = static_cast<uint8_t>(tcnet::TimeCodeType::kDf);
                    break;
                case 30:
                    __attribute__((fallthrough));
                    /* no break */
                default:
                    timecode.type = static_cast<uint8_t>(tcnet::TimeCodeType::kSmpte30Fps);
                    break;
            }
        } else {
            auto lx_time = *reinterpret_cast<const uint32_t*>(buffer + lx_time_offset);

            const auto kHours = lx_time / 3600000U;
            lx_time -= kHours * 3600000U;
            const auto kMinutes = lx_time / 60000U;
            lx_time -= kMinutes * 60000U;
            const auto kSeconds = lx_time / 1000U;
            const auto kMillis = lx_time - (kSeconds * 1000U);
            const auto kFrames = (kMillis * tcnet::kFps[static_cast<uint32_t>(timecode_type_)]) / 1000U;

            timecode.frames = static_cast<uint8_t>(kFrames);
            timecode.seconds = static_cast<uint8_t>(kSeconds);
            timecode.minutes = static_cast<uint8_t>(kMinutes);
            timecode.hours = static_cast<uint8_t>(kHours);
            timecode.type = static_cast<uint8_t>(timecode_type_);
        }

        const auto* src = reinterpret_cast<uint8_t*>(&timecode);
        auto* dst = reinterpret_cast<uint8_t*>(&timecode_previous);
        auto do_send{false};

        for (uint32_t index = 0; index < sizeof(struct tcnet::Timecode); index++) {
            do_send |= (*src != *dst);
            *dst++ = *src++;
        }

        if (do_send) {
            Handle(&timecode);
        }
    }
}

void TimerOptInOutgoing([[maybe_unused]] TimerHandle_t handle) {
    packet_opt_in.management_header.seq++;
    packet_opt_in.management_header.time_stamp = timing::Micros();
    packet_opt_in.up_time = static_cast<uint16_t>(timing::UpTime());

    network::udp::Send(handles[0], reinterpret_cast<const uint8_t*>(&packet_opt_in), sizeof(struct packet::OptIn), network::GetBroadcastIp(), Broadcast::kPort0);
}
} // namespace

using tcnet::packet_opt_in;

void Start() {
    TCNET_DEBUG_ENTRY();

    memset(&packet_opt_in, 0, sizeof(packet::OptIn));

    packet_opt_in.management_header.protocol_version_major = 3;
    packet_opt_in.management_header.protocol_version_minor = 3;
    memcpy(packet_opt_in.management_header.header, "TCN", 3);
    packet_opt_in.management_header.message_type = std::to_underlying(packet::MessageType::kMessageTypeOptin);
    packet_opt_in.management_header.seq = 0;
    packet_opt_in.management_header.node_type = std::to_underlying(packet::NodeType::kSlave);
    packet_opt_in.management_header.node_options = 0;
    packet_opt_in.node_count = 1;
    packet_opt_in.node_listener_port = Unicast::kPort;
    memcpy(&packet_opt_in.vendor_name, "gd32-dmx.org", packet::kVendorNameLength);
    memcpy(&packet_opt_in.device_name, "LTC SMPTE Node  ", packet::kDeviceNameLength);
    packet_opt_in.device_major_version = static_cast<uint8_t>(_TIME_STAMP_YEAR_ - 2000);
    packet_opt_in.device_minor_version = _TIME_STAMP_MONTH_;
    packet_opt_in.device_bug_version = _TIME_STAMP_DAY_;

    SetNodeName(tcnet::kNodeNameDefault);
    SetLayer(tcnet::Layer::kLayerM);
    SetTimeCodeType(tcnet::TimeCodeType::kSmpte30Fps);

    handles[0] = network::udp::Begin(Broadcast::kPort0, InputPort60000);
    assert(handles[0] >= 0);

    handles[1] = network::udp::Begin(Broadcast::kPort1, InputPort60001);
    assert(handles[1] >= 0);

    if (timer_id != kTimerIdNone) {
        SoftwareTimerDelete(timer_id);
    }
    timer_id = SoftwareTimerAdd(1000, TimerOptInOutgoing);

    TCNET_DEBUG_EXIT();
}

void Stop() {
    TCNET_DEBUG_ENTRY();

    if (timer_id != kTimerIdNone) {
        SoftwareTimerDelete(timer_id);
        timer_id = kTimerIdNone;
    }

    network::udp::End(Broadcast::kPort1);
    handles[1] = -1;

    network::udp::End(Broadcast::kPort0);
    handles[0] = -1;

    TCNET_DEBUG_EXIT();
}

void Print() {
    puts("TCNet");
    printf(" Node : %.8s\n", packet_opt_in.management_header.node_name);
    printf(" L%c", tcnet::LayerToChar(layer_));
    if (use_time_code) {
        puts(" TC");
    } else {
        printf(" T%u\n", static_cast<unsigned>(tcnet::kFps[static_cast<uint32_t>(timecode_type_)]));
    }

    printf("%u:%u:%u\n", static_cast<unsigned>(layer_), static_cast<unsigned>(lx_time_offset), static_cast<unsigned>(lx_time_code_offset));
}

void SetNodeName(const char* node_name) {
    strncpy(reinterpret_cast<char*>(packet_opt_in.management_header.node_name), node_name, sizeof packet_opt_in.management_header.node_name - 1);
    packet_opt_in.management_header.node_name[sizeof packet_opt_in.management_header.node_name - 1] = '\0';
}

const char* NodeName() {
    return reinterpret_cast<char*>(packet_opt_in.management_header.node_name);
}

void SetLayer(enum Layer lay) {
    layer_ = lay;
    lx_time_offset = offsetof(struct packet::Time, L1Time) + (4 * static_cast<uint32_t>(lay));
    lx_time_code_offset = offsetof(struct packet::Time, L1TimeCode) + (static_cast<uint32_t>(lay) * sizeof(struct packet::TimeCode));
}

enum Layer Layer() {
    return layer_;
}

void SetUseTimeCode(bool use) {
    use_time_code = use;
}

bool IsUseTimeCode() {
    return use_time_code;
}

void SetTimeCodeType(enum tcnet::TimeCodeType type) {
    switch (type) {
        case tcnet::TimeCodeType::kFilm:
            type_divider = 1000.0F / 24;
            break;
        case tcnet::TimeCodeType::kEbu25Fps:
            type_divider = 1000.0F / 25;
            break;
        case tcnet::TimeCodeType::kDf:
            type_divider = 1000.0F / 29.97f;
            break;
        case tcnet::TimeCodeType::kSmpte30Fps:
            type_divider = 1000.0F / 30;
            break;
        default:
            return;
            break;
    }

    timecode_type_ = type;
}

enum TimeCodeType GetTimeCodeType() {
    return timecode_type_;
}
} // namespace tcnet