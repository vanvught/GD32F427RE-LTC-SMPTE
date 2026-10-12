/**
 * @file tcnet_packet.h
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

/**
 * Specification V3.3.3 11/11/2019
 */

#ifndef TCNETPACKETS_H_
#define TCNETPACKETS_H_

#include <cstdint>

namespace tcnet::packet {
enum class MessageType : uint8_t {
    kMessageTypeOptin = 2,
    kMessageTypeOptout = 3,
    kMessageTypeStatus = 5,
    kMessageTypeTimesync = 10,
    kMessageTypeErrorNotifiction = 13,
    kMessageTypeRequest = 20,
    kMessageTypeApplication = 30,
    kMessageTypeControl = 101,
    kMessageTypeTextdata = 128,
    kMessageTypeTime = 254,
};

enum class NodeType {
    kAuto = 1,     //
    kMaster = 2,   //
    kSlave = 4,    //
    kRepeater = 8, //
};

inline constexpr uint32_t kNodeNameLength = 8;
inline constexpr uint32_t kVendorNameLength = 16;
inline constexpr uint32_t kDeviceNameLength = 16;

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

struct ManagementHeader {
    uint16_t node_id;                   //  0:2
    uint8_t protocol_version_major;     //  2:1
    uint8_t protocol_version_minor;     //  3:1
    uint8_t header[3];                  //  4:3
    uint8_t message_type;               //  7:1
    uint8_t node_name[kNodeNameLength]; //  8:8
    uint8_t seq;                        // 16:1
    uint8_t node_type;                  // 17:1
    uint16_t node_options;              // 18:2
    uint32_t time_stamp;                // 20:4
} PACKED;

struct OptIn {
    ManagementHeader management_header;
    uint16_t node_count;
    uint16_t node_listener_port;
    uint16_t up_time;
    uint8_t reserved1[2];
    uint8_t vendor_name[kVendorNameLength];
    uint8_t device_name[kDeviceNameLength];
    uint8_t device_major_version;
    uint8_t device_minor_version;
    uint8_t device_bug_version;
    uint8_t reserved2;
} PACKED;

struct OptOut {
    ManagementHeader management_header;
    uint16_t node_count;
    uint16_t node_listener_port;
} PACKED;

struct Status {
    struct ManagementHeader management_header;
    uint16_t node_count;         // 24:2
    uint16_t node_listener_port; // 26:2
    uint8_t reserved1[6];        // 28:6
    uint8_t Layer1Source;
    uint8_t Layer2Source;
    uint8_t Layer3Source;
    uint8_t Layer4Source;
    uint8_t LayerASource;
    uint8_t LayerBSource;
    uint8_t LayerMSource;
    uint8_t LayerCSource;
    uint8_t Layer1Status;
    uint8_t Layer2Status;
    uint8_t Layer3Status;
    uint8_t Layer4Status;
    uint8_t LayerAStatus;
    uint8_t LayerBStatus;
    uint8_t LayerMStatus;
    uint8_t LayerCStatus;
    uint32_t Layer1TrackID;
    uint32_t Layer2TrackID;
    uint32_t Layer3TrackID;
    uint32_t Layer4TrackID;
    uint32_t LayerATrackID;
    uint32_t LayerBTrackID;
    uint32_t LayerMTrackID;
    uint32_t LayerCTrackID;
    uint8_t reserved2;
    uint8_t SMPTEMode;
    uint8_t AutoMasterMode;
    uint8_t Reserved3[15];
    uint8_t AppSpecific[72];
} PACKED;

struct TimeSync {
    ManagementHeader management_header;
    uint8_t step;
    uint8_t reserved1;
    uint16_t node_listener_port;
    uint32_t remote_timestamp;
} PACKED;

struct ErrorNotification {
    ManagementHeader management_header;
    uint8_t data_type;
    uint8_t layer_id;
    uint16_t code;
    uint16_t message_type;
} PACKED;

struct Request {
    ManagementHeader management_header;
    uint8_t data_type;
    uint8_t layer;
} PACKED;

struct Application {
    ManagementHeader management_header;
    uint8_t data;
} PACKED;

#define APPLICATION_DATA_DATA_SIZE(x) ((x) - sizeof(struct ManagementHeader management_header))

struct Control {
    ManagementHeader management_header;
    uint8_t control_path;
} PACKED;

struct TextData {
    ManagementHeader management_header;
    uint8_t text_data;
} PACKED;

struct TimeCode {
    uint8_t smpte_mode;
    uint8_t state;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t frames;
} PACKED;

struct Time {
    ManagementHeader management_header;
    uint32_t L1Time;      //  24:4
    uint32_t L2Time;      //  28:4
    uint32_t L3Time;      //  32:4
    uint32_t L4Time;      //  36:4
    uint32_t LATime;      //  40:4
    uint32_t LBTime;      //  44:4
    uint32_t LMTime;      //  48:4
    uint32_t LCTime;      //  52:4
    uint32_t L1TotalTime; //  56:4
    uint32_t L2TotalTime; //  60:4
    uint32_t L3TotalTime; //  64:4
    uint32_t L4TotalTime; //  68:4
    uint32_t LATotalTime; //  72:4
    uint32_t LBTotalTime; //  76:4
    uint32_t LMTotalTime; //  80:4
    uint32_t LCTotalTime; //  84:4
    uint8_t L1BeatMarker; //  88:1
    uint8_t L2BeatMarker; //  89:1
    uint8_t L3BeatMarker; //  90:1
    uint8_t L4BeatMarker; //  91:1
    uint8_t LABeatMarker; //  92:1
    uint8_t LBBeatMarker; //  93:1
    uint8_t LMBeatMarker; //  94:1
    uint8_t LCBeatMarker; //  95:1
    uint8_t L1LayerState; //  96:1
    uint8_t L2LayerState; //  97:1
    uint8_t L3LayerState; //  98:1
    uint8_t L4LayerState; //  99:1
    uint8_t LALayerState; // 100:1
    uint8_t LBLayerState; // 101:1
    uint8_t LMLayerState; // 102:1
    uint8_t LCLayerState; // 103:1
    uint8_t reserved1;    // 104:1
    uint8_t SMPTEMode;    // 105:1
    TimeCode L1TimeCode;  // 106:6
    TimeCode L2TimeCode;  // 112:6
    TimeCode L3TimeCode;  // 118:6
    TimeCode L4TimeCode;  // 124:6
    TimeCode LATimeCode;  // 130:6
    TimeCode LBTimeCode;  // 136:6
    TimeCode LMTimeCode;  // 142:6
    TimeCode LCTimeCode;  // 148:6
    uint8_t L1LayerOnAir; // 154:1
    uint8_t L2LayerOnAir; // 155:1
    uint8_t L3LayerOnAir; // 156:1
    uint8_t L4LayerOnAir; // 157:1
    uint8_t LALayerOnAir; // 158:1
    uint8_t LBLayerOnAir; // 159:1
    uint8_t LMLayerOnAir; // 160:1
    uint8_t LCLayerOnAir; // 161:1
    uint8_t reserved2;    // 162:1
} PACKED;

struct Packet {
    union {
        struct ManagementHeader management_header;
        struct OptIn opt_in;
        struct OptOut opt_out;
        struct Status status;
        struct TimeSync timesync;
        struct ErrorNotification error_notification;
        struct Request request;
        struct Application application;
        struct Control control;
        struct TextData text_data;
        struct Time time;
    } u;
    uint8_t filler[512];
} PACKED;
} // namespace tcnet::packet

#endif // TCNETPACKETS_H_
