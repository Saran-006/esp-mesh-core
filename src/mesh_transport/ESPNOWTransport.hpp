#pragma once

#include "mesh_core/Packet.hpp"
#include "mesh_core/MeshContext.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <esp_now.h>
#include <esp_idf_version.h>
#include <cstdint>

namespace mesh {

class ESPNOWTransport {
public:
    ESPNOWTransport();
    ~ESPNOWTransport();

    // Initialize WiFi STA + ESP-NOW.  rawQueue receives RawIncoming from callback.
    bool init(QueueHandle_t rawQueue);

    // Send raw bytes to a MAC address
    bool send(const uint8_t destMac[6], const uint8_t* data, size_t len);

    // Get this device's STA MAC address
    void getOwnMac(uint8_t mac[6]) const;

private:
    QueueHandle_t rawQueue_;

    static ESPNOWTransport* instance_;

    // ESP-NOW receive callback (static, forwards to instance)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len);
#else
    static void onRecv(const uint8_t* mac, const uint8_t* data, int len);
#endif

    // ESP-NOW send callback
    static void onSend(const uint8_t* mac, esp_now_send_status_t status);
};

} // namespace mesh
