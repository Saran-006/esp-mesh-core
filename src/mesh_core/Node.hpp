#pragma once

#include <cstdint>
#include <cstring>

namespace mesh {

struct Node {
    uint8_t  node_hash[16];
    uint8_t  mac[6];
    float    lat;
    float    lon;
    int64_t  last_seen;  // millis since boot

    Node() : lat(0.0f), lon(0.0f), last_seen(0) {
        memset(node_hash, 0, sizeof(node_hash));
        memset(mac, 0, sizeof(mac));
    }

    bool hashEquals(const uint8_t* other) const {
        return memcmp(node_hash, other, 16) == 0;
    }

    bool macEquals(const uint8_t* other) const {
        return memcmp(mac, other, 6) == 0;
    }

    bool hasLocation() const {
        return !(lat == 0.0f && lon == 0.0f);
    }
};

} // namespace mesh
