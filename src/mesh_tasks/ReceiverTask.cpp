#include "mesh_tasks/ReceiverTask.hpp"
#include "mesh_core/Logger.hpp"
#include "mesh_core/Packet.hpp"
#include <cstring>

static const char* TAG = "RecvTask";

namespace mesh {

void receiverTaskFn(void* param) {
    MeshContext* ctx = static_cast<MeshContext*>(param);

    LOG_INFO(TAG, "Receiver task started (HIGH priority)");

    while (ctx->running) {
        RawIncoming raw;

        // Block waiting for raw data from ESP-NOW callback
        if (xQueueReceive(ctx->rawIncomingQueue, &raw, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Deserialize: raw bytes → Packet struct
            Packet pkt;
            if (!Packet::deserialize(raw.data, raw.length, pkt)) {
                LOG_WARN(TAG, "Failed to deserialize, dropping %d bytes", raw.length);
                continue;
            }

            // Stamp last_hop_mac from the sender MAC reported by ESP-NOW
            memcpy(pkt.header.last_hop_mac, raw.senderMac, 6);

            // Enqueue to packetQueue for the Dispatcher to process
            // Receiver does NOT process logic — only enqueue.
            if (xQueueSend(ctx->packetQueue, &pkt, pdMS_TO_TICKS(10)) != pdTRUE) {
                LOG_WARN(TAG, "Packet queue full, dropping packet");
            }
        }
    }

    LOG_INFO(TAG, "Receiver task stopped");
    vTaskDelete(nullptr);
}

} // namespace mesh
