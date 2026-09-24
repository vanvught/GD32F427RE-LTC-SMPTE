/**
 * @file  pixel_const.h
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

#ifndef PIXEL_CONST_H_
#define PIXEL_CONST_H_

#include <cstdint>

#include "gd32_board.h"

namespace pixel {
inline constexpr uint32_t kRtzTimerPeriod = (0.00000125F * MCU_CLOCK_FREQ) - 1U;
inline constexpr uint32_t kTicksPerBit = kRtzTimerPeriod + 1U;
inline constexpr float kBitus = 1.25F;
// WS2812B
inline constexpr float kT0Hus = 0.4F;
inline constexpr float kT1Hus = 0.8F;

inline constexpr uint32_t kT0H = static_cast<uint32_t>(((kTicksPerBit * kT0Hus) / kBitus) + 0.5F);
static_assert(kT0H < UINT16_MAX);
inline constexpr uint32_t kT1H = static_cast<uint32_t>(((kTicksPerBit * kT1Hus) / kBitus) + 0.5F);
static_assert(kT1H < UINT16_MAX);

inline constexpr uint32_t kPixelsMax = 512;
} // namespace pixel

#endif // PIXEL_CONST_H_
