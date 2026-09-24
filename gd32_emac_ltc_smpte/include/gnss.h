/**
 * @file gps.h
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

#ifndef GPS_H_
#define GPS_H_

#include <cassert>
#include <cstdint>
#include <cstring>
#include <strings.h>
#include <ctime>
#include <utility>

#include "common/utils/utils_string.h"
#include "common/utils/utils_hex.h"

#define GPS_UARTx 						USART5
#define GPS_UARTx_IRQHandler			USART5_IRQHandler
#define GPS_UARTx_IRQn 					USART5_IRQn
#define GPS_UARTx_DMAx 					USART5_DMAx
#define GPS_UARTx_RX_DMA_CHx 			USART2_RX_DMA_CHx
#define GPS_UARTx_RX_DMA_SUBPERIx 		USART2_RX_DMA_SUBPERIx

// gnss             generic GNSS receiver functionality
// gnss::nmea       NMEA protocol

namespace gnss {
namespace nmea {
enum class State { kStartDelimiter, kData, kChecksum1, kChecksum2, kCr, kLf };

constexpr uint32_t kMaxSentenceLength = 82; ///< including the $ and <CR><LF>
constexpr char kStartDelimiter = '$';       ///< The start delimiter is normally '$' (ASCII 36)

enum class Tag : uint8_t { kRmc, kGga, kZda, kUndefined };

constexpr uint32_t kTalkerIdLength = 2;
constexpr uint32_t kTagLength = 3;

constexpr char kTag[][kTagLength] = {
    {'R', 'M', 'C'}, // Recommended Minimum Navigation Information
    {'G', 'G', 'A'}, // Global Positioning System Fix Data
    {'Z', 'D', 'A'}, // Time & Date - UTC, day, month, year and local time zone
};

constexpr uint32_t kFieldIndexShift = 8;

namespace rmc {
constexpr uint32_t kTimeField = 1;
constexpr uint32_t kStatusField = 2;
constexpr uint32_t kDateField = 9;
} // namespace rmc

namespace gga {
constexpr uint32_t kTimeField = 1;
}

namespace zda {
constexpr uint32_t kTimeField = 1;
}

constexpr uint32_t FieldKey(nmea::Tag tag, uint32_t field) {
    return std::to_underlying(tag) | (field << 8);
}
} // namespace nmea

enum class Module : uint8_t { kAtgM336H, kUbloxNeo, kMtK3339, kUndefined };
enum class Status { kIdle, kWarning, kValid, kStopped, kUndefined };

constexpr uint32_t kMaxNameLength = 11; // Including '\0'
constexpr const char kModule[][kMaxNameLength] = {"ATGM336H", "ublox-NEO7", "MTK3339"};

constexpr uint32_t kMaxStatusLength = 10; // Including '\0'
constexpr const char kStatus[][kMaxStatusLength] = {"Idle", "Warning", "Valid", "Stopped", "Undefined"};

constexpr int32_t kTmYearBase = 1900;
constexpr int32_t kTmMonthOffset = 1;
constexpr int32_t kNmeaTwoDigitYearBase = 2000;

constexpr uint32_t kPpsIrqPriority = 2;
constexpr uint32_t kUartIrqPriority = 3;

[[nodiscard]] constexpr const char* ModuleToName(Module module) {
    if (module < Module::kUndefined) {
        return kModule[static_cast<uint32_t>(module)];
    }

    return common::kUndefined;
}

inline Module ModuleFromName(const char* name) {
    uint32_t index = 0;

    for (const auto& input : kModule) {
        if (strcasecmp(name, input) == 0) {
            return static_cast<Module>(index);
        }

        ++index;
    }

    return Module::kUndefined;
}

[[nodiscard]] constexpr const char* StatusToName(Status status) {
    if (status < Status::kUndefined) {
        return kStatus[static_cast<uint32_t>(status)];
    }

    return common::kUndefined;
}

inline nmea::Tag ParseTag(const char* tag) {
    uint32_t index = 0;

    for (const auto& input : nmea::kTag) {
        if (memcmp(tag, input, nmea::kTagLength) == 0) {
            return static_cast<nmea::Tag>(index);
        }

        ++index;
    }

    return nmea::Tag::kUndefined;
}

inline const char* Sentence(const char* bytes, uint32_t& bytes_available, uint32_t& bytes_processed) {
    uint16_t count{0};
    uint16_t data_index{0};
    uint16_t bytes_offset{0};
    uint8_t checksum{0};
    auto done{false};
    auto state = gnss::nmea::State::kStartDelimiter;

    while (!done && (bytes_available-- > 0)) {
        const auto kByte = bytes[count++];

        switch (state) {
            case nmea::State::kStartDelimiter:
                if (kByte == gnss::nmea::kStartDelimiter) {
                    state = nmea::State::kData;
                    bytes_offset = count;
                    data_index = 0;
                    checksum = 0;
                }
                break;

            case nmea::State::kData:
                if (kByte != '*') {
                    data_index++;
                    checksum ^= kByte;
                    break;
                }

                state = nmea::State::kChecksum1;
                break;

            case nmea::State::kChecksum1:
                if (common::hex::FromChar(kByte) == ((checksum >> 4) & 0xF)) {
                    state = nmea::State::kChecksum2;
                    break;
                }

                state = nmea::State::kStartDelimiter;
                break;

            case nmea::State::kChecksum2:
                if (common::hex::FromChar(kByte) == (checksum & 0xF)) {
                    state = nmea::State::kCr;
                    break;
                }

                state = nmea::State::kStartDelimiter;
                break;

            case nmea::State::kCr:
                if (kByte == '\r') {
                    state = nmea::State::kLf;
                    break;
                }

                state = nmea::State::kStartDelimiter;
                break;

            case nmea::State::kLf:
                if (kByte == '\n') {
                    done = true;
                    break;
                }

                state = nmea::State::kStartDelimiter;
                break;
        }

        if (data_index >= nmea::kMaxSentenceLength) {
            state = nmea::State::kStartDelimiter;
        }
    }

    bytes_processed = count;

    if (!done) {
        return nullptr;
    }

    return &bytes[bytes_offset];
}

class Receiver {
   public:
    Receiver() noexcept;

    void SetUtcOffset(int32_t hours, uint32_t minutes);
    [[nodiscard]] int32_t UtcOffset() const { return utc_offset_; }

    void SetDate(uint32_t year, uint32_t month, uint32_t day);

    void SetModule(gnss::Module module);
    [[nodiscard]] gnss::Module Module() const { return module_; }

    void Start();
    void Stop();
    void Run();

    void SetTime(int32_t time) {
        if (time != 0) {
            is_time_updated_ = true;

            time /= 100;
            time_date_.tm_sec = time % 100;
            time /= 100;
            time_date_.tm_min = time % 100;
            time_date_.tm_hour = time / 100;
        }
    }

    void SetDate(int32_t date) {
        if (date != 0) {
            is_date_updated_ = true;

            time_date_.tm_year = 100 + (date % 100); // The number of years since 1900.
            date /= 100;
            time_date_.tm_mon = (date % 100) - 1; // The number of months since January, in the range 0 to 11.
            time_date_.tm_mday = date / 100;      // The day of the month, in the range 1 to 31.
        }
    }

    time_t LocalSeconds() {
        is_time_updated_ = is_date_updated_ = false;
        return mktime(&time_date_) + utc_offset_;
    }

    static Receiver& Instance() {
        assert(s_instance != nullptr);
        return *s_instance;
    }

   private:
    void ParseDateTime(const char* sentence, nmea::Tag tag);
    void SetStatus(gnss::Status status);

    int32_t ParseDecimal(const char* p, uint32_t& length) {
        const auto kIsNegative = (*p == '-');

        length = kIsNegative ? 1 : 0;
        int32_t value = 0;

        while ((p[length] != '.') && (p[length] != ',')) {
            value = (value * 10) + p[length] - '0';
            length++;
        }

        if (p[length] == '.') {
            length++;
            value = (value * 10) + p[length] - '0';
            length++;
            value = (value * 10) + p[length] - '0';
            length++;
        }

        return kIsNegative ? -value : value;
    }

    int32_t utc_offset_{0};
    bool is_time_updated_{false};
    bool is_date_updated_{false};
    gnss::Module module_{gnss::Module::kUndefined};
    gnss::Status status_{gnss::Status::kUndefined};
    struct tm time_date_;

    inline static Receiver* s_instance;
};

void StatusChanged(gnss::Status status);
} // namespace gnss

#endif // GPS_H_
