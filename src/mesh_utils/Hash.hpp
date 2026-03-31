#pragma once

#include <cstdint>
#include <cstddef>

namespace mesh {

class Hash {
public:
    // Compute MD5 hash of input → 16 bytes output.
    // Uses mbedtls on ESP32.
    static void md5(const uint8_t* data, size_t len, uint8_t out[16]);

    // Compute node hash from MAC address (MD5 of MAC)
    static void nodeHashFromMac(const uint8_t mac[6], uint8_t out[16]);
};

} // namespace mesh
