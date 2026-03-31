#include "mesh_utils/UUID.hpp"
#include "esp_random.h"
#include <cstring>

namespace mesh {

void UUID::generate(uint8_t out[16]) {
    uint32_t r0 = esp_random();
    uint32_t r1 = esp_random();
    uint32_t r2 = esp_random();
    uint32_t r3 = esp_random();
    memcpy(out,      &r0, 4);
    memcpy(out + 4,  &r1, 4);
    memcpy(out + 8,  &r2, 4);
    memcpy(out + 12, &r3, 4);
    // Set version 4 (random) bits
    out[6] = (out[6] & 0x0F) | 0x40;
    out[8] = (out[8] & 0x3F) | 0x80;
}

} // namespace mesh
