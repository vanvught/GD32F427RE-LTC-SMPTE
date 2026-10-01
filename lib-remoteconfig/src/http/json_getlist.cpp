/**
 * @file json_getlist.cpp
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

#include <cstdint>
#include <cstring>
#include <cstdio>

#include "common/utils/utils_string.h"
#include "configstore.h"
#include "configurationstore.h"
#include "ip4/ip4_address.h"
#include "network_config.h"

namespace json {
uint32_t GetList(char* out_buffer, uint32_t out_buffer_size) {
    assert(out_buffer != nullptr);
    assert(out_buffer_size != 0);

    uint8_t display_name[common::store::remoteconfig::kDisplayNameLength];

    ConfigStore::Instance().RemoteConfigCopyArray(display_name, &common::store::RemoteConfig::display_name);

    display_name[common::store::remoteconfig::kDisplayNameLength - 1U] = '\0';

#ifdef DMXNODE_PORTS
    constexpr uint32_t kPortCount = DMXNODE_PORTS;
#else
    constexpr uint32_t kPortCount = 0U;
#endif // DMXNODE_PORTS

    const char* node_type = common::kUndefined;
    const char* output_type = common::kUndefined;

    const int kLength = snprintf(out_buffer, out_buffer_size,
                                 "{\"list\":{\"ip\":\"" IPSTR
                                 "\",\"name\":\"%s\","
                                 "\"node\":{\"type\":\"%s\","
                                 "\"output\":{\"type\":\"%s\",\"count\":%u}}}}",
                                 IP2STR(network::GetPrimaryIp()), reinterpret_cast<const char*>(display_name), node_type, output_type, static_cast<unsigned>(kPortCount));

    if (kLength < 0) {
        return 0U;
    }

    if (static_cast<uint32_t>(kLength) >= out_buffer_size) {
        return out_buffer_size - 1U;
    }

    return static_cast<uint32_t>(kLength);
}
} // namespace json