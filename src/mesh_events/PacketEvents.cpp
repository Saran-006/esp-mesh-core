#include "mesh_events/PacketEvents.hpp"
#include "mesh_core/Logger.hpp"

static const char* TAG = "PktEvt";

namespace mesh {

void onPacketReceived(MeshContext* ctx, const Packet& pkt, const uint8_t senderMac[6]) {
    LOG_INFO(TAG, "PKT_RECV from %02X:%02X:%02X:%02X:%02X:%02X, %d bytes",
             senderMac[0], senderMac[1], senderMac[2],
             senderMac[3], senderMac[4], senderMac[5],
             pkt.header.payload_size);

    MeshEvent evt;
    evt.type = MeshEventType::PACKET_RECEIVED;
    memcpy(&evt.packetData.packet, &pkt, sizeof(Packet));
    memcpy(evt.packetData.sender_mac, senderMac, 6);
    ctx->eventBus->post(evt, 0);
}

void onPacketSent(MeshContext* ctx, const Packet& pkt) {
    MeshEvent evt;
    evt.type = MeshEventType::PACKET_SENT;
    memcpy(&evt.packetData.packet, &pkt, sizeof(Packet));
    ctx->eventBus->post(evt, 0);
}

void onPacketDropped(MeshContext* ctx, const Packet& pkt, const char* reason) {
    LOG_WARN(TAG, "PKT_DROP: %s", reason);

    MeshEvent evt;
    evt.type = MeshEventType::PACKET_DROPPED;
    memcpy(&evt.packetData.packet, &pkt, sizeof(Packet));
    ctx->eventBus->post(evt, 0);
}

} // namespace mesh
