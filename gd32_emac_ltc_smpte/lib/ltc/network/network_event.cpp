/**
 * @file event.cpp
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

#include "core/netif.h"
#include "display.h"
#include "ip4/ip4_address.h"
#include "emac/emac.h"
#include "network_config.h"
#include "network_iface.h"
#include "output/ltc_output_artnet.h"
#include "output/ltc_output_applemidi.h"
#include "firmware/debug/debug_debug.h"
#include "network_ptp_pps.h"

static constexpr uint32_t kLineIp = 4;

namespace network::event {
void Ipv4AddressChanged() {
    DEBUG_ENTRY();

    Display::Get()->ClearLine(kLineIp);
    Display::Get()->Printf(kLineIp, "" IPSTR "/%d %c", IP2STR(network::GetPrimaryIp()), network::GetNetmaskCIDR(), network::iface::AddressingMode());

    if (ltc::output::artnet::DestinationIp() == 0) {
        ltc::output::artnet::SetDestinationIp(netif::BroadcastIpAddr());
    }

    ltc::output::applemidi::Restart();

    network::ptp::pps::Start();

    DEBUG_EXIT();
}

void Ipv4NetmaskChanged() {
    Ipv4AddressChanged();
}

void Ipv4GatewayChanged() {}

void LinkUp() {
    emac::display::Status(true);
}

void LinkDown() {
    emac::display::Status(false);
}
} // namespace network::event
