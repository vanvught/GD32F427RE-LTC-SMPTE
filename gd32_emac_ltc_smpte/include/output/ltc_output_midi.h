/**
 * @file ltc_output_midi.h
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

#ifndef OUTPUT_LTC_OUTPUT_MIDI_H_
#define OUTPUT_LTC_OUTPUT_MIDI_H_

#include <utility>

#include "midi.h"
#include "net/rtpmidi.h"
#include "usb/usbd/midi/usbd_midi.h"

namespace ltc::output {
namespace midi {
void Start();
void Stop();
void Output(const ::midi::Timecode* timecode);
void Output(::midi::Type type);
void Output(uint8_t value);
} // namespace midi

namespace applemidi {
void Start();
void Stop();
void Restart();

inline void Output(const ::midi::Timecode* timecode) {
    RtpMidi::Get()->SendTimeCode(reinterpret_cast<const struct ::midi::Timecode*>(timecode));
}

inline void Output(::midi::Type type) {
    RtpMidi::Get()->TransmitRaw(type);
}

inline void Output(const uint8_t kValue) {
    RtpMidi::Get()->SendQf(kValue);
}
} // namespace applemidi

namespace usbmidi {
void Start();
void Stop();

inline void Output(const ::midi::Timecode* timecode) {
    const auto kHours = static_cast<uint8_t>(((timecode->type & 0x03) << 5) | (timecode->hours & 0x1F));
    const uint8_t kData[16] = {0x04, 0xF0, 0x7F, 0x7F, 0x04, 0x01, 0x01, kHours, 0x04, static_cast<uint8_t>(timecode->minutes & 0x3F), static_cast<uint8_t>(timecode->seconds & 0x3F), static_cast<uint8_t>(timecode->frames & 0x1F),
                               0x05, 0xF7, 0x00, 0x00};

    ::usbmidi::Send(kData, sizeof(kData));
}

inline void Output(::midi::Type type) {
    assert((type >= ::midi::Type::kClock) && (type <= ::midi::Type::kSystemReset));

    const uint8_t kData[4] = {0x0F, std::to_underlying(type), 0x00, 0x00};

    ::usbmidi::Send(kData, sizeof(kData));
}

inline void Output(const uint8_t kValue) {
    const uint8_t kData[4] = {0x02, std::to_underlying(::midi::Type::kTimeCodeQuarterFrame), kValue, 0x00};

    ::usbmidi::Send(kData, sizeof(kData));
}
} // namespace usbmidi
} // namespace ltc::output

#endif // OUTPUT_LTC_OUTPUT_MIDI_H_
