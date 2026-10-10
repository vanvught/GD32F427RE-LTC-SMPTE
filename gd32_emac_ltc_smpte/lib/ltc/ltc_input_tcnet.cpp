/**
 * @file ltc_input_tcnet.cpp
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

#include "ltc.h"
#include "ltc_commands.h"
#include "output/ltc_output.h"
#include "ltc_debug.h"
#include "tcnet.h"
#include "tcnet_timecode.h"
#include "ltc_actions.h"
#include "network_udp.h"
#include "ltc_udp_port.h"

namespace ltc::actions::tcnet {
void SetLayer(std::string_view layer) {
    if (layer.size() != 1) {
        return;
    }

    const auto kLayer = ::tcnet::LayerFromChar(layer.front());

    if (kLayer == ::tcnet::Layer::kLayerUndefined) {
        return;
    }

    ::tcnet::SetLayer(kLayer);
    // update Display
}

void SetType(std::string_view type) {
    if (type.size() != 2) {
        return;
    }

    auto is_valid{false};
    const auto kValue = common::Atoi(type);

    switch (kValue) {
        case 24:
            ::tcnet::SetTimeCodeType(::tcnet::TimeCodeType::kFilm);
            is_valid = true;
            break;

        case 25:
            ::tcnet::SetTimeCodeType(::tcnet::TimeCodeType::kEbu25Fps);
            is_valid = true;
            break;

        case 29:
            ::tcnet::SetTimeCodeType(::tcnet::TimeCodeType::kDf);
            is_valid = true;
            break;

        case 30:
            ::tcnet::SetTimeCodeType(::tcnet::TimeCodeType::kSmpte30Fps);
            is_valid = true;
            break;

        default:
            break;
    }

    if (is_valid) {
        // update Display
    }
}

void SetUseTimecode(std::string_view use_timecode) {
    if (use_timecode.size() != 1) {
        return;
    }

    const auto kUse = use_timecode.front() == 'y';
    ::tcnet::SetUseTimeCode(kUse);
}

void HandleAction(std::string_view action) {
    if (action.starts_with(ltc::commands::kLayer)) {
        action.remove_prefix(ltc::commands::kLayer.size());
        SetLayer(action);
        return;
    }

    if (action.starts_with(ltc::commands::kType)) {
        action.remove_prefix(ltc::commands::kType.size());
        SetType(action);
        return;
    }

    if (action.starts_with(ltc::commands::kTimecode)) {
        action.remove_prefix(ltc::commands::kTimecode.size());
        SetUseTimecode(action);
        return;
    }
}
} // namespace ltc::actions::tcnet

namespace ltc::input::tcnet {
namespace {
bool is_started{false};
int32_t handle{-1};

constexpr std::string_view kTcnet{"tcnet!"};

void Input(const uint8_t* buffer, uint32_t size, [[maybe_unused]] uint32_t from_ip, [[maybe_unused]] uint16_t from_port) {
    assert(buffer != nullptr);

    std::string_view request{reinterpret_cast<const char*>(buffer), size};

    if (!request.starts_with(kTcnet)) {
        return;
    }

    request.remove_prefix(kTcnet.size());

    actions::tcnet::HandleAction(request);
}
} // namespace

void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    if (is_started) {
        LTC_INPUT_DEBUG_EXIT();
        return;
    }

    is_started = true;

    output::Destination::Instance().SetType(::ltc::Type::kUnknown);

    ::tcnet::Start();

    assert(handle == -1);
    handle = ::network::udp::Begin(::ltc::udp::port::kTCNet, Input);
    assert(handle != -1);

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    if (!is_started) {
        LTC_INPUT_DEBUG_EXIT();
        return;
    }

    is_started = false;

    assert(handle != -1);
    ::network::udp::End(::ltc::udp::port::kTCNet);
    handle = -1;

    ::tcnet::Stop();

    LTC_INPUT_DEBUG_EXIT();
}
} // namespace ltc::input::tcnet

namespace tcnet {
void Handle([[maybe_unused]] const tcnet::Timecode* timecode) {
    ltc::output::Destination::Instance().Distribute(reinterpret_cast<const ::ltc::TimeCode*>(timecode));
}
} // namespace tcnet