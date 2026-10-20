/**
 * @file pixel_output.cpp
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

#include "pixel_output.h"
#include "pixel_const.h"
#include "ltc_gpio_config.h"
#include "gd32_gpio.h"
#include "gd32.h" // IWYU pragma: keep
#include "gd32_dma.h"

namespace pixel::output {
namespace {
constexpr uint16_t kGpioPin[] __attribute__((aligned(4))) = {PIXEL_OUTPUT_GPIO_PINx};
const auto* const kPgpioPin = reinterpret_cast<const uint32_t*>(&kGpioPin[0]);

constexpr uint32_t kBufferSize = ::pixel::kPixelsMax * 3 * 8;

[[gnu::aligned(4), gnu::used]]
uint16_t data_buffer[kBufferSize];

volatile bool is_updating{false};

void GpioConfig() {
    rcu_periph_clock_enable(PIXEL_OUTPUT_RCU_GPIOx);
    gpio_mode_set(PIXEL_OUTPUT_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, PIXEL_OUTPUT_GPIO_PINx);
    gpio_output_options_set(PIXEL_OUTPUT_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PIXEL_OUTPUT_GPIO_PINx);

    GPIO_BC(PIXEL_OUTPUT_GPIOx) = PIXEL_OUTPUT_GPIO_PINx;
}

void TimersConfig() {
    // Timer 4 is Master -> TIMER4_TRGO
    // Timer 7 is Slave -> ITI3

    timer_parameter_struct timer_initpara;

    // Timer 4 Master
    // There is no DMA Channel for Timer 4 Channel 1

    rcu_periph_clock_enable(RCU_TIMER4);

    timer_deinit(TIMER4);

    timer_initpara.prescaler = 0;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = pixel::kRtzTimerPeriod; // 1.25 us
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;

    timer_init(TIMER4, &timer_initpara);

    timer_master_slave_mode_config(TIMER4, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_master_output_trigger_source_select(TIMER4, TIMER_TRI_OUT_SRC_CH0);

    timer_channel_output_mode_config(TIMER4, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_mode_config(TIMER4, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_mode_config(TIMER4, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);

    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, 1); // High
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, static_cast<uint16_t>(pixel::kT0H));
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, static_cast<uint16_t>(pixel::kT1H));

    // Timer 7 Slave

    rcu_periph_clock_enable(RCU_TIMER7);

    timer_deinit(TIMER7);

    timer_initpara.prescaler = 0;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = UINT32_MAX;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;

    timer_init(TIMER7, &timer_initpara);

    timer_master_slave_mode_config(TIMER7, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_slave_mode_select(TIMER7, TIMER_SLAVE_MODE_EXTERNAL0);
    timer_input_trigger_source_select(TIMER7, TIMER_SMCFG_TRGSEL_ITI3);

    timer_channel_output_mode_config(TIMER7, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
    static_assert((1 + kBufferSize) < UINT16_MAX);
    timer_channel_output_pulse_value_config(TIMER7, TIMER_CH_0, static_cast<uint16_t>(1 + kBufferSize));

    timer_interrupt_enable(TIMER7, TIMER_INT_CH0);

    NVIC_SetPriority(TIMER7_Channel_IRQn, 0);
    NVIC_EnableIRQ(TIMER7_Channel_IRQn);
}

void DmaConfig() {
    DMA_PARAMETER_STRUCT dma_init_struct;
    rcu_periph_clock_enable(TIMER4_RCU_DMAx);

    // Timer 4 Channel 0
    dma_deinit(TIMER4_DMAx, TIMER4_CH0_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_DISABLE;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(TIMER4_DMAx, TIMER4_CH0_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(TIMER4_DMAx, TIMER4_CH0_DMA_CHx);
    dma_memory_to_memory_disable(TIMER4_DMAx, TIMER4_CH0_DMA_CHx);
    dma_channel_subperipheral_select(TIMER4_DMAx, TIMER4_CH0_DMA_CHx, TIMER4_CH0_DMA_SUBPERIx);

    // Timer 4 Channel 2
    dma_deinit(TIMER4_DMAx, TIMER4_CH2_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(TIMER4_DMAx, TIMER4_CH2_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(TIMER4_DMAx, TIMER4_CH2_DMA_CHx);
    dma_memory_to_memory_disable(TIMER4_DMAx, TIMER4_CH2_DMA_CHx);
    dma_channel_subperipheral_select(TIMER4_DMAx, TIMER4_CH2_DMA_CHx, TIMER4_CH2_DMA_SUBPERIx);

    // Timer 4 Channel 3
    dma_deinit(TIMER4_DMAx, TIMER4_CH3_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_DISABLE;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(TIMER4_DMAx, TIMER4_CH3_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(TIMER4_DMAx, TIMER4_CH3_DMA_CHx);
    dma_memory_to_memory_disable(TIMER4_DMAx, TIMER4_CH3_DMA_CHx);
    dma_channel_subperipheral_select(TIMER4_DMAx, TIMER4_CH3_DMA_CHx, TIMER4_CH3_DMA_SUBPERIx);
}
} // namespace

void Start() {
    PIXEL_OUTPUT_DEBUG_ENTRY();

    GpioConfig();
    TimersConfig();
    DmaConfig();

    PIXEL_OUTPUT_DEBUG_EXIT();
}

void Stop() {
    PIXEL_OUTPUT_DEBUG_ENTRY();

    PIXEL_OUTPUT_DEBUG_EXIT();
}

#define BIT_SET(Addr, Bit)                                                                                                                                  \
    {                                                                                                                                                       \
        *reinterpret_cast<volatile uint32_t*>((BITBAND_SRAM_BASE + ((reinterpret_cast<uint32_t>(&(Addr))) - SRAM_BASE) * 32U + ((Bit) & 0xFF) * 4U)) = 0x1; \
    }

#define BIT_CLEAR(Addr, Bit)                                                                                                                                \
    {                                                                                                                                                       \
        *reinterpret_cast<volatile uint32_t*>((BITBAND_SRAM_BASE + ((reinterpret_cast<uint32_t>(&(Addr))) - SRAM_BASE) * 32U + ((Bit) & 0xFF) * 4U)) = 0x0; \
    }

void SetPixel(uint32_t index, uint8_t red, uint8_t green, uint8_t blue) {
    assert(index < ::pixel::kPixelsMax);

    uint32_t j = 0;
    const auto kIndex = index * 24;
    constexpr auto kBit = PIXEL_OUTPUT_GPIO_PIN_OFFSET;
    auto* p = &data_buffer[kIndex];

    for (uint8_t mask = 0x80; mask != 0; mask = static_cast<uint8_t>(mask >> 1)) {
        auto& p1 = p[j];
        auto& p2 = p[8 + j];
        auto& p3 = p[16 + j];

        if (!(mask & green)) {
            BIT_SET(p1, kBit);
        } else {
            BIT_CLEAR(p1, kBit);
        }
        if (!(mask & red)) {
            BIT_SET(p2, kBit);
        } else {
            BIT_CLEAR(p2, kBit);
        }
        if (!(mask & blue)) {
            BIT_SET(p3, kBit);
        } else {
            BIT_CLEAR(p3, kBit);
        }

        j++;
    }
}

void Blackout(uint32_t pixels) {
    PIXEL_OUTPUT_DEBUG_ENTRY();
    assert(pixels <= ::pixel::kPixelsMax);

    const uint32_t kSize = pixels * 3 * 8;

    for (uint32_t i = 0; i < kSize; i++) {
        data_buffer[i] |= PIXEL_OUTPUT_GPIO_PINx;
    }

    PIXEL_OUTPUT_DEBUG_EXIT();
}

void Update(uint32_t pixels) {
    assert(pixels <= ::pixel::kPixelsMax);
    const uint32_t kDmaSize = pixels * 3 * 8;
    assert(!is_updating);

    is_updating = true;

    auto timer7_ctl0 = TIMER_CTL0(TIMER7);
    timer7_ctl0 &= ~TIMER_CTL0_CEN;
    TIMER_CTL0(TIMER7) = timer7_ctl0;
    TIMER_CNT(TIMER7) = 0;

    auto timer4_ctl0 = TIMER_CTL0(TIMER4);
    timer4_ctl0 &= ~TIMER_CTL0_CEN;
    TIMER_CTL0(TIMER4) = timer4_ctl0;
    TIMER_CNT(TIMER4) = 0;

    // Timer 4 Channel 0
    uint32_t chctl_ch0 = DMA_CHCTL(TIMER4_DMAx, TIMER4_CH0_DMA_CHx);
    chctl_ch0 &= ~DMA_CHXCTL_CHEN;
    DMA_CHCTL(TIMER4_DMAx, TIMER4_CH0_DMA_CHx) = chctl_ch0;
    Gd32DmaInterruptFlagClear<TIMER4_DMAx, TIMER4_CH0_DMA_CHx, DMA_INTF_FTFIF>();
    DMA_CHPADDR(TIMER4_DMAx, TIMER4_CH0_DMA_CHx) = PIXEL_OUTPUT_GPIOx + GPIOx_BOP_OFFSET;
    DMA_CHMADDR(TIMER4_DMAx, TIMER4_CH0_DMA_CHx) = reinterpret_cast<uint32_t>(kPgpioPin);
    DMA_CHCNT(TIMER4_DMAx, TIMER4_CH0_DMA_CHx) = (kDmaSize & DMA_CHXCNT_CNT);
    chctl_ch0 |= DMA_CHXCTL_CHEN;
    DMA_CHCTL(TIMER4_DMAx, TIMER4_CH0_DMA_CHx) = chctl_ch0;

    // Timer 4 Channel 2
    uint32_t chctl_ch2 = DMA_CHCTL(TIMER4_DMAx, TIMER4_CH2_DMA_CHx);
    chctl_ch2 &= ~DMA_CHXCTL_CHEN;
    DMA_CHCTL(TIMER4_DMAx, TIMER4_CH2_DMA_CHx) = chctl_ch2;
    Gd32DmaInterruptFlagClear<TIMER4_DMAx, TIMER4_CH2_DMA_CHx, DMA_INTF_FTFIF>();
    DMA_CHPADDR(TIMER4_DMAx, TIMER4_CH2_DMA_CHx) = PIXEL_OUTPUT_GPIOx + GPIOx_BC_OFFSET;
    DMA_CHMADDR(TIMER4_DMAx, TIMER4_CH2_DMA_CHx) = reinterpret_cast<uint32_t>(data_buffer);
    DMA_CHCNT(TIMER4_DMAx, TIMER4_CH2_DMA_CHx) = (kDmaSize & DMA_CHXCNT_CNT);
    chctl_ch2 |= DMA_CHXCTL_CHEN;
    DMA_CHCTL(TIMER4_DMAx, TIMER4_CH2_DMA_CHx) = chctl_ch2;

    // Timer 4 Channel 3
    uint32_t chctl_ch3 = DMA_CHCTL(TIMER4_DMAx, TIMER4_CH3_DMA_CHx);
    chctl_ch3 &= ~DMA_CHXCTL_CHEN;
    DMA_CHCTL(TIMER4_DMAx, TIMER4_CH3_DMA_CHx) = chctl_ch3;
    Gd32DmaInterruptFlagClear<TIMER4_DMAx, TIMER4_CH3_DMA_CHx, DMA_INTF_FTFIF>();
    DMA_CHPADDR(TIMER4_DMAx, TIMER4_CH3_DMA_CHx) = PIXEL_OUTPUT_GPIOx + GPIOx_BC_OFFSET;
    DMA_CHMADDR(TIMER4_DMAx, TIMER4_CH3_DMA_CHx) = reinterpret_cast<uint32_t>(kPgpioPin);
    DMA_CHCNT(TIMER4_DMAx, TIMER4_CH3_DMA_CHx) = (kDmaSize & DMA_CHXCNT_CNT);
    chctl_ch3 |= DMA_CHXCTL_CHEN;
    DMA_CHCTL(TIMER4_DMAx, TIMER4_CH3_DMA_CHx) = chctl_ch3;

    TIMER_DMAINTEN(TIMER4) |= (TIMER_DMA_CH0D | TIMER_DMA_CH2D | TIMER_DMA_CH3D);

    timer7_ctl0 |= TIMER_CTL0_CEN;
    TIMER_CTL0(TIMER7) = timer7_ctl0;

    timer4_ctl0 |= TIMER_CTL0_CEN;
    TIMER_CTL0(TIMER4) = timer4_ctl0;
}

bool IsUpdating() {
    return pixel::output::is_updating;
}
} // namespace pixel::output

extern "C" {
void TIMER7_Channel_IRQHandler() { // Slave
    const auto kIntFlag = TIMER_INTF(TIMER7);

    if ((kIntFlag & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {
        TIMER_CTL0(TIMER4) &= (~TIMER_CTL0_CEN);

        TIMER_DMAINTEN(TIMER4) &= static_cast<uint32_t>(~(TIMER_DMA_CH0D | TIMER_DMA_CH2D | TIMER_DMA_CH3D));

        GPIO_BC(PIXEL_OUTPUT_GPIOx) = PIXEL_OUTPUT_GPIO_PINx;

        pixel::output::is_updating = false;
        __DMB();
    }

    TIMER_INTF(TIMER7) = ~kIntFlag;
}
}