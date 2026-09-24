/**
 * @file ltc_output.cpp
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

#include "ltc.h"
#include "ltc_gpio_config.h"
#include "output/ltc_output_midi.h"
#include "output/ltc_output.h"
#include "ltc_debug.h"
#include "ltc_ntpserver.h"
#include "net/rtpmidi.h"
#include "midi.h"
#include "timecodeconst.h"
#include "ltc_display_max7219.h"
#include "ltc_encoder.h"
#include "gd32.h" // IWYU pragma: keep

// LTC Output
// Timer 3 is Master -> TIMERO_TRGO
// Timer 0 is Slave -> ITI3
// Timer 10 is MIDI QF message

namespace ltc::global {
::ltc::TimeCode timecode_running;
volatile bool timecode_available;
volatile uint32_t timecode_counter;
} // namespace ltc::global

namespace {
// TIMER0
volatile uint32_t timecode_update_counter;
// TIMER10
volatile bool is_midi_quarter_frame_message;
volatile uint32_t midi_quarter_frame_piece;
auto midi_quarter_frame_piece_running = false;

template <uint32_t kN>
bool constexpr IsAdjustmentNeeded() {
    static_assert(kN < sizeof(TimeCodeConst::kTmrIntv) / sizeof(TimeCodeConst::kTmrIntv[0]), "Index out of bounds for TMR_INTV.");
    static_assert(kN < sizeof(TimeCodeConst::kFps) / sizeof(TimeCodeConst::kFps[0]), "Index out of bounds for FPS.");
    return ((TimeCodeConst::kTmrIntv[kN] + 1) * TimeCodeConst::kFps[kN]) != FREQUENCY_EFFECTIVE;
}
} // namespace

namespace ltc::output {
namespace {
[[gnu::section(".sram1"), gnu::aligned(4), gnu::used]]
uint32_t dma_buffer1[::ltc::encoder::kBufferSize];
[[gnu::section(".sram1"), gnu::aligned(4), gnu::used]]
uint32_t dma_buffer2[::ltc::encoder::kBufferSize];

void GpioConfig() {
    rcu_periph_clock_enable(LTC_OUTPUT_RCU_GPIOx);
    gpio_mode_set(LTC_OUTPUT_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LTC_OUTPUT_GPIO_PINx);
    gpio_output_options_set(LTC_OUTPUT_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LTC_OUTPUT_GPIO_PINx);

    GPIO_BC(LTC_OUTPUT_GPIOx) = LTC_OUTPUT_GPIO_PINx;
}

// TIMER0 LTC output
void Timer0Dma1Ch5Config() {
    dma_single_data_parameter_struct dma_init_struct;
    dma_single_data_para_struct_init(&dma_init_struct);

    rcu_periph_clock_enable(RCU_DMA1);

    dma_deinit(DMA1, DMA_CH5);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = reinterpret_cast<uint32_t>(&dma_buffer1);
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_addr = LTC_OUTPUT_GPIOx + 0x18U; // Port bit operate register (BOP)
                                                            // Bits 31:16 -> 1 Clear
                                                            // Bits 15:0 -> 1 Set
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_32BIT;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    dma_init_struct.number = ::ltc::encoder::kFormatSizeBits * 2U;
    dma_single_data_mode_init(DMA1, DMA_CH5, &dma_init_struct);

    dma_channel_subperipheral_select(DMA1, DMA_CH5, DMA_SUBPERI6);

    dma_switch_buffer_mode_config(DMA1, DMA_CH5, reinterpret_cast<uint32_t>(&dma_buffer2), DMA_MEMORY_1);
    dma_switch_buffer_mode_enable(DMA1, DMA_CH5, ENABLE);

    dma_channel_enable(DMA1, DMA_CH5);
}

// TIMER0
void Timer0Config() {
    rcu_periph_clock_enable(RCU_TIMER0);
    timer_deinit(TIMER0);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PRESCALER;
    timer_initpara.period = UINT32_MAX;

    timer_init(TIMER0, &timer_initpara);

    timer_counter_value_config(TIMER0, 0);

    timer_master_slave_mode_config(TIMER0, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_slave_mode_select(TIMER0, TIMER_SLAVE_MODE_RESTART);
    timer_input_trigger_source_select(TIMER0, TIMER_SMCFG_TRGSEL_ITI0);

    timer_dma_enable(TIMER0, TIMER_DMA_UPD);
}

// TIMER3 timecode_available
void Timer3Config() {
    LTC_OUTPUT_DEBUG_ENTRY();

    rcu_periph_clock_enable(RCU_TIMER3);
    timer_deinit(TIMER3);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PRESCALER;
    timer_initpara.period = UINT32_MAX;
    timer_init(TIMER3, &timer_initpara);

    timer_counter_value_config(TIMER3, 0);

    timer_master_slave_mode_config(TIMER3, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_master_output_trigger_source_select(TIMER3, TIMER_TRI_OUT_SRC_UPDATE);

    timer_interrupt_flag_clear(TIMER3, UINT32_MAX);
    timer_interrupt_enable(TIMER3, TIMER_INT_UP);

    NVIC_SetPriority(TIMER3_IRQn, 0);
    NVIC_EnableIRQ(TIMER3_IRQn);

#ifdef DEBUG_LTC_TIMER3
    printf("MASTER_TIMER_CLOCK  : %u Hz\n", static_cast<unsigned>(MASTER_TIMER_CLOCK));
    printf("FREQUENCY_EFFECTIVE : %u Hz\n", static_cast<unsigned>(FREQUENCY_EFFECTIVE));

    auto is_needed = false;

    for (uint32_t index = 0; index < sizeof(TimeCodeConst::kTmrIntv) / sizeof(TimeCodeConst::kTmrIntv[0]); index++) {
        switch (index) {
            case 0:
                is_needed = IsAdjustmentNeeded<0>();
                break;
            case 1:
                is_needed = IsAdjustmentNeeded<1>();
                break;
            case 2:
                is_needed = IsAdjustmentNeeded<2>();
                break;
            case 3:
                is_needed = IsAdjustmentNeeded<3>();
                break;
            default:
                break;
        }

        const auto kCAR = TimeCodeConst::kTmrIntv[index];
        printf("FPS = %u, CAR = %u [%s]\n", TimeCodeConst::kFps[index], static_cast<unsigned>(kCAR), is_needed ? "ERORR" : "Ok");
    }
#endif // DEBUG_LTC_TIMER3

    LTC_OUTPUT_DEBUG_EXIT();
}

// TIMER 10 is_midi_quarter_frame_message
void Timer10Config() {
    //   return;
    LTC_OUTPUT_DEBUG_ENTRY();

    rcu_periph_clock_enable(RCU_TIMER10);
    timer_deinit(TIMER10);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PRESCALER;
    timer_initpara.period = UINT32_MAX;

    timer_init(TIMER10, &timer_initpara);

    timer_counter_value_config(TIMER10, 0);

    timer_interrupt_flag_clear(TIMER10, UINT32_MAX);
    timer_interrupt_enable(TIMER10, TIMER_INT_UP);

    NVIC_SetPriority(TIMER0_TRG_CMT_TIMER10_IRQn, 0);
    NVIC_EnableIRQ(TIMER0_TRG_CMT_TIMER10_IRQn);

    LTC_OUTPUT_DEBUG_EXIT();
}

void Timer0SetType(uint32_t type) {
    TIMER_CTL0(TIMER0) &= TIMER_CTL0_CEN;
    TIMER_CAR(TIMER0) = ((TimeCodeConst::kTmrIntv[type] + 1) / (::ltc::encoder::kFormatSizeBits * 2U)) - 1U;
    TIMER_CNT(TIMER0) = 0;
    TIMER_CTL0(TIMER0) |= TIMER_CTL0_CEN;
}

void Timer3SetType(uint32_t type) {
    TIMER_CTL0(TIMER3) &= ~TIMER_CTL0_CEN;

    if (type < static_cast<uint8_t>(::ltc::Type::kUnknown)) {
        TIMER_CAR(TIMER3) = TimeCodeConst::kTmrIntv[type];
        TIMER_CNT(TIMER3) = 0;
        TIMER_CTL0(TIMER3) |= TIMER_CTL0_CEN;
    }
}
} // namespace

Destination::Destination() {
    LTC_OUTPUT_DEBUG_ENTRY();
    s_this = this;

    DisplayTimecodeInit();
    global::timecode_running.type = static_cast<uint8_t>(::ltc::Type::kUnknown);

    GpioConfig();

    Timer0Dma1Ch5Config();
    Timer0Config();
    Timer3Config();
    Timer10Config();

    encoder::Init();

    LTC_OUTPUT_DEBUG_EXIT();
}

void Destination::StartEnabled() {
    LTC_OUTPUT_DEBUG_ENTRY();

    for (const auto kOutput : ::ltc::kOutputs) {
        const auto& entry = kCommandtab[kOutput];

        if (entry.start == nullptr) {
            disabled_requested_ |= static_cast<uint16_t>(kOutput);
            continue;
        }

        if (IsEnabled(kOutput)) {
            entry.start();
        }
    }

    disabled_current_ = disabled_requested_;

    debug::PrintBits(disabled_requested_, "disabled_requested_");
    LTC_OUTPUT_DEBUG_EXIT();
}

void Destination::SetType(::ltc::Type type) {
    LTC_OUTPUT_DEBUG_ENTRY();
    LTC_OUTPUT_DEBUG_PRINTF("type=%u", static_cast<unsigned>(type));

    Timer3SetType(static_cast<uint32_t>(type));

    global::timecode_running.type = static_cast<uint8_t>(type);

    LTC_OUTPUT_DEBUG_EXIT();
}

::ltc::Type Destination::Type() const {
    return static_cast<::ltc::Type>(global::timecode_running.type);
}
} // namespace ltc::output

#pragma GCC push_options
#pragma GCC optimize("O3")

namespace {
uint8_t CreateQuarterFrame(const struct midi::Timecode* timecode) {
    uint8_t data = 0;

    switch (midi_quarter_frame_piece) {
        case 0:
            data = 0x00 | (timecode->frames & 0x0F);
            break;
        case 1:
            data = 0x10 | static_cast<uint8_t>((timecode->frames & 0x10) >> 4);
            break;
        case 2:
            data = 0x20 | (timecode->seconds & 0x0F);
            break;
        case 3:
            data = 0x30 | static_cast<uint8_t>((timecode->seconds & 0x30) >> 4);
            break;
        case 4:
            data = 0x40 | (timecode->minutes & 0x0F);
            break;
        case 5:
            data = 0x50 | static_cast<uint8_t>((timecode->minutes & 0x30) >> 4);
            break;
        case 6:
            data = 0x60 | (timecode->hours & 0x0F);
            break;
        case 7:
            data = static_cast<uint8_t>(0x70 | (timecode->type << 1) | ((timecode->hours & 0x10) >> 4));
            break;
        default:
            break;
    }

    midi_quarter_frame_piece = (midi_quarter_frame_piece + 1) & 0x07;

    return data;
}
} // namespace

namespace ltc::output {
void Destination::DistributeInternal(const ::ltc::TimeCode* timecode) {
    if (timecode->type != type_previous_) [[unlikely]] {
        type_previous_ = timecode->type;
        SetType(static_cast<::ltc::Type>(timecode->type));

        TIMER_CTL0(TIMER10) &= ~TIMER_CTL0_CEN;
        is_midi_quarter_frame_message = false;
        midi_quarter_frame_piece_running = false;
        midi_quarter_frame_piece = 0;

        if (IsEnabled(::ltc::Output::kApplemidi)) {
            RtpMidi::Get()->SendTimeCode(reinterpret_cast<const struct ::midi::Timecode*>(timecode));
        }

        if (IsEnabled(::ltc::Output::kMidi)) {
            ::ltc::output::midi::OutputTimeCode(reinterpret_cast<const struct ::midi::Timecode*>(timecode));
        }

        timecode_[::ltc::timecode::index::kColon3] = (timecode->type != static_cast<uint8_t>(::ltc::Type::kDf) ? ':' : ';');

        DisplayType();
    }

    if (midi_quarter_frame_piece_running) {
        TIMER_CTL0(TIMER10) &= ~TIMER_CTL0_CEN;
        TIMER_CAR(TIMER10) = ((TimeCodeConst::kTmrIntv[timecode->type] + 1U) / 4U) - 1U;

        if ((midi_quarter_frame_piece != 0) && (midi_quarter_frame_piece != 4)) {
            midi_quarter_frame_piece = 0;
        }

        const auto kData = CreateQuarterFrame(reinterpret_cast<const struct ::midi::Timecode*>(timecode));

        if (IsEnabled(::ltc::Output::kApplemidi)) {
            RtpMidi::Get()->SendQf(kData);
        }

        if (IsEnabled(::ltc::Output::kMidi)) {
            ::ltc::output::midi::OutputQf(kData);
        }

        TIMER_CNT(TIMER10) = 0;
        TIMER_CTL0(TIMER10) |= TIMER_CTL0_CEN;

        is_midi_quarter_frame_message = false;
    }

    midi_quarter_frame_piece_running = IsEnabled(::ltc::Output::kApplemidi) || IsEnabled(::ltc::Output::kMidi);

    if (IsEnabled(::ltc::Output::kApplemidi)) {
        ntpserver::SetTimeCode(timecode);
    }

    ConvertToString(timecode, timecode_);

    if (IsEnabled(::ltc::Output::kDisplayOled)) {
        Display::Get()->TextLine(1, timecode_, timecode::kCodeMaxLength);
    }

    if (IsEnabled(::ltc::Output::kMaX7219)) {
        ::ltc::display::max7219::Show(timecode_);
    }

    if (IsEnabled(::ltc::Output::kPixel)) {
        ::ltc::display::pixel::Show(timecode_);
    }
}

namespace ltc {
namespace {
uint8_t timecode_previous{UINT8_MAX};
}
void Start() {
    LTC_OUTPUT_DEBUG_ENTRY();

    timecode_previous = UINT8_MAX;

    LTC_OUTPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_OUTPUT_DEBUG_ENTRY();

    LTC_OUTPUT_DEBUG_EXIT();
}

void Output(const TimeCode* timecode) {
    timecode_update_counter = 0;
    ::ltc::encoder::SetTimeCode<false>(timecode);
    uint32_t* dst;

    if ((DMA_CHCTL(DMA1, DMA_CH5)) & DMA_CHXCTL_MBS) {
        dst = ::ltc::output::dma_buffer1;
    } else {
        dst = ::ltc::output::dma_buffer2;
    }

    ::ltc::encoder::Encode(dst);

    if (timecode_previous != timecode->type) {
        timecode_previous = timecode->type;
        ::ltc::output::Timer0SetType(static_cast<uint32_t>(timecode->type));
    }
}
} // namespace ltc
} // namespace ltc::output

extern "C" {
// TIMER3
void TIMER3_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(TIMER3);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
        ltc::global::timecode_available = true;
        ltc::global::timecode_counter = ltc::global::timecode_counter + 1;

        if (timecode_update_counter >= 2U) {
            if ((DMA_CHCTL(DMA1, DMA_CH5)) & DMA_CHXCTL_MBS) {
                memcpy(ltc::output::dma_buffer1, ltc::output::dma_buffer2, sizeof(ltc::output::dma_buffer1));
            } else {
                memcpy(ltc::output::dma_buffer2, ltc::output::dma_buffer1, sizeof(ltc::output::dma_buffer2));
            }
        }

        timecode_update_counter = timecode_update_counter + 1;
    }

    TIMER_INTF(TIMER3) = ~kIntFlag;
}
// TIMER10
void TIMER0_TRG_CMT_TIMER10_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(TIMER10);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
        if ((midi_quarter_frame_piece == 0) || (midi_quarter_frame_piece == 4)) {
            TIMER_INTF(TIMER10) = ~kIntFlag;
            return;
        }

        const auto kData = CreateQuarterFrame(reinterpret_cast<const struct midi::Timecode*>(&ltc::global::timecode_running));

        if (ltc::output::Destination::Instance().IsEnabled(::ltc::Output::kApplemidi)) {
            RtpMidi::Get()->SendQf(kData);
        }

        if (ltc::output::Destination::Instance().IsEnabled(::ltc::Output::kMidi)) {
            ::ltc::output::midi::OutputQf(kData);
        }

        is_midi_quarter_frame_message = true;
    }

    TIMER_INTF(TIMER10) = ~kIntFlag;
}
}

#pragma GCC pop_options
