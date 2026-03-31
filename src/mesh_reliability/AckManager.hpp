#pragma once

#include "mesh_core/Packet.hpp"
#include "mesh_core/MeshConfig.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstdint>

namespace mesh {

static constexpr int MAX_PENDING_ACKS = 32;

class AckManager {
public:
    AckManager(int maxRetries, int backoffMs);
    ~AckManager();

    // Track a sent packet that requires ACK
    void trackPacket(const Packet& pkt, const uint8_t destMac[6]);

    // Called when an ACK packet is received
    void onAckReceived(const uint8_t packetId[16]);

    // Check for timed-out entries, retry or discard.
    // Returns packets to re-send via the provided callback.
    using RetryCb = void(*)(const Packet& pkt, const uint8_t destMac[6], void* userCtx);
    void processRetries(int64_t nowMs, RetryCb cb, void* userCtx);

    // Check if a packet_id is pending ACK
    bool isPending(const uint8_t packetId[16]) const;

private:
    struct PendingEntry {
        Packet   pkt;
        uint8_t  destMac[6];
        int64_t  sentAtMs;
        int      retryCount;
        bool     active;
    };
    PendingEntry      entries_[MAX_PENDING_ACKS];
    int               maxRetries_;
    int               backoffMs_;
    SemaphoreHandle_t mutex_;
};

} // namespace mesh
