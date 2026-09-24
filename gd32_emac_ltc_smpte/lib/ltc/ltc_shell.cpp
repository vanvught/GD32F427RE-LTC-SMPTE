/**
 * @file shell.cpp
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
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdint>

#include "gnss.h"
#include "shell.h"

#include "common/utils/utils_string.h"

#include "input/ltc_input.h"
#include "output/ltc_output.h"

#include "ltc.h"
#include "ltc_display_max7219.h"
#include "ltc_gps.h"
#include "uart0.h"

namespace shell::ltc {

using ::ltc::input::Source;

namespace {

constexpr int32_t kMaxType = 30;
constexpr int32_t kMaxIntensity = 15;
constexpr auto kMaxOutput = static_cast<int32_t>(sizeof(::ltc::kOutputs) / sizeof(::ltc::kOutputs[0]));

} // namespace

void Input(Arguments args) {
    if (args.size() == 1) {
        const auto kInput = Source::Instance().Input();

        uart0::Printf("Source: %s\n\n", ::ltc::InputToName(kInput));
        return;
    }

    if (args.size() != 2) {
        uart0::Puts(common::kUnknown);
        return;
    }

    const auto kInput = ::ltc::InputFromName(args[1]);

    uart0::Printf("Source: %s\n\n", ::ltc::InputToName(kInput));
    Source::Instance().Select(kInput);
}

void Type(Arguments args) {
    if (args.size() != 2) {
        uart0::Puts(common::kUnknown);
        return;
    }

    const auto kValue = shell::GetValue(args[1], kMaxType);

    if (!kValue) {
        uart0::Puts(common::kUnknown);
        return;
    }

    switch (*kValue) {
        case 24:
            ::ltc::output::Destination::Instance().SetType(::ltc::Type::kFilm);
            return;

        case 25:
            ::ltc::output::Destination::Instance().SetType(::ltc::Type::kEbu);
            return;

        case 29:
            ::ltc::output::Destination::Instance().SetType(::ltc::Type::kDf);
            return;

        case 30:
            ::ltc::output::Destination::Instance().SetType(::ltc::Type::kSmpte);
            return;

        default:
            uart0::Puts(common::kUnknown);
            return;
    }
}

void UtcOffset(Arguments args) {
    if (args.size() == 1) {
        const auto kSeconds = ::ltc::output::Destination::Instance().UtcOffset();

        int32_t hours;
        uint32_t minutes;
        utc::SplitOffset(kSeconds, hours, minutes);

        uart0::Printf("UTC offset: %.2d:%.2u\n\n", static_cast<signed>(hours), static_cast<unsigned>(minutes));
        return;
    }

    if (args.size() == 2) {
        int32_t hours;
        uint32_t minutes;

        if (utc::ParseOffset(args[1].data(), args[1].size(), hours, minutes)) {
            ::ltc::output::Destination::Instance().SetUtcOffset(hours, minutes);
        } else {
            uart0::Puts(common::kUndefined);
        }

        return;
    }

    uart0::Puts(common::kUnknown);
}

void Enable(Arguments args) {
    if (args.size() != 2) {
        uart0::Puts(common::kUnknown);
        return;
    }

    const auto kValue = shell::GetValue(args[1], kMaxOutput);

    if (!kValue) {
        uart0::Puts(common::kUnknown);
        return;
    }

    ::ltc::output::Destination::Instance().Enable(static_cast<::ltc::Output>(1U << *kValue));
}

void Disable(Arguments args) {
    if (args.size() != 2) {
        uart0::Puts(common::kUnknown);
        return;
    }

    const auto kValue = shell::GetValue(args[1], kMaxOutput);

    if (!kValue) {
        uart0::Puts(common::kUnknown);
        return;
    }

    ::ltc::output::Destination::Instance().Disable(static_cast<::ltc::Output>(1U << *kValue));
}

void Intensity(Arguments args) {
    if (args.size() != 2) {
        uart0::Puts(common::kUnknown);
        return;
    }

    const auto kValue = shell::GetValue(args[1], kMaxIntensity);

    if (!kValue) {
        uart0::Puts(common::kUnknown);
        return;
    }

    ::ltc::display::max7219::SetIntensity(static_cast<uint8_t>(*kValue));
}

void Gps(Arguments args) {
    if (args.size() == 2) {
        if (args[1] == "start") {
            ::ltc::gps::Start();
            return;
        }

        if (args[1] == "stop") {
            ::ltc::gps::Stop();
            return;
        }

        if (args[1] == "utc") {
            const auto kSeconds = gnss::Receiver::Instance().UtcOffset();

            int32_t hours;
            uint32_t minutes;
            utc::SplitOffset(kSeconds, hours, minutes);

            uart0::Printf("UTC offset: %.2d:%.2u\n\n", static_cast<signed>(hours), static_cast<unsigned>(minutes));
            return;
        }
    }

    if ((args.size() == 3) && (args[1] == "utc")) {
        int32_t hours;
        uint32_t minutes;

        if (utc::ParseOffset(args[2].data(), args[2].size(), hours, minutes)) {
            gnss::Receiver::Instance().SetUtcOffset(hours, minutes);
        } else {
            uart0::Puts(common::kUndefined);
        }

        return;
    }

    uart0::Puts(common::kUnknown);
}

} // namespace shell::ltc