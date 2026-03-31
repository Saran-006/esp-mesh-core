#pragma once

#include "mesh_core/Packet.hpp"
#include "mesh_core/Node.hpp"
#include "mesh_core/MeshConfig.hpp"
#include "mesh_core/MeshEvent.hpp"
#include "mesh_core/ILocationProvider.hpp"
#include "mesh_core/PeerManager.hpp"
#include "mesh_core/EventBus.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Forward declarations to avoid circular includes
namespace mesh {
    class ESPNOWTransport;
    class NodeRegistry;
    class DedupCache;
    class DirectionalRouter;
    class AckManager;
    class FragmentManager;
    class RouteCache;
    class RequestManager;
}

namespace mesh {

// Shared context passed to all FreeRTOS tasks via void*
struct MeshContext {
    MeshConfig*          config;
    Node*                selfNode;
    ESPNOWTransport*     transport;
    PeerManager*         peerManager;
    NodeRegistry*        nodeRegistry;
    DedupCache*          dedupCache;
    DirectionalRouter*   router;
    AckManager*          ackManager;
    FragmentManager*     fragmentManager;
    ILocationProvider*   locationProvider;
    EventBus*            eventBus;
    RouteCache*          routeCache;
    RequestManager*      requestManager;

    // Queues
    QueueHandle_t rawIncomingQueue;     // RawIncoming from ESP-NOW callback → ReceiverTask
    QueueHandle_t packetQueue;          // Packet from ReceiverTask → Dispatcher
    QueueHandle_t outgoingQueueHigh;   // Control packets (HIGH)
    QueueHandle_t outgoingQueueMed;    // Location packets (MEDIUM)
    QueueHandle_t outgoingQueueLow;    // Data packets (LOW)

    volatile bool running;
};

// Queue helper: enqueue a packet respecting its priority
bool enqueueOutgoing(MeshContext* ctx, const Packet& pkt);

} // namespace mesh
