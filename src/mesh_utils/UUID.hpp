#pragma once

#include <cstdint>

namespace mesh {

class UUID {
public:
    // Generate a 16-byte pseudo-random UUID using esp_random()
    static void generate(uint8_t out[16]);
};

} // namespace mesh
