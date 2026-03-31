#include "mesh_core/Security.hpp"
#include "mesh_utils/Hash.hpp"
#include "mbedtls/md.h"
#include <cstring>

namespace mesh {

void Security::computeSignature(const Packet& packet, const uint8_t* key,
                                 size_t keyLen, uint8_t outSig[16]) {
    // HMAC-like: MD5(key || header || payload || key)
    // Using high-level mbedtls MD API for cross-version compatibility
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, md_info, 0);
    mbedtls_md_starts(&ctx);

    // Prefix with key
    mbedtls_md_update(&ctx, key, keyLen);

    // Hash the header
    mbedtls_md_update(&ctx, reinterpret_cast<const uint8_t*>(&packet.header),
                      sizeof(PacketHeader));

    // Hash the payload
    if (packet.header.payload_size > 0) {
        mbedtls_md_update(&ctx, packet.payload, packet.header.payload_size);
    }

    // Suffix with key (HMAC-like construction)
    mbedtls_md_update(&ctx, key, keyLen);

    mbedtls_md_finish(&ctx, outSig);
    mbedtls_md_free(&ctx);
}

void Security::signPacket(Packet& packet, const uint8_t* key, size_t keyLen) {
    computeSignature(packet, key, keyLen, packet.signature);
}

bool Security::verifyPacket(const Packet& packet, const uint8_t* key, size_t keyLen) {
    uint8_t expected[16];
    computeSignature(packet, key, keyLen, expected);
    return memcmp(expected, packet.signature, 16) == 0;
}

} // namespace mesh
