/**
 * @file pixel_display_matrix.h
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

#ifndef PIXEL_DISPLAY_MATRIX_H_
#define PIXEL_DISPLAY_MATRIX_H_

#include <cstdint>

#include "pixel_output.h"
#include "ltc_debug.h"
#include "../../lib-device/src/font_cp437.h"
#include "pixel_const.h"
#include "gd32.h"

namespace pixel::display::matrix {
struct Colon {
    uint8_t bits;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

template <uint32_t kColumns, uint32_t kRows>
class Matrix {
    static_assert((kColumns * kRows) <= ::pixel::kPixelsMax);

   public:
    Matrix() noexcept = default;
    ~Matrix() noexcept { Stop(); }

    void Start() {
        LTC_DISPLAY_DEBUG_ENTRY();

        Stop();

        SetColonsOff();

        pixel::output::Start();

        LTC_DISPLAY_DEBUG_EXIT();
    }

    void Stop() {
        LTC_DISPLAY_DEBUG_ENTRY();

        pixel::output::Stop();

        LTC_DISPLAY_DEBUG_EXIT();
    }

    void PutChar(char character, uint8_t red, uint8_t green, uint8_t blue) {
        if (static_cast<uint32_t>(character) >= Cp437FontSize()) {
            character = ' ';
        }

        while (pixel::output::IsUpdating()) {
            // wait for completion
        }

        auto offset = static_cast<uint16_t>((kFontCP437CharW * kFontCP437CharH) * position_);

        for (uint32_t width = 0; width < kFontCP437CharW; width++) {
            uint8_t byte = kCp437Font[static_cast<int>(character)][width];

            if (width == (kFontCP437CharW - 1)) {
                if (colons_[position_].bits != 0) {
                    byte = colons_[position_].bits;
                    red = colons_[position_].red;
                    green = colons_[position_].green;
                    blue = colons_[position_].blue;
                }
            }

            if ((width & 0x1) != 0) {
                byte = ReverseBits(byte);
            }

            for (uint32_t height = 0; height < kFontCP437CharH; height++) {
                if ((byte & (1 << height)) == (1 << height)) {
                    pixel::output::SetPixel(offset, red, green, blue);
                } else {
                    pixel::output::SetPixel(offset, 0x00, 0x00, 0x00);
                }

                offset++;
            }
        }

        position_++;

        if (position_ == kMaxPosition) {
            position_ = 0;
            line_++;

            if (line_ == kMaxLine) {
                line_ = 0;
            }
        }

        update_needed_ = true;
    }

    void PutString(const char* string, uint8_t red, uint8_t green, uint8_t blue) {
        char character;

        while ((character = *string++) != 0) {
            PutChar(character, red, green, blue);
        }
    }

    void Text(const char* text, uint32_t length, uint8_t red, uint8_t green, uint8_t blue) {
        if (length > kMaxPosition) {
            length = kMaxPosition;
        }

        for (uint32_t i = 0; i < length; i++) {
            PutChar(text[i], red, green, blue);
        }
    }

    // line starts at 1
    void TextLine(uint8_t line, const char* text, uint32_t length, uint8_t red, uint8_t green, uint8_t blue) {
        if ((line == 0) || (line > kMaxLine)) {
            return;
        }

        SetCursorPos(0, static_cast<uint8_t>(line - 1));
        Text(text, length, red, green, blue);
    }

    // line starts at 1
    void ClearLine(uint8_t line) {
        if ((line == 0) || (line > kMaxLine)) {
            return;
        }

        while (pixel::output::IsUpdating()) {
            // wait for completion
        }

        for (uint32_t i = 0; i < kMaxPixels; i++) {
            pixel::output::SetPixel(i, 0, 0, 0); // Note: Currently working for single row only
        }

        SetCursorPos(0, static_cast<uint8_t>(line - 1));
    };

    // 0,0 is top left
    void SetCursorPos(uint8_t column, uint8_t row) {
        if ((column >= kMaxPosition) || (row >= kMaxLine)) {
            return;
        }

        position_ = column;
        line_ = row;
    }

    void SetColon(char character, uint32_t position, uint8_t red, uint8_t green, uint8_t blue) {
        if (position >= kMaxPosition) {
            return;
        }

        switch (character) {
            case ':':
                colons_[position].bits = 0x66;
                break;
            case '.':
                colons_[position].bits = 0x60;
                break;
            default:
                colons_[position].bits = 0;
                break;
        }

        colons_[position].red = red;
        colons_[position].blue = blue;
        colons_[position].green = green;
    };

    void SetColonsOff() {
        for (uint32_t position = 0; position < kMaxPosition; position++) {
            colons_[position].bits = 0;
            colons_[position].red = 0;
            colons_[position].green = 0;
            colons_[position].blue = 0;
        }
    }

    void Cls() {
        while (pixel::output::IsUpdating()) {
            // wait for completion
        }
        pixel::output::Blackout(kMaxPixels);
    }

    void Show() {
        if (update_needed_) {
            update_needed_ = false;
            pixel::output::Update(kMaxPixels);
        }
    }

   private:
    uint8_t ReverseBits(uint8_t bits) {
        const auto kInput = static_cast<uint32_t>(bits);
        const auto kOutput = __RBIT(kInput);
        return static_cast<uint8_t>((kOutput >> 24));
    }

    static constexpr auto kOffset = ((kRows - kFontCP437CharH) * 2U);
    static constexpr auto kMaxPixels = kColumns * kRows;
    static constexpr auto kMaxPosition = kColumns / kFontCP437CharW;
    static constexpr auto kMaxLine = kRows / kFontCP437CharH;

    static_assert(kMaxPixels == pixel::output::kMatrixDisplayPixels);

    Colon colons_[kMaxPosition];

    uint8_t position_{0};
    uint8_t line_{0};

    bool update_needed_{false};
};
} // namespace pixel::display::matrix

#endif // PIXEL_DISPLAY_MATRIX_H_
