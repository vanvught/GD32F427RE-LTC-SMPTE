/**
 * @file midi.h
 *
 */
/* Copyright (C) 2016-2023 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef MIDI_H_
#define MIDI_H_

#include <cstdint>

namespace midi {
namespace defaults {
inline constexpr auto kBaudrate = 31250;
} // namespace defaults

enum class ActiveSenseState { kNotEnabled, kEnabled, kFailed };

enum class Type : uint8_t {
    kInvalideType = 0x00,         ///< For notifying errors
    kNoteOff = 0x80,              ///< Note Off
    kNoteOn = 0x90,               ///< Note On
    kAfterTouchPoly = 0xA0,       ///< Polyphonic AfterTouch
    kControlChange = 0xB0,        ///< Control Change / Channel Mode
    kProgramChange = 0xC0,        ///< Program Change
    kAfterTouchChannel = 0xD0,    ///< Channel (monophonic) AfterTouch
    kPitchBend = 0xE0,            ///< Pitch Bend
    kSystemExclusive = 0xF0,      ///< System Exclusive
    kTimeCodeQuarterFrame = 0xF1, ///< System Common - MIDI Time Code Quarter Frame
    kSongPosition = 0xF2,         ///< System Common - Song Position Pointer
    kSongSelect = 0xF3,           ///< System Common - Song Select
    kTuneRequest = 0xF6,          ///< System Common - Tune Request
    kClock = 0xF8,                ///< System Real Time - Timing Clock
    kStart = 0xFA,                ///< System Real Time - Start
    kContinue = 0xFB,             ///< System Real Time - Continue
    kStop = 0xFC,                 ///< System Real Time - Stop
    kActiveSensing = 0xFE,        ///< System Real Time - Active Sensing
    kSystemReset = 0xFF,          ///< System Real Time - System Reset
};

#define MIDI_SYSTEM_EXCLUSIVE_INDEX_ENTRIES 128

struct Message {
    uint32_t timestamp;
    midi::Type type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
    uint8_t system_exclusive[MIDI_SYSTEM_EXCLUSIVE_INDEX_ENTRIES];
    uint8_t bytes_count;
};

struct Timecode {
    uint8_t frames;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t type;
} __attribute__((packed));

namespace bpm {
inline constexpr auto kMin = 8;
inline constexpr auto kMax = 300;
} // namespace bpm
} // namespace midi

#endif // MIDI_H_
