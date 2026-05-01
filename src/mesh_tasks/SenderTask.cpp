#include "mesh_tasks/SenderTask.hpp"
#include "mesh_core/Logger.hpp"
#include "mesh_core/Security.hpp"
#include "mesh_core/Packet.hpp"
#include "mesh_transport/ESPNOWTransport.hpp"
#include "mesh_reliability/AckManager.hpp"
#include "mesh_routing/RouteCache.hpp"
#include "mesh_registry/NodeRegistry.hpp"
#include "mesh_routing/DirectionalRouter.hpp"
#include "mesh_events/PacketEvents.hpp"
#include <cstring>
#include <Arduino.h>

static const char* TAG = "SendTask";

namespace mesh {

void senderTaskFn(void* param) {
    MeshContext* ctx = static_cast<MeshContext*>(param);

    LOG_INFO(TAG, "Sender task started (MEDIUM priority)");

    uint8_t wireBuf[ESPNOW_MAX_DATA_LEN];

    while (ctx->running) {
        Packet* pkt = new Packet();
        bool got = false;

        // Priority polling: HIGH → MEDIUM → LOW
        if (xQueueReceive(ctx->outgoingQueueHigh, pkt, 0) == pdTRUE) {
            got = true;
        } else if (xQueueReceive(ctx->outgoingQueueMed, pkt, 0) == pdTRUE) {
            got = true;
        } else if (xQueueReceive(ctx->outgoingQueueLow, pkt, pdMS_TO_TICKS(50)) == pdTRUE) {
            got = true;
        }

        if (!got) {
            delete pkt;
            continue;
        }

        // Fill our MAC as last_hop
        uint8_t ourMac[6];
        ctx->transport->getOwnMac(ourMac);
        memcpy(pkt->header.last_hop_mac, ourMac, 6);

        if (ctx->config->networkKeyLen > 0) {
            Security::signPacket(*pkt, ctx->config->networkKey, ctx->config->networkKeyLen);
        }

        // Determine destination MACs based on routing strategy
        uint8_t strategy = pkt->header.routing_strategy;
        
        if (strategy == (uint8_t)RoutingStrategy::STRAT_DIRECT) {
            uint8_t destMac[6];
            bool found = false;

            // 1. Try cached route
            if (ctx_->routeCache->lookupNextHop(pkt->header.dest_hash, destMac)) {
                found = true;
            } else {
                // 2. Try looking up in registry
                const Node* destNode = ctx_->nodeRegistry->findByHash(pkt->header.dest_hash);
                if (destNode) {
                    memcpy(destMac, destNode->mac, 6);
                    found = true;
                }
            }

            if (found) {
                ctx_->peerManager->addPeer(destMac);
                size_t written = pkt->serialize(wireBuf, sizeof(wireBuf));
                if (written > 0 && ctx_->transport->send(destMac, wireBuf, written)) {
                    onPacketSent(ctx, *pkt);
                    if (pkt->isAckRequired()) ctx_->ackManager->trackPacket(*pkt, destMac);
                }
            } else {
                LOG_WARN(TAG, "Direct routing requested but no path found");
            }
        } else {
            // STRAT_GEO_FLOOD or STRAT_BROADCAST
            uint8_t floodMacs[MAX_NODES][6];
            int count = ctx_->router->getFloodTargets(*pkt, *ctx_->selfNode, floodMacs, MAX_NODES);
            
            if (count == 0 && strategy == (uint8_t)RoutingStrategy::STRAT_BROADCAST) {
                // Last resort blind broadcast if registry is empty
                uint8_t bMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                ctx_->peerManager->addPeer(bMac);
                size_t written = pkt->serialize(wireBuf, sizeof(wireBuf));
                if (written > 0) ctx_->transport->send(bMac, wireBuf, written);
            } else {
                for (int i = 0; i < count; i++) {
                    ctx_->peerManager->addPeer(floodMacs[i]);
                    size_t written = pkt->serialize(wireBuf, sizeof(wireBuf));
                    if (written > 0) ctx_->transport->send(floodMacs[i], wireBuf, written);
                }
                LOG_INFO(TAG, "Flood sent to %d peers (Strategy: %d)", count, strategy);
                if (pkt->isAckRequired() && count > 0) {
                    ctx_->ackManager->trackPacket(*pkt, floodMacs[0]);
                }
            }
            onPacketSent(ctx, *pkt);
        }
        delete pkt;
    }

    LOG_INFO(TAG, "Sender task stopped");
    vTaskDelete(nullptr);
}

} // namespace mesh
