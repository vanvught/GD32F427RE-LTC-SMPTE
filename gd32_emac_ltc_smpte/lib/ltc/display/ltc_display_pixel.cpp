/**
 * @file ltc_display_pixel.cpp
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

#include "ltc_display_pixel.h"
#include <cstdint>

#include "ltc.h"
#include "ltc_debug.h"
#include "ltc_display.h"
#include "pixel_display_7semgent.h"
#include "pixel_display_matrix.h"

namespace ltc::display::pixel {
namespace {
auto type_active{ltc::display::Types::kMatrix};

::pixel::display::matrix::Matrix<64, 8> matrix_instance;
::pixel::display::segment::Segment segment_instance;

Colours colours{.red = 0xFF, .green = 0x0FF, .blue = 0x00};
Colours colours_colons{.red = 0x00, .green = 0x00, .blue = 0x00};
} // namespace

void Start() {
    LTC_DISPLAY_DEBUG_ENTRY();

    if (type_active == ltc::display::Types::kMatrix) {
        matrix_instance.Start();
        LTC_DISPLAY_DEBUG_EXIT();
        return;
    }

    matrix_instance.Start();

    LTC_DISPLAY_DEBUG_EXIT();
}

void Stop() {
    LTC_DISPLAY_DEBUG_ENTRY();

    if (type_active == ltc::display::Types::kMatrix) {
        matrix_instance.Stop();
        LTC_DISPLAY_DEBUG_EXIT();
        return;
    }

    matrix_instance.Stop();

    LTC_DISPLAY_DEBUG_EXIT();
}

void Show(const char* timecode) {
    if (type_active == ltc::display::Types::kMatrix) {
        matrix_instance.SetColonsOff();
        matrix_instance.SetColon(timecode[ltc::timecode::index::kColon1], 1, colours_colons.red, colours_colons.green, colours_colons.blue);
        matrix_instance.SetColon(timecode[ltc::timecode::index::kColon2], 3, colours_colons.red, colours_colons.green, colours_colons.blue);
        matrix_instance.SetColon(timecode[ltc::timecode::index::kColon3], 5, colours_colons.red, colours_colons.green, colours_colons.blue);

        const char kLine[]{timecode[0], timecode[1], timecode[3], timecode[4], timecode[6], timecode[7], timecode[9], timecode[10]};

        matrix_instance.TextLine(1, kLine, sizeof(kLine), colours.red, colours.green, colours.blue);
        matrix_instance.Show();
        return;
    }

	auto red = colours.red;
	auto green = colours.green;
	auto blue = colours.blue;

	const char kChars[]{timecode[0], timecode[1], timecode[3], timecode[4], timecode[6], timecode[7], timecode[9], timecode[10]};
	assert(sizeof(kChars) <= ::pixel::display::segment::Config::kNumOfDigits);

	segment_instance.WriteAll(kChars, red, green, blue);

	red = colours_colons.red;
	green = colours_colons.green;
	blue = colours_colons.blue;

	segment_instance.SetColon(timecode[ltc::timecode::index::kColon1], 0, red, green, blue);
	segment_instance.SetColon(timecode[ltc::timecode::index::kColon2], 1, red, green, blue);
	segment_instance.SetColon(timecode[ltc::timecode::index::kColon3], 2, red, green, blue);
	segment_instance.Show();
}
} // namespace ltc::display::pixel
