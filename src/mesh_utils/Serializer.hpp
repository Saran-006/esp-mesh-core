#pragma once

#include <cstdint>
#include <cstddef>

namespace mesh {

class Serializer {
public:
    // Write float to buffer (little-endian). Returns bytes written (4).
    static size_t writeFloat(uint8_t* buf, float val);

    // Read float from buffer (little-endian).
    static float  readFloat(const uint8_t* buf);

    // Write uint16 to buffer (little-endian).
    static size_t writeU16(uint8_t* buf, uint16_t val);

    // Read uint16 from buffer (little-endian).
    static uint16_t readU16(const uint8_t* buf);

    // Write uint32 to buffer (little-endian).
    static size_t writeU32(uint8_t* buf, uint32_t val);

    // Read uint32 from buffer (little-endian).
    static uint32_t readU32(const uint8_t* buf);
};

} // namespace mesh
