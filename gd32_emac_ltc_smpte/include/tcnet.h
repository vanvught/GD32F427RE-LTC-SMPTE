/**
 * @file tcnet.h
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

#ifndef TCNET_H_
#define TCNET_H_

#include <cstdint>

#include "firmware/debug/debug_debug.h"

#ifdef DEBUG_TCNET
#define TCNET_DEBUG_ENTRY() DEBUG_ENTRY()
#define TCNET_DEBUG_EXIT() DEBUG_EXIT()
#define TCNET_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define TCNET_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define TCNET_DEBUG_ENTRY() \
    do {                    \
    } while (false)
#define TCNET_DEBUG_EXIT() \
    do {                   \
    } while (false)
#define TCNET_DEBUG_PRINTF(...) \
    do {                        \
    } while (false)
#define TCNET_DEBUG_PUTS(...) \
    do {                      \
    } while (false)
#endif // DEBUG_TCNET

namespace tcnet {
inline constexpr char kNodeNameDefault[] = "AvV";

enum class Layer : uint8_t { kLayer1, kLayer2, kLayer3, kLayer4, kLayerA, kLayerB, kLayerM, kLayerC, kLayerUndefined };

enum class TimeCodeType : uint8_t { kFilm = 0, kEbu25Fps = 1, kDf = 2, kSmpte30Fps = 3, kInvalid = 0xFF };

inline constexpr const uint8_t kFps[] = {24, 25, 29, 30};

[[nodiscard]] constexpr char LayerToChar(Layer layer) {
    switch (layer) {
        case Layer::kLayer1:
        case Layer::kLayer2:
        case Layer::kLayer3:
        case Layer::kLayer4:
            return static_cast<char>(static_cast<char>(layer) + '1');
            break;
        case Layer::kLayerA:
            return 'A';
            break;
        case Layer::kLayerB:
            return 'B';
            break;
        case Layer::kLayerM:
            return 'M';
            break;
        case Layer::kLayerC:
            return 'C';
            break;
        default:
            break;
    }

    return ' ';
}

[[nodiscard]] constexpr Layer LayerFromChar(char character) {
    switch (character | 0x20) { // to lower case
        case '1':
        case '2':
        case '3':
        case '4':
            return static_cast<tcnet::Layer>(character - '1');
            break;
        case 'a':
            return Layer::kLayerA;
            break;
        case 'b':
            return Layer::kLayerB;
            break;
        case 'm':
            return Layer::kLayerM;
            break;
        case 'c':
            return Layer::kLayerC;
            break;
        default:
            break;
    }

    return Layer::kLayerUndefined;
}

void Start();
void Stop();

void Print();

void SetNodeName(const char* node_name);
const char* NodeName();

void SetLayer(tcnet::Layer layer);
tcnet::Layer Layer();

void SetUseTimeCode(bool use_time_code);
bool IsUseTimeCode();

void SetTimeCodeType(TimeCodeType type);
tcnet::TimeCodeType TimeCodeType();
} // namespace tcnet

#endif // TCNET_H_
