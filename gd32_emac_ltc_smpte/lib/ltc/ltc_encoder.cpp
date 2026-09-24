/**
 * @file ltcencoder.cpp
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

#include "ltc_encoder.h"
#include "ltc.h"
#include "ltc_gpio_config.h"
#include "gd32.h" // IWYU pragma: keep

namespace ltc::encoder {
namespace {

uint8_t ltc_bits[sizeof(struct ltc::encoder::FormatTemplate)];
static bool GetParity(uint32_t value) {
    value ^= value >> 16;
    value ^= value >> 8;
    value ^= value >> 4;
    value &= 0xf;

    return static_cast<bool>((0x6996 >> value) & 1);
}

void SetPolarity(uint32_t type, uint8_t* bits) {
    /* "Polarity correction bit" (bit 59 at 25 frame/s, bit 27 at other rates):
     * this bit is chosen to provide an even number of 0 bits in the whole frame, including the sync code.
     * (Since the frame is an even number of bits long, this implies an even number of 1 bits, and is thus an even parity bit.
     * Since the sync code includes an odd number of 1 bits, it is an odd parity bit over the data.)
     * This keeps the phase of each frame consistent, so it always starts with a rising edge at the beginning of bit 0.
     */

    auto* p = reinterpret_cast<struct ltc::encoder::FormatTemplate*>(bits);

    if (type == static_cast<uint8_t>(ltc::Type::kEbu)) {
        auto b = p->format.bytes[7];
        b &= static_cast<uint8_t>(~(1U << 4));
        p->format.bytes[7] = b;
    } else {
        auto b = p->format.bytes[3];
        b &= static_cast<uint8_t>(~(1U << 4));
        p->format.bytes[3] = b;
    }

    const auto kParityOnes = GetParity(p->format.words[0]) ^ GetParity(p->format.words[1]);

    if (!kParityOnes) {
        if (type == static_cast<uint8_t>(ltc::Type::kEbu)) {
            auto b = p->format.bytes[7];
            b |= (1 << 4);
            p->format.bytes[7] = b;
        } else {
            auto b = p->format.bytes[3];
            b |= (1 << 4);
            p->format.bytes[3] = b;
        }
    }
}

uint8_t ReverseBits(uint8_t bits) {
    return (static_cast<uint8_t>(__RBIT(static_cast<uint32_t>(bits)) >> 24));
}
} // namespace

void Init() {
    auto* p = reinterpret_cast<struct ltc::encoder::FormatTemplate*>(ltc_bits);

    for (uint32_t& word : p->format.words) {
        word = 0;
    }

    p->format.half_words[4] = __builtin_bswap16(ltc::encoder::kSyncWordValue);
}

void Encode(void* buffer) {
    const auto* p{reinterpret_cast<struct ltc::encoder::FormatTemplate*>(ltc_bits)};
    auto* dst{reinterpret_cast<uint32_t*>(buffer)};
    uint32_t index{};
    uint32_t index_previous{1}; // Force rising first

    constexpr auto kGpioShiftSet = static_cast<uint32_t>(1U << LTC_OUTPUT_GPIO_PIN_OFFSET); // BOP register: Bits 15:0 -> 1 Set
    constexpr auto kGpioShiftClear = (kGpioShiftSet << 16);                                 // BOP register: Bits 31:16 -> 1 Clear

    for (uint8_t kW : p->format.bytes) {
        for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
            if (mask & kW) { // '1'
                if ((index_previous == 0) || (index_previous == 2)) {
                    index = 2;
                    *dst++ = kGpioShiftClear;
                    *dst++ = kGpioShiftSet;
                } else {
                    index = 3;
                    *dst++ = kGpioShiftSet;
                    *dst++ = kGpioShiftClear;
                }
            } else { // '0'
                if ((index_previous == 0) || (index_previous == 2)) {
                    index = 1;
                    *dst++ = kGpioShiftClear;
                    *dst++ = kGpioShiftClear;
                } else {
                    index = 0;
                    *dst++ = kGpioShiftSet;
                    *dst++ = kGpioShiftSet;
                }
            }

            index_previous = index;
        }
    }
}

template <bool kUseExternalClock>
void SetTimeCode(const struct ltc::TimeCode* timecode) {
    auto* p = reinterpret_cast<struct ltc::encoder::FormatTemplate*>(ltc_bits);

    uint8_t tens = timecode->frames / 10U;

    p->format.bytes[0] = ReverseBits(static_cast<uint8_t>(timecode->frames - (10U * tens)));
    p->format.bytes[1] = ReverseBits(tens);

    tens = timecode->seconds / 10U;

    p->format.bytes[2] = ReverseBits(static_cast<uint8_t>(timecode->seconds - (10U * tens)));
    p->format.bytes[3] = ReverseBits(tens);

    tens = timecode->minutes / 10U;

    p->format.bytes[4] = ReverseBits(static_cast<uint8_t>(timecode->minutes - (10U * tens)));
    p->format.bytes[5] = ReverseBits(tens);

    tens = timecode->hours / 10U;

    p->format.bytes[6] = ReverseBits(static_cast<uint8_t>(timecode->hours - (10U * tens)));
    p->format.bytes[7] = ReverseBits(tens);

    /* Bit 10 is set to 1 if drop frame numbering is in use;
     * frame numbers 0 and 1 are skipped during the first second of every minute, except multiples of 10 minutes.
     * This converts 30 frame/second time code to the 29.97 frame/second NTSC standard.
     */

    if (timecode->type == static_cast<uint8_t>(ltc::Type::kDf)) {
        p->format.bytes[1] |= (1U << 5);
    }

    /* Bit 58, unused in earlier versions of the specification,
     * is now defined as "binary group flag 1" and indicates that the time code is synchronized to an external clock.
     * If zero, the time origin is arbitrary.
     */

    if constexpr (kUseExternalClock) {
        p->format.bytes[7] |= (1U << 5);
    }

    SetPolarity(timecode->type, ltc_bits);
}

template void SetTimeCode<false>(const ltc::TimeCode*);
} // namespace ltc::encoder
