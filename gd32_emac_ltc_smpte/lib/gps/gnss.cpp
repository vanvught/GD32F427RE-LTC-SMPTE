/**
 * @file gps.cpp
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
#include <cstring>
#include <ctime>
#include <utility>

#include "gnss.h"
#include "common/utils/utils_units.h"
#include "gps_debug.h"
#include "firmware/utc.h"
#include "uart.h"
#include "watchdog.h"
#include "gd32_dma.h"
#include "gd32.h" // IWYU pragma: keep

namespace gnss {
namespace {
constexpr uint32_t kInitialBaudrate = 9600;
constexpr uint32_t kOperatingBaudrate = 115200;

constexpr uint32_t kBaudrateSwitchDelayMs = 100;
constexpr uint32_t kBaudrateDetectionTimeoutMs = 1000;

constexpr const char kBaud115200[][nmea::kMaxSentenceLength] = {
    "$PCAS01,5*19\r\n",
    "$PUBX,41,1,0007,0003,115200,0*18\r\n",
    "$PMTK251,115200*1F\r\n",
};

constexpr uint32_t kRxSentenceCapacity = 4;

bool is_started{false};

alignas(uint32_t) char dma_buffer[nmea::kMaxSentenceLength * kRxSentenceCapacity];
constexpr uint32_t kSizeRxBuffer = sizeof(dma_buffer);

volatile uint16_t bytes_received{0};
volatile bool receive_pending{false};

void InitDma() {
    rcu_periph_clock_enable(USART2_RCU_DMAx);

    dma_single_data_parameter_struct dma_init_struct;

    dma_deinit(USART2_DMAx, USART2_RX_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory0_addr = reinterpret_cast<uint32_t>(dma_buffer);
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number = kSizeRxBuffer;
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_DATA(USART2));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(GPS_UARTx_DMAx, GPS_UARTx_RX_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(GPS_UARTx_DMAx, GPS_UARTx_RX_DMA_CHx);
    dma_channel_subperipheral_select(GPS_UARTx_DMAx, GPS_UARTx_RX_DMA_CHx, GPS_UARTx_RX_DMA_SUBPERIx);
    dma_channel_enable(GPS_UARTx_DMAx, GPS_UARTx_RX_DMA_CHx);
    // USART
    usart_dma_receive_config(GPS_UARTx, USART_RECEIVE_DMA_ENABLE);
    // NVIC
    NVIC_SetPriority(GPS_UARTx_IRQn, kUartIrqPriority);
    NVIC_EnableIRQ(GPS_UARTx_IRQn);
    usart_interrupt_enable(GPS_UARTx, USART_INT_IDLE);
}
} // namespace

extern "C" void GPS_UARTx_IRQHandler() {
    if (RESET != usart_interrupt_flag_get(GPS_UARTx, USART_INT_FLAG_IDLE)) {
        usart_data_receive(GPS_UARTx);

        bytes_received = static_cast<uint16_t>(kSizeRxBuffer - dma_transfer_number_get(GPS_UARTx_IRQn, GPS_UARTx_RX_DMA_CHx));
        receive_pending = true;

        auto chtl = DMA_CHCTL(GPS_UARTx_DMAx, GPS_UARTx_RX_DMA_CHx);
        chtl &= ~DMA_CHXCTL_CHEN;
        DMA_CHCTL(GPS_UARTx_DMAx, GPS_UARTx_RX_DMA_CHx) = chtl;
        Gd32DmaInterruptFlagClear<GPS_UARTx_DMAx, GPS_UARTx_RX_DMA_CHx, DMA_FLAG_FTF>(); // Needed for GD32F4xx
        DMA_CHCNT(GPS_UARTx_DMAx, GPS_UARTx_RX_DMA_CHx) = kSizeRxBuffer;
        chtl |= DMA_CHXCTL_CHEN;
        DMA_CHCTL(GPS_UARTx_DMAx, GPS_UARTx_RX_DMA_CHx) = chtl;
    }
}

Receiver::Receiver() noexcept {
    GPS_DEBUG_ENTRY();

    s_instance = this;

    memset(&time_date_, 0, sizeof(struct tm));

    GPS_DEBUG_EXIT();
}

void Receiver::SetUtcOffset(int32_t hours, uint32_t minutes) {
    GPS_DEBUG_ENTRY();
    GPS_DEBUG_PRINTF("utc_offset=%.2d:%.2u", static_cast<signed>(hours), static_cast<unsigned>(minutes));

    int32_t utc_offset;
    if (!utc::ValidateOffset(hours, minutes, utc_offset)) {
        GPS_DEBUG_EXIT();
        return;
    }

    utc_offset_ = utc_offset;

    GPS_DEBUG_EXIT();
}

void Receiver::SetDate(uint32_t year, uint32_t month, uint32_t day) {
    GPS_DEBUG_ENTRY();
    GPS_DEBUG_PRINTF("year=%u, month=%u, day=%u", static_cast<unsigned>(year), static_cast<unsigned>(month), static_cast<unsigned>(day));

    if (year < static_cast<uint32_t>(_TIME_STAMP_YEAR_)) {
        GPS_DEBUG_EXIT();
        return;
    }

    time_date_.tm_year = static_cast<int>(year - kTmYearBase);
    time_date_.tm_mon = static_cast<int>(month - kTmMonthOffset);
    time_date_.tm_mday = static_cast<int>(day);

    GPS_DEBUG_EXIT();
}
void Receiver::SetModule(gnss::Module module) {
    GPS_DEBUG_ENTRY();

    module_ = module;

    GPS_DEBUG_EXIT();
}

void Receiver::Start() {
    GPS_DEBUG_ENTRY();

    if (is_started) {
        GPS_DEBUG_EXIT();
        return;
    }

    uart::Begin(GPS_UARTx, kInitialBaudrate, uart::BITS_8, uart::PARITY_NONE, uart::STOP_1BIT);

    if (module_ != gnss::Module::kUndefined) {
        const auto* string = gnss::kBaud115200[static_cast<uint32_t>(module_)];
        uart::TransmitString(EXT_UART_BASE, string);
        for (int32_t wait = kBaudrateSwitchDelayMs; wait > 0; wait--) {
            watchdog::Feed();
            timing::DelayUs(common::units::kUsPerMs);
        }
        uart::SetBaudrate(EXT_UART_BASE, kOperatingBaudrate);
        uart::TransmitString(EXT_UART_BASE, string);

        const auto kMillis = timing::Millis();
        uint8_t data = ' ';

        while ((timing::Millis() - kMillis) < kBaudrateDetectionTimeoutMs) {
            data = uart::GetRxData(EXT_UART_BASE);
            if (data == nmea::kStartDelimiter) {
                break;
            }
        }

        if (data != nmea::kStartDelimiter) {
            uart::SetBaudrate(EXT_UART_BASE, kInitialBaudrate);
        }
    }

    InitDma();

    SetStatus(gnss::Status::kIdle);
    is_started = true;

    GPS_DEBUG_EXIT();
}

void Receiver::Stop() {
    GPS_DEBUG_ENTRY();

    if (!is_started) {
        GPS_DEBUG_EXIT();
        return;
    }

    NVIC_DisableIRQ(USART2_IRQn);
    usart_interrupt_disable(USART2, USART_INT_IDLE);

    is_started = false;

    SetStatus(gnss::Status::kStopped);

    GPS_DEBUG_EXIT();
}

void Receiver::SetStatus(gnss::Status status) {
    if (status_ == status) {
        return;
    }

    status_ = status;

    gnss::StatusChanged(status);
}

void Receiver::ParseDateTime(const char* sentence, const nmea::Tag kTag) {
    auto offset = nmea::kTalkerIdLength + nmea::kTagLength + 1; // Skipping the ','
    uint32_t field_index{1};
    auto status{gnss::Status::kUndefined};

    do {
        uint32_t length = 0;
        switch (nmea::FieldKey(kTag, field_index)) {
            case nmea::FieldKey(nmea::Tag::kRmc, nmea::rmc::kTimeField): // UTC Time of position, hhmmss.ss
            case nmea::FieldKey(nmea::Tag::kGga, nmea::gga::kTimeField):
            case nmea::FieldKey(nmea::Tag::kZda, nmea::zda::kTimeField):
                SetTime(ParseDecimal(&sentence[offset], length));
                offset += length;
                break;

            case nmea::FieldKey(nmea::Tag::kRmc, nmea::rmc::kStatusField): // Status, A = Valid, V = Warning
                status = (sentence[offset] == 'A' ? gnss::Status::kValid : gnss::Status::kWarning);
                offset += 1;
                break;

            case nmea::FieldKey(nmea::Tag::kRmc, nmea::rmc::kDateField): // Date, ddmmyy
                SetDate(ParseDecimal(&sentence[offset], length));
                offset += length;
                break;

            default:
                break;
        }

        field_index++;

        while ((sentence[offset] != '*') && (sentence[offset] != ',')) {
            offset++;
        }

    } while (sentence[offset++] != '*');

    if (status != gnss::Status::kUndefined) {
        SetStatus(status);
    }
}

void Receiver::Run() {
    if (!is_started) {
        return;
    }

    if (receive_pending) {
        receive_pending = false;
        GPS_DEBUG_PRINTF("received bytes=%u\n", static_cast<unsigned>(bytes_received));

        uint32_t bytes_available{bytes_received};
        uint32_t bytes_processed{0};
        const auto* bytes = dma_buffer;

        while (bytes_available > 0) {
            const auto* sentence = Sentence(bytes, bytes_available, bytes_processed);
            if (sentence == nullptr) {
                break;
            }

            const auto kTag = ParseTag(&sentence[nmea::kTalkerIdLength]);

            if (kTag != nmea::Tag::kUndefined) {
                GPS_DEBUG_PRINTF("%.*s", static_cast<unsigned>(bytes_processed - 2), sentence);
                ParseDateTime(sentence, kTag);
            }

            bytes += bytes_processed;
        }
    }
}
} // namespace gnss
