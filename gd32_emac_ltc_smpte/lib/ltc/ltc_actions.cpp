/**
 * @file ltc_actions.cpp
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
#include <string_view>

#include "ltc_actions.h"
#include "common/utils/utils_string.h"
#include "input/ltc_input.h"
#include "input/ltc_input_internal.h"
#include "ltc.h"
#include "output/ltc_output.h"
#include "ltc_commands.h"
#include "ltc_debug.h"
#include "ltc_udp_port.h"
#include "network_udp.h"

using ::ltc::output::Destination;

namespace ltc::actions {
namespace {
bool Parse(std::string_view timecode_sv, const char delimiters[3], ltc::TimeCode& timecode) {
    if (timecode_sv.size() != ltc::timecode::kCodeMaxLength) {
        return false;
    }

    const auto kHours = common::Atoi(timecode_sv.substr(0, 2));
    if ((kHours < 0) || (kHours >= 24)) {
        return false;
    }

    timecode_sv.remove_prefix(2);

    if (!timecode_sv.starts_with(delimiters[0])) {
        return false;
    }

    timecode_sv.remove_prefix(1);

    const auto kMinutes = common::Atoi(timecode_sv.substr(0, 2));
    if ((kMinutes < 0) || (kMinutes >= 60)) {
        return false;
    }

    timecode_sv.remove_prefix(2);

    if (!timecode_sv.starts_with(delimiters[1])) {
        return false;
    }

    timecode_sv.remove_prefix(1);

    const auto kSeconds = common::Atoi(timecode_sv.substr(0, 2));
    if ((kSeconds < 0) || (kSeconds >= 60)) {
        return false;
    }

    timecode_sv.remove_prefix(2);

    if (!timecode_sv.starts_with(delimiters[2])) {
        return false;
    }

    timecode_sv.remove_prefix(1);

    const auto kFrames = common::Atoi(timecode_sv.substr(0, 2));
    if ((kFrames < 0) || (kFrames >= 30)) {
        return false;
    }

    timecode.hours = static_cast<uint8_t>(kHours);
    timecode.minutes = static_cast<uint8_t>(kMinutes);
    timecode.seconds = static_cast<uint8_t>(kSeconds);
    timecode.frames = static_cast<uint8_t>(kFrames);

    return true;
}

bool ParseUdp(std::string_view timecode_sv, ltc::TimeCode& timecode) {
    constexpr char kDelimiters[] = {':', ':', '.'};
    return Parse(timecode_sv, kDelimiters, timecode);
}

bool ParseOsc(std::string_view timecode_sv, ltc::TimeCode& timecode) {
    constexpr char kDelimiters[] = {'/', '/', '/'};
    return Parse(timecode_sv, kDelimiters, timecode);
}

bool ParseCommand(std::string_view command, std::string_view udp_prefix, std::string_view osc_prefix, ltc::TimeCode& timecode) {
    if (command.starts_with(udp_prefix)) {
        command.remove_prefix(udp_prefix.size());
        return ParseUdp(command, timecode);
    }

    if (command.starts_with(osc_prefix)) {
        command.remove_prefix(osc_prefix.size());
        return ParseOsc(command, timecode);
    }

    return false;
}

void HandleSkip(Skip skip, std::string_view skip_sv) {
    if (skip_sv.empty() || skip_sv.size() > 2) {
        return;
    }

    const auto kSeconds = common::Atoi(skip_sv);
    if (kSeconds > 0) {
        SetSkip(skip, static_cast<uint32_t>(kSeconds));
    }
}
} // namespace

void SetType(std::string_view type) {
    if (type.size() != 2) {
        return;
    }

    auto is_valid{false};
    const auto kValue = common::Atoi(type.data(), type.size());

    switch (kValue) {
        case 24:
            Destination::Instance().SetType(::ltc::Type::kFilm);
            is_valid = true;
            break;

        case 25:
            Destination::Instance().SetType(::ltc::Type::kEbu);
            is_valid = true;
            break;

        case 29:
            Destination::Instance().SetType(::ltc::Type::kDf);
            is_valid = true;
            break;

        case 30:
            Destination::Instance().SetType(::ltc::Type::kSmpte);
            is_valid = true;
            break;

        default:
            break;
    }

    if (is_valid) {
        const auto kInput = ::ltc::input::Source::Instance().Input();
        if ((kInput == Input::kInternal) || (kInput == Input::kSystime)) {
            Destination::Instance().DisplayType(Destination::Instance().Type());
        }
    }
}

void SetStart(std::string_view start) {
    const auto kInput = ::ltc::input::Source::Instance().Input();

    if (start.empty()) {
        switch (kInput) {
            case Input::kLtc:
            case Input::kArtnet:
            case Input::kMidi:
            case Input::kTcnet:
                break;
            case Input::kInternal:
                input::internal::Start();
                break;
            case Input::kApplemidi:
            case Input::kUsbmidi:
                break;
            case Input::kSystime:
                input::systime::Start();
                break;
            case Input::kEtc:
            case Input::kUndefined:
                break;
        }

        return;
    }

    if (kInput != Input::kInternal) {
        return;
    }

    ltc::TimeCode timecode{};
    LTC_DEBUG_PRINTF("%.*s", static_cast<int>(start.size()), start.data());

    if (ParseCommand(start, commands::udp::kSet, commands::osc::kSet, timecode)) {
        input::internal::SetStart(timecode);
        return;
    }

    if (ParseCommand(start, commands::udp::kGoto, commands::osc::kGoto, timecode)) {
        input::internal::SetGoto(timecode);
        return;
    }

    if (ParseCommand(start, commands::udp::kRunning, commands::osc::kRunning, timecode)) {
        input::internal::SetRunning(timecode);
    }
}

void SetStop(std::string_view stop) {
    const auto kInput = ::ltc::input::Source::Instance().Input();

    if (stop.empty()) {
        switch (kInput) {
            case Input::kLtc:
            case Input::kArtnet:
            case Input::kMidi:
            case Input::kTcnet:
                break;
            case Input::kInternal:
                input::internal::Stop();
                break;
            case Input::kApplemidi:
            case Input::kUsbmidi:
            case Input::kSystime:
                input::systime::Stop();
                break;
            case Input::kEtc:
            case Input::kUndefined:
                break;
        }

        return;
    }

    if (kInput != Input::kInternal) {
        return;
    }

    ltc::TimeCode timecode{};
    auto set{false};

    if (stop.starts_with(commands::udp::kSet)) {
        stop.remove_prefix(commands::udp::kSet.size());
        set = ParseUdp(stop, timecode);
    }

    if (stop.starts_with(commands::osc::kSet)) {
        stop.remove_prefix(commands::osc::kSet.size());
        set = ParseOsc(stop, timecode);
    }

    if (set) {
        input::internal::SetStop(timecode);
    }
}

void SetResume(std::string_view resume) {
    if (resume.empty()) {
        const auto kInput = ::ltc::input::Source::Instance().Input();

        switch (kInput) {
            case Input::kLtc:
            case Input::kArtnet:
            case Input::kMidi:
            case Input::kTcnet:
                break;
            case Input::kInternal:
                input::internal::Resume();
                break;
            case Input::kApplemidi:
            case Input::kUsbmidi:
            case Input::kSystime:
            case Input::kEtc:
            case Input::kUndefined:
                break;
        }

        return;
    }
}

void SetDirection(std::string_view direction) {
    const auto kInput = ::ltc::input::Source::Instance().Input();

    if (kInput != Input::kInternal) {
        return;
    }

    if (direction == ltc::commands::kDirectionForward) {
        input::internal::SetDirection(input::internal::Direction::kForward);
        LTC_DEBUG_EXIT();
        return;
    }

    if (direction == ltc::commands::kDirectionBackward) {
        input::internal::SetDirection(input::internal::Direction::kBackward);
        LTC_DEBUG_EXIT();
        return;
    }
}

void SetSkip(Skip skip, uint32_t seconds) {
    const auto kInput = ::ltc::input::Source::Instance().Input();

    if (kInput != Input::kInternal) {
        return;
    }

    if (skip == ltc::actions::Skip::kForward) {
        ltc::input::internal::SetForward(seconds);
        return;
    }

    ltc::input::internal::SetBackward(seconds);
}

void HandleAction(std::string_view action) {
    if (action.starts_with(ltc::commands::kSource)) {
        action.remove_prefix(ltc::commands::kSource.size());

        if (!action.empty()) {
            const auto kInput = ::ltc::InputFromName(action);
            ::ltc::input::Source::Instance().Select(kInput);
        }
        return;
    }

    if (action.starts_with(ltc::commands::kRate)) {
        action.remove_prefix(ltc::commands::kRate.size());

        if (action.size() == 2) {
            SetType(action);
        }
        return;
    }

    if (action.starts_with(ltc::commands::kType)) {
        action.remove_prefix(ltc::commands::kType.size());
        SetType(action);
        return;
    }

    if (action.starts_with(ltc::commands::kStart)) {
        action.remove_prefix(ltc::commands::kStart.size());
        SetStart(action);
        return;
    }

    if (action.starts_with(ltc::commands::kStop)) {
        action.remove_prefix(ltc::commands::kStop.size());
        SetStop(action);
        return;
    }

    if (action.starts_with(ltc::commands::kResume)) {
        action.remove_prefix(ltc::commands::kResume.size());
        SetResume(action);
        return;
    }

    if (action.starts_with(ltc::commands::kForward)) {
        action.remove_prefix(ltc::commands::kForward.size());
        HandleSkip(Skip::kForward, action);
        return;
    }

    if (action.starts_with(ltc::commands::kBackward)) {
        action.remove_prefix(ltc::commands::kBackward.size());
        HandleSkip(Skip::kBackward, action);
        return;
    }

    if (action.starts_with(ltc::commands::kDirection)) {
        action.remove_prefix(ltc::commands::kDirection.size());
        SetDirection(action);
        return;
    }

    if (action.starts_with(ltc::commands::kEnable)) {
        action.remove_prefix(ltc::commands::kEnable.size());
        const auto kEnable = ltc::OutputFromName(action);
        Destination::Instance().Enable(kEnable);
        return;
    }

    if (action.starts_with(ltc::commands::kDisable)) {
        action.remove_prefix(ltc::commands::kDisable.size());
        const auto kDisable = ltc::OutputFromName(action);
        Destination::Instance().Disable(kDisable);
        return;
    }
}

namespace udp {
namespace {
int32_t handle{-1};

void Input(const uint8_t* buffer, uint32_t size, [[maybe_unused]] uint32_t from_ip, [[maybe_unused]] uint16_t from_port) {
    assert(buffer != nullptr);

    std::string_view request{reinterpret_cast<const char*>(buffer), size};

    if (!request.starts_with("ltc!")) {
        LTC_DEBUG_EXIT();
        return;
    }

    request.remove_prefix(4);

    HandleAction(request);
}
} // namespace

void Start() {
    LTC_DEBUG_ENTRY();

    if (handle != -1) {
        ::network::udp::End(::ltc::udp::port::kLtc);
    }

    handle = ::network::udp::Begin(::ltc::udp::port::kLtc, Input);
    assert(handle != -1);

    LTC_DEBUG_EXIT();
}

void Stop() {
    LTC_DEBUG_ENTRY();

    if (handle != -1) {
        ::network::udp::End(::ltc::udp::port::kLtc);
        handle = -1;
    }

    LTC_DEBUG_EXIT();
}
} // namespace udp
} // namespace ltc::actions
