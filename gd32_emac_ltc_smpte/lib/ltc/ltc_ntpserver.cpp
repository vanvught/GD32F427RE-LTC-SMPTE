/**
 * @file ltc_ntpserver.cpp
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

#include "ltc_debug.h"
#include "ltc_ntpclient.h"
#include "ltc_network_ntpserver.h"

namespace ltc::ntpserver {
void Start() {
    LTC_NTPSERVER_DEBUG_ENTRY();

    ltc::ntpclient::Stop();
    ltc::network::ntpserver::Start();

    LTC_NTPSERVER_DEBUG_EXIT();
}

void Stop() {
    LTC_NTPSERVER_DEBUG_ENTRY();

    ltc::network::ntpserver::Stop();
    ltc::ntpclient::Start();

    LTC_NTPSERVER_DEBUG_EXIT();
}
} // namespace ltc::ntpserver
