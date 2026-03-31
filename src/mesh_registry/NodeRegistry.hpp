#pragma once

#include "mesh_core/Node.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstdint>

namespace mesh {

static constexpr int MAX_NODES = 64;

class NodeRegistry {
public:
    NodeRegistry();
    ~NodeRegistry();

    // Add or update a node. Returns true if new.
    bool upsert(const Node& node);

    // Find a node by hash. Returns pointer or nullptr.
    const Node* findByHash(const uint8_t hash[16]) const;

    // Find a node by MAC. Returns pointer or nullptr.
    const Node* findByMac(const uint8_t mac[6]) const;

    // Remove nodes not seen within timeoutMs. Returns count removed.
    int pruneStale(int64_t nowMs, int timeoutMs);

    // Get number of known nodes
    int count() const;

    // Iterate: fills outArr up to maxOut, returns count
    int getAll(Node* outArr, int maxOut) const;

private:
    Node              nodes_[MAX_NODES];
    int               count_;
    SemaphoreHandle_t mutex_;
};

} // namespace mesh
