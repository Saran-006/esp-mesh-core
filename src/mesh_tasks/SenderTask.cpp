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

        // Determine destination MAC
        uint8_t destMac[6];
        bool foundDest = false;

        uint8_t zeros[16] = {};
        bool isBroadcast = (memcmp(pkt->header.dest_hash, zeros, 16) == 0);

        if (isBroadcast) {
            uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            memcpy(destMac, broadcastMac, 6);
            foundDest = true;
        } else {
            // 2. Try cached route (takes Mutex)
            if (ctx->routeCache->lookupNextHop(pkt->header.dest_hash, destMac)) {
                foundDest = true;
            }

            // 3. Try looking up dest_hash in registry
            if (!foundDest) {
                const Node* destNode = ctx->nodeRegistry->findByHash(pkt->header.dest_hash);
                if (destNode) {
                    memcpy(destMac, destNode->mac, 6);
                    foundDest = true;
                }
            }
        }

        if (!foundDest) {
            // 4. Flood: send to all known peers
            Node* nodes = new Node[MAX_NODES];
            int count = ctx->nodeRegistry->getAll(nodes, MAX_NODES);
            if (count == 0) {
                // Send to broadcast as last resort
                uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                memcpy(destMac, broadcastMac, 6);
                foundDest = true;
            } else {
                for (int i = 0; i < count; i++) {
                    if (memcmp(nodes[i].mac, ourMac, 6) == 0) continue;

                    ctx->peerManager->addPeer(nodes[i].mac);
                    size_t written = pkt->serialize(wireBuf, sizeof(wireBuf));
                    if (written > 0) {
                        ctx->transport->send(nodes[i].mac, wireBuf, written);
                    }
                }

                if (pkt->isAckRequired() && count > 0) {
                    ctx->ackManager->trackPacket(*pkt, nodes[0].mac);
                }

                onPacketSent(ctx, *pkt);
                delete[] nodes;
                delete pkt;
                continue;
            }
            delete[] nodes;
        }

        // Single destination send (Broadcast MAC or Registered Peer)
        ctx->peerManager->addPeer(destMac);
        size_t written = pkt->serialize(wireBuf, sizeof(wireBuf));
        if (written > 0) {
            if (ctx->transport->send(destMac, wireBuf, written)) {
                onPacketSent(ctx, *pkt);
                if (pkt->isAckRequired()) {
                    ctx->ackManager->trackPacket(*pkt, destMac);
                }
            }
        }
        delete pkt;
    }

    LOG_INFO(TAG, "Sender task stopped");
    vTaskDelete(nullptr);
}

} // namespace mesh
