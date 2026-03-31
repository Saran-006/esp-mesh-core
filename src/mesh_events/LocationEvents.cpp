#include "mesh_events/LocationEvents.hpp"
#include "mesh_core/Logger.hpp"

static const char* TAG = "LocEvt";

namespace mesh {

void onLocationUpdated(MeshContext* ctx, float lat, float lon) {
    MeshEvent evt;
    evt.type = MeshEventType::LOCATION_UPDATED;
    evt.locationData.lat = lat;
    evt.locationData.lon = lon;
    evt.locationData.valid = true;
    ctx->eventBus->post(evt, 0);
}

void onLocationLost(MeshContext* ctx) {
    LOG_WARN(TAG, "Location fix lost");

    MeshEvent evt;
    evt.type = MeshEventType::LOCATION_LOST;
    evt.locationData.lat = 0;
    evt.locationData.lon = 0;
    evt.locationData.valid = false;
    ctx->eventBus->post(evt, 0);
}

} // namespace mesh
