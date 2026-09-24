/**
 * @file ltc.h
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

#ifndef LTC_H_
#define LTC_H_

#include <cstdint>
#include <string_view>
#include <strings.h>

#include "common/utils/utils_string.h"

namespace ltc::global {
extern volatile uint32_t updates;
extern volatile uint32_t updates_per_second;
} // namespace ltc::global

namespace ltc {
enum class Input : uint8_t {
    kLtc,       //
    kArtnet,    //
    kMidi,      //
    kTcnet,     //
    kInternal,  //
    kApplemidi, //
    kSystime,   //
    kEtc,       //
    kUndefined, //
};

inline constexpr uint32_t kInputMaxNameLength = 9; // Includes '\0'.

inline constexpr char kInput[][kInputMaxNameLength] = {
    "ltc",      //
    "artnet",   //
    "midi",     //
    "tcnet",    //
    "internal", //
    "rtp-midi", //
    "systime",  //
    "etc",      //
};

inline constexpr uint32_t kInputPrettyMaxNameLength = 12; // Includes '\0'.

inline constexpr char kInputPretty[][kInputPrettyMaxNameLength] = {
    "LTC",         //
    "Art-Net",     //
    "MIDI",        //
    "TCNet",       //
    "Internal",    //
    "rtpMIDI",     //
    "System-Time", //
    "ETC",         //
};

static_assert(sizeof(kInput) / sizeof(kInput[0]) == static_cast<uint32_t>(Input::kUndefined));

enum class Type : uint8_t {
    kFilm = 0,      //
    kEbu = 1,       //
    kDf = 2,        //
    kSmpte = 3,     //
    kUnknown = 4,   //
    kInvalid = 255, //
};

inline constexpr uint32_t kTypeMaxNameLength = 12; // Includes '\0'.

static constexpr char kType[][kTypeMaxNameLength] = {
    "Film 24fps ", //
    "EBU 25fps  ", //
    "DF 29.97fps", //
    "SMPTE 30fps", //
    "----- -----", //
};

struct TimeCode {
    uint8_t frames;  ///< Frames time. 0 – 29 depending on mode.
    uint8_t seconds; ///< Seconds. 0 - 59.
    uint8_t minutes; ///< Minutes. 0 - 59.
    uint8_t hours;   ///< Hours. 0 - 23.
    uint8_t type;    ///< 0 = Film (24fps) , 1 = EBU (25fps), 2 = DF (29.97fps), 3 SMPTE (30fps)
};

static_assert(sizeof(TimeCode) == sizeof(TimeCode::frames) + sizeof(TimeCode::seconds) + sizeof(TimeCode::minutes) + sizeof(TimeCode::hours) + sizeof(TimeCode::type), "TimeCode must not contain padding");

enum class Output : uint16_t {
    kLtc = (1U << static_cast<uint16_t>(Input::kLtc)),                     //
    kArtnet = (1U << static_cast<uint16_t>(Input::kArtnet)),               //
    kMidi = (1U << static_cast<uint16_t>(Input::kMidi)),                   //
    kTcnet = (1U << static_cast<uint16_t>(Input::kTcnet)),                 //
    kInternal = (1U << static_cast<uint16_t>(Input::kInternal)),           //
    kApplemidi = (1U << static_cast<uint16_t>(Input::kApplemidi)),         //
    kSystime = (1U << static_cast<uint16_t>(Input::kSystime)),             //
    kEtc = (1U << static_cast<uint16_t>(Input::kEtc)),                     //
    kNtpServer = (1U << (static_cast<uint16_t>(Input::kUndefined) + 0)),   //                                                                  //
    kDisplayOled = (1U << (static_cast<uint16_t>(Input::kUndefined) + 1)), //                                                                          //
    kMaX7219 = (1U << (static_cast<uint16_t>(Input::kUndefined) + 2)),     //
    kPixel = (1U << (static_cast<uint16_t>(Input::kUndefined) + 3)),       //
};

inline constexpr Output kOutputs[] = {
    Output::kLtc,         //
    Output::kArtnet,      //
    Output::kMidi,        //
    Output::kTcnet,       //
    Output::kInternal,    //
    Output::kApplemidi,   //
    Output::kSystime,     //
    Output::kEtc,         //
    Output::kNtpServer,   //
    Output::kDisplayOled, //
    Output::kMaX7219,     //
    Output::kPixel,       //
};

template <uint32_t kN>
consteval bool AreUniqueOutputBits(const Output (&values)[kN]) {
    uint32_t used{0};

    for (uint32_t i = 0; i < kN; ++i) {
        const auto kValue = static_cast<uint32_t>(values[i]);

        // Must contain exactly one bit.
        if ((kValue == 0) || ((kValue & (kValue - 1U)) != 0)) {
            return false;
        }

        // Bit must not already be used.
        if ((used & kValue) != 0) {
            return false;
        }

        used |= kValue;
    }

    return true;
}

static_assert(AreUniqueOutputBits(kOutputs), "Output contains overlapping values");

[[nodiscard]] constexpr const char* InputToName(Input input) {
    if (input < Input::kUndefined) {
        return kInput[static_cast<uint32_t>(input)];
    }

    return common::kUndefined;
}

inline Input InputFromName(std::string_view name) {
    uint32_t index = 0;

    for (const auto* input : kInput) {
        if (name == input) {
            return static_cast<Input>(index);
        }

        ++index;
    }

    return Input::kUndefined;
}

[[nodiscard]] constexpr const char* InputToNamePretty(Input input) {
    if (input < Input::kUndefined) {
        return kInputPretty[static_cast<uint32_t>(input)];
    }

    return common::kUndefined;
}

[[nodiscard]] constexpr const char* TypeToName(Type type) {
    if (type < ltc::Type::kUnknown) {
        return kType[static_cast<uint32_t>(type)];
    }

    return kType[static_cast<uint32_t>(ltc::Type::kUnknown)];
}

void ConvertToString(const struct TimeCode* ltc_timecode, char* timecode);

namespace timecode {
inline constexpr auto kCodeMaxLength = 11;
inline constexpr auto kTypeMaxLength = 11;
inline constexpr auto kRateMaxLength = 2;
inline constexpr auto kSystimeMaxLength = kCodeMaxLength;

namespace index {
inline constexpr auto kHours = 0;
inline constexpr auto kHoursTens = 0;
inline constexpr auto kHoursUnits = 1;
inline constexpr auto kColon1 = 2;
inline constexpr auto kMinutes = 3;
inline constexpr auto kMinutesTens = 3;
inline constexpr auto kMinutesUnits = 4;
inline constexpr auto kColon2 = 5;
inline constexpr auto kSeconds = 6;
inline constexpr auto kSecondsTens = 6;
inline constexpr auto kSecondsUnits = 7;
inline constexpr auto kColon3 = 8;
inline constexpr auto kFrames = 9;
inline constexpr auto kFramesTens = 9;
inline constexpr auto kFramesUnits = 10;
} // namespace index
} // namespace timecode
} // namespace ltc

#endif // LTC_H_
