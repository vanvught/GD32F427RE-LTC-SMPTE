/**
 * @file usbd_midi_desc.h
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

#ifndef USB_USBD_MIDI_USBD_MIDI_DESC_H_
#define USB_USBD_MIDI_USBD_MIDI_DESC_H_

#ifndef MIDI_IN_PORTS_NUM
#define MIDI_IN_PORTS_NUM 1
#endif

#ifndef MIDI_OUT_PORTS_NUM
#define MIDI_OUT_PORTS_NUM 1
#endif

#define USB_AUDIO_CLASS 0x01
#define USB_MIDISTREAMING_SUBCLASS 0x03

#define USB_MIDI_IN_EP 0x81U
#define USB_MIDI_OUT_EP 0x01U
#define USB_MIDI_PACKET_SIZE 64U

#define MIDI_EPIN_ADDR USB_MIDI_IN_EP
#define MIDI_EPIN_SIZE USB_MIDI_PACKET_SIZE

#define MIDI_EPOUT_ADDR USB_MIDI_OUT_EP
#define MIDI_EPOUT_SIZE USB_MIDI_PACKET_SIZE

#define USB_MIDI_CLASS_DESC_SHIFT 18
#define USB_MIDI_REPORT_DESC_SIZE ((MIDI_IN_PORTS_NUM * 16) + (MIDI_OUT_PORTS_NUM * 16) + 33)
#define USB_MIDI_CONFIG_DESC_SIZE (USB_MIDI_REPORT_DESC_SIZE + USB_MIDI_CLASS_DESC_SHIFT)

#define MIDI_DESCRIPTOR_TYPE 0x21

#define MIDI_REQ_SET_PROTOCOL 0x0B
#define MIDI_REQ_GET_PROTOCOL 0x03

#define MIDI_REQ_SET_IDLE 0x0A
#define MIDI_REQ_GET_IDLE 0x02

#define MIDI_REQ_SET_REPORT 0x09
#define MIDI_REQ_GET_REPORT 0x01

#define MIDI_JACK_1 0x01
#define MIDI_JACK_2 0x02
#define MIDI_JACK_3 0x03
#define MIDI_JACK_4 0x04
#define MIDI_JACK_5 0x05
#define MIDI_JACK_6 0x06
#define MIDI_JACK_7 0x07
#define MIDI_JACK_8 0x08
#define MIDI_JACK_9 0x09
#define MIDI_JACK_10 0x0a
#define MIDI_JACK_11 0x0b
#define MIDI_JACK_12 0x0c
#define MIDI_JACK_13 0x0d
#define MIDI_JACK_14 0x0e
#define MIDI_JACK_15 0x0f
#define MIDI_JACK_16 0x10
#define MIDI_JACK_17 (MIDI_IN_PORTS_NUM * 2 + 0x01)
#define MIDI_JACK_18 (MIDI_IN_PORTS_NUM * 2 + 0x02)
#define MIDI_JACK_19 (MIDI_IN_PORTS_NUM * 2 + 0x03)
#define MIDI_JACK_20 (MIDI_IN_PORTS_NUM * 2 + 0x04)
#define MIDI_JACK_21 (MIDI_IN_PORTS_NUM * 2 + 0x05)
#define MIDI_JACK_22 (MIDI_IN_PORTS_NUM * 2 + 0x06)
#define MIDI_JACK_23 (MIDI_IN_PORTS_NUM * 2 + 0x07)
#define MIDI_JACK_24 (MIDI_IN_PORTS_NUM * 2 + 0x08)
#define MIDI_JACK_25 (MIDI_IN_PORTS_NUM * 2 + 0x09)
#define MIDI_JACK_26 (MIDI_IN_PORTS_NUM * 2 + 0x0a)
#define MIDI_JACK_27 (MIDI_IN_PORTS_NUM * 2 + 0x0b)
#define MIDI_JACK_28 (MIDI_IN_PORTS_NUM * 2 + 0x0c)
#define MIDI_JACK_29 (MIDI_IN_PORTS_NUM * 2 + 0x0d)
#define MIDI_JACK_30 (MIDI_IN_PORTS_NUM * 2 + 0x0e)
#define MIDI_JACK_31 (MIDI_IN_PORTS_NUM * 2 + 0x0f)
#define MIDI_JACK_32 (MIDI_IN_PORTS_NUM * 2 + 0x10)

#define STR_IDX_COUNT 4

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#include "drv_usb_core.h"

/*/
struct midi_cfg_tree_t {
    usb_desc_config config;
    usb_desc_itf itf;
    usb_desc_ep ep_out;
    usb_desc_ep ep_in;
};

extern const midi_cfg_tree_t kMidiConfigDesc;
*/

extern usb_desc midi_desc;
extern const usb_desc_ep midi_ep_in;
extern const usb_desc_ep midi_ep_out;
extern usb_core_driver usb_midi;

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus
using UsbClass = usb_class_core;
extern UsbClass midi_class;
#endif // __cplusplus

#endif // USB_USBD_MIDI_USBD_MIDI_DESC_H_
