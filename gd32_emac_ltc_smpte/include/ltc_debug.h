/**
 * ltc_debug.h
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

#ifndef LTC_DEBUG_H_
#define LTC_DEBUG_H_

#include "firmware/debug/debug_debug.h"

#ifdef DEBUG_LTC
#define LTC_DEBUG_ENTRY() DEBUG_ENTRY()
#define LTC_DEBUG_EXIT() DEBUG_EXIT()
#define LTC_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define LTC_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define LTC_DEBUG_ENTRY() \
    do {                  \
    } while (false)
#define LTC_DEBUG_EXIT() \
    do {                 \
    } while (false)
#define LTC_DEBUG_PRINTF(...) \
    do {                      \
    } while (false)
#define LTC_DEBUG_PUTS(...) \
    do {                    \
    } while (false)
#endif // DEBUG_LTC

#ifdef DEBUG_LTC_INPUT
#define LTC_INPUT_DEBUG_ENTRY() DEBUG_ENTRY()
#define LTC_INPUT_DEBUG_EXIT() DEBUG_EXIT()
#define LTC_INPUT_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define LTC_INPUT_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define LTC_INPUT_DEBUG_ENTRY() \
    do {                        \
    } while (false)
#define LTC_INPUT_DEBUG_EXIT() \
    do {                       \
    } while (false)
#define LTC_INPUT_DEBUG_PRINTF(...) \
    do {                            \
    } while (false)
#define LTC_INPUT_DEBUG_PUTS(...) \
    do {                          \
    } while (false)
#endif // DEBUG_LTC_INPUT

#ifdef DEBUG_LTC_OUTPUT
#define LTC_OUTPUT_DEBUG_ENTRY() DEBUG_ENTRY()
#define LTC_OUTPUT_DEBUG_EXIT() DEBUG_EXIT()
#define LTC_OUTPUT_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define LTC_OUTPUT_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define LTC_OUTPUT_DEBUG_ENTRY() \
    do {                         \
    } while (false)
#define LTC_OUTPUT_DEBUG_EXIT() \
    do {                        \
    } while (false)
#define LTC_OUTPUT_DEBUG_PRINTF(...) \
    do {                             \
    } while (false)
#define LTC_OUTPUT_DEBUG_PUTS(...) \
    do {                           \
    } while (false)
#endif // DEBUG_LTC_OUTPUT

#ifdef DEBUG_LTC_DISPLAY
#define LTC_DISPLAY_DEBUG_ENTRY() DEBUG_ENTRY()
#define LTC_DISPLAY_DEBUG_EXIT() DEBUG_EXIT()
#define LTC_DISPLAY_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define LTC_DISPLAY_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define LTC_DISPLAY_DEBUG_ENTRY() \
    do {                          \
    } while (false)
#define LTC_DISPLAY_DEBUG_EXIT() \
    do {                         \
    } while (false)
#define LTC_DISPLAY_DEBUG_PRINTF(...) \
    do {                              \
    } while (false)
#define LTC_DISPLAY_DEBUG_PUTS(...) \
    do {                            \
    } while (false)
#endif // DEBUG_LTC_DISPLAY

#ifdef DEBUG_LTC_GPS
#define LTC_GPS_DEBUG_ENTRY() DEBUG_ENTRY()
#define LTC_GPS_DEBUG_EXIT() DEBUG_EXIT()
#define LTC_GPS_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define LTC_GPS_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define LTC_GPS_DEBUG_ENTRY() \
    do {                      \
    } while (false)
#define LTC_GPS_DEBUG_EXIT() \
    do {                     \
    } while (false)
#define LTC_GPS_DEBUG_PRINTF(...) \
    do {                          \
    } while (false)
#define LTC_GPS_DEBUG_PUTS(...) \
    do {                        \
    } while (false)
#endif // DEBUG_LTC_GPS

#ifdef DEBUG_LTC_PTP
#define LTC_PTP_DEBUG_ENTRY() DEBUG_ENTRY()
#define LTC_PTP_DEBUG_EXIT() DEBUG_EXIT()
#define LTC_PTP_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define LTC_PTP_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define LTC_PTP_DEBUG_ENTRY() \
    do {                      \
    } while (false)
#define LTC_PTP_DEBUG_EXIT() \
    do {                     \
    } while (false)
#define LTC_PTP_DEBUG_PRINTF(...) \
    do {                          \
    } while (false)
#define LTC_PTP_DEBUG_PUTS(...) \
    do {                        \
    } while (false)
#endif // DEBUG_LTC_PTP

#ifdef DEBUG_LTC_NTP
#define LTC_NTP_DEBUG_ENTRY() DEBUG_ENTRY()
#define LTC_NTP_DEBUG_EXIT() DEBUG_EXIT()
#define LTC_NTP_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define LTC_NTP_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define LTC_NTP_DEBUG_ENTRY() \
    do {                      \
    } while (false)
#define LTC_NTP_DEBUG_EXIT() \
    do {                     \
    } while (false)
#define LTC_NTP_DEBUG_PRINTF(...) \
    do {                          \
    } while (false)
#define LTC_NTP_DEBUG_PUTS(...) \
    do {                        \
    } while (false)
#endif // DEBUG_LTC_NTP

#endif // LTC_DEBUG_H_
