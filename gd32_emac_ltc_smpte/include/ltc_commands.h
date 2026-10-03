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

#include <string_view>

namespace ltc::commands {
// Generic
inline constexpr std::string_view kStart{"start"};
inline constexpr std::string_view kStop{"stop"};
inline constexpr std::string_view kResume{"resume"};
inline constexpr std::string_view kRate{"rate#"};
// Internal
inline constexpr std::string_view kDirection{"direction#"};
inline constexpr std::string_view kPitch{"pitch#"};
inline constexpr std::string_view kForward{"forward#"};
inline constexpr std::string_view kBackward{"backward#"};
// MIDI
inline constexpr std::string_view kBpm{"bpm#"};
inline constexpr std::string_view kContinue{"continue"};
// TCNet
inline constexpr std::string_view kLayer{"layer#"};
inline constexpr std::string_view kType{"type#"};
inline constexpr std::string_view kTimecode{"timecode#"};
} // namespace ltc::commands

#endif // LTC_COMMANDS_H_
