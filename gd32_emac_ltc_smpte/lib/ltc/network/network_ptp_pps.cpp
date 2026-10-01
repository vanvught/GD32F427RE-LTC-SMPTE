/*
 * network_ptp_pps.cpp
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

#include <sys/time.h>

#include "gd32.h"
#include "gd32_ptp.h"
#include "gd32f4xx_enet.h"
#include "ltc.h"
#include "ltc_timecode.h"
#include "input/ltc_input.h"
#include "ltc_debug.h"

namespace network::ptp::pps {
void Start() {
    LTC_DEBUG_ENTRY();

    enet_ptp_systime_struct systime{};
    enet_ptp_system_time_get(&systime);

    LTC_DEBUG_PRINTF("PTP now: %u", static_cast<unsigned>(systime.second));

    enet_ptp_expected_time_config(systime.second + 1, 0);

    LTC_DEBUG_PRINTF("target: %u", static_cast<unsigned>(ENET_PTP_ETH));

    enet_ptp_feature_enable(ENET_PTP_TIMESTAMP_INT);
    enet_interrupt_enable(ENET_MAC_INT_TMSTIM);
    NVIC_EnableIRQ(ENET_IRQn);

    LTC_DEBUG_EXIT();
}
void Stop() {
    LTC_DEBUG_ENTRY();

    NVIC_DisableIRQ(ENET_IRQn);

    enet_interrupt_disable(ENET_MAC_INT_TMSTIM);

    enet_ptp_feature_disable(ENET_PTP_TIMESTAMP_INT);

    LTC_DEBUG_EXIT();
}
} // namespace network::ptp::pps

extern "C" void ENET_IRQHandler() {
    if (RESET == enet_interrupt_flag_get(ENET_MAC_INT_FLAG_TMST)) {
        return;
    }

    if (SET != enet_flag_get(ENET_PTP_FLAG_TTM)) {
    }
   
    const auto kCurrentSeconds = ENET_PTP_ETH;
    enet_ptp_expected_time_config(kCurrentSeconds + 1U, 0U);
    enet_ptp_feature_enable(ENET_PTP_TIMESTAMP_INT);

    if (ltc::input::Source::Instance().Input() == ltc::Input::kSystime) {
        const timeval kTimeVal{
            .tv_sec = static_cast<int>(kCurrentSeconds),
            .tv_usec = 0,
        };

        ltc::timecode::Sync(kTimeVal);
    }

    GPIO_TG(LED2_GPIOx) = LED2_GPIO_PINx;
}