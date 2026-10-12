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
#include <cstdio>

#include "ltc.h"
#include "ltc_gpio_config.h"
#include "output/ltc_output_midi.h"
#include "output/ltc_output.h"
#include "ltc_debug.h"
#include "ltc_network_ntpserver.h"
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
volatile uint32_t timecode_update_counter{0};
// TIMER10
volatile bool is_midi_quarter_frame_message{false};
volatile uint32_t midi_quarter_frame_piece{0};
auto midi_quarter_frame_piece_running{false};

uint8_t timecode_previous{UINT8_MAX};

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
    TIMER_CTL0(TIMER0) &= ~TIMER_CTL0_CEN;
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

void Destination::SetType(::ltc::Type type) const {
    LTC_OUTPUT_DEBUG_ENTRY();
    LTC_OUTPUT_DEBUG_PRINTF("type=%u", static_cast<unsigned>(type));

    Timer3SetType(static_cast<uint32_t>(type));

    global::timecode_running.type = static_cast<uint8_t>(type);

    LTC_OUTPUT_DEBUG_EXIT();
}

::ltc::Type Destination::Type() const {
    return static_cast<::ltc::Type>(global::timecode_running.type);
}

namespace ltc {
void Start() {
    LTC_OUTPUT_DEBUG_ENTRY();

    timecode_previous = UINT8_MAX;

    GpioConfig();

    Timer0Dma1Ch5Config();
    Timer0Config();
    Timer3Config();
    Timer10Config();

    encoder::Init();

    LTC_OUTPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_OUTPUT_DEBUG_ENTRY();

    LTC_OUTPUT_DEBUG_EXIT();
}
} // namespace ltc
} // namespace ltc::output

namespace ltc::input::ltc {
namespace {
// LTC Input; Timer 2 Channel 0 -> PA6
void GpioConfig() {
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_af_set(GPIOA, GPIO_AF_2, GPIO_PIN_6);
}

// TIMER2 LTC Input
void Timer2Config() {
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER2);

    timer_deinit(TIMER2);

    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler = TIMER_PSC_1MHZ;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = UINT16_MAX;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER2, &timer_initpara);

    timer_ic_parameter_struct timer_icinitpara;
    timer_channel_input_struct_para_init(&timer_icinitpara);
    timer_icinitpara.icpolarity = TIMER_IC_POLARITY_BOTH_EDGE;
    timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
    timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
    timer_icinitpara.icfilter = 0x3;
    timer_input_capture_config(TIMER2, TIMER_CH_0, &timer_icinitpara);

    timer_auto_reload_shadow_enable(TIMER2);

    timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_CH0);
    timer_interrupt_enable(TIMER2, TIMER_INT_CH0);

    timer_enable(TIMER2);
}
} // namespace

void Start() {
    LTC_INPUT_DEBUG_ENTRY();

    GpioConfig();
    Timer2Config();

    NVIC_SetPriority(TIMER2_IRQn, 1);
    NVIC_EnableIRQ(TIMER2_IRQn);

    ::ltc::output::Destination::Instance().SetType(::ltc::Type::kUnknown);

    LTC_INPUT_DEBUG_EXIT();
}

void Stop() {
    LTC_INPUT_DEBUG_ENTRY();

    NVIC_DisableIRQ(TIMER2_IRQn);

    timer_interrupt_disable(TIMER2, TIMER_INT_CH0);
    timer_disable(TIMER2);

    LTC_INPUT_DEBUG_EXIT();
}
} // namespace ltc::input::ltc

// Optimized

#pragma GCC push_options
#pragma GCC optimize("O2")

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
            ::ltc::output::applemidi::Output(reinterpret_cast<const struct ::midi::Timecode*>(timecode));
        }

		if (IsEnabled(::ltc::Output::kUsbmidi)) {
		    ::ltc::output::usbmidi::Output(reinterpret_cast<const struct ::midi::Timecode*>(timecode));
		}
        
        if (IsEnabled(::ltc::Output::kMidi)) {
            ::ltc::output::midi::Output(reinterpret_cast<const struct ::midi::Timecode*>(timecode));
        }

        timecode_[::ltc::timecode::index::kColon3] = (timecode->type != static_cast<uint8_t>(::ltc::Type::kDf) ? ':' : ';');

        DisplayType(static_cast<::ltc::Type>(timecode->type));
    }

    if (midi_quarter_frame_piece_running) {
        TIMER_CTL0(TIMER10) &= ~TIMER_CTL0_CEN;
        TIMER_CAR(TIMER10) = ((TimeCodeConst::kTmrIntv[timecode->type] + 1U) / 4U) - 1U;

        if ((midi_quarter_frame_piece != 0) && (midi_quarter_frame_piece != 4)) {
            midi_quarter_frame_piece = 0;
        }

        const auto kData = CreateQuarterFrame(reinterpret_cast<const struct ::midi::Timecode*>(timecode));

        if (IsEnabled(::ltc::Output::kApplemidi)) {
            ::ltc::output::applemidi::Output(kData);
        }

        if (IsEnabled(::ltc::Output::kUsbmidi)) {
		    ::ltc::output::usbmidi::Output(kData);
		}
        
        if (IsEnabled(::ltc::Output::kMidi)) {
            ::ltc::output::midi::Output(kData);
        }

        TIMER_CNT(TIMER10) = 0;
        TIMER_CTL0(TIMER10) |= TIMER_CTL0_CEN;

        is_midi_quarter_frame_message = false;
    }

    midi_quarter_frame_piece_running = IsEnabled(::ltc::Output::kMidi) || IsEnabled(::ltc::Output::kApplemidi) || IsEnabled(::ltc::Output::kUsbmidi);

    if (IsEnabled(::ltc::Output::kNtpServer)) {
        network::ntpserver::SetTimeCode(timecode);
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
            ::ltc::output::applemidi::Output(kData);
        }

		if (ltc::output::Destination::Instance().IsEnabled(::ltc::Output::kUsbmidi)) {
		    ::ltc::output::usbmidi::Output(kData);
		}
        
        if (ltc::output::Destination::Instance().IsEnabled(::ltc::Output::kMidi)) {
            ::ltc::output::midi::Output(kData);
        }

        is_midi_quarter_frame_message = true;
    }

    TIMER_INTF(TIMER10) = ~kIntFlag;
}

namespace {
volatile uint16_t prev_capture = 0;
volatile uint16_t delta = 0;

volatile uint32_t total_bits = 0;
volatile uint32_t current_bit = 0;
volatile uint32_t sync_count = 0;
volatile uint32_t half_delta_sum = 0;
volatile uint32_t half_delta_count = 0;

volatile bool ones_bit_count{false};
volatile bool received_timecode_valid{false};
volatile bool timecode_sync{false};
volatile uint8_t timecode_bits[8];

constexpr uint32_t kDelta24Fps = 260; ///< 24 FPS * 80 bits = 1920Hz, 1E6/1920Hz = 521us -> 521us/2 = 260us
constexpr uint32_t kDelta25Fps = 250; ///< 25 FPS * 80 bits = 2000Hz, 1E6/2000Hz = 500us -> 500us/2 = 250us
constexpr uint32_t kDelta30Fps = 208; ///< 30 FPS * 80 bits = 2400Hz, 1E6/2400Hz = 417us -> 417us/2 = 208us

// In the case of 'zero', there is no second transition in the bit periode.
// In the case of 'one' , there is a second transition, half-bit period after the start of the bit.
//
// 1 bit -> half-bit interval
// 0 bit -> full-bit interval
//
// Half-bit intervals:
//
// 30 fps : 208 us
// 25 fps : 250 us
// 24 fps : 260 us
//
// Full-bit intervals:
//
// 30 fps : 417 us
// 25 fps : 500 us
// 24 fps : 521 us
//
// 150        208 250 260      380      417 500 521        600
// |-----------|===1===|--------|--------|====0====|---------|
//
//
// invalid  valid
// |---------|====================
// 0        150      208
constexpr uint32_t kOneTimeMin = 150;
// 260                    380                  417                521
//  |------ half-bit ------|------ guard -------|---- full-bit ----|
constexpr uint32_t kZeroTimeMin = 380;
//                   largest valid LTC
//                          |
//                          v
// 0 --------------------- 521 ----------- 600 ------------------>
//                       accepted        rejected
constexpr uint32_t kZeroTimeMax = 600;

// Constant           | Theoretical LTC value              | Reason                   |
// ------------------ | ---------------------------------- | ------------------------ |
// kOneTimeMin = 150  | smallest valid ≈ 208 us            | allows measurement error |
// kZeroTimeMin = 380 | separator between ~260 and ~417 us | chosen safely in the gap |
// kZeroTimeMax = 600 | largest valid ≈ 521 us             | allows measurement error |

constexpr uint32_t kEndDataPosition = 63;
constexpr uint32_t kEndSyncPosition = 77;
constexpr uint32_t kEndSmptePosition = 80;

volatile bool received_timecode_available{false};
volatile ltc::TimeCode timecode_input{};
} // namespace

// TIMER2 LTC Input
void TIMER2_IRQHandler() {
    if (TIMER_INT_FLAG_CH0 == (TIMER_INTF(TIMER2) & TIMER_INT_FLAG_CH0)) [[likely]] {
        TIMER_INTF(TIMER2) = ~TIMER_INT_FLAG_CH0;

        const auto kCapture = static_cast<uint16_t>(TIMER_CH0CV(TIMER2));

        delta = static_cast<uint16_t>(kCapture - prev_capture);

        prev_capture = kCapture;

        if ((delta < kOneTimeMin) || (delta > kZeroTimeMax)) {
            total_bits = 0;
            sync_count = 0;
            ones_bit_count = false;
            timecode_sync = false;
            half_delta_sum = 0;
            half_delta_count = 0;
        } else {
            if (delta < kZeroTimeMin) {
                half_delta_sum += delta;
                half_delta_count = half_delta_count + 1;
            }

            if (ones_bit_count) {
                ones_bit_count = false;
            } else {
                if (delta > kZeroTimeMin) {
                    current_bit = 0;
                    sync_count = 0;
                } else {
                    current_bit = 1;
                    ones_bit_count = true;
                    sync_count = sync_count + 1;

                    if (sync_count == 12) {
                        sync_count = 0;
                        timecode_sync = true;
                        total_bits = kEndSyncPosition;
                    }
                }

                if (total_bits <= kEndDataPosition) {
                    timecode_bits[0] = static_cast<uint8_t>(timecode_bits[0] >> 1);

                    for (uint32_t n = 1; n < 8; n++) {
                        if ((timecode_bits[n] & 1) == 1) {
                            timecode_bits[n - 1] |= 0x80;
                        }

                        timecode_bits[n] = static_cast<uint8_t>(timecode_bits[n] >> 1);
                    }

                    if (current_bit == 1) {
                        timecode_bits[7] |= 0x80;
                    }
                }

                total_bits = total_bits + 1;
            }

            if (total_bits == kEndSmptePosition) {
                total_bits = 0;

                if (timecode_sync) {
                    timecode_sync = false;
                    received_timecode_valid = true;
                }
            }

            if (received_timecode_valid) {
                received_timecode_valid = false;

                timecode_input.frames = static_cast<uint8_t>((10U * (timecode_bits[1] & 0x03)) + (timecode_bits[0] & 0x0F));
                timecode_input.seconds = static_cast<uint8_t>((10U * (timecode_bits[3] & 0x07)) + (timecode_bits[2] & 0x0F));
                timecode_input.minutes = static_cast<uint8_t>((10U * (timecode_bits[5] & 0x07)) + (timecode_bits[4] & 0x0F));
                timecode_input.hours = static_cast<uint8_t>((10U * (timecode_bits[7] & 0x03)) + (timecode_bits[6] & 0x0F));

                const auto kIsDropFrameFlagSet = (timecode_bits[1] & (1U << 2)) == (1U << 2);

                if (!kIsDropFrameFlagSet) {
                    if (half_delta_count != 0) {
                        const auto kAvgDelta = half_delta_sum / half_delta_count;

                        if (kAvgDelta < (kDelta30Fps + 5)) {
                            timecode_input.type = std::to_underlying(ltc::Type::kSmpte);
                        } else if (kAvgDelta < (kDelta25Fps + 5)) {
                            timecode_input.type = std::to_underlying(ltc::Type::kEbu);
                        } else if (kAvgDelta < (kDelta24Fps + 5)) {
                            timecode_input.type = std::to_underlying(ltc::Type::kFilm);
                        } else {
                            timecode_input.type = std::to_underlying(ltc::Type::kUnknown);
                        }
                    } else {
                        timecode_input.type = std::to_underlying(ltc::Type::kUnknown);
                    }
                } else {
                    timecode_input.type = std::to_underlying(ltc::Type::kDf);
                }

                half_delta_sum = 0;
                half_delta_count = 0;

                received_timecode_available = true;
            }
        }
    }
}
}

namespace ltc::input::ltc {
void Run() {
    if (received_timecode_available) [[unlikely]] {
        received_timecode_available = false;
        ::ltc::output::Destination::Instance().Distribute(const_cast<const ::ltc::TimeCode*>(&timecode_input));
    }
}
} // namespace ltc::input::ltc

#pragma GCC pop_options
