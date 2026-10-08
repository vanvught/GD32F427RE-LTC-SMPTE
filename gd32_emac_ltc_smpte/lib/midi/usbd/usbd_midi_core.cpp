/**
 * @file usbd_midi_core.cpp
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
#include <cstring>
#include <cstdio>

#include "usb/usbd//midi//usbd_midi_desc.h"
#include "usb/usbd/midi/usbd_midi_core.h"
#include "usb/usbd/midi/usbd_midi_queue.h"
#include "usb/usbd/midi/usbd_midi.h"

extern "C" {
#include "drv_usb_core.h"
#include "usbd_enum.h"
}

uint8_t UsbMidiItfopRegister(usb_dev* udev, struct midi_fop_handler* midi_fop) {
    if (nullptr != midi_fop) {
        udev->dev.user_data = midi_fop;
        return USBD_OK;
    }
    return USBD_FAIL;
}

static uint8_t MidiInit(usb_dev* udev, uint8_t config_index);
static uint8_t MidiDeinit(usb_dev* udev, uint8_t config_index);
static uint8_t MidiReqProc(usb_dev* udev, usb_req* req);
static uint8_t MidiIn(usb_dev* udev, uint8_t ep_num);
static uint8_t MidiOut(usb_dev* udev, uint8_t ep_num);
static uint8_t MidiSof(usb_dev* udev);

usb_class_core midi_class = {
    .command = NO_CMD,
    .alter_set = 0U,
    .init = MidiInit,
    .deinit = MidiDeinit,
    .req_proc = MidiReqProc,
    .set_intf = nullptr, // No alternate settings for USB MIDI
    .ctlx_in = nullptr,  // Control IN transfer (not needed for MIDI)
    .ctlx_out = nullptr, // Control OUT transfer (not needed for MIDI)
    .data_in = MidiIn,
    .data_out = MidiOut,
    .SOF = MidiSof,                 // Kick queued MIDI after USB configuration
    .incomplete_isoc_in = nullptr,  // MIDI does not use ISO IN
    .incomplete_isoc_out = nullptr, // MIDI does not use ISO OUT
};

static __ALIGN_BEGIN usb_midi_handler midi_handler __ALIGN_END;

static TxQueue<4> tx_queue;
static volatile auto tx_busy{false};

static uint8_t MidiInit(usb_dev* udev, [[maybe_unused]] uint8_t config_index) {
    puts("> MidiInit");

    memset(&midi_handler, 0, sizeof(usb_midi_handler));

    tx_busy = false;

    // Setup MIDI IN and OUT endpoints
    usbd_ep_setup(static_cast<usb_core_driver*>(udev), &midi_ep_in);
    usbd_ep_setup(static_cast<usb_core_driver*>(udev), &midi_ep_out);

    // Assign class data storage
    udev->dev.class_data[0] = &midi_handler;

    // Prepare for receiving MIDI data
    usbd_ep_recev(udev, MIDI_EPOUT_ADDR, midi_handler.rx_packet, MIDI_EPOUT_SIZE);

    puts("< MidiInit");
    return USBD_OK;
}

static uint8_t MidiDeinit(usb_dev* udev, [[maybe_unused]] uint8_t config_index) {
    puts("> MidiDeinit");

    usbd_ep_clear(udev, MIDI_EPIN_ADDR);
    usbd_ep_clear(udev, MIDI_EPOUT_ADDR);

    puts("< MidiDeinit");
    return USBD_OK;
}

static uint8_t MidiReqProc([[maybe_unused]] usb_dev* udev, [[maybe_unused]] usb_req* req) {
    puts("> MidiReqProc");

    puts("< MidiReqProc");
    return USBD_OK;
}

static bool IsConfigured(usb_dev* udev) {
    return (udev->dev.cur_status == USBD_CONFIGURED);
}

static void KickNextIn(usb_dev* udev) {
    if (!IsConfigured(udev)) [[unlikely]] {
        return;
    }

    if (tx_busy) {
        return;
    }

    uint8_t* ptr;
    uint16_t length;

    if (!tx_queue.NextPacket(&ptr, &length)) {
        return;
    }

    tx_busy = true;
    __DMB();

    usbd_ep_send(udev, MIDI_EPIN_ADDR, ptr, length);
}

static uint8_t MidiSof(usb_dev* udev) {
    KickNextIn(udev);
    return USBD_OK;
}

static uint8_t MidiIn(usb_dev* udev, uint8_t ep_num) {
    if ((ep_num & 0x7FU) != (MIDI_EPIN_ADDR & 0x7FU)) {
        return USBD_OK;
    }

    tx_queue.OnPacketSent();
    tx_busy = false;

    KickNextIn(udev);

    return USBD_OK;
}

static RxQueue<32U> rx_queue;

// In file included from ./include/usb/usbd//midi//usbd_midi_desc.h:103,
//                  from lib/midi/usbd/usbd_midi_core.cpp:30:
// lib/midi/usbd/usbd_midi_core.cpp: In function 'uint8_t MidiDataOut(usb_dev*, uint8_t)':
//../lib-gd32/gd32f4xx/GD32F4xx_usb_library/driver/Include/drv_usb_core.h:44:67: error: use of old-style cast to 'uint8_t' {aka 'unsigned char'} [-Werror=old-style-cast]
//    44 | #define EP_ID(x)                            ((uint8_t)((x) & 0x7FU))            /*!< endpoint number */
//       |                                                                   ^
// lib/midi/usbd/usbd_midi_core.cpp:116:87: note: in expansion of macro 'EP_ID'
//   116 |     const auto kBytesReceived = (static_cast<usb_core_driver*>(udev))->dev.transc_out[EP_ID(ep_num)].xfer_count;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

static uint8_t MidiOut(usb_dev* udev, uint8_t ep_num) {
    if ((ep_num & 0x7F) != (MIDI_EPOUT_ADDR & 0x7F)) {
        return USBD_OK;
    }

    auto* midi = static_cast<usb_midi_handler*>(udev->dev.class_data[0]);

    const auto kBytesReceived = static_cast<usb_core_driver*>(udev)->dev.transc_out[EP_ID(ep_num)].xfer_count;

    assert((kBytesReceived % 4U) == 0);

    for (uint32_t offset = 0; offset < kBytesReceived; offset += 4U) {
        rx_queue.Push(&midi->rx_packet[offset]);
    }

    usbd_ep_recev(udev, MIDI_EPOUT_ADDR, midi->rx_packet, MIDI_EPOUT_SIZE);

    return USBD_OK;
}

#pragma GCC diagnostic pop

namespace usbmidi {
bool Message(MidiEvent& event) {
    const auto* segment = rx_queue.GetFront();
    if (segment == nullptr) {
        return false;
    }

    event.cable_cin = segment->data[0];
    event.midi[0] = segment->data[1];
    event.midi[1] = segment->data[2];
    event.midi[2] = segment->data[3];

    rx_queue.Pop();

    return true;
}

bool Send(const uint8_t* data, uint32_t length) {
    if (!tx_queue.Enqueue(data, length)) [[unlikely]] {
        return false;
    }

    KickNextIn(&usb_midi);

    return true;
}
} // namespace usbmidi
