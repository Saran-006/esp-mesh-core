#include "mesh_tasks/HealthTask.hpp"
#include "mesh_core/Logger.hpp"
#include "mesh_core/Packet.hpp"
#include "mesh_registry/NodeRegistry.hpp"
#include "mesh_reliability/AckManager.hpp"
#include "mesh_reliability/FragmentManager.hpp"
#include "mesh_routing/RouteCache.hpp"
#include "mesh_events/NodeEvents.hpp"
#include "mesh_transport/ESPNOWTransport.hpp"
#include <Arduino.h>

static const char* TAG = "HealthTask";

namespace mesh {

// ACK retry callback: re-enqueue the packet for sending
static void ackRetryCb(const Packet& pkt, const uint8_t destMac[6], void* userCtx) {
    MeshContext* ctx = static_cast<MeshContext*>(userCtx);
    enqueueOutgoing(ctx, pkt);
}

// Failure callback: if a node fails to ACK, prune it from registry and cache
static void ackFailureCb(const uint8_t destMac[6], void* userCtx) {
    MeshContext* ctx = static_cast<MeshContext*>(userCtx);
    LOG_WARN(TAG, "Node %02X:%02X:%02X:%02X:%02X:%02X failed to ACK, initiating self-healing", 
             destMac[0], destMac[1], destMac[2], destMac[3], destMac[4], destMac[5]);
    
    ctx->nodeRegistry->removeByMac(destMac);
    ctx->routeCache->removeByNextHop(destMac);
}

void healthTaskFn(void* param) {
    MeshContext* ctx = static_cast<MeshContext*>(param);

    LOG_INFO(TAG, "Health task started (LOW priority)");

    while (ctx->running) {
        int64_t now = millis();

        // 1. Prune stale nodes
        int pruned = ctx->nodeRegistry->pruneStale(now, ctx->config->nodeTimeoutMs);
        if (pruned > 0) {
            LOG_INFO(TAG, "Pruned %d stale nodes", pruned);
        }

        // 2. Process ACK retries and handle dead links
        ctx->ackManager->processRetries(now, ackRetryCb, ackFailureCb, ctx);

        // 3. Prune stale fragment assemblies
        ctx->fragmentManager->pruneStale(now);

        // 4. Prune stale route cache entries
        ctx->routeCache->pruneStale(now);

        vTaskDelay(pdMS_TO_TICKS(1000));  // check every second
    }

    LOG_INFO(TAG, "Health task stopped");
    vTaskDelete(nullptr);
}

} // namespace mesh
