#pragma once

#include "Packet.hpp"
#include <cstdint>
#include <cstddef>

namespace mesh {

class Security {
public:
    static void signPacket(Packet& packet, const uint8_t* key, size_t keyLen);
    static bool verifyPacket(const Packet& packet, const uint8_t* key, size_t keyLen);
private:
    static void computeSignature(const Packet& packet, const uint8_t* key,
                                 size_t keyLen, uint8_t outSig[16]);
};

} // namespace mesh
