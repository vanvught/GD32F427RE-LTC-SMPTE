/**
 * @file kjsoninfos.cpp
 */
/* Copyright (C) 2025-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cstddef>
#include <cstdint>

#ifdef NODE_LTC_SMPTE
#undef NODE_LTC_SMPTE
#endif

#include "common/utils/utils_hash.h"
#include "http/json_infos.h"

#ifdef NODE_LTC_SMPTE
#undef NODE_LTC_SMPTE
#endif

namespace json {
uint32_t GetList(char*, uint32_t);
uint32_t GetVersion(char*, uint32_t);
uint32_t GetUptime(char*, uint32_t);

uint32_t GetTimeofday(char*, uint32_t);
void SetTimeofday(const char*, uint32_t);
uint32_t ShowFileDirectory(char*, uint32_t);

// Status
namespace status {
uint32_t Directory(char*, uint32_t);
uint32_t Identify(char*, uint32_t);
uint32_t Display(char*, uint32_t);
uint32_t Dmx(char*, uint32_t);
uint32_t Rdm(char*, uint32_t);
uint32_t RdmQueue(char*, uint32_t);
uint32_t ShowFile(char*, uint32_t);
uint32_t Pixel(char*, uint32_t);
uint32_t PixelDmx(char*, uint32_t);

namespace emac {
uint32_t Phy(char*, uint32_t);
uint32_t Emac(char*, uint32_t);
} // namespace emac
} // namespace status

namespace storage {
uint32_t Directory(char*, uint32_t);
}

namespace action {
void Set(const char*, uint32_t);
void SetShowFile(const char*, uint32_t);
} // namespace action

// Config
namespace config {
uint32_t Directory(char*, uint32_t);

uint32_t GetRemoteConfig(char*, uint32_t);
void SetRemoteConfig(const char*, uint32_t);

uint32_t GetGlobal(char*, uint32_t);
void SetGlobal(const char*, uint32_t);

uint32_t GetNetwork(char*, uint32_t);
void SetNetwork(const char*, uint32_t);

uint32_t GetGps(char*, uint32_t);
void SetGps(const char*, uint32_t);

uint32_t GetTcNet(char*, uint32_t);
void SetTcNet(const char*, uint32_t);

uint32_t GetLtc(char*, uint32_t);
void SetLtc(const char*, uint32_t);

uint32_t GetLtcDisplay(char*, uint32_t);
void SetLtcDisplay(const char*, uint32_t);

uint32_t GetLtcEtc(char*, uint32_t);
void SetLtcEtc(const char*, uint32_t);
} // namespace config

#define ENTRY(get, set, del, filename_literal, config_label, status_label) MakeJsonFileInfo(get, set, del, filename_literal, Fnv1a32(filename_literal, static_cast<uint8_t>(sizeof(filename_literal) - 1)), config_label, status_label)

constexpr Info kFileInfos[] = {
    ENTRY(GetList, nullptr, nullptr, "list", nullptr, nullptr),
    ENTRY(GetVersion, nullptr, nullptr, "version", nullptr, nullptr),
    ENTRY(GetUptime, nullptr, nullptr, "uptime", nullptr, nullptr),
    ENTRY(GetTimeofday, SetTimeofday, nullptr, "timedate", nullptr, nullptr),
    // Status
    ENTRY(status::Directory, nullptr, nullptr, "status/directory", nullptr, nullptr),
    ENTRY(status::Identify, nullptr, nullptr, "status/identify", nullptr, "Identify"),
    ENTRY(status::Display, nullptr, nullptr, "status/display", nullptr, "Display"),
    ENTRY(status::emac::Phy, nullptr, nullptr, "status/phy", nullptr, "Phy"),
    ENTRY(status::emac::Emac, nullptr, nullptr, "status/emac", nullptr, "Emac"),
    // Config
    ENTRY(config::Directory, nullptr, nullptr, "config/directory", nullptr, nullptr),
    ENTRY(config::GetRemoteConfig, config::SetRemoteConfig, nullptr, "config/remote", "Remote configuration", nullptr),
    ENTRY(config::GetGlobal, config::SetGlobal, nullptr, "config/global", "Global", nullptr),
    ENTRY(config::GetNetwork, config::SetNetwork, nullptr, "config/network", "Network", nullptr),
    // LTC
    // ENTRY(config::GetLtc, config::SetLtc, nullptr, "config/ltc", "LTC SMPTE", nullptr),
    // ENTRY(config::GetLtcDisplay, config::SetLtcDisplay, nullptr, "config/ltcdisplays", "LTC Displays", nullptr),
    // ENTRY(config::GetGps, config::SetGps, nullptr, "config/gps", "GPS", nullptr),
    // ENTRY(config::GetTcNet, config::SetTcNet, nullptr, "config/tcnet", "TCNet", nullptr),
    // ENTRY(config::GetLtcEtc, config::SetLtcEtc, nullptr, "config/etc", "ETC Connect", nullptr),
};

constexpr size_t kFileInfosSize = sizeof(kFileInfos) / sizeof(kFileInfos[0]);

static_assert(HasUniqueHashes(kFileInfos, kFileInfosSize), "Duplicate filename hashes detected in kFileInfos!");
} // namespace json