#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstdint>
#include <cstring>

namespace mesh {

// Reverse-path route cache: when we forward a packet from source S,
// we remember "to reach S, send to the MAC we received it from."
// This enables responses to follow the same route back without flooding.

static constexpr int ROUTE_CACHE_SIZE = 64;
static constexpr int ROUTE_ENTRY_TTL_MS = 60000;  // 1 minute

class RouteCache {
public:
    RouteCache();
    ~RouteCache();

    // Record: "packets from source_hash arrived via receivedFromMac"
    void recordRoute(const uint8_t sourceHash[16], const uint8_t receivedFromMac[6]);

    // Lookup: "to reach destHash, which MAC should I send to?"
    // Returns true if a cached route exists.
    bool lookupNextHop(const uint8_t destHash[16], uint8_t outMac[6]) const;

    // Remove stale entries
    void pruneStale(int64_t nowMs);

private:
    struct RouteEntry {
        uint8_t destHash[16];
        uint8_t nextHopMac[6];
        int64_t recordedAtMs;
        bool    active;
    };
    RouteEntry         entries_[ROUTE_CACHE_SIZE];
    SemaphoreHandle_t  mutex_;
};

} // namespace mesh
