#include "mesh_events/ServiceEvents.hpp"
#include "mesh_core/Logger.hpp"

static const char* TAG = "SvcEvt";

namespace mesh {

void onServiceRegistered(MeshContext* ctx, uint8_t serviceId) {
    LOG_INFO(TAG, "Service registered: %d", serviceId);

    MeshEvent evt;
    evt.type = MeshEventType::SERVICE_REGISTERED;
    evt.serviceData.service_id = serviceId;
    ctx->eventBus->post(evt, 0);
}

void onServiceUnregistered(MeshContext* ctx, uint8_t serviceId) {
    LOG_INFO(TAG, "Service unregistered: %d", serviceId);

    MeshEvent evt;
    evt.type = MeshEventType::SERVICE_UNREGISTERED;
    evt.serviceData.service_id = serviceId;
    ctx->eventBus->post(evt, 0);
}

} // namespace mesh
