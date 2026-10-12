/**
 * @file usbd_midi_desc.cpp
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

#include "usb_conf.h"
#include "usb/usbd/midi/usbd_midi_desc.h"
#include "device/usbd/usbd_def.h"

extern "C" {
#include "usb_ch9_std.h"
#include "usbd_core.h" // IWYU pragma: keep
#include "usbd_enum.h"

__ALIGN_BEGIN const usb_desc_dev kMidiDevDesc __ALIGN_END = {
    .header =
        {
            .bLength = USB_DEV_DESC_LEN,
            .bDescriptorType = USB_DESCTYPE_DEV,
        },
    .bcdUSB = 0x0200U,
    .bDeviceClass = 0x00,//USB_AUDIO_CLASS,
    .bDeviceSubClass = 0x00,//USB_MIDISTREAMING_SUBCLASS,
    .bDeviceProtocol = 0x00U,
    .bMaxPacketSize0 = USB_FS_EP0_MAX_LEN,
    .idVendor = 0x28E9,
    .idProduct = 0x0381,
    .bcdDevice = 0x0100U,
    .iManufacturer = STR_IDX_MFC,
    .iProduct = STR_IDX_PRODUCT,
    .iSerialNumber = STR_IDX_SERIAL,
    .bNumberConfigurations = 0x01U,
};

__ALIGN_BEGIN const uint8_t kMidiConfigDesc[USB_MIDI_CONFIG_DESC_SIZE] __ALIGN_END = {
    0x09,                        /* bLength: Configuration Descriptor size */
    USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType: Configuration */
    USB_MIDI_CONFIG_DESC_SIZE,
    0x00, /*Length of the total configuration block, including this descriptor, in bytes.*/
    0x01, /*bNumInterfaces: 1 interface*/
    0x01, /*bConfigurationValue: ID of this configuration. */
    0x00, /*iConfiguration: Index of string descriptor describing the configuration (Unused.)*/
    0x80, /*bmAttributes: Bus Powered device, not Self Powered, no Remote wakeup capability. */
    0xFA, /*MaxPower 500 mA: this current is used for detecting Vbus*/

    /************** MIDI Adapter Standard MS Interface Descriptor ****************/
    0x09,                    /*bLength: Interface Descriptor size*/
    USB_DESC_TYPE_INTERFACE, /*bDescriptorType: Interface descriptor type*/
    0x00,                    /*bInterfaceNumber: Index of this interface.*/
    0x00,                    /*bAlternateSetting: Alternate setting*/
    0x02,                    /*bNumEndpoints*/
    0x01,                    /*bInterfaceClass: AUDIO*/
    0x03,                    /*bInterfaceSubClass : MIDISTREAMING*/
    0x00,                    /*nInterfaceProtocol : Unused*/
    0x00,                    /*iInterface: Unused*/

    /******************** MIDI Adapter Class-specific MS Interface Descriptor ********************/
    /* USB_MIDI_CLASS_DESC_SHIFT */
    0x07, /*bLength: Descriptor size*/
    0x24, /*bDescriptorType: CS_INTERFACE descriptor*/
    0x01, /*bDescriptorSubtype: MS_HEADER subtype*/
    0x00,
    0x01, /*BcdADC: Revision of this class specification*/
    USB_MIDI_REPORT_DESC_SIZE,
    0x00, /*wTotalLength: Total size of class-specific descriptors*/

#if MIDI_IN_PORTS_NUM >= 1
    /******************** MIDI Adapter MIDI IN Jack Descriptor (External) ********************/
    0x06,        /*bLength: Size of this descriptor, in bytes*/
    0x24,        /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,        /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x02,        /*bJackType: EXTERNAL.*/
    MIDI_JACK_1, /*bJackID: ID of this Jack.*/
    0x00,        /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (Embedded) ********************/
    0x09,        /*bLength: Size of this descriptor, in bytes*/
    0x24,        /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,        /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x01,        /*bJackType: EMBEDDED*/
    MIDI_JACK_2, /*bJackID: ID of this Jack.*/
    0x01,        /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_1, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,        /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,        /*iJack: Unused.*/
#endif

#if MIDI_IN_PORTS_NUM >= 2
    /******************** MIDI Adapter MIDI IN Jack Descriptor (External) ********************/
    0x06,        /*bLength: Size of this descriptor, in bytes*/
    0x24,        /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,        /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x02,        /*bJackType: EXTERNAL.*/
    MIDI_JACK_3, /*bJackID: ID of this Jack.*/
    0x00,        /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (Embedded) ********************/
    0x09,        /*bLength: Size of this descriptor, in bytes*/
    0x24,        /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,        /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x01,        /*bJackType: EMBEDDED*/
    MIDI_JACK_4, /*bJackID: ID of this Jack.*/
    0x01,        /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_3, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,        /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,        /*iJack: Unused.*/
#endif

#if MIDI_IN_PORTS_NUM >= 3
    /******************** MIDI Adapter MIDI IN Jack Descriptor (External) ********************/
    0x06,        /*bLength: Size of this descriptor, in bytes*/
    0x24,        /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,        /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x02,        /*bJackType: EXTERNAL.*/
    MIDI_JACK_5, /*bJackID: ID of this Jack.*/
    0x00,        /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (Embedded) ********************/
    0x09,        /*bLength: Size of this descriptor, in bytes*/
    0x24,        /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,        /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x01,        /*bJackType: EMBEDDED*/
    MIDI_JACK_6, /*bJackID: ID of this Jack.*/
    0x01,        /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_5, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,        /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,        /*iJack: Unused.*/
#endif

#if MIDI_IN_PORTS_NUM >= 4
    /******************** MIDI Adapter MIDI IN Jack Descriptor (External) ********************/
    0x06,        /*bLength: Size of this descriptor, in bytes*/
    0x24,        /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,        /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x02,        /*bJackType: EXTERNAL.*/
    MIDI_JACK_7, /*bJackID: ID of this Jack.*/
    0x00,        /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (Embedded) ********************/
    0x09,        /*bLength: Size of this descriptor, in bytes*/
    0x24,        /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,        /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x01,        /*bJackType: EMBEDDED*/
    MIDI_JACK_8, /*bJackID: ID of this Jack.*/
    0x01,        /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_7, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,        /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,        /*iJack: Unused.*/
#endif

#if MIDI_IN_PORTS_NUM >= 5
    /******************** MIDI Adapter MIDI IN Jack Descriptor (External) ********************/
    0x06,        /*bLength: Size of this descriptor, in bytes*/
    0x24,        /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,        /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x02,        /*bJackType: EXTERNAL.*/
    MIDI_JACK_9, /*bJackID: ID of this Jack.*/
    0x00,        /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (Embedded) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_10, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_9,  /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_IN_PORTS_NUM >= 6
    /******************** MIDI Adapter MIDI IN Jack Descriptor (External) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_11, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (Embedded) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_12, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_11, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_IN_PORTS_NUM >= 7
    /******************** MIDI Adapter MIDI IN Jack Descriptor (External) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_13, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (Embedded) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_14, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_13, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_IN_PORTS_NUM >= 8
    /******************** MIDI Adapter MIDI IN Jack Descriptor (External) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_15, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (Embedded) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_16, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_15, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_OUT_PORTS_NUM >= 1
    /******************** MIDI Adapter MIDI IN Jack Descriptor (Embedded) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_17, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (External) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_18, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_17, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_OUT_PORTS_NUM >= 2
    /******************** MIDI Adapter MIDI IN Jack Descriptor (Embedded) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_19, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (External) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_20, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_19, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_OUT_PORTS_NUM >= 3
    /******************** MIDI Adapter MIDI IN Jack Descriptor (Embedded) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_21, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (External) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_22, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_21, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_OUT_PORTS_NUM >= 4
    /******************** MIDI Adapter MIDI IN Jack Descriptor (Embedded) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_23, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (External) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_24, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_23, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_OUT_PORTS_NUM >= 5
    /******************** MIDI Adapter MIDI IN Jack Descriptor (Embedded) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_25, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (External) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_26, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_25, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_OUT_PORTS_NUM >= 6
    /******************** MIDI Adapter MIDI IN Jack Descriptor (Embedded) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_27, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (External) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_28, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_27, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_OUT_PORTS_NUM >= 7
    /******************** MIDI Adapter MIDI IN Jack Descriptor (Embedded) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_29, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (External) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_30, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_29, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

#if MIDI_OUT_PORTS_NUM >= 8
    /******************** MIDI Adapter MIDI IN Jack Descriptor (Embedded) ********************/
    0x06,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x02,         /*bDescriptorSubtype: MIDI_IN_JACK subtype*/
    0x01,         /*bJackType: EMBEDDED*/
    MIDI_JACK_31, /*bJackID: ID of this Jack.*/
    0x00,         /*iJack: Unused.*/

    /******************** MIDI Adapter MIDI OUT Jack Descriptor (External) ********************/
    0x09,         /*bLength: Size of this descriptor, in bytes*/
    0x24,         /*bDescriptorType: CS_INTERFACE descriptor.*/
    0x03,         /*bDescriptorSubtype: MIDI_OUT_JACK subtype*/
    0x02,         /*bJackType: EXTERNAL.*/
    MIDI_JACK_32, /*bJackID: ID of this Jack.*/
    0x01,         /*bNrInputPins: Number of Input Pins of this Jack.*/
    MIDI_JACK_31, /*BaSourceID(1): ID of the Entity to which this Pin is connected.*/
    0x01,         /*BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected.*/
    0x00,         /*iJack: Unused.*/
#endif

    /******************** MIDI Adapter Standard Bulk OUT Endpoint Descriptor ********************/
    0x09,                   /*bLength: Size of this descriptor, in bytes*/
    USB_DESC_TYPE_ENDPOINT, /*bDescriptorType: ENDPOINT descriptor.*/
    MIDI_EPOUT_ADDR,        /*bEndpointAddress: OUT Endpoint 1.*/
    0x02,                   /*bmAttributes: Bulk, not shared.*/
    MIDI_EPOUT_SIZE,
    0x00, /*wMaxPacketSize*/
    0x00, /*bInterval: Ignored for Bulk. Set to zero.*/
    0x00, /*bRefresh: Unused.*/
    0x00, /*bSynchAddress: Unused.*/

    /******************** MIDI Adapter Class-specific Bulk OUT Endpoint Descriptor ********************/
    (4 + MIDI_OUT_PORTS_NUM), /*bLength: Size of this descriptor, in bytes*/
    0x25,                     /*bDescriptorType: CS_ENDPOINT descriptor*/
    0x01,                     /*bDescriptorSubtype: MS_GENERAL subtype.*/
    MIDI_OUT_PORTS_NUM,       /*bNumEmbMIDIJack: Number of embedded MIDI IN Jacks.*/
#if MIDI_OUT_PORTS_NUM >= 1
    MIDI_JACK_17, /*BaAssocJackID(1): ID of the Embedded MIDI IN Jack.*/
#endif
#if MIDI_OUT_PORTS_NUM >= 2
    MIDI_JACK_19, /*BaAssocJackID(2): ID of the Embedded MIDI IN Jack.*/
#endif
#if MIDI_OUT_PORTS_NUM >= 3
    MIDI_JACK_21, /*BaAssocJackID(3): ID of the Embedded MIDI IN Jack.*/
#endif
#if MIDI_OUT_PORTS_NUM >= 4
    MIDI_JACK_23, /*BaAssocJackID(4): ID of the Embedded MIDI IN Jack.*/
#endif
#if MIDI_OUT_PORTS_NUM >= 5
    MIDI_JACK_25, /*BaAssocJackID(5): ID of the Embedded MIDI IN Jack.*/
#endif
#if MIDI_OUT_PORTS_NUM >= 6
    MIDI_JACK_27, /*BaAssocJackID(6): ID of the Embedded MIDI IN Jack.*/
#endif
#if MIDI_OUT_PORTS_NUM >= 7
    MIDI_JACK_29, /*BaAssocJackID(7): ID of the Embedded MIDI IN Jack.*/
#endif
#if MIDI_OUT_PORTS_NUM >= 8
    MIDI_JACK_31, /*BaAssocJackID(8): ID of the Embedded MIDI IN Jack.*/
#endif

    /******************** MIDI Adapter Standard Bulk IN Endpoint Descriptor ********************/
    0x09,                   /*bLength: Size of this descriptor, in bytes*/
    USB_DESC_TYPE_ENDPOINT, /*bDescriptorType: ENDPOINT descriptor.*/
    MIDI_EPIN_ADDR,         /*bEndpointAddress: IN Endpoint 1.*/
    0x02,                   /*bmAttributes: Bulk, not shared.*/
    MIDI_EPIN_SIZE,
    0x00, /*wMaxPacketSize*/
    0x00, /*bInterval: Ignored for Bulk. Set to zero.*/
    0x00, /*bRefresh: Unused.*/
    0x00, /*bSynchAddress: Unused.*/

    /******************** MIDI Adapter Class-specific Bulk IN Endpoint Descriptor ********************/
    (4 + MIDI_IN_PORTS_NUM), /*bLength: Size of this descriptor, in bytes*/
    0x25,                    /*bDescriptorType: CS_ENDPOINT descriptor*/
    0x01,                    /*bDescriptorSubtype: MS_GENERAL subtype.*/
    MIDI_IN_PORTS_NUM,       /*bNumEmbMIDIJack: Number of embedded MIDI OUT Jacks.*/
#if MIDI_IN_PORTS_NUM >= 1
    MIDI_JACK_2, /*BaAssocJackID(1): ID of the Embedded MIDI OUT Jack.*/
#endif
#if MIDI_IN_PORTS_NUM >= 2
    MIDI_JACK_4, /*BaAssocJackID(2): ID of the Embedded MIDI OUT Jack.*/
#endif
#if MIDI_IN_PORTS_NUM >= 3
    MIDI_JACK_6, /*BaAssocJackID(3): ID of the Embedded MIDI OUT Jack.*/
#endif
#if MIDI_IN_PORTS_NUM >= 4
    MIDI_JACK_8, /*BaAssocJackID(4): ID of the Embedded MIDI OUT Jack.*/
#endif
#if MIDI_IN_PORTS_NUM >= 5
    MIDI_JACK_10, /*BaAssocJackID(5): ID of the Embedded MIDI OUT Jack.*/
#endif
#if MIDI_IN_PORTS_NUM >= 6
    MIDI_JACK_12, /*BaAssocJackID(6): ID of the Embedded MIDI OUT Jack.*/
#endif
#if MIDI_IN_PORTS_NUM >= 7
    MIDI_JACK_14, /*BaAssocJackID(7): ID of the Embedded MIDI OUT Jack.*/
#endif
#if MIDI_IN_PORTS_NUM >= 8
    MIDI_JACK_16, /*BaAssocJackID(8): ID of the Embedded MIDI OUT Jack.*/
#endif
};

static __ALIGN_BEGIN const usb_desc_LANGID kLangIdDesc __ALIGN_END = {
    .header =
        {
            .bLength = sizeof(usb_desc_LANGID),
            .bDescriptorType = USB_DESCTYPE_STR,
        },
    .wLANGID = ENG_LANGID,
};

static __ALIGN_BEGIN const usb_desc_str kManufacturerStr __ALIGN_END = {
    .header = {.bLength = USB_STRING_LEN(12U), .bDescriptorType = USB_DESCTYPE_STR},
    .unicode_string = {'g', 'd', '3', '2', '-', 'd', 'm', 'x', '.', 'o', 'r', 'g'},
};

static __ALIGN_BEGIN const usb_desc_str kProductStr __ALIGN_END = {
    .header = {.bLength = USB_STRING_LEN(13U), .bDescriptorType = USB_DESCTYPE_STR},
    .unicode_string = {'M', 'I', 'D', 'I', ' ', 'T', 'i', 'm', 'e',  'c', 'o', 'd', 'e'},
};

static __ALIGN_BEGIN usb_desc_str serial_str __ALIGN_END = {
    .header = {.bLength = USB_STRING_LEN(12U), .bDescriptorType = USB_DESCTYPE_STR},
    .unicode_string = {},
};

static_assert(STR_IDX_LANGID == 0, "STR_IDX_LANGID must be 0");
static_assert(STR_IDX_MFC == 1, "STR_IDX_MFC must be 1");
static_assert(STR_IDX_PRODUCT == 2, "STR_IDX_PRODUCT must be 2");
static_assert(STR_IDX_SERIAL == 3, "STR_IDX_SERIAL must be 3");
static_assert(STR_IDX_COUNT == 4, "Update initializer list if you add indices");

using MidiStringPtr = void*;
MidiStringPtr const kUsbdMidiStrings[STR_IDX_COUNT] = {
    const_cast<void*>(static_cast<const void*>(&kLangIdDesc)),
    const_cast<void*>(static_cast<const void*>(&kManufacturerStr)),
    const_cast<void*>(static_cast<const void*>(&kProductStr)),
    const_cast<void*>(static_cast<const void*>(&serial_str)),
};

usb_desc midi_desc = {
    .dev_desc = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&kMidiDevDesc)),
    .config_desc = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&kMidiConfigDesc)),
    .bos_desc = nullptr,
    .strings = kUsbdMidiStrings,
};

__ALIGN_BEGIN const usb_desc_ep midi_ep_in __ALIGN_END = {
    .header = {.bLength = sizeof(usb_desc_ep), .bDescriptorType = USB_DESCTYPE_EP},
    .bEndpointAddress = USB_MIDI_IN_EP,
    .bmAttributes = USB_EP_ATTR_BULK,
    .wMaxPacketSize = USB_MIDI_PACKET_SIZE,
    .bInterval = 0x00,
};

__ALIGN_BEGIN const usb_desc_ep midi_ep_out __ALIGN_END = {
    .header = {.bLength = sizeof(usb_desc_ep), .bDescriptorType = USB_DESCTYPE_EP}, .bEndpointAddress = USB_MIDI_OUT_EP, .bmAttributes = USB_EP_ATTR_BULK, .wMaxPacketSize = USB_MIDI_PACKET_SIZE, .bInterval = 0x00,};
}