/**
 * @file ltc_display.h
 *
 */

#ifndef LTC_DISPLAY_H_
#define LTC_DISPLAY_H_

#include <cstdint>
#include <strings.h>

namespace ltc::display {
enum class Types : uint8_t { kMatrix = 0, k7Segment = 1 };

constexpr char kMatrix[] = "Matrix";
constexpr char k7Segment[] = "7Segment";

inline const char* TypeToName(Types type) {
    return type == Types::kMatrix ? kMatrix : k7Segment;
}

inline Types TypeFromName(const char* name) {
    if (strcasecmp(kMatrix, name) == 0) {
        return Types::kMatrix;
    };

    return Types::k7Segment;
}

} // namespace ltc::display

#endif // LTC_DISPLAY_H_
