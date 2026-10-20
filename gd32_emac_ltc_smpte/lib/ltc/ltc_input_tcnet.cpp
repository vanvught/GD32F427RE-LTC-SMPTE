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

#include <cstddef>
#include <type_traits>

#include "input/ltc_input.h"
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
    input::tcnet::DisplayLayerInfo();
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
        input::tcnet::DisplayLayerInfo();
    }
}

void SetUseTimecode(std::string_view use_timecode) {
    if (use_timecode.size() != 1) {
        LTC_INPUT_DEBUG_EXIT();
        return;
    }

    const auto kUse = use_timecode.front() == 'y';
    ::tcnet::SetUseTimeCode(kUse);

    input::tcnet::DisplayLayerInfo();
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

void DisplayLayerInfo() {
    constexpr char kFps[4][6] = {" T24", " T25", " T29", " T30"};
    static_assert(kDisplayInputLine >= 1);

    Display::Get()->SetCursorPos(6, input::kDisplayInputLine - 1);
    Display::Get()->PutChar('L');
    Display::Get()->PutChar(::tcnet::LayerToChar(::tcnet::Layer()));

    if (::tcnet::IsUseTimeCode()) {
        Display::Get()->PutString(" TC ");
    } else {
        const auto kIndex = static_cast<uint32_t>(::tcnet::TimeCodeType());
        if (kIndex < sizeof(kFps) / sizeof(kFps[0])) {
            Display::Get()->PutString(kFps[kIndex]);
        }
    }
}
} // namespace ltc::input::tcnet

#define ASSERT_TIMECODE_LAYOUT(Type)                \
    static_assert(std::is_standard_layout_v<Type>); \
    static_assert(sizeof(Type) == 5);               \
    static_assert(offsetof(Type, frames) == 0);     \
    static_assert(offsetof(Type, seconds) == 1);    \
    static_assert(offsetof(Type, minutes) == 2);    \
    static_assert(offsetof(Type, hours) == 3);      \
    static_assert(offsetof(Type, type) == 4)

ASSERT_TIMECODE_LAYOUT(ltc::TimeCode);
ASSERT_TIMECODE_LAYOUT(tcnet::Timecode);

namespace tcnet {
void Handle([[maybe_unused]] const tcnet::Timecode* timecode_tcnet) {
    ltc::TimeCode timecode;
    static_assert(sizeof(timecode) == 5);
    std::memcpy(&timecode, &timecode_tcnet->frames, sizeof(timecode));
    ltc::output::Destination::Instance().Distribute(&timecode);
}
} // namespace tcnet