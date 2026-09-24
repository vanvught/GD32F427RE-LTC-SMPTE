/**
 * @file midi.h
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

#ifndef MIDI_H_
#define MIDI_H_

#include <cstdint>

namespace midi {
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

struct Timecode {
    uint8_t frames;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t type;
} __attribute__((packed));
} // namespace midi

#endif // INCLUDE_MIDI_H_
