#include "mesh_routing/RouteCache.hpp"
#include "mesh_core/Logger.hpp"
#include <Arduino.h>

namespace mesh {

RouteCache::RouteCache() {
    for (int i = 0; i < ROUTE_CACHE_SIZE; i++) {
        entries_[i].active = false;
    }
    mutex_ = xSemaphoreCreateMutex();
}

RouteCache::~RouteCache() {
    if (mutex_) vSemaphoreDelete(mutex_);
}

void RouteCache::recordRoute(const uint8_t sourceHash[16], const uint8_t receivedFromMac[6]) {
    xSemaphoreTake(mutex_, portMAX_DELAY);

    // Update existing entry if present
    for (int i = 0; i < ROUTE_CACHE_SIZE; i++) {
        if (entries_[i].active && memcmp(entries_[i].destHash, sourceHash, 16) == 0) {
            memcpy(entries_[i].nextHopMac, receivedFromMac, 6);
            entries_[i].recordedAtMs = millis();
            xSemaphoreGive(mutex_);
            return;
        }
    }

    // Insert new — find empty slot or evict oldest
    int slot = -1;
    int64_t oldest = INT64_MAX;
    int oldestIdx = 0;

    for (int i = 0; i < ROUTE_CACHE_SIZE; i++) {
        if (!entries_[i].active) {
            slot = i;
            break;
        }
        if (entries_[i].recordedAtMs < oldest) {
            oldest = entries_[i].recordedAtMs;
            oldestIdx = i;
        }
    }

    if (slot < 0) slot = oldestIdx; // evict oldest

    memcpy(entries_[slot].destHash, sourceHash, 16);
    memcpy(entries_[slot].nextHopMac, receivedFromMac, 6);
    entries_[slot].recordedAtMs = millis();
    entries_[slot].active = true;

    xSemaphoreGive(mutex_);
}

bool RouteCache::lookupNextHop(const uint8_t destHash[16], uint8_t outMac[6]) const {
    if (!mutex_) {
        LOG_ERROR("RouteCache", "CRITICAL: mutex_ is NULL");
        return false;
    }
    LOG_INFO("RouteCache", "Taking mutex for lookup");
    xSemaphoreTake(mutex_, portMAX_DELAY);

    for (int i = 0; i < ROUTE_CACHE_SIZE; i++) {
        if (entries_[i].active && memcmp(entries_[i].destHash, destHash, 16) == 0) {
            memcpy(outMac, entries_[i].nextHopMac, 6);
            xSemaphoreGive(mutex_);
            return true;
        }
    }

    xSemaphoreGive(mutex_);
    return false;
}

void RouteCache::pruneStale(int64_t nowMs) {
    xSemaphoreTake(mutex_, portMAX_DELAY);

    for (int i = 0; i < ROUTE_CACHE_SIZE; i++) {
        if (entries_[i].active &&
            (nowMs - entries_[i].recordedAtMs) > ROUTE_ENTRY_TTL_MS) {
            entries_[i].active = false;
        }
    }

    xSemaphoreGive(mutex_);
}

} // namespace mesh
