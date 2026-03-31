#include "mesh_tasks/LocationTask.hpp"
#include "mesh_core/Logger.hpp"
#include "mesh_core/Packet.hpp"
#include "mesh_events/LocationEvents.hpp"
#include "mesh_utils/UUID.hpp"
#include <cstring>
#include <Arduino.h>

static const char* TAG = "LocTask";

namespace mesh {

void locationTaskFn(void* param) {
    MeshContext* ctx = static_cast<MeshContext*>(param);

    LOG_INFO(TAG, "Location task started (LOW priority)");

    bool hadFix = false;

    while (ctx->running) {
        if (ctx->locationProvider) {
            ctx->locationProvider->update();

            if (ctx->locationProvider->hasValidFix()) {
                float lat = ctx->locationProvider->getLatitude();
                float lon = ctx->locationProvider->getLongitude();

                // Update self node
                ctx->selfNode->lat = lat;
                ctx->selfNode->lon = lon;

                if (!hadFix) {
                    LOG_INFO(TAG, "GPS fix acquired: %.6f, %.6f", lat, lon);
                    hadFix = true;
                }

                onLocationUpdated(ctx, lat, lon);

                // Broadcast location update to peers
                Packet locPkt;
                memset(&locPkt, 0, sizeof(locPkt));
                locPkt.header.version = 1;
                locPkt.header.ttl = 2;  // 2 hops for location updates
                locPkt.header.flags = FLAG_CONTROL | FLAG_ROUTE_RECORD;
                locPkt.header.priority = static_cast<uint8_t>(Priority::PRIO_MEDIUM);

                UUID::generate(locPkt.header.packet_id);
                memcpy(locPkt.header.source_hash, ctx->selfNode->node_hash, 16);
                memset(locPkt.header.dest_hash, 0, 16); // broadcast

                locPkt.payload[0] = CTRL_LOCATION_UPDATE;
                memcpy(locPkt.payload + 1, &lat, 4);
                memcpy(locPkt.payload + 5, &lon, 4);
                locPkt.header.payload_size = 9;

                enqueueOutgoing(ctx, locPkt);
            } else {
                if (hadFix) {
                    LOG_WARN(TAG, "GPS fix lost");
                    hadFix = false;
                    onLocationLost(ctx);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));  // update every 2 seconds
    }

    LOG_INFO(TAG, "Location task stopped");
    vTaskDelete(nullptr);
}

} // namespace mesh
