/**
 * @file usbd_midi.cpp
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

#include "device/usb.h"
#include "usb/usbd/midi/usbd_midi_core.h"   // IWYU pragma: keep"
#include "gd32.h"                           // IWYU pragma: keep

extern "C" {
#include "drv_usbd_int.h"
#include "usbd_core.h"
}

usb_core_driver usb_midi;

#ifndef USE_USB_FS
#error
#endif // USE_USB_FS

namespace usb {
void Init() {
    RcuConfig();
    GpioConfig();

    usbd_init(&usb_midi, USB_CORE_ENUM_FS, &midi_desc, &midi_class);

    IntrConfig();
}
} // namespace usb

extern "C" void USBFS_IRQHandler() {
    usbd_isr(&usb_midi);
}
