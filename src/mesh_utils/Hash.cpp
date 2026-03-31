#include "mesh_utils/Hash.hpp"
#include "mbedtls/md.h"
#include <cstring>

namespace mesh {

void Hash::md5(const uint8_t* data, size_t len, uint8_t out[16]) {
    // Use high-level mbedtls MD API for cross-version compatibility
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
    if (md_info) {
        mbedtls_md(md_info, data, len, out);
    } else {
        memset(out, 0, 16);
    }
}

void Hash::nodeHashFromMac(const uint8_t mac[6], uint8_t out[16]) {
    md5(mac, 6, out);
}

} // namespace mesh
