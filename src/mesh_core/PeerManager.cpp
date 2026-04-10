#include "mesh_core/PeerManager.hpp"
#include "mesh_core/Logger.hpp"
#include <esp_now.h>
#include <cstring>

static const char* TAG = "PeerMgr";

namespace mesh {

PeerManager::PeerManager() : count_(0) {
    for (int i = 0; i < PEER_TABLE_MAX; i++) {
        peers_[i].active = false;
        memset(peers_[i].mac, 0, 6);
    }
    mutex_ = xSemaphoreCreateMutex();
}

PeerManager::~PeerManager() {
    if (mutex_) vSemaphoreDelete(mutex_);
}

bool PeerManager::addPeer(const uint8_t mac[6]) {
    if (!mutex_) {
        LOG_ERROR(TAG, "addPeer CRITICAL: mutex_ is NULL!");
        return false;
    }
    LOG_INFO(TAG, "addPeer: taking mutex for %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    xSemaphoreTake(mutex_, portMAX_DELAY);

    // Check if already registered
    for (int i = 0; i < PEER_TABLE_MAX; i++) {
        if (peers_[i].active && memcmp(peers_[i].mac, mac, 6) == 0) {
            xSemaphoreGive(mutex_);
            return true; // already exists
        }
    }

    if (count_ >= PEER_TABLE_MAX) {
        LOG_WARN(TAG, "Peer table full, cannot add peer");
        xSemaphoreGive(mutex_);
        return false;
    }

    // Register with ESP-NOW
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;  // use current channel
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(mac)) {
        esp_err_t err = esp_now_add_peer(&peerInfo);
        if (err != ESP_OK) {
            LOG_ERROR(TAG, "esp_now_add_peer failed: %d", err);
            xSemaphoreGive(mutex_);
            return false;
        }
    }

    // Add to our table
    for (int i = 0; i < PEER_TABLE_MAX; i++) {
        if (!peers_[i].active) {
            memcpy(peers_[i].mac, mac, 6);
            peers_[i].active = true;
            count_++;
            LOG_INFO(TAG, "Added peer %02X:%02X:%02X:%02X:%02X:%02X (total: %d)",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], count_);
            break;
        }
    }

    xSemaphoreGive(mutex_);
    return true;
}

bool PeerManager::removePeer(const uint8_t mac[6]) {
    xSemaphoreTake(mutex_, portMAX_DELAY);

    for (int i = 0; i < PEER_TABLE_MAX; i++) {
        if (peers_[i].active && memcmp(peers_[i].mac, mac, 6) == 0) {
            esp_now_del_peer(mac);
            peers_[i].active = false;
            memset(peers_[i].mac, 0, 6);
            count_--;
            LOG_INFO(TAG, "Removed peer %02X:%02X:%02X:%02X:%02X:%02X (total: %d)",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], count_);
            xSemaphoreGive(mutex_);
            return true;
        }
    }

    xSemaphoreGive(mutex_);
    return false;
}

bool PeerManager::hasPeer(const uint8_t mac[6]) const {
    // Note: const method, but FreeRTOS mutex isn't const-friendly.
    // Safe because we only read.
    for (int i = 0; i < PEER_TABLE_MAX; i++) {
        if (peers_[i].active && memcmp(peers_[i].mac, mac, 6) == 0) {
            return true;
        }
    }
    return false;
}

int PeerManager::peerCount() const {
    return count_;
}

} // namespace mesh
