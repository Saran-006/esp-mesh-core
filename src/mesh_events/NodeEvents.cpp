#include "mesh_events/NodeEvents.hpp"
#include "mesh_core/Logger.hpp"

static const char* TAG = "NodeEvt";

namespace mesh {

void onNodeDiscovered(MeshContext* ctx, const Node& node) {
    LOG_INFO(TAG, "NODE_DISCOVERED: %02X:%02X:%02X:%02X:%02X:%02X (lat=%.4f lon=%.4f)",
             node.mac[0], node.mac[1], node.mac[2],
             node.mac[3], node.mac[4], node.mac[5],
             node.lat, node.lon);

    MeshEvent evt;
    evt.type = MeshEventType::NODE_DISCOVERED;
    evt.nodeData.node = node;
    ctx->eventBus->post(evt, 0);
}

void onNodeLost(MeshContext* ctx, const Node& node) {
    LOG_WARN(TAG, "NODE_LOST: %02X:%02X:%02X:%02X:%02X:%02X",
             node.mac[0], node.mac[1], node.mac[2],
             node.mac[3], node.mac[4], node.mac[5]);

    MeshEvent evt;
    evt.type = MeshEventType::NODE_LOST;
    evt.nodeData.node = node;
    ctx->eventBus->post(evt, 0);
}

void onNodeUpdated(MeshContext* ctx, const Node& node) {
    MeshEvent evt;
    evt.type = MeshEventType::NODE_UPDATED;
    evt.nodeData.node = node;
    ctx->eventBus->post(evt, 0);
}

} // namespace mesh
