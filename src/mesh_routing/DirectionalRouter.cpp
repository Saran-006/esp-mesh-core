#include "mesh_routing/DirectionalRouter.hpp"
#include "mesh_core/Logger.hpp"
#include <cmath>
#include <cstring>

static const char* TAG = "Router";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace mesh {

DirectionalRouter::DirectionalRouter(NodeRegistry* registry, float angleThreshold,
                                     float distanceTolerance)
    : registry_(registry), angleThreshold_(angleThreshold),
      distanceTolerance_(distanceTolerance) {}

float DirectionalRouter::distanceM(float lat1, float lon1, float lat2, float lon2) {
    constexpr float R = 6371000.0f; // Earth radius in meters
    float dLat = (lat2 - lat1) * M_PI / 180.0f;
    float dLon = (lon2 - lon1) * M_PI / 180.0f;
    float a = sinf(dLat / 2.0f) * sinf(dLat / 2.0f) +
              cosf(lat1 * M_PI / 180.0f) * cosf(lat2 * M_PI / 180.0f) *
              sinf(dLon / 2.0f) * sinf(dLon / 2.0f);
    float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
    return R * c;
}

float DirectionalRouter::bearing(float lat1, float lon1, float lat2, float lon2) {
    float dLon = (lon2 - lon1) * M_PI / 180.0f;
    float lat1r = lat1 * M_PI / 180.0f;
    float lat2r = lat2 * M_PI / 180.0f;
    float y = sinf(dLon) * cosf(lat2r);
    float x = cosf(lat1r) * sinf(lat2r) - sinf(lat1r) * cosf(lat2r) * cosf(dLon);
    float brng = atan2f(y, x) * 180.0f / M_PI;
    return fmodf(brng + 360.0f, 360.0f);
}

float DirectionalRouter::angleDiff(float a, float b) {
    float diff = fabsf(a - b);
    if (diff > 180.0f) diff = 360.0f - diff;
    return diff;
}

bool DirectionalRouter::selectNextHop(const Packet& pkt, const Node& self,
                                      uint8_t outMac[6]) const {
    // Need destination coordinates
    if (pkt.header.dest_lat == 0.0f && pkt.header.dest_lon == 0.0f) {
        return false;
    }

    // Need self location
    if (!self.hasLocation()) {
        return false;
    }

    // Bearing from self to destination
    float targetBearing = bearing(self.lat, self.lon,
                                  pkt.header.dest_lat, pkt.header.dest_lon);
    float distToDest = distanceM(self.lat, self.lon,
                                 pkt.header.dest_lat, pkt.header.dest_lon);

    Node nodes[MAX_NODES];
    int count = registry_->getAll(nodes, MAX_NODES);

    float bestScore = 1e9f;
    int bestIdx = -1;

    for (int i = 0; i < count; i++) {
        // Skip nodes without location
        if (!nodes[i].hasLocation()) continue;

        // Skip last hop to avoid ping-pong
        if (memcmp(nodes[i].mac, pkt.header.last_hop_mac, 6) == 0) continue;

        // Bearing from self to this peer
        float peerBearing = bearing(self.lat, self.lon, nodes[i].lat, nodes[i].lon);

        // Angle difference between peer direction and destination direction
        float aDiff = angleDiff(peerBearing, targetBearing);

        // Only consider peers within angle threshold
        if (aDiff > angleThreshold_) continue;

        // Distance from peer to destination
        float peerDistToDest = distanceM(nodes[i].lat, nodes[i].lon,
                                         pkt.header.dest_lat, pkt.header.dest_lon);

        // Distance tolerance: peer should be closer to dest than we are (with tolerance)
        if (peerDistToDest > distToDest + distanceTolerance_) continue;

        // Score: lower is better. Prefer peers closer to destination and in-line.
        float score = peerDistToDest + aDiff * 10.0f;

        if (score < bestScore) {
            bestScore = score;
            bestIdx = i;
        }
    }

    if (bestIdx >= 0) {
        memcpy(outMac, nodes[bestIdx].mac, 6);
        LOG_INFO(TAG, "Next hop: %02X:%02X:%02X:%02X:%02X:%02X (score: %.1f)",
                 outMac[0], outMac[1], outMac[2], outMac[3], outMac[4], outMac[5], bestScore);
        return true;
    }

    return false;
}

int DirectionalRouter::getFloodTargets(const Packet& pkt, const Node& self,
                                       uint8_t outMacs[][6], int maxOut) const {
    // If it's a blind broadcast, just flood to everyone
    if (pkt.header.routing_strategy == (uint8_t)RoutingStrategy::STRAT_BROADCAST) {
        Node nodes[MAX_NODES];
        int count = registry_->getAll(nodes, MAX_NODES);
        int idx = 0;
        for (int i = 0; i < count && idx < maxOut; i++) {
            if (memcmp(nodes[i].mac, pkt.header.last_hop_mac, 6) == 0) continue;
            memcpy(outMacs[idx], nodes[i].mac, 6);
            idx++;
        }
        return idx;
    }

    // Otherwise, handle Directional Flood (STRAT_GEO_FLOOD)
    // We only forward to neighbors that make "progress" towards the destination
    if (pkt.header.dest_lat == 0.0f && pkt.header.dest_lon == 0.0f) return 0;
    if (!self.hasLocation()) return 0;

    float targetBearing = bearing(self.lat, self.lon, pkt.header.dest_lat, pkt.header.dest_lon);
    float distToDest = distanceM(self.lat, self.lon, pkt.header.dest_lat, pkt.header.dest_lon);

    Node nodes[MAX_NODES];
    int count = registry_->getAll(nodes, MAX_NODES);
    int idx = 0;

    for (int i = 0; i < count && idx < maxOut; i++) {
        if (!nodes[i].hasLocation()) continue;
        if (memcmp(nodes[i].mac, pkt.header.last_hop_mac, 6) == 0) continue;

        float peerBearing = bearing(self.lat, self.lon, nodes[i].lat, nodes[i].lon);
        float aDiff = angleDiff(peerBearing, targetBearing);

        // Angle filtering: only flood within the directional cone
        if (aDiff > angleThreshold_) continue;

        float peerDistToDest = distanceM(nodes[i].lat, nodes[i].lon, pkt.header.dest_lat, pkt.header.dest_lon);

        // Progress Check: peer MUST be closer to destination than we are
        // This prevents the "border waste" the user mentioned
        if (peerDistToDest > distToDest - distanceTolerance_) continue;

        memcpy(outMacs[idx], nodes[i].mac, 6);
        idx++;
    }

    return idx;
}

} // namespace mesh
