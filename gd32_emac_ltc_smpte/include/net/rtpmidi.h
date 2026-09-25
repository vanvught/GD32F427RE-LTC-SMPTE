/**
 * @file rtpmidi.h
 * @brief RTP-MIDI implementation for real-time MIDI data transfer.
 *
 * This file provides the definition and implementation of the RtpMidi class,
 * which extends AppleMidi to add functionality for RTP-MIDI communication.
 * It supports sending and receiving raw MIDI data, timecodes, and MIDI quarter frames.
 */
/* Copyright (C) 2019-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef NET_RTPMIDI_H_
#define NET_RTPMIDI_H_

#include <cstdint>
#include <cassert>

#include "net/applemidi.h"
#include "midi.h"
#include "midi_debug.h"

namespace rtpmidi {
inline constexpr auto kBufferSize = 512U;

struct Header {
    uint16_t fixed;
    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t sender_ssrc;
} __attribute__((packed));

inline constexpr auto kCommandOffset = sizeof(struct Header);

void MidiMessage(const struct midi::Message* message);
} // namespace rtpmidi

class RtpMidi final : public AppleMidi {
   public:
    RtpMidi() noexcept {
        RTPMIDI_DEBUG_ENTRY();

        assert(s_this == nullptr);
        s_this = this;

        RTPMIDI_DEBUG_EXIT();
    }

    void Start() {
        RTPMIDI_DEBUG_ENTRY();

        AppleMidi::Start();

        send_buffer_ = new uint8_t[rtpmidi::kBufferSize];
        assert(send_buffer_ != nullptr);

        auto* header = reinterpret_cast<rtpmidi::Header*>(send_buffer_);
        header->fixed = 0x6180;
        header->sender_ssrc = AppleMidi::GetSSRC();

        RTPMIDI_DEBUG_EXIT();
    }

    void Stop() {
        RTPMIDI_DEBUG_ENTRY();

        AppleMidi::Stop();

        RTPMIDI_DEBUG_EXIT();
    }

    void TransmitRaw(uint8_t byte) {
        auto* data = &send_buffer_[rtpmidi::kCommandOffset + 1];
        data[0] = byte;
        Send(1);
    }

    void TransmitRaw(midi::Type type) { TransmitRaw(static_cast<uint8_t>(type)); }

    void SendTimeCode(const midi::Timecode* timecode) {
        auto* data = &send_buffer_[rtpmidi::kCommandOffset + 1];

        data[0] = 0xF0;
        data[1] = 0x7F;
        data[2] = 0x7F;
        data[3] = 0x01;
        data[4] = 0x01;
        data[5] = static_cast<uint8_t>(((timecode->type) & 0x03) << 5) | (timecode->hours & 0x1F);
        data[6] = timecode->minutes & 0x3F;
        data[7] = timecode->seconds & 0x3F;
        data[8] = timecode->frames & 0x1F;
        data[9] = 0xF7;

        Send(10);
    }

    void SendQf(uint8_t value) {
        auto* data = &send_buffer_[rtpmidi::kCommandOffset + 1];

        data[0] = 0xF1;
        data[1] = value;

        Send(2);
    }

    void Print() { AppleMidi::Print(); }

    static RtpMidi* Get() { return s_this; }

   private:
    void HandleRtpMidi(const uint8_t* buffer) override;

    int32_t DecodeTime(uint32_t command_length, uint32_t offset);

    int32_t DecodeMidi(uint32_t command_length, uint32_t offset);

    midi::Type GetTypeFromStatusByte(uint8_t status_byte) {
        if ((status_byte < 0x80) || (status_byte == 0xf4) || (status_byte == 0xf5) || (status_byte == 0xf9) || (status_byte == 0xfD)) {
            return midi::Type::kInvalideType;
        }

        if (status_byte < 0xF0) {
            return static_cast<midi::Type>(status_byte & 0xF0);
        }

        return static_cast<midi::Type>(status_byte);
    }

    uint8_t GetChannelFromStatusByte(uint8_t status_byte) { return static_cast<uint8_t>((status_byte & 0x0F) + 1); }

    void Send(uint32_t length) {
        auto* header = reinterpret_cast<rtpmidi::Header*>(send_buffer_);

        header->sequence_number = __builtin_bswap16(sequence_number_++);
        header->timestamp = __builtin_bswap32(AppleMidi::Now());

        send_buffer_[rtpmidi::kCommandOffset] = static_cast<uint8_t>(length); // FIXME BUG works now only

        AppleMidi::Send(send_buffer_, 1 + sizeof(struct rtpmidi::Header) + length);
    }

    midi::Message message_;
    uint8_t* receive_buffer_{nullptr}; ///< Receive buffer pointer.
    uint8_t* send_buffer_{nullptr};    ///< Send buffer pointer.
    uint16_t sequence_number_{0};      ///< Sequence number for outgoing messages.

    static inline RtpMidi* s_this; ///< Static pointer to the current instance.
};

#endif // NET_RTPMIDI_H_
