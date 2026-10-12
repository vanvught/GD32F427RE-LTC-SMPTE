/**
 * @file usbd_midi_queue.h
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

#ifndef USB_USBD_BULKMSGQUEUE_H_
#define USB_USBD_BULKMSGQUEUE_H_

#include <cstdint>
#include <cstring>
#include <cassert>

#include "gd32.h" // IWYU pragma: keep

template <uint32_t kQueueSize>
class RxQueue {
    static_assert(kQueueSize > 0 && (kQueueSize & (kQueueSize - 1)) == 0, "Value must be a power of 2!");

   public:
    static_assert(kQueueSize >= 2);

    static constexpr uint32_t kDataSize = 4;

    struct MidiEvent {
        uint8_t data[kDataSize];
    };

    RxQueue() = default;
    RxQueue(const RxQueue&) = delete;
    RxQueue& operator=(const RxQueue&) = delete;
    RxQueue(RxQueue&&) = delete;
    RxQueue& operator=(RxQueue&&) = delete;

    bool Push(const uint8_t* data) {
        assert(data != nullptr);

        const auto kNext = (head_ + 1) % kQueueSize;

        if (kNext == tail_) {
            return false;
        }

        auto& data_segment = midi_event_[head_];

        memcpy(data_segment.data, data, 4);

        __DMB();
        head_ = kNext;
        return true;
    }

    void Pop() {
        if (head_ == tail_) {
            return;
        }

        __DMB();
        tail_ = (tail_ + 1) % kQueueSize;
    }

    MidiEvent* GetFront() {
        if (IsEmpty()) [[unlikely]] {
            return nullptr;
        }

        __DMB();
        return &midi_event_[tail_];
    }

    [[nodiscard]] bool IsEmpty() const { return head_ == tail_; }

    [[nodiscard]] bool IsFull() const { return ((head_ + 1) % kQueueSize) == tail_; }

   private:
    MidiEvent midi_event_[kQueueSize];
    volatile uint32_t head_{0}; // written by ISR, read by main
    volatile uint32_t tail_{0}; // written by main, read by ISR
};

template <uint32_t kQueueSize>
class TxQueue {
    static_assert(kQueueSize > 0 && (kQueueSize & (kQueueSize - 1)) == 0, "Value must be a power of 2!");
    static_assert(kQueueSize >= 2);

   public:
    static constexpr uint32_t kDataSize = 64;

    struct DataSegment {
        uint8_t buffer[kDataSize];
        uint16_t length;
    };

    TxQueue() = default;
    TxQueue(const TxQueue&) = delete;
    TxQueue& operator=(const TxQueue&) = delete;
    TxQueue(TxQueue&&) = delete;
    TxQueue& operator=(TxQueue&&) = delete;

    bool Enqueue(const uint8_t* data, uint32_t length) {
        assert(data != nullptr);
        assert(length != 0);
        assert(length <= kDataSize);

        const auto kNext = (head_ + 1U) % kQueueSize;

        if (kNext == tail_) [[unlikely]] {
            return false;
        }

        auto& segment = data_segment_[head_];

        memcpy(segment.buffer, data, length);
        segment.length = static_cast<uint16_t>(length);

        __DMB();
        head_ = kNext;

        return true;
    }

    bool NextPacket(uint8_t** ptr, uint16_t* length) {
        assert(ptr != nullptr);
        assert(length != nullptr);

        if (head_ == tail_) {
            return false;
        }

        auto& segment = data_segment_[tail_];

        __DMB();

        *ptr = segment.buffer;
        *length = segment.length;

        return true;
    }

    void OnPacketSent() {
        if (head_ == tail_) [[unlikely]] {
            return;
        }

        __DMB();
        tail_ = (tail_ + 1U) % kQueueSize;
    }

    [[nodiscard]] bool IsEmpty() const { return head_ == tail_; }

    [[nodiscard]] bool IsFull() const { return ((head_ + 1U) % kQueueSize) == tail_; }

   private:
    DataSegment data_segment_[kQueueSize];
    volatile uint32_t head_{0};
    volatile uint32_t tail_{0};
};
#endif // USB_USBD_BULKMSGQUEUE_H_
