#include "mesh_core/EventBus.hpp"
#include "mesh_core/Logger.hpp"
#include <cstring>

static const char* TAG = "EventBus";

namespace mesh {

EventBus::EventBus() : queue_(nullptr), subCount_(0), subMutex_(nullptr) {
    for (int i = 0; i < MAX_EVENT_HANDLERS; i++) {
        subs_[i].active = false;
    }
}

EventBus::~EventBus() {
    if (queue_)    vQueueDelete(queue_);
    if (subMutex_) vSemaphoreDelete(subMutex_);
}

bool EventBus::init(int queueSize) {
    queue_    = xQueueCreate(queueSize, sizeof(MeshEvent));
    subMutex_ = xSemaphoreCreateMutex();
    if (!queue_ || !subMutex_) {
        LOG_ERROR(TAG, "Failed to create event queue or mutex");
        return false;
    }
    return true;
}

bool EventBus::subscribe(MeshEventType type, EventHandler handler, void* userCtx) {
    xSemaphoreTake(subMutex_, portMAX_DELAY);

    if (subCount_ >= MAX_EVENT_HANDLERS) {
        LOG_WARN(TAG, "Max event handlers reached");
        xSemaphoreGive(subMutex_);
        return false;
    }

    for (int i = 0; i < MAX_EVENT_HANDLERS; i++) {
        if (!subs_[i].active) {
            subs_[i].type    = type;
            subs_[i].handler = handler;
            subs_[i].userCtx = userCtx;
            subs_[i].active  = true;
            subCount_++;
            xSemaphoreGive(subMutex_);
            return true;
        }
    }

    xSemaphoreGive(subMutex_);
    return false;
}

bool EventBus::post(const MeshEvent& event, TickType_t timeout) {
    if (!queue_) return false;
    BaseType_t ret = xQueueSend(queue_, &event, timeout);
    if (ret != pdTRUE) {
        LOG_WARN(TAG, "Event queue full, dropping event type %d", (int)event.type);
        return false;
    }
    return true;
}

bool EventBus::processOne(TickType_t timeout) {
    if (!queue_) return false;
    MeshEvent event;
    if (xQueueReceive(queue_, &event, timeout) != pdTRUE) {
        return false;
    }

    // Handlers are loaded at boot time. No lock needed for iteration.
    for (int i = 0; i < MAX_EVENT_HANDLERS; i++) {
        if (subs_[i].active && subs_[i].type == event.type) {
            subs_[i].handler(event, subs_[i].userCtx);
        }
    }
    return true;
}

} // namespace mesh
