/**
 * @file ltc_commands.h
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

#ifndef LTC_COMMANDS_H_
#define LTC_COMMANDS_H_

#include <cstdint>
#include "common/utils/utils_string.h"

namespace ltc::commands {
struct Cmd {
    const char* name;
    uint8_t length;
};

#define MAKE_CMD(id, str) constexpr Cmd k##id = {.name = str, .length = common::ConstStrLen(str)}

// Generic
MAKE_CMD(Start, "start");
MAKE_CMD(Stop, "stop");
MAKE_CMD(Resume, "resume");
MAKE_CMD(Rate, "rate#");
// Internal
MAKE_CMD(Direction, "direction#");
MAKE_CMD(Pitch, "pitch#");
MAKE_CMD(Forward, "forward#");
MAKE_CMD(Backward, "backward#");
// MIDI
MAKE_CMD(Bpm, "bpm#");
MAKE_CMD(Continue, "continue");
// TCNet
MAKE_CMD(Layer, "layer#");
MAKE_CMD(Type, "type#");
MAKE_CMD(Timecode, "timecode#");

#undef MAKE_CMD
} // namespace ltc::commands

#endif // LTC_COMMANDS_H_
