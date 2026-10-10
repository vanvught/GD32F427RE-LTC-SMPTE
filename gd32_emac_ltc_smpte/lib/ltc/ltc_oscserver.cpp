/**
 * @file ltc_oscserver.cpp
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

#include <sys/signal.h>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <string_view>

#include "ltc_actions.h"
#include "ltc_commands.h"
#include "network_iface.h"
#include "network_udp.h"
#include "ltc_debug.h"
#include "ltc_udp_port.h"
#include "input/ltc_input.h"
#include "osc.h"
#include "oscsimplemessage.h"

using ::ltc::output::Destination;

namespace ltc::oscserver {
namespace {
int32_t handle{-1};
bool is_started{false};
char path[network::iface::kHostnameSize + 8];
uint32_t path_length{0};

namespace commands {
// Generic
inline constexpr std::string_view kSource{"source/"};
inline constexpr std::string_view kType{"type/"};
inline constexpr std::string_view kUtc{"utc"};
inline constexpr std::string_view kEnable{"enable/"};
inline constexpr std::string_view kDisable{"disable/"};
inline constexpr std::string_view kDirection{"direction/"};
inline constexpr std::string_view kForward{"forward"};
inline constexpr std::string_view kBackward{"backward"};
inline constexpr std::string_view kPitch{"pitch"};
inline constexpr std::string_view kGpsUtc{"gps/utc"};
inline constexpr std::string_view kGps{"gps/"};
inline constexpr std::string_view kGoto{"goto"};
inline constexpr std::string_view kMidiBpm{"midi/bpm"};
inline constexpr std::string_view kMidi{"midi/"};   // start, stop and continue
inline constexpr std::string_view kTCNet{"tcnet/"}; // layer, type and timecode
inline constexpr std::string_view kLayer{"layer/"};
inline constexpr std::string_view kUseTimecide{"timecode/"};
} // namespace commands

bool ValidateUtc(const uint8_t* buffer, uint32_t size, int32_t& hours, uint32_t& minutes) {
    OscSimpleMessage msg(buffer, size);

    if ((msg.GetType(0) != osc::type::kInt32) || (msg.GetType(1) != osc::type::kInt32)) {
        return false;
    }

    const auto kMinutes = msg.GetInt(1);

    if (kMinutes < 0) {
        return false;
    }

    hours = static_cast<int32_t>(msg.GetInt(0));
    minutes = static_cast<uint32_t>(kMinutes);

    return true;
}

void HandleSkip(const uint8_t* buffer, uint32_t size, ltc::actions::Skip skip) {
    OscSimpleMessage msg(buffer, size);

    if (msg.GetType(0) != osc::type::kInt32) {
        return;
    }

    const auto kValue = msg.GetInt(0);
    if ((kValue > 0) && (kValue <= 99)) {
        ltc::actions::SetSkip(skip, static_cast<uint32_t>(kValue));
    }
}

void HandlePitch(const uint8_t* buffer, uint32_t size) {
    OscSimpleMessage msg(buffer, size);

    if (msg.GetType(0) != osc::type::kFloat) {
        return;
    }

    const auto kPitch = msg.GetFloat(0);
    if ((kPitch >= -1.0F) && (kPitch <= 1.0F)) {
        ltc::actions::SetPitch(kPitch);
    }
}

void HandleMidiBpm(const uint8_t* buffer, uint32_t size) {
    uint32_t bpm{0};

    OscSimpleMessage msg(buffer, size);

    if (msg.GetType(0) == osc::type::kFloat) {
        const auto kValue = msg.GetFloat(0);
        if (kValue >= 0) {
            bpm = static_cast<uint32_t>(kValue);
        }
    } else if (msg.GetType(0) == osc::type::kInt32) {
        const auto kValue = msg.GetInt(0);
        if (kValue >= 0) {
            bpm = static_cast<uint32_t>(kValue);
        }
    }

    ltc::actions::midi::SetBpm(bpm);
}

void HandleUtc(const uint8_t* buffer, uint32_t size) {
    int32_t hours;
    uint32_t minutes;

    if (!ValidateUtc(buffer, size, hours, minutes)) {
        return;
    }

    ltc::actions::SetUtcOffset(hours, minutes);
}

void HandleGpsUtc(const uint8_t* buffer, uint32_t size) {
    int32_t hours;
    uint32_t minutes;

    if (!ValidateUtc(buffer, size, hours, minutes)) {
        return;
    }

    ltc::actions::gps::SetUtcOffset(hours, minutes);
}

void HandleTCNet(std::string_view tcnet) {
    if (tcnet.starts_with(commands::kLayer)) {
        tcnet.remove_prefix(commands::kLayer.size());
        ltc::actions::tcnet::SetLayer(tcnet);
        return;
    }

    if (tcnet.starts_with(commands::kType)) {
        tcnet.remove_prefix(commands::kType.size());
        ltc::actions::tcnet::SetType(tcnet);
        return;
    }

    if (tcnet.starts_with(commands::kUseTimecide)) {
        tcnet.remove_prefix(commands::kUseTimecide.size());
        ltc::actions::tcnet::SetUseTimecode(tcnet);
        return;
    }
}

void Input(const uint8_t* buffer, uint32_t size, [[maybe_unused]] uint32_t from_ip, [[maybe_unused]] uint16_t from_port) {
    assert(buffer != nullptr);

    if (size <= 4) {
        return;
    }

    const auto* char_buffer = reinterpret_cast<const char*>(buffer);
    std::string_view request{char_buffer, std::strlen(char_buffer)};

    if (!request.starts_with(path)) {
        return;
    }

    request.remove_prefix(path_length);

    LTC_OSCSERVER_DEBUG_PRINTF("%.*s [%d]", static_cast<int>(request.size()), request.data(), static_cast<int>(request.size()));

    if (request.starts_with(commands::kSource)) {
        request.remove_prefix(commands::kSource.size());
        if (request.empty()) {
            return;
        }
        const auto kInput = ::ltc::InputFromName(request);
        ::ltc::input::Source::Instance().Select(kInput);
        return;
    }

    if (request.starts_with(commands::kType)) {
        request.remove_prefix(commands::kType.size());
        ltc::actions::SetType(request);
        return;
    }

    if (request.starts_with(commands::kUtc)) {
        request.remove_prefix(commands::kUtc.size());
        HandleUtc(buffer, size);
        return;
    }

    if (request.starts_with(ltc::commands::kStart)) {
        request.remove_prefix(ltc::commands::kStart.size());
        ltc::actions::SetStart(request);
        return;
    }

    if (request.starts_with(commands::kGoto)) {
        ltc::actions::SetStart(request);
        return;
    }

    if (request.starts_with(commands::kDirection)) {
        request.remove_prefix(commands::kDirection.size());
        ltc::actions::SetDirection(request);
        return;
    }

    if (request.starts_with(commands::kForward)) {
        HandleSkip(buffer, size, ltc::actions::Skip::kForward);
        return;
    }

    if (request.starts_with(commands::kBackward)) {
        HandleSkip(buffer, size, ltc::actions::Skip::kBackward);
        return;
    }

    if (request.starts_with(commands::kPitch)) {
        HandlePitch(buffer, size);
        return;
    }

    if (request.starts_with(ltc::commands::kStop)) {
        request.remove_prefix(ltc::commands::kStop.size());
        ltc::actions::SetStop(request);
        return;
    }

    if (request.starts_with(ltc::commands::kResume)) {
        request.remove_prefix(ltc::commands::kResume.size());
        ltc::actions::SetResume(request);
        return;
    }

    if (request.starts_with(commands::kEnable)) {
        request.remove_prefix(commands::kEnable.size());
        const auto kEnable = ltc::OutputFromName(request);
        Destination::Instance().Enable(kEnable);
        return;
    }

    if (request.starts_with(commands::kDisable)) {
        request.remove_prefix(commands::kDisable.size());
        const auto kDisable = ltc::OutputFromName(request);
        Destination::Instance().Disable(kDisable);
        return;
    }

    if (request.starts_with(commands::kMidiBpm)) {
        request.remove_prefix(commands::kMidiBpm.size());
        HandleMidiBpm(buffer, size);
        return;
    }

    if (request.starts_with(commands::kMidi)) {
        request.remove_prefix(commands::kMidi.size());
        ltc::actions::midi::HandleAction(request);
        return;
    }

    if (request.starts_with(commands::kGpsUtc)) {
        request.remove_prefix(commands::kGpsUtc.size());
        HandleGpsUtc(buffer, size);
        return;
    }

    if (request.starts_with(commands::kGps)) {
        request.remove_prefix(commands::kGps.size());
        ltc::actions::gps::HandleAction(request);
        return;
    }

    if (request.starts_with(commands::kTCNet)) {
        request.remove_prefix(commands::kTCNet.size());
        HandleTCNet(request);
        return;
    }
}

} // namespace
void Start() {
    if (is_started) {
        return;
    }

    is_started = true;

    path_length = static_cast<uint32_t>(snprintf(path, sizeof(path), "/%s/tc/", network::iface::HostName()));
    assert(path_length < sizeof(path));

    assert(handle == -1);
    handle = ::network::udp::Begin(::ltc::udp::port::kOsc, Input);
    assert(handle != -1);
}

void Stop() {
    if (!is_started) {
        return;
    }

    is_started = false;

    assert(handle != -1);
    ::network::udp::End(::ltc::udp::port::kOsc);
    handle = -1;
}
} // namespace ltc::oscserver
