#include "mesh_reliability/AckManager.hpp"
#include "mesh_core/Logger.hpp"
#include <cstring>
#include <Arduino.h>

static const char* TAG = "AckMgr";

namespace mesh {

AckManager::AckManager(int maxRetries, int backoffMs)
    : maxRetries_(maxRetries), backoffMs_(backoffMs) {
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        entries_[i].active = false;
    }
    mutex_ = xSemaphoreCreateMutex();
}

AckManager::~AckManager() {
    if (mutex_) vSemaphoreDelete(mutex_);
}

void AckManager::trackPacket(const Packet& pkt, const uint8_t destMac[6]) {
    xSemaphoreTake(mutex_, portMAX_DELAY);

    // Check if we are already tracking this packet_id (idempotency for retries)
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (entries_[i].active &&
            memcmp(entries_[i].pkt.header.packet_id, pkt.header.packet_id, 16) == 0) {
            entries_[i].sentAtMs = millis();
            memcpy(entries_[i].destMac, destMac, 6);
            xSemaphoreGive(mutex_);
            return;
        }
    }

    // Find free slot
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (!entries_[i].active) {
            memcpy(&entries_[i].pkt, &pkt, sizeof(Packet));
            memcpy(entries_[i].destMac, destMac, 6);
            entries_[i].sentAtMs = millis();
            entries_[i].retryCount = 0;
            entries_[i].active = true;
            LOG_DEBUG(TAG, "Tracking packet for ACK (slot %d)", i);
            xSemaphoreGive(mutex_);
            return;
        }
    }

    LOG_WARN(TAG, "ACK tracking table full, cannot track");
    xSemaphoreGive(mutex_);
}

void AckManager::onAckReceived(const uint8_t packetId[16]) {
    xSemaphoreTake(mutex_, portMAX_DELAY);

    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (entries_[i].active &&
            memcmp(entries_[i].pkt.header.packet_id, packetId, 16) == 0) {
            entries_[i].active = false;
            LOG_DEBUG(TAG, "ACK matched, cleared slot %d", i);
            xSemaphoreGive(mutex_);
            return;
        }
    }

    LOG_WARN(TAG, "ACK for unknown packet_id");
    xSemaphoreGive(mutex_);
}

void AckManager::processRetries(int64_t nowMs, RetryCb rcb, FailureCb fcb, void* userCtx) {
    if (!rcb) return;

    struct RetryJob {
        Packet pkt;
        uint8_t destMac[6];
        bool failure;
    };
    
    RetryJob* jobs = new RetryJob[MAX_PENDING_ACKS];
    int jobCount = 0;

    xSemaphoreTake(mutex_, portMAX_DELAY);
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (!entries_[i].active) continue;

        int64_t elapsed = nowMs - entries_[i].sentAtMs;
        int nextRetryMs = backoffMs_ * (entries_[i].retryCount + 1);

        if (elapsed >= nextRetryMs) {
            if (entries_[i].retryCount >= maxRetries_) {
                LOG_WARN(TAG, "Max retries reached, giving up on slot %d", i);
                entries_[i].active = false;
                
                memcpy(jobs[jobCount].destMac, entries_[i].destMac, 6);
                jobs[jobCount].failure = true;
                jobCount++;
                continue;
            }

            entries_[i].retryCount++;
            entries_[i].sentAtMs = nowMs;
            LOG_DEBUG(TAG, "Retry %d/%d for slot %d", entries_[i].retryCount, maxRetries_, i);

            memcpy(&jobs[jobCount].pkt, &entries_[i].pkt, sizeof(Packet));
            memcpy(jobs[jobCount].destMac, entries_[i].destMac, 6);
            jobs[jobCount].failure = false;
            jobCount++;
        }
    }
    xSemaphoreGive(mutex_);

    for (int i = 0; i < jobCount; i++) {
        if (jobs[i].failure) {
            if (fcb) fcb(jobs[i].destMac, userCtx);
        } else {
            rcb(jobs[i].pkt, jobs[i].destMac, userCtx);
        }
    }
    
    delete[] jobs;
}

bool AckManager::isPending(const uint8_t packetId[16]) const {
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (entries_[i].active &&
            memcmp(entries_[i].pkt.header.packet_id, packetId, 16) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace mesh
