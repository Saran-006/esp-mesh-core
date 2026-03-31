#include "mesh_core/MeshContext.hpp"
#include "mesh_core/Logger.hpp"

static const char* TAG = "QueueMgr";

namespace mesh {

bool enqueueOutgoing(MeshContext* ctx, const Packet& pkt) {
    QueueHandle_t targetQueue = nullptr;
    const char* queueName = nullptr;

    Priority p = static_cast<Priority>(pkt.header.priority);
    switch (p) {
        case Priority::PRIO_HIGH:
            targetQueue = ctx->outgoingQueueHigh;
            queueName = "HIGH";
            break;
        case Priority::PRIO_MEDIUM:
            targetQueue = ctx->outgoingQueueMed;
            queueName = "MEDIUM";
            break;
        case Priority::PRIO_LOW:
        default:
            targetQueue = ctx->outgoingQueueLow;
            queueName = "LOW";
            break;
    }

    if (!targetQueue) {
        LOG_ERROR(TAG, "Outgoing queue %s is null", queueName);
        return false;
    }

    // Try to enqueue
    BaseType_t ret = xQueueSend(targetQueue, &pkt, 0);
    if (ret != pdTRUE) {
        // Queue full — for LOW priority, silently drop
        if (p == Priority::PRIO_LOW) {
            LOG_WARN(TAG, "LOW priority queue full, dropping packet");
        } else if (p == Priority::PRIO_MEDIUM) {
            // Try dropping a LOW priority packet to make room
            Packet discarded;
            if (ctx->outgoingQueueLow &&
                xQueueReceive(ctx->outgoingQueueLow, &discarded, 0) == pdTRUE) {
                LOG_WARN(TAG, "Dropped LOW pkt to make room for MEDIUM");
            }
            // Still try to enqueue to own queue
            ret = xQueueSend(targetQueue, &pkt, 0);
            if (ret != pdTRUE) {
                LOG_WARN(TAG, "MEDIUM priority queue still full, dropping");
                return false;
            }
        } else {
            // HIGH priority: try dropping LOW first, then MEDIUM
            Packet discarded;
            if (ctx->outgoingQueueLow &&
                xQueueReceive(ctx->outgoingQueueLow, &discarded, 0) == pdTRUE) {
                LOG_WARN(TAG, "Dropped LOW pkt to make room for HIGH");
            } else if (ctx->outgoingQueueMed &&
                       xQueueReceive(ctx->outgoingQueueMed, &discarded, 0) == pdTRUE) {
                LOG_WARN(TAG, "Dropped MEDIUM pkt to make room for HIGH");
            }
            ret = xQueueSend(targetQueue, &pkt, pdMS_TO_TICKS(10));
            if (ret != pdTRUE) {
                LOG_ERROR(TAG, "HIGH priority queue full! Dropping critical packet");
                return false;
            }
        }
        return ret == pdTRUE;
    }

    return true;
}

} // namespace mesh
