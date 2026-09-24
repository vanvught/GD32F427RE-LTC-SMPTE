/**
 * @file main.cpp
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

#include "apps/ntpclient.h"
#include "board.h"
#include "configstore.h"
#include "display.h"
#include "firmware/firmwareversion.h"
#include "ltc_node.h"
#include "network.h"
#include "remoteconfig.h"
#include "shell.h"
#include "software_version.h"
#include "watchdog.h"

namespace board {
void RebootHandler() {}
} // namespace board

int main() { // NOLINT
	board::Init();
	Display display(4);
	ConfigStore config_store;
	network::Init();
	FirmwareVersion firmware(kSoftwareVersion, __DATE__, __TIME__);

	firmware.Print("LTC SMPTE");

	RemoteConfig remote_config(remoteconfig::Output::TIMECODE, 0);

    ltc::Node node;
    node.SetUtcOffset(2, 0);
    network::apps::ntpclient::ptp::SetServerIp(network::ConvertToUint(45,138,55,61));

	watchdog::Init();

	for (;;) {
		watchdog::Feed();
		network::Run();
		board::Run();
		node.Run();
		Shell::Instance().Run();
    }

    return 0;
}
