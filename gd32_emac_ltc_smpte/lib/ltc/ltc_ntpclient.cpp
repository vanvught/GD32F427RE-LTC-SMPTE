/**
 * @file ltc_ntpclient.cpp
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

#include "apps/ntpclient.h"
#include "ltc_gps.h"
#include "ltc_debug.h"

namespace ltc::ntpclient {
void Start() {
    LTC_NTPCLIENT_DEBUG_ENTRY();

    if (ltc::gps::IsRunning()) {
        LTC_NTPCLIENT_DEBUG_EXIT();
        return;
    }

#ifdef CONFIG_NET_ENABLE_NTP_CLIENT
    ::network::apps::ntpclient::Stop(false);
    ::network::apps::ntpclient::Start();
#endif // CONFIG_NET_ENABLE_NTP_CLIENT
#ifdef CONFIG_NET_ENABLE_PTP_NTP_CLIENT
    ::network::apps::ntpclient::ptp::Stop(false);
    ::network::apps::ntpclient::ptp::Start();
#endif // CONFIG_NET_ENABLE_PTP_NTP_CLIENT

    LTC_NTPCLIENT_DEBUG_EXIT();
}

void Stop() {
    LTC_NTPCLIENT_DEBUG_ENTRY();

#ifdef CONFIG_NET_ENABLE_NTP_CLIENT
    ::network::apps::ntpclient::Stop(true);
#endif // CONFIG_NET_ENABLE_NTP_CLIENT
#ifdef CONFIG_NET_ENABLE_PTP_NTP_CLIENT
    ::network::apps::ntpclient::ptp::Stop(true);
#endif // CONFIG_NET_ENABLE_PTP_NTP_CLIENT

    LTC_NTPCLIENT_DEBUG_EXIT();
}
} // namespace ltc::ntpclient
