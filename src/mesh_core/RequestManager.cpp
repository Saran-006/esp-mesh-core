#include "mesh_core/RequestManager.hpp"
#include "mesh_core/Logger.hpp"
#include <cstring>

static const char* TAG = "ReqMgr";

namespace mesh {

RequestManager::RequestManager() {
    for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
        requests_[i].active = false;
        requests_[i].completed = false;
        requests_[i].sem = nullptr;
    }
    mutex_ = xSemaphoreCreateMutex();
}

RequestManager::~RequestManager() {
    for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
        if (requests_[i].sem) {
            vSemaphoreDelete(requests_[i].sem);
        }
    }
    if (mutex_) vSemaphoreDelete(mutex_);
}

bool RequestManager::registerRequest(const uint8_t packetId[16]) {
    xSemaphoreTake(mutex_, portMAX_DELAY);

    for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
        if (!requests_[i].active) {
            memcpy(requests_[i].packetId, packetId, 16);
            requests_[i].active = true;
            requests_[i].completed = false;
            requests_[i].response.success = false;
            requests_[i].response.payloadLen = 0;

            // Create binary semaphore (starts "taken")
            if (!requests_[i].sem) {
                requests_[i].sem = xSemaphoreCreateBinary();
            }
            // Ensure it's in "taken" state so sendAndWait blocks
            xSemaphoreTake(requests_[i].sem, 0);

            xSemaphoreGive(mutex_);
            LOG_INFO(TAG, "Registered TCP request slot %d", i);
            return true;
        }
    }

    LOG_WARN(TAG, "No free request slots");
    xSemaphoreGive(mutex_);
    return false;
}

MeshResponse RequestManager::sendAndWait(const uint8_t packetId[16], int timeoutMs) {
    MeshResponse result;
    result.success = false;
    result.payloadLen = 0;

    // Find the registered request
    xSemaphoreTake(mutex_, portMAX_DELAY);
    PendingRequest* req = findByPacketId(packetId);
    if (!req || !req->sem) {
        LOG_ERROR(TAG, "sendAndWait: request not registered");
        xSemaphoreGive(mutex_);
        return result;
    }
    SemaphoreHandle_t sem = req->sem;
    xSemaphoreGive(mutex_);

    // Block calling task until response arrives or timeout
    LOG_INFO(TAG, "Blocking for TCP response (timeout: %d ms)", timeoutMs);
    BaseType_t got = xSemaphoreTake(sem, pdMS_TO_TICKS(timeoutMs));

    xSemaphoreTake(mutex_, portMAX_DELAY);
    req = findByPacketId(packetId);
    if (req && got == pdTRUE && req->completed) {
        result = req->response;
        LOG_INFO(TAG, "TCP response received, payload %d bytes", result.payloadLen);
    } else {
        LOG_WARN(TAG, "TCP request timed out");
        result.success = false;
    }

    // Clean up slot
    if (req) {
        req->active = false;
        req->completed = false;
    }
    xSemaphoreGive(mutex_);

    return result;
}

bool RequestManager::onResponseReceived(const uint8_t requestId[16], const Packet& responsePkt) {
    xSemaphoreTake(mutex_, portMAX_DELAY);

    PendingRequest* req = findByPacketId(requestId);
    if (!req) {
        xSemaphoreGive(mutex_);
        return false; // no one waiting for this
    }

    // Fill response
    req->response.success = true;
    memcpy(&req->response.responsePacket, &responsePkt, sizeof(Packet));
    req->response.payloadLen = responsePkt.header.payload_size;
    req->completed = true;

    // Unblock the waiting task
    SemaphoreHandle_t sem = req->sem;
    xSemaphoreGive(mutex_);

    xSemaphoreGive(sem);  // signal the blocked caller
    LOG_INFO(TAG, "Unblocked TCP caller");
    return true;
}

void RequestManager::cancelRequest(const uint8_t packetId[16]) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    PendingRequest* req = findByPacketId(packetId);
    if (req) {
        req->active = false;
        req->completed = false;
        // Unblock if someone is waiting
        if (req->sem) xSemaphoreGive(req->sem);
    }
    xSemaphoreGive(mutex_);
}

RequestManager::PendingRequest* RequestManager::findByPacketId(const uint8_t packetId[16]) {
    for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
        if (requests_[i].active && memcmp(requests_[i].packetId, packetId, 16) == 0) {
            return &requests_[i];
        }
    }
    return nullptr;
}

} // namespace mesh
