/**
 * @file ntpserver.cpp
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
/*
 * https://tools.ietf.org/html/rfc5905
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cassert>

#include "ltc_ntp.h"
#include "ltc.h"
#include "network_config.h"
#include "network_udp.h"
#include "core/protocol/ntp.h"
#include "core/protocol/iana.h"
#include "ltc_debug.h"

namespace ltc::ntpserver {
namespace {
time_t time{0};
time_t time_date{0};
uint32_t fraction{0};
int32_t handle{-1};
::ntp::Packet reply;

void Input(const uint8_t* buffer, uint32_t size, uint32_t from_ip, uint16_t from_port) {
    if (size != sizeof(struct ::ntp::Packet)) [[unlikely]] {
        return;
    }

    const auto* request = reinterpret_cast<const ::ntp::Packet*>(buffer);

    if ((request->li_vn_mode & ::ntp::kModeClient) != ::ntp::kModeClient) [[unlikely]] {
        return;
    }

    reply.reference_id = network::GetPrimaryIp();
    reply.origin_timestamp_s = request->transmit_timestamp_s;
    reply.origin_timestamp_f = request->transmit_timestamp_f;

    network::udp::Send(handle, reinterpret_cast<const uint8_t*>(&reply), sizeof(struct ::ntp::Packet), from_ip, from_port);
}

void Print() {
    printf("NTP v%u Server\n", static_cast<unsigned>(::ntp::kVersion >> 3));
    printf(" Port : %u\n", static_cast<unsigned>(network::iana::Ports::kPortNtp));
    printf(" Stratum : %u\n", static_cast<unsigned>(::ntp::kStratum));

    const auto kTime = static_cast<time_t>(static_cast<uint32_t>(time) - ::ntp::kJan1970);

    printf(" %s", asctime(localtime(&kTime)));
}
} // namespace

void Init(uint32_t year, uint32_t month, uint32_t day) {
    LTC_NTP_DEBUG_ENTRY();
    LTC_NTP_DEBUG_PRINTF("year=%u, month=%u, day=%u", static_cast<unsigned>(year), static_cast<unsigned>(month), static_cast<unsigned>(day));

    struct tm time_date;

    memset(&time_date, 0, sizeof(struct tm));
    time_date.tm_year = static_cast<int>(year - 1900);
    time_date.tm_mon = static_cast<int>(month - 1);
    time_date.tm_mday = static_cast<int>(day);

    time = mktime(&time_date);
    assert(time != -1);

    LTC_NTP_DEBUG_PRINTF("time_=%.8x %u", static_cast<unsigned>(time), static_cast<unsigned>(time));

    time += static_cast<time_t>(::ntp::kJan1970);

    LTC_NTP_DEBUG_PRINTF("time_=%.8x %u", static_cast<unsigned>(time), static_cast<unsigned>(time));
    LTC_NTP_DEBUG_EXIT();
}

void Start() {
    LTC_NTP_DEBUG_ENTRY();

    ltc::ntp::Start();

    handle = network::udp::Begin(network::iana::Ports::kPortNtp, Input);
    assert(handle != -1);

    reply.li_vn_mode = ::ntp::kVersion | ::ntp::kModeServer;
    reply.stratum = ::ntp::kStratum;
    reply.poll = ::ntp::kMinpoll;
    reply.precision = static_cast<uint8_t>(-10); // -9.9 = LOG2(0.0001) -> milliseconds
    reply.root_delay = 0;
    reply.root_dispersion = 0;

    Print();

    LTC_NTP_DEBUG_EXIT();
}

void Stop() {
    LTC_NTP_DEBUG_ENTRY();

    network::udp::End(network::iana::Ports::kPortNtp);
    handle = -1;

    ltc::ntp::Stop();

    LTC_NTP_DEBUG_EXIT();
}

void SetTimeCode(const struct ltc::TimeCode* timecode) {
    const auto kType = static_cast<ltc::Type>(timecode->type);

    if (kType == ltc::Type::kUnknown) {
        return;
    }

    time_date = time;
    time_date += timecode->seconds;
    time_date += static_cast<time_t>(timecode->minutes * 60U);
    time_date += static_cast<time_t>(timecode->hours * 60U * 60U);

    if (kType == ltc::Type::kFilm) {
        fraction = static_cast<uint32_t>((178956970.625 * timecode->frames));
    } else if (kType == ltc::Type::kEbu) {
        fraction = static_cast<uint32_t>((171798691.8 * timecode->frames));
    } else if ((kType == ltc::Type::kDf) || (kType == ltc::Type::kSmpte)) {
        fraction = static_cast<uint32_t>((143165576.5 * timecode->frames));
    }

    reply.reference_timestamp_s = __builtin_bswap32(static_cast<uint32_t>(time_date));
    reply.reference_timestamp_f = __builtin_bswap32(fraction);
    reply.receive_timestamp_s = __builtin_bswap32(static_cast<uint32_t>(time_date));
    reply.receive_timestamp_f = __builtin_bswap32(fraction);
    reply.transmit_timestamp_s = __builtin_bswap32(static_cast<uint32_t>(time_date));
    reply.transmit_timestamp_f = __builtin_bswap32(fraction);
}
} // namespace ltc::ntpserver
