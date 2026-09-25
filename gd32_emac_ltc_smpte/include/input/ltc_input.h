/**
 * @file ltc_input.h
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

#ifndef INPUT_LTC_INPUT_H_
#define INPUT_LTC_INPUT_H_

#include <cassert>
#include <cstdint>

#include "input/ltc_input_applemidi.h"
#include "input/ltc_input_artnet.h"
#include "input/ltc_input_etc.h"
#include "input/ltc_input_internal.h"
#include "input/ltc_input_ltc.h"
#include "input/ltc_input_midi.h"
#include "input/ltc_input_systime.h"
#include "input/ltc_input_tcnet.h"
#include "ltc.h"
#include "ltc_debug.h"
#include "display.h"
#include "output/ltc_output.h"

namespace ltc::input {
struct Entry {
    void (*start)();
    void (*stop)();
};

template <uint32_t kN>
struct EntryTable {
    Entry entries[kN]{};

    constexpr Entry& operator[](const ::ltc::Input kInput) { return entries[static_cast<uint32_t>(kInput)]; }

    constexpr const Entry& operator[](const ::ltc::Input kInput) const { return entries[static_cast<uint32_t>(kInput)]; }
};

constexpr auto MakeCommandTable() {
    EntryTable<static_cast<uint32_t>(Input::kUndefined)> table{};

    table[::ltc::Input::kLtc] = {
        .start = ::ltc::input::ltc::Start,
        .stop = ::ltc::input::ltc::Stop,
    };

    table[::ltc::Input::kArtnet] = {
        .start = ::ltc::input::artnet::Start,
        .stop = ::ltc::input::artnet::Stop,
    };

    table[::ltc::Input::kMidi] = {
        .start = ::ltc::input::midi::Start,
        .stop = ::ltc::input::midi::Stop,
    };

    table[::ltc::Input::kTcnet] = {
        .start = ::ltc::input::tcnet::Start,
        .stop = ::ltc::input::tcnet::Stop,
    };

    table[::ltc::Input::kInternal] = {
        .start = ::ltc::input::internal::Start,
        .stop = ::ltc::input::internal::Stop,
    };

    table[::ltc::Input::kApplemidi] = {
        .start = ::ltc::input::applemidi::Start,
        .stop = ::ltc::input::applemidi::Stop,
    };

    table[::ltc::Input::kSystime] = {
        .start = ::ltc::input::systime::Start,
        .stop = ::ltc::input::systime::Stop,
    };

    table[::ltc::Input::kEtc] = {
        .start = ::ltc::input::etc::Start,
        .stop = ::ltc::input::etc::Stop,
    };

    return table;
}

inline constexpr auto kCommandtab = MakeCommandTable();

class Source : public output::Destination {
   public:
    Source() {
        LTC_INPUT_DEBUG_ENTRY();

        s_this = this;
        DisplayInput();

        LTC_INPUT_DEBUG_EXIT();
    }

    bool Select(const ::ltc::Input kInput) {
        LTC_INPUT_DEBUG_ENTRY();
        LTC_INPUT_DEBUG_PRINTF("input=%s", ::ltc::InputToName(kInput));

        if ((kInput == input_) || (kInput == ::ltc::Input::kUndefined)) {
            LTC_INPUT_DEBUG_EXIT();
            return true;
        }

        const auto& entry_new = kCommandtab[kInput];

        if (entry_new.start != nullptr) {
            if (input_ != ::ltc::Input::kUndefined) {
                const auto& entry_current = kCommandtab[input_];

                if (entry_current.stop != nullptr) {
                    entry_current.stop();
                }
            }

            output::Destination::Update(kInput);
            output::Destination::Reset();

            entry_new.start();
            input_ = kInput;

            DisplayInput();
        }

        LTC_INPUT_DEBUG_EXIT();

        return false;
    }

    [[nodiscard]] ::ltc::Input Input() const { return input_; }

    void RunInput() {
        switch (input_) {
            case Input::kLtc:
            case Input::kArtnet:
                break;
            case Input::kMidi:
                input::midi::Run();
                break;
            case Input::kTcnet:
                break;
            case Input::kInternal:
                input::internal::Run();
                break;
            case Input::kApplemidi:
                break;
            case Input::kSystime:
                input::systime::Run();
                break;
            case Input::kEtc:
            case Input::kUndefined:
                break;
        }
    }

    static Source& Instance() {
        assert(s_this != nullptr);
        return *s_this;
    }

   private:
    void DisplayInput() {
        static constexpr uint32_t kDisplayInputLine = 3;
        Display::Get()->ClearLine(kDisplayInputLine);
        Display::Get()->PutString(::ltc::InputToNamePretty(input_));
    }

    ::ltc::Input input_{Input::kUndefined};
    inline static Source* s_this;
};
} // namespace ltc::input

#endif // INPUT_LTC_INPUT_H_
