#include "mesh_registry/DedupCache.hpp"
#include <cstring>
#include <cstdlib>

namespace mesh {

DedupCache::DedupCache(int capacity) : capacity_(capacity), head_(0) {
    ring_ = static_cast<Entry*>(calloc(capacity_, sizeof(Entry)));
    for (int i = 0; i < capacity_; i++) {
        ring_[i].used = false;
    }
    mutex_ = xSemaphoreCreateMutex();
}

DedupCache::~DedupCache() {
    if (ring_)  free(ring_);
    if (mutex_) vSemaphoreDelete(mutex_);
}

bool DedupCache::isDuplicate(const uint8_t packetId[16]) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    for (int i = 0; i < capacity_; i++) {
        if (ring_[i].used && memcmp(ring_[i].id, packetId, 16) == 0) {
            xSemaphoreGive(mutex_);
            return true;
        }
    }
    xSemaphoreGive(mutex_);
    return false;
}

void DedupCache::insert(const uint8_t packetId[16]) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    memcpy(ring_[head_].id, packetId, 16);
    ring_[head_].used = true;
    head_ = (head_ + 1) % capacity_;
    xSemaphoreGive(mutex_);
}

bool DedupCache::checkAndInsert(const uint8_t packetId[16]) {
    xSemaphoreTake(mutex_, portMAX_DELAY);

    // Check
    for (int i = 0; i < capacity_; i++) {
        if (ring_[i].used && memcmp(ring_[i].id, packetId, 16) == 0) {
            xSemaphoreGive(mutex_);
            return true; // duplicate
        }
    }

    // Insert at head
    memcpy(ring_[head_].id, packetId, 16);
    ring_[head_].used = true;
    head_ = (head_ + 1) % capacity_;

    xSemaphoreGive(mutex_);
    return false; // not duplicate
}

} // namespace mesh
