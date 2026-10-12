/**
 * @file ltc_encoder.h
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

#ifndef LTC_ENCODER_H_
#define LTC_ENCODER_H_

#include <cstdint>

#include "ltc.h"

namespace ltc::encoder {

#define LTCENCODER_CEILING(x, y) (((x) + (y) - 1) / (y))
inline constexpr uint32_t kFormatSizeBits = 80;
inline constexpr uint32_t kFormatSizeBytes = LTCENCODER_CEILING(kFormatSizeBits, 8);
inline constexpr uint32_t kFormatSizeHalfwords = LTCENCODER_CEILING(kFormatSizeBytes, 2);
inline constexpr uint32_t kFormatSizeWords = LTCENCODER_CEILING(kFormatSizeBytes, 4);
#undef LTCENCODER_CEILING

inline constexpr uint16_t kSyncWordValue = 0x3FFD;
inline constexpr uint32_t kBufferSize = kFormatSizeBits * 2;

struct FormatTemplate {
    union Format {
        uint8_t bytes[kFormatSizeBytes];
        uint16_t half_words[kFormatSizeHalfwords];
        uint32_t words[kFormatSizeWords];
        uint64_t data;
    } format;
};

void Init();

template <bool kUseExternalClock>
void SetTimeCode(const ltc::TimeCode* timecode);
extern template void SetTimeCode<false>(const ltc::TimeCode* timecode);

void Encode(void* buffer);

inline uint32_t GetBufferSize() {
    const uint32_t kSize = ltc::encoder::kBufferSize;
    return kSize;
}
} // namespace ltc::encoder

#endif // LTC_ENCODER_H_
