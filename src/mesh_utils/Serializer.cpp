#include "mesh_utils/Serializer.hpp"
#include <cstring>

namespace mesh {

size_t Serializer::writeFloat(uint8_t* buf, float val) {
    memcpy(buf, &val, 4);
    return 4;
}

float Serializer::readFloat(const uint8_t* buf) {
    float val;
    memcpy(&val, buf, 4);
    return val;
}

size_t Serializer::writeU16(uint8_t* buf, uint16_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    return 2;
}

uint16_t Serializer::readU16(const uint8_t* buf) {
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

size_t Serializer::writeU32(uint8_t* buf, uint32_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
    return 4;
}

uint32_t Serializer::readU32(const uint8_t* buf) {
    return (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
}

} // namespace mesh
