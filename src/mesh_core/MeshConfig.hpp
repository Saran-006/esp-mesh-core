#pragma once

#include <cstdint>
#include <cstddef>

namespace mesh {

struct MeshConfig {
    int   maxPeers            = 20;
    int   maxRetries          = 3;
    int   retryBackoffMs      = 200;
    int   eventQueueSize      = 32;
    int   outgoingQueueSize   = 16;
    int   dedupSize           = 512;
    int   ttlDefault          = 10;
    float angleThreshold      = 90.0f;
    float distanceTolerance   = 500.0f;   // meters
    int   nodeTimeoutMs       = 30000;
    int   discoveryIntervalMs = 10000;
    int   maxPayloadSize      = 200;

    uint8_t networkKey[32]    = {};
    size_t  networkKeyLen     = 0;
};

} // namespace mesh
