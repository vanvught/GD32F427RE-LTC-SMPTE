/**
 * @file rtpmidi.cpp
 */
/* Copyright (C) 2019-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "net/rtpmidi.h"
#include "midi_debug.h"
#include "firmware/debug/debug_dump.h"

#define RTP_MIDI_COMMAND_STATUS_FLAG 0x80

#define RTP_MIDI_DELTA_TIME_OCTET_MASK 0x7f
#define RTP_MIDI_DELTA_TIME_EXTENSION 0x80

#define RTP_MIDI_CS_FLAG_B 0x80
#define RTP_MIDI_CS_FLAG_J 0x40
#define RTP_MIDI_CS_FLAG_Z 0x20
#define RTP_MIDI_CS_FLAG_P 0x10
#define RTP_MIDI_CS_MASK_SHORTLEN 0x0f
#define RTP_MIDI_CS_MASK_LONGLEN 0x0fff

int32_t RtpMidi::DecodeTime([[maybe_unused]] uint32_t command_length, uint32_t offset) {
    RTPMIDI_DEBUG_ENTRY();

    int32_t size = 0;
    uint32_t deltatime = 0;

    for (uint32_t i = 0; i < 4; i++) {
        const auto kOctet = receive_buffer_[offset + static_cast<uint32_t>(size)];
        deltatime = (deltatime << 7) | (kOctet & RTP_MIDI_DELTA_TIME_OCTET_MASK);
        size++;

        if ((kOctet & RTP_MIDI_DELTA_TIME_EXTENSION) == 0) {
            break;
        }
    }

    RTPMIDI_DEBUG_PRINTF("size=%u, deltatime=%x", static_cast<unsigned>(size), static_cast<unsigned>(deltatime));
    RTPMIDI_DEBUG_EXIT();
    return size;
}

int32_t RtpMidi::DecodeMidi(uint32_t command_length, uint32_t offset) {
    RTPMIDI_DEBUG_ENTRY();

    int32_t size = -1;

    const auto kStatusByte = receive_buffer_[offset];
    const auto kType = GetTypeFromStatusByte(kStatusByte);

    message_.timestamp = __builtin_bswap32(*reinterpret_cast<uint32_t*>(&receive_buffer_[4]));
    message_.type = kType;
    message_.channel = 0;
    message_.data1 = 0;
    message_.data2 = 0;

    switch (static_cast<midi::Type>(kType)) {
        case midi::Type::kActiveSensing:
        case midi::Type::kStart:
        case midi::Type::kStop:
        case midi::Type::kContinue:
        case midi::Type::kClock:
        case midi::Type::kTuneRequest:
        case midi::Type::kSystemReset:
            message_.bytes_count = 1;
            size = 1;
            break;
        case midi::Type::kProgramChange:
        case midi::Type::kAfterTouchChannel:
        case midi::Type::kTimeCodeQuarterFrame:
        case midi::Type::kSongSelect:
            message_.channel = GetChannelFromStatusByte(kStatusByte);
            message_.data1 = receive_buffer_[++offset];
            message_.bytes_count = 2;
            size = 2;
            break;
        case midi::Type::kNoteOn:
        case midi::Type::kNoteOff:
        case midi::Type::kControlChange:
        case midi::Type::kPitchBend:
        case midi::Type::kAfterTouchPoly:
        case midi::Type::kSongPosition:
            message_.channel = GetChannelFromStatusByte(kStatusByte);
            message_.data1 = receive_buffer_[++offset];
            message_.data2 = receive_buffer_[++offset];
            message_.bytes_count = 3;
            size = 3;
            break;
        case midi::Type::kSystemExclusive: {
            for (size = 0; (static_cast<uint32_t>(size) < command_length) && (static_cast<uint32_t>(size) < MIDI_SYSTEM_EXCLUSIVE_INDEX_ENTRIES); size++) {
                message_.system_exclusive[size] = receive_buffer_[offset++];
                if (message_.system_exclusive[size] == 0xF7) {
                    break;
                }
            }
            size++;
            message_.data1 = static_cast<uint8_t>(size & 0xFF); // LSB
            message_.data2 = static_cast<uint8_t>(size >> 8);   // MSB
            message_.bytes_count = static_cast<uint8_t>(size);
        } break;
        default:
            break;
    }

    RTPMIDI_DEBUG_PRINTF("size=%d", static_cast<unsigned>(size));


    rtpmidi::MidiMessage(&message_);
    
    RTPMIDI_DEBUG_EXIT();
    return size;
}

void RtpMidi::HandleRtpMidi(const uint8_t* buffer) {
    RTPMIDI_DEBUG_ENTRY();

    receive_buffer_ = const_cast<uint8_t*>(buffer);

    const auto kFlags = receive_buffer_[rtpmidi::kCommandOffset];

    int32_t command_length = kFlags & RTP_MIDI_CS_MASK_SHORTLEN;
    int32_t offset;

    if ((kFlags & RTP_MIDI_CS_FLAG_B) == RTP_MIDI_CS_FLAG_B) {
        const auto kOctet = receive_buffer_[rtpmidi::kCommandOffset + 1];
        command_length = (command_length << 8) | kOctet;
        offset = rtpmidi::kCommandOffset + 2;
    } else {
        offset = rtpmidi::kCommandOffset + 1;
    }

    RTPMIDI_DEBUG_PRINTF("command_length=%u, offset=%u", static_cast<unsigned>(command_length), static_cast<unsigned>(offset));

    debug::Dump(&receive_buffer_[offset], static_cast<uint16_t>(command_length));

    uint32_t command_count = 0;

    while (command_length != 0) {
        if ((command_count != 0) || ((kFlags & RTP_MIDI_CS_FLAG_Z) == RTP_MIDI_CS_FLAG_Z)) {
            auto size = DecodeTime(static_cast<uint32_t>(command_length), static_cast<uint32_t>(offset));

            if (size < 0) {
                return;
            }

            offset += size;
            command_length -= size;
        }

        if (command_length != 0) {
            auto size = DecodeMidi(static_cast<uint32_t>(command_length), static_cast<uint32_t>(offset));

            if (size < 0) {
                return;
            }

            offset += size;
            command_length -= size;
            command_count++;
        }
    }

    RTPMIDI_DEBUG_EXIT();
}
