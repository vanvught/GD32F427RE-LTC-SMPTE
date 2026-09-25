/**
 * @file ltc_output.h
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

#ifndef OUTPUT_LTC_OUTPUT_H_
#define OUTPUT_LTC_OUTPUT_H_

#include <cassert>
#include <cstdint>
#include <cstring>
#include <bit>
#include <utility>
#include <sys/time.h>

#include "firmware/utc.h"
#include "ltc_display_pixel.h"
#include "ltc_display_max7219.h"
#include "ltc_display_oled.h"
#include "output/ltc_output_applemidi.h"
#include "output/ltc_output_artnet.h"
#include "output/ltc_output_etc.h"
#include "output/ltc_output_ltc.h"
#include "output/ltc_output_midi.h"
#include "ltc_ntpserver.h"
#include "ltc.h"
#include "display.h"
#include "board_statusled.h"
#include "firmware/debug/debug_printbits.h"
#include "ltc_debug.h"

namespace ltc::global {
extern ::ltc::TimeCode timecode_running;
} // namespace ltc::global

namespace ltc::output {
enum class Direction { kForward, kBackward };
enum class Pitch { kNormal, kFaster, kSlower };

struct Entry {
    void (*start)();
    void (*stop)();
};

[[nodiscard]] constexpr uint32_t OutputToIndex(const Output kOutput) {
    return static_cast<uint32_t>(std::countr_zero(static_cast<uint16_t>(kOutput)));
}

template <uint32_t kN>
struct EntryTable {
    Entry entries[kN]{};

    constexpr Entry& operator[](const ::ltc::Output kOutput) { return entries[OutputToIndex(kOutput)]; }

    constexpr const Entry& operator[](const ::ltc::Output kOutput) const { return entries[OutputToIndex(kOutput)]; }
};

consteval uint32_t OutputTableSize() {
    uint32_t size = 0;

    for (const auto kOutput : kOutputs) {
        const auto kIndex = static_cast<uint32_t>(std::countr_zero(static_cast<uint16_t>(kOutput)));

        if ((kIndex + 1U) > size) {
            size = kIndex + 1U;
        }
    }

    return size;
}

inline constexpr auto kOutputTableSize = OutputTableSize();

inline void Nothing() {}

constexpr auto MakeCommandTable() {
    EntryTable<kOutputTableSize> table{};

    table[::ltc::Output::kLtc] = {
        .start = ::ltc::output::ltc::Start,
        .stop = ::ltc::output::ltc::Stop,
    };

    table[::ltc::Output::kArtnet] = {
        .start = ::ltc::output::artnet::Start,
        .stop = ::ltc::output::artnet::Stop,
    };

    table[::ltc::Output::kMidi] = {
        .start = ::ltc::output::midi::Start,
        .stop = ::ltc::output::midi::Stop,
    };

    table[::ltc::Output::kApplemidi] = {
        .start = ::ltc::output::applemidi::Start,
        .stop = ::ltc::output::applemidi::Stop,
    };

    table[::ltc::Output::kEtc] = {
        .start = ::ltc::output::etc::Start,
        .stop = ::ltc::output::etc::Stop,
    };

    table[::ltc::Output::kNtpServer] = {
        .start = ntpserver::Start,
        .stop = ntpserver::Stop,
    };

    table[::ltc::Output::kDisplayOled] = {
        .start = ::ltc::display::oled::Start,
        .stop = ::ltc::display::oled::Stop,
    };

    table[::ltc::Output::kMaX7219] = {
        .start = ::ltc::display::max7219::Start,
        .stop = Nothing,
    };

    table[::ltc::Output::kPixel] = {
        .start = ::ltc::display::pixel::Start,
        .stop = ::ltc::display::pixel::Stop,
    };

    return table;
}

inline constexpr auto kCommandtab = MakeCommandTable();

class Destination {
   public:
    Destination();

    void StartEnabled();

    void Update(const ::ltc::Input kInput) {
        LTC_OUTPUT_DEBUG_ENTRY();
        LTC_OUTPUT_DEBUG_PRINTF("input=%s", ::ltc::InputToName(kInput));

        debug::PrintBits(disabled_requested_, "disabled_requested_");

        const auto kOutput = static_cast<::ltc::Output>(1U << static_cast<uint16_t>(kInput));

        disabled_current_ = disabled_requested_;
        disabled_current_ |= static_cast<uint16_t>(kOutput);

        const auto& entry = kCommandtab[kOutput];

        if (entry.stop != nullptr) {
            entry.stop();
        }

        debug::PrintBits(disabled_current_, "disabled_current_");

        LTC_OUTPUT_DEBUG_EXIT();
    }

    void Disable(::ltc::Output output) {
        LTC_OUTPUT_DEBUG_ENTRY();
        debug::PrintBits(disabled_current_, "disabled_current_");

        const auto& entry = kCommandtab[output];

        if (entry.stop != nullptr) {
            disabled_requested_ |= static_cast<uint16_t>(output);
            disabled_current_ = disabled_requested_;

            entry.stop();
        }

        debug::PrintBits(disabled_current_, "disabled_current_");
        LTC_OUTPUT_DEBUG_EXIT();
    }

    void Enable(::ltc::Output output) {
        LTC_OUTPUT_DEBUG_ENTRY();
        debug::PrintBits(disabled_current_, "disabled_current_");

        const auto& entry = kCommandtab[output];

        if (entry.start != nullptr) {
            disabled_requested_ &= ~static_cast<uint16_t>(output);
            disabled_current_ = disabled_requested_;

            entry.start();
        }

        debug::PrintBits(disabled_current_, "disabled_current_");
        LTC_OUTPUT_DEBUG_EXIT();
    }

    void SetType(::ltc::Type type);
    [[nodiscard]] ::ltc::Type Type() const;

    void SetUtcOffset(int32_t hours, uint32_t minutes) {
        int32_t utc_offset_seconds;
        if (!utc::ValidateOffset(hours, minutes, utc_offset_seconds)) {
            return;
        }
        utc_offset_ = utc_offset_seconds;
    }
    [[nodiscard]] int32_t UtcOffset() const { return utc_offset_; }

    void Distribute(const ::ltc::TimeCode* timecode) {
        global::updates = global::updates + 1;

        if (IsEnabled(::ltc::Output::kLtc)) {
            ltc::Output(timecode);
        }

        if (IsEnabled(::ltc::Output::kArtnet)) {
            artnet::Output(timecode);
        }

        DistributeInternal(timecode);
    }

    void DisplayType() const {
        LTC_OUTPUT_DEBUG_ENTRY();

        const auto kType = static_cast<::ltc::Type>(global::timecode_running.type);

        LTC_OUTPUT_DEBUG_PRINTF("global::timecode.type=%u", static_cast<unsigned>(kType));

        if (IsEnabled(::ltc::Output::kDisplayOled)) {
            Display::Get()->TextLine(2, ::ltc::TypeToName(kType), kTypeMaxNameLength - 1);
        }

        LTC_OUTPUT_DEBUG_EXIT();
    }

    void Reset() {
        LTC_OUTPUT_DEBUG_ENTRY();

        DisplayTimecodeInit();
        DisplayType();

        if (IsEnabled(::ltc::Output::kDisplayOled)) {
            Display::Get()->TextLine(1, timecode_, timecode::kCodeMaxLength);
        }

        if (IsEnabled(::ltc::Output::kMaX7219)) {
            ::ltc::display::max7219::Show(timecode_);
        }

        if (IsEnabled(::ltc::Output::kPixel)) {
            ::ltc::display::pixel::Show(timecode_);
        }

        type_previous_ = static_cast<uint8_t>(::ltc::Type::kInvalid);

        LTC_OUTPUT_DEBUG_EXIT();
    }

    void RunOutput() {
        if (global::updates_per_second > 10) {
            board::statusled::SetMode(board::statusled::Mode::kData);
        } else {
            board::statusled::SetMode(board::statusled::Mode::kNormal);
        }
    }

    [[nodiscard]] bool IsDisabled(::ltc::Output output) const { return (disabled_current_ & static_cast<uint16_t>(output)) == static_cast<uint16_t>(output); };

    [[nodiscard]] bool IsEnabled(::ltc::Output output) const { return !IsDisabled(output); }

    static Destination& Instance() {
        assert(s_this != nullptr);
        return *s_this;
    }

   private:
    void DisplayTimecodeInit() {
        memset(reinterpret_cast<void*>(timecode_), ' ', sizeof(timecode_));
        timecode_[::ltc::timecode::index::kColon1] = ':';
        timecode_[::ltc::timecode::index::kColon2] = ':';
        timecode_[::ltc::timecode::index::kColon3] = ':';
    }

    void DistributeInternal(const ::ltc::TimeCode* timecode);

    uint16_t disabled_current_{std::to_underlying(Output::kPixel)};
    uint16_t disabled_requested_{std::to_underlying(Output::kPixel)};

    uint8_t type_previous_{static_cast<uint8_t>(::ltc::Type::kInvalid)};
    int32_t utc_offset_{0};

    char timecode_[timecode::kCodeMaxLength];

    Direction direction_{Direction::kForward};
    Pitch pitch_{Pitch::kNormal};

    inline static Destination* s_this;
};
} // namespace ltc::output

#endif // OUTPUT_LTC_OUTPUT_H_
