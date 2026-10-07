/**
 * @file ltc_actions.h
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

#ifndef LTC_ACTIONS_H_
#define LTC_ACTIONS_H_

#include <cstdint>
#include <string_view>

namespace ltc::actions {
void HandleAction(std::string_view action);
void SetType(std::string_view type);
void SetStart(std::string_view start);
void SetStop(std::string_view stop);
void SetResume(std::string_view resume);
void SetDirection(std::string_view direction);

enum class Skip { kForward = 0, kBackward = 1 };
void SetSkip(Skip skip, uint32_t seconds);
void SetPitch(float pitch);

namespace udp {
void Start();
void Stop();
} // namespace udp
} // namespace ltc::actions

#endif // LTC_ACTIONS_H_
