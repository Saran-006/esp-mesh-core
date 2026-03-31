#pragma once

#include "mesh_core/Packet.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstdint>

namespace mesh {

// TCP/UDP-like delivery modes
enum class DeliveryMode : uint8_t {
    UDP = 0,   // fire-and-forget (may have ACK for reliability, but no app response)
    TCP = 1    // request-response: caller blocks until response arrives or timeout
};

static constexpr int MAX_PENDING_REQUESTS = 16;
static constexpr int TCP_DEFAULT_TIMEOUT_MS = 10000;

// Response structure returned to the blocking caller
struct MeshResponse {
    bool     success;
    Packet   responsePacket;
    uint16_t payloadLen;
};

class RequestManager {
public:
    RequestManager();
    ~RequestManager();

    // Send a TCP-style request: enqueue the packet and block the calling task
    // until a response matching this packet_id arrives, or timeout.
    // Returns the response. Caller's thread is blocked.
    MeshResponse sendAndWait(const uint8_t packetId[16], int timeoutMs = TCP_DEFAULT_TIMEOUT_MS);

    // Called by the Dispatcher when a response packet arrives.
    // Matches to a pending request and unblocks the caller.
    bool onResponseReceived(const uint8_t requestId[16], const Packet& responsePkt);

    // Register a pending request (called before enqueue).
    bool registerRequest(const uint8_t packetId[16]);

    // Cancel a pending request
    void cancelRequest(const uint8_t packetId[16]);

private:
    struct PendingRequest {
        uint8_t            packetId[16];
        SemaphoreHandle_t  sem;        // binary semaphore, blocks caller
        MeshResponse       response;
        bool               active;
        bool               completed;
    };
    PendingRequest     requests_[MAX_PENDING_REQUESTS];
    SemaphoreHandle_t  mutex_;

    PendingRequest* findByPacketId(const uint8_t packetId[16]);
};

} // namespace mesh
