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
#include "input/ltc_input.h"
#include "input/ltc_input_internal.h"
#include "ltc.h"
#include "output/ltc_output.h"
#include "output/ltc_output_internal.h"
#include "ltc_commands.h"
#include "ltc_debug.h"
#include "ltc_udp_port.h"
#include "network_udp.h"

using ::ltc::output::Destination;

namespace ltc::actions {
namespace {
void SetEnable(std::string_view enable) {
    const auto kEnable = ltc::OutputFromName(enable);
    Destination::Instance().Enable(kEnable);
}

void SetDisable(std::string_view disable) {
    const auto kDisable = ltc::OutputFromName(disable);
    Destination::Instance().Disable(kDisable);
}

void SetSource(std::string_view source) {
    const auto kInput = ::ltc::InputFromName(source);
    ::ltc::input::Source::Instance().Select(kInput);
}

bool SetType(std::string_view type) {
    const auto kValue = common::Atoi(type.data(), type.size());

    switch (kValue) {
        case 24:
            Destination::Instance().SetType(::ltc::Type::kFilm);
            return true;

        case 25:
            Destination::Instance().SetType(::ltc::Type::kEbu);
            return true;

        case 29:
            Destination::Instance().SetType(::ltc::Type::kDf);
            return true;

        case 30:
            Destination::Instance().SetType(::ltc::Type::kSmpte);
            return true;

        default:
            return false;
    }
}

void SetStart(std::string_view start) {
    LTC_DEBUG_ENTRY();

    if (start.empty()) {
        const auto kInput = ::ltc::input::Source::Instance().Input();

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
            case Input::kSystime:
            case Input::kEtc:
            case Input::kUndefined:
                break;
        }

        LTC_DEBUG_EXIT();
        return;
    }

    LTC_DEBUG_EXIT();
}

void SetStop(std::string_view stop) {
    LTC_DEBUG_ENTRY();

    if (stop.empty()) {
        const auto kInput = ::ltc::input::Source::Instance().Input();

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
            case Input::kEtc:
            case Input::kUndefined:
                break;
        }

        LTC_DEBUG_EXIT();
        return;
    }

    LTC_DEBUG_EXIT();
}

void SetDirection(std::string_view direction) {
    LTC_DEBUG_ENTRY();

    if (direction == ltc::commands::kDirectionForward) {
        ltc::output::internal::SetDirection(::ltc::output::Direction::kForward);
        LTC_DEBUG_EXIT();
        return;
    }

    if (direction == ltc::commands::kDirectionBackward) {
        ltc::output::internal::SetDirection(::ltc::output::Direction::kBackward);
        LTC_DEBUG_EXIT();
        return;
    }

    LTC_DEBUG_EXIT();
}
} // namespace

void HandleAction(std::string_view action) {
    if (action.starts_with(ltc::commands::kSource)) {
        action.remove_prefix(ltc::commands::kSource.size());

        if (!action.empty()) {
            SetSource(action);
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

    if (action.starts_with(ltc::commands::kDirection)) {
        action.remove_prefix(ltc::commands::kDirection.size());
        SetDirection(action);
        return;
    }

    if (action.starts_with(ltc::commands::kEnable)) {
        action.remove_prefix(ltc::commands::kEnable.size());
        SetEnable(action);
        return;
    }

    if (action.starts_with(ltc::commands::kDisable)) {
        action.remove_prefix(ltc::commands::kDisable.size());
        SetDisable(action);
        return;
    }
}

namespace udp {
namespace {
int32_t handle = -1;

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
