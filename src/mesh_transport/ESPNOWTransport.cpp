#include "mesh_transport/ESPNOWTransport.hpp"
#include "mesh_core/Logger.hpp"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

static const char* TAG = "Transport";

namespace mesh {

ESPNOWTransport* ESPNOWTransport::instance_ = nullptr;

ESPNOWTransport::ESPNOWTransport() : rawQueue_(nullptr) {
    instance_ = this;
}

ESPNOWTransport::~ESPNOWTransport() {
    esp_now_deinit();
    instance_ = nullptr;
}

bool ESPNOWTransport::init(QueueHandle_t rawQueue) {
    rawQueue_ = rawQueue;

    // Initialize WiFi in STA mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Optionally set long-range mode for better range
    // esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);

    LOG_INFO(TAG, "WiFi STA initialized");

    // Initialize ESP-NOW
    esp_err_t err = esp_now_init();
    if (err != ESP_OK) {
        LOG_ERROR(TAG, "esp_now_init failed: %d", err);
        return false;
    }

    // Register callbacks
    esp_now_register_recv_cb(onRecv);
    esp_now_register_send_cb(reinterpret_cast<esp_now_send_cb_t>(onSend));

    uint8_t mac[6];
    getOwnMac(mac);
    LOG_INFO(TAG, "ESP-NOW initialized. MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return true;
}

bool ESPNOWTransport::send(const uint8_t destMac[6], const uint8_t* data, size_t len) {
    if (len > ESPNOW_MAX_DATA_LEN) {
        LOG_ERROR(TAG, "Send data too large: %d > %d", (int)len, (int)ESPNOW_MAX_DATA_LEN);
        return false;
    }

    esp_err_t err = esp_now_send(destMac, data, len);
    if (err != ESP_OK) {
        LOG_ERROR(TAG, "esp_now_send failed: %d", err);
        return false;
    }
    return true;
}

void ESPNOWTransport::getOwnMac(uint8_t mac[6]) const {
    esp_wifi_get_mac(WIFI_IF_STA, mac);
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void ESPNOWTransport::onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    if (!instance_ || !instance_->rawQueue_) return;
    const uint8_t* mac = info->src_addr;
#else
void ESPNOWTransport::onRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (!instance_ || !instance_->rawQueue_) return;
#endif

    if (len <= 0 || len > (int)ESPNOW_MAX_DATA_LEN) return;

    RawIncoming raw;
    memcpy(raw.senderMac, mac, 6);
    memcpy(raw.data, data, len);
    raw.length = len;

    // Enqueue from ISR/callback context — use FromISR variant
    BaseType_t higherWoken = pdFALSE;
    xQueueSendFromISR(instance_->rawQueue_, &raw, &higherWoken);
    if (higherWoken) {
        portYIELD_FROM_ISR();
    }
}

void ESPNOWTransport::onSend(const uint8_t* mac, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        LOG_WARN(TAG, "Send to %02X:%02X:%02X:%02X:%02X:%02X failed (PHY)",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

} // namespace mesh
