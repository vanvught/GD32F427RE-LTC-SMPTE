/**
 * @file shell.h
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

#ifndef SHELL_H_
#define SHELL_H_

#ifdef CONFIG_CLIB_USE_NULL
#error CONFIG_CLIB_USE_NULL cannot be defined
#endif // CONFIG_CLIB_USE_NULL

/*
UART
 ↓
line_buffer_
 ↓
std::string_view line
 ↓
Tokenize()
 ↓
std::array<std::string_view, kMaxTokens>
 ↓
std::span<const std::string_view>
 ↓
command(Arguments)
*/

#include <optional>
#include <string_view>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <span>

#include "uart0.h"
#include "common/utils/utils_string.h"

namespace shell {
inline constexpr char kTooMany[] = "Too many arguments";

inline std::optional<int32_t> GetValue(std::string_view text, int32_t max_value) {
    const auto kValue = common::Atoi(text.data(), text.size());

    if (kValue <= max_value) {
        return kValue;
    }

    return std::nullopt;
}

using Arguments = std::span<const std::string_view>;
using CommandFunction = void (*)(Arguments);

struct Entry {
    std::string_view name;
    CommandFunction function;
};

void Reboot(Arguments args);
void Version(Arguments args);
#ifdef HAVE_TIMEOFDAY
void Date(Arguments args);
#endif // HAVE_TIMEOFDAY
#ifndef DISABLE_RTC
void HwClock(Arguments args);
#endif // DISABLE_RTC
namespace ltc {
void Input(Arguments args);
void Type(Arguments args);
void UtcOffset(Arguments args);
void Enable(Arguments args);
void Disable(Arguments args);
void Intensity(Arguments args);
void Gps(Arguments args);
} // namespace ltc

inline constexpr Entry kCommandTable[] = {
    {.name = "reboot", .function = Reboot},   //
    {.name = "version", .function = Version}, //
#ifdef HAVE_TIMEOFDAY
    {.name = "date", .function = Date}, //
#endif
#ifndef DISABLE_RTC
    {.name = "hwclock", .function = HwClock}, //
#endif
    {.name = "input", .function = ltc::Input},         //
    {.name = "type", .function = ltc::Type},           //
    {.name = "utc", .function = ltc::UtcOffset},       //
    {.name = "enable", .function = ltc::Enable},       //
    {.name = "disable", .function = ltc::Disable},     //
    {.name = "intensity", .function = ltc::Intensity}, //
    {.name = "gps", .function = ltc::Gps},             //
};

struct TokenizeResult {
    std::span<const std::string_view> args;
    bool valid;
};
} // namespace shell

class Shell {
    static constexpr uint32_t kMaxTokens = 6;
    static constexpr uint32_t kMaxLineLength = 40;

   public:
    static Shell& Instance() {
        static Shell instance;
        return instance;
    }

    void Run() {
        const auto kLine = ReadLine();

        if (!kLine.empty()) [[unlikely]] {
            HandleLine(kLine);
        }
    }

   private:
    Shell() { uart0::Init(); }

    static constexpr bool IsQuote(char character) { return (character == '\'') || (character == '\"'); }

    shell::TokenizeResult Tokenize(std::string_view line) {
        std::size_t position = 0;
        std::size_t token_count = 0;

        while (position < line.size()) {
            while ((position < line.size()) && (std::isspace(static_cast<unsigned char>(line[position])) != 0)) {
                ++position;
            }

            if (position == line.size()) {
                break;
            }

            if (token_count == tokens_.size()) {
                return {.args={}, .valid=false};
            }

            if (IsQuote(line[position])) {
                const auto kQuote = line[position++];
                const auto kStart = position;

                while ((position < line.size()) && (line[position] != kQuote)) {
                    ++position;
                }

                if (position == line.size()) {
                    return {.args={}, .valid=false};
                }

                tokens_[token_count++] = {line.data() + kStart, position - kStart};

                ++position;
            } else {
                const auto kStart = position;

                while ((position < line.size()) && (std::isspace(static_cast<unsigned char>(line[position])) == 0)) {
                    ++position;
                }

                tokens_[token_count++] = {line.data() + kStart, position - kStart};
            }
        }

        return {.args=std::span<const std::string_view>{tokens_.data(), token_count}, .valid=true};
    }

    void HandleLine(std::string_view line) {
        const auto kResult = Tokenize(line);

        if (!kResult.valid) {
            uart0::Puts("argument parse error");
            return;
        }

        if (kResult.args.empty()) {
            return;
        }

        for (const auto& command : shell::kCommandTable) {
            if (command.name == kResult.args.front()) {
                command.function(kResult.args);
                return;
            }
        }

        uart0::Puts("command not found");
    }

    std::string_view ReadLine() {
        const auto kChar = uart0::GetChar();

        if (kChar == EOF) {
            return {};
        }

        if (kChar == '\r') {
            uart0::PutChar('\n');

            const std::string_view kLine{line_buffer_, chars_};
            chars_ = 0;

            return kLine;
        }

        if (std::isprint(kChar) != 0) {
            uart0::PutChar(kChar);
            line_buffer_[chars_++] = static_cast<char>(kChar);

            if (chars_ == sizeof(line_buffer_) - 1) {
                chars_ = 0;
            }
        } else {
            if (kChar == '\b') {
                if (chars_ != 0) {
                    line_buffer_[--chars_] = '\0';
                }
            }

            uart0::PutChar(kChar);
            uart0::PutChar(' ');
            uart0::PutChar(kChar);
        }

        return {};
    }

    char line_buffer_[kMaxLineLength]{};
    uint32_t chars_{};
    std::array<std::string_view, kMaxTokens> tokens_{};
};

#endif // SHELL_H_