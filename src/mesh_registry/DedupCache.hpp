#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstdint>

namespace mesh {

class DedupCache {
public:
    explicit DedupCache(int capacity = 512);
    ~DedupCache();

    // Returns true if this packet_id was already seen (duplicate)
    bool isDuplicate(const uint8_t packetId[16]);

    // Insert a packet_id into the cache
    void insert(const uint8_t packetId[16]);

    // Check + insert atomically. Returns true if duplicate.
    bool checkAndInsert(const uint8_t packetId[16]);

private:
    struct Entry {
        uint8_t id[16];
        bool    used;
    };
    Entry*            ring_;
    int               capacity_;
    int               head_;
    SemaphoreHandle_t mutex_;
};

} // namespace mesh
