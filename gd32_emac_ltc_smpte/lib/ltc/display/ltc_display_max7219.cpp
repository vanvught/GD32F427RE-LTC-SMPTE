/**
 * @file display_max7219.cpp
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
#include <cstring>

#include "ltc_display_max7219.h"
#include "max72197segment.h"
#include "max7219matrix.h"
#include "ltc.h"
#include "ltc_debug.h"

namespace ltc::display::max7219 {
namespace {
auto type_active{Types::kMatrix};
uint8_t intensity_active{0x7f};

Max72197Segment segment_instance;
Max7219Matrix matrix_instance;

constexpr auto kSegments = 8;
char buffer[kSegments];

constexpr uint8_t kCharsColon[][8] = {
    {0x3E, 0x7F, 0x71, 0x59, 0x4D, 0x7F, 0x3E, 0x66}, // 0:
    {0x40, 0x42, 0x7F, 0x7F, 0x40, 0x40, 0x00, 0x66}, // 1:
    {0x62, 0x73, 0x59, 0x49, 0x6F, 0x66, 0x00, 0x66}, // 2:
    {0x22, 0x63, 0x49, 0x49, 0x7F, 0x36, 0x00, 0x66}, // 3:
    {0x18, 0x1C, 0x16, 0x53, 0x7F, 0x7F, 0x50, 0x66}, // 4:
    {0x27, 0x67, 0x45, 0x45, 0x7D, 0x39, 0x00, 0x66}, // 5:
    {0x3C, 0x7E, 0x4B, 0x49, 0x79, 0x30, 0x00, 0x66}, // 6:
    {0x03, 0x03, 0x71, 0x79, 0x0F, 0x07, 0x00, 0x66}, // 7:
    {0x36, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00, 0x66}, // 8:
    {0x06, 0x4F, 0x49, 0x69, 0x3F, 0x1E, 0x00, 0x66}  // 9:
};

constexpr uint8_t kCharsBlinkSemiColon[][8] = {
    {0x3E, 0x7F, 0x71, 0x59, 0x4D, 0x7F, 0x3E | 0x80, 0x66}, // 0;
    {0x40, 0x42, 0x7F, 0x7F, 0x40, 0x40, 0x00 | 0x80, 0x66}, // 1;
    {0x62, 0x73, 0x59, 0x49, 0x6F, 0x66, 0x00 | 0x80, 0x66}, // 2;
    {0x22, 0x63, 0x49, 0x49, 0x7F, 0x36, 0x00 | 0x80, 0x66}, // 3;
    {0x18, 0x1C, 0x16, 0x53, 0x7F, 0x7F, 0x50 | 0x80, 0x66}, // 4;
    {0x27, 0x67, 0x45, 0x45, 0x7D, 0x39, 0x00 | 0x80, 0x66}, // 5;
    {0x3C, 0x7E, 0x4B, 0x49, 0x79, 0x30, 0x00 | 0x80, 0x66}, // 6;
    {0x03, 0x03, 0x71, 0x79, 0x0F, 0x07, 0x00 | 0x80, 0x66}, // 7;
    {0x36, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00 | 0x80, 0x66}, // 8;
    {0x06, 0x4F, 0x49, 0x69, 0x3F, 0x1E, 0x00 | 0x80, 0x66}  // 9;
};

constexpr uint8_t kCharsBlinkComma[][8] = {
    {0x3E, 0x7F, 0x71, 0x59, 0x4D, 0x7F, 0x3E | 0x80, 0x00}, // 0,
    {0x40, 0x42, 0x7F, 0x7F, 0x40, 0x40, 0x00 | 0x80, 0x60}, // 1,
    {0x62, 0x73, 0x59, 0x49, 0x6F, 0x66, 0x00 | 0x80, 0x00}, // 2,
    {0x22, 0x63, 0x49, 0x49, 0x7F, 0x36, 0x00 | 0x80, 0x60}, // 3,
    {0x18, 0x1C, 0x16, 0x53, 0x7F, 0x7F, 0x50 | 0x80, 0x00}, // 4,
    {0x27, 0x67, 0x45, 0x45, 0x7D, 0x39, 0x00 | 0x80, 0x60}, // 5,
    {0x3C, 0x7E, 0x4B, 0x49, 0x79, 0x30, 0x00 | 0x80, 0x00}, // 6,
    {0x03, 0x03, 0x71, 0x79, 0x0F, 0x07, 0x00 | 0x80, 0x60}, // 7,
    {0x36, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00 | 0x80, 0x00}, // 8,
    {0x06, 0x4F, 0x49, 0x69, 0x3F, 0x1E, 0x00 | 0x80, 0x60}  // 9,
};

int32_t Offset(char character, char seconds) {
    const auto kEven = !((seconds & 0x01) == 0x01);

    if (kEven) {
        switch (character) {
            case ':':
                return 0 - '0';
                break;
            case ';':
                return 10 - '0';
                break;
            case ',':
                return 20 - '0';
                break;
            default:
                break;
        }
    }

    return 0;
}

} // namespace

void Start() {
    LTC_DISPLAY_DEBUG_ENTRY();

    if (type_active == Types::kMatrix) {
        for (uint32_t i = 0; i < sizeof(kCharsColon) / sizeof(kCharsColon[0]); i++) {
            matrix_instance.UpdateCharacter(i, kCharsColon[i]);
        }

        for (uint32_t i = 10; i < 10 + (sizeof(kCharsBlinkSemiColon) / sizeof(kCharsBlinkSemiColon[0])); i++) {
            matrix_instance.UpdateCharacter(i, kCharsBlinkSemiColon[i - 10]);
        }

        for (uint32_t i = 20; i < 20 + (sizeof(kCharsBlinkComma) / sizeof(kCharsBlinkComma[0])); i++) {
            matrix_instance.UpdateCharacter(i, kCharsBlinkComma[i - 20]);
        }

        matrix_instance.Init(kSegments, intensity_active);
        matrix_instance.Write("Waiting", 7);

        LTC_DISPLAY_DEBUG_EXIT();
        return;
    }

    segment_instance.Init(intensity_active);

    segment_instance.WriteRegister(::max7219::reg::kDigit6, 0x80, true);
    segment_instance.WriteRegister(::max7219::reg::kDigit4, 0x80, false);
    segment_instance.WriteRegister(::max7219::reg::kDigit2, 0x80, false);

    LTC_DISPLAY_DEBUG_EXIT();
}

void SetType(Types type) {
    type_active = type;
}

Types Type() {
    return type_active;
}

void SetIntensity(uint8_t intensity) {
    LTC_DISPLAY_DEBUG_PRINTF("intensity=%u", static_cast<unsigned>(intensity));

    intensity_active = intensity;

    if (type_active == Types::kMatrix) {
        matrix_instance.SetIntensity(intensity);
        return;
    }

    segment_instance.SetIntensity(intensity);
}

uint8_t Intensity() {
    return intensity_active;
}

void Show(const char* timecode) {
    if (type_active == Types::kMatrix) {
        const auto kSeconds = timecode[ltc::timecode::index::kSecondsUnits];

        buffer[0] = timecode[0];
        buffer[1] = static_cast<char>(Offset(timecode[ltc::timecode::index::kColon1], kSeconds) + timecode[1]);
        buffer[2] = timecode[3];
        buffer[3] = static_cast<char>(Offset(timecode[ltc::timecode::index::kColon2], kSeconds) + timecode[4]);
        buffer[4] = timecode[6];
        buffer[5] = static_cast<char>(Offset(timecode[ltc::timecode::index::kColon3], kSeconds) + timecode[7]);
        buffer[6] = timecode[9];
        buffer[7] = timecode[10];

        matrix_instance.Write(buffer, kSegments);
        return;
    }

    segment_instance.WriteRegister(::max7219::reg::kDigit7, static_cast<uint32_t>(timecode[0] - '0'), true);
    segment_instance.WriteRegister(::max7219::reg::kDigit6, static_cast<uint32_t>((timecode[1] - '0') | 0x80), false);
    segment_instance.WriteRegister(::max7219::reg::kDigit5, static_cast<uint32_t>(timecode[3] - '0'), false);
    segment_instance.WriteRegister(::max7219::reg::kDigit4, static_cast<uint32_t>((timecode[4] - '0') | 0x80), false);
    segment_instance.WriteRegister(::max7219::reg::kDigit3, static_cast<uint32_t>(timecode[6] - '0'), false);
    segment_instance.WriteRegister(::max7219::reg::kDigit2, static_cast<uint32_t>((timecode[7] - '0') | 0x80), false);
    segment_instance.WriteRegister(::max7219::reg::kDigit1, static_cast<uint32_t>(timecode[9] - '0'), false);
    segment_instance.WriteRegister(::max7219::reg::kDigit0, static_cast<uint32_t>(timecode[10] - '0'), false);
}
} // namespace ltc::display::max7219