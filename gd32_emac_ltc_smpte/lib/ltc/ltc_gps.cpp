/**
 * @file ltc_gps.cpp
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

#include "gnss.h"
#include "ltc_gps.h"
#include "ltc_gpio_config.h"
#include "ltc_debug.h"
#include "display.h" // IWYU pragma: keep
#include "gd32.h"    // IWYU pragma: keep

namespace ltc::gps {
namespace {
gnss::Receiver receiver;
}
void Start() {
    LTC_GPS_DEBUG_ENTRY();

    rcu_periph_clock_enable(RCU_SYSCFG);
    rcu_periph_clock_enable(PPS_INPUT_RCU_GPIOx);

    gpio_mode_set(PPS_INPUT_GPIOx, GPIO_MODE_INPUT, GPIO_PUPD_NONE, PPS_INPUT_GPIO_PINx);

    syscfg_exti_line_config(PPS_INPUT_EXTI_SOURCE_GPIOx, PPS_INPUT_EXTI_SOURCE_PINx);

    exti_init(PPS_INPUT_EXTI_x, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_flag_clear(PPS_INPUT_EXTI_x);

    NVIC_ClearPendingIRQ(PPS_INPUT_EXTIx_IRQn);
    NVIC_SetPriority(PPS_INPUT_EXTIx_IRQn, gnss::kPpsIrqPriority);
    NVIC_EnableIRQ(PPS_INPUT_EXTIx_IRQn);

    receiver.Start();

    LTC_GPS_DEBUG_EXIT();
}

void Stop() {
    LTC_GPS_DEBUG_ENTRY();

    NVIC_DisableIRQ(PPS_INPUT_EXTIx_IRQn);

    receiver.Stop();

    LTC_GPS_DEBUG_EXIT();
}
} // namespace ltc::gps

namespace gnss {
void StatusChanged(gnss::Status status) {
    switch (status) {
        case gnss::Status::kIdle:
            Display::Get()->TextStatus("GPS Idle");
            break;
        case gnss::Status::kWarning:
            Display::Get()->TextStatus("GPS Warning");
            break;
        case gnss::Status::kValid:
            Display::Get()->TextStatus("GPS Valid");
            break;
        case gnss::Status::kStopped:
            Display::Get()->TextStatus("GPS Stopped");
            break;
        case gnss::Status::kUndefined:
            Display::Get()->TextStatus("GPS Error");
            break;
    }
}
} // namespace gnss

namespace network::apps::ntpclient::systime {
void TimeUpdated(const timeval& time_val);
}

extern "C" {
void PPS_INPUT_EXTIx_IRQHandler() {
    if (RESET != exti_interrupt_flag_get(PPS_INPUT_EXTI_x)) {
        exti_interrupt_flag_clear(PPS_INPUT_EXTI_x);

        const auto kLocalSeconds = gnss::Receiver::Instance().LocalSeconds();
        const timeval kTimeVal{.tv_sec = (kLocalSeconds + 1), .tv_usec = 0};

        settimeofday(&kTimeVal, nullptr);

        network::apps::ntpclient::systime::TimeUpdated(kTimeVal);

        GPIO_TG(LED2_GPIOx) = LED2_GPIO_PINx;
    }
}
}