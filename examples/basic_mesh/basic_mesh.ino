/*
 * MeshFramework — Basic Mesh Example
 *
 * This sketch demonstrates:
 *   1. Creating and configuring a Mesh instance
 *   2. Attaching a GPS provider (or manual location)
 *   3. Starting the mesh
 *   4. Subscribing to events
 *   5. Sending data in UDP or TCP mode
 *
 * HARDWARE:
 *   - ESP32 board
 *   - Optional: NEO-6M GPS on UART1 (RX=GPIO16, TX=GPIO17)
 *
 * LIBRARY DEPENDENCIES:
 *   - MeshFramework (this library)
 *   - TinyGPSPlus (install via Library Manager)
 *
 * SETUP:
 *   1. Copy the mesh_esp32_cpp folder into Arduino/libraries/
 *   2. Install TinyGPSPlus from Library Manager
 *   3. Select your ESP32 board in Tools > Board
 *   4. Upload this sketch
 */

#include <MeshFramework.hpp>

// ---- Configuration ----
static const uint8_t NETWORK_KEY[] = "MySecretMeshKey!";  // 16-byte shared key

// ---- GPS pins (NEO-6M) ----
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// ---- Global instances ----
mesh::Mesh meshNode;

// Use GPS for outdoor, or ManualLocationProvider for testing
// mesh::GPSLocationProvider gps(GPS_RX_PIN, GPS_TX_PIN, 9600, 1);
mesh::ManualLocationProvider location(12.9716f, 77.5946f);  // Bangalore coords for testing

// ---- Event callbacks ----
void onNodeFound(const mesh::MeshEvent& evt, void* ctx) {
    const mesh::Node& node = evt.nodeData.node;
    Serial.printf("[APP] New node found: %02X:%02X:%02X:%02X:%02X:%02X at (%.4f, %.4f)\n",
                  node.mac[0], node.mac[1], node.mac[2],
                  node.mac[3], node.mac[4], node.mac[5],
                  node.lat, node.lon);
}

void onDataReceived(const mesh::MeshEvent& evt, void* ctx) {
    const mesh::Packet& pkt = evt.packetData.packet;
    const uint8_t* sender = pkt.header.source_hash;
    
    Serial.printf("\n[APP] <<< MESSAGE RECEIVED <<<\n");
    Serial.printf("[APP] From Node Hash: %02X%02X%02X%02X...\n", 
                  sender[0], sender[1], sender[2], sender[3]);
    Serial.printf("[APP] Size: %d bytes\n", pkt.header.payload_size);

    // Print payload as string if it looks like text
    if (pkt.header.payload_size > 0 && pkt.header.payload_size < 200) {
        char buf[201];
        memcpy(buf, pkt.payload, pkt.header.payload_size);
        buf[pkt.header.payload_size] = '\0';
        Serial.printf("[APP] Payload: %s\n", buf);
    }

    // If this is a TCP request, send a response back
    if (pkt.header.flags & mesh::FLAG_TCP_REQUEST) {
        Serial.println("[APP] TCP request received, sending response...");

        // Build response: FLAG_TCP_RESPONSE with original packet_id in first 16 bytes
        mesh::Packet resp;
        memset(&resp, 0, sizeof(resp));
        resp.header.version = 1;
        resp.header.ttl = 10;
        resp.header.flags = mesh::FLAG_DATA | mesh::FLAG_TCP_RESPONSE | mesh::FLAG_ROUTE_RECORD;
        resp.header.priority = static_cast<uint8_t>(mesh::Priority::PRIO_MEDIUM);

        mesh::UUID::generate(resp.header.packet_id);
        memcpy(resp.header.source_hash,
               meshNode.getSelf().node_hash, 16);
        memcpy(resp.header.dest_hash,
               pkt.header.source_hash, 16);

        // First 16 bytes = original request packet_id (for RequestManager matching)
        memcpy(resp.payload, pkt.header.packet_id, 16);

        // Append response data after the request_id
        const char* reply = "ACK:OK";
        size_t replyLen = strlen(reply);
        memcpy(resp.payload + 16, reply, replyLen);
        resp.header.payload_size = 16 + replyLen;

        mesh::enqueueOutgoing(meshNode.getContext(), resp);
    }
}

void onLocationUpdate(const mesh::MeshEvent& evt, void* ctx) {
    Serial.printf("[APP] Location updated: %.6f, %.6f\n",
                  evt.locationData.lat, evt.locationData.lon);
}

// ---- Arduino setup ----
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n========================================");
    Serial.println("  ESP32 Mesh Framework — Starting...");
    Serial.println("========================================\n");

    // Configure the mesh with fluent API
    meshNode.setMaxPeers(20)
            .setMaxRetries(3)
            .setQueueSize(32)
            .setDirectionAngle(90.0f)
            .setDistanceTolerance(500.0f)
            .setNetworkKey(NETWORK_KEY, sizeof(NETWORK_KEY) - 1)
            .setLocationProvider(&location);

    // Initialize
    meshNode.init();

    // Subscribe to events
    meshNode.getEventBus().subscribe(mesh::MeshEventType::NODE_DISCOVERED, onNodeFound);
    meshNode.getEventBus().subscribe(mesh::MeshEventType::PACKET_RECEIVED, onDataReceived);
    meshNode.getEventBus().subscribe(mesh::MeshEventType::LOCATION_UPDATED, onLocationUpdate);

    // Start the mesh
    meshNode.start();

    Serial.println("[APP] Mesh is running. Waiting for peers...\n");
}

// ---- Arduino loop ----
void loop() {
    // Process events on the main thread (optional, for event callbacks)
    meshNode.getEventBus().processOne(pdMS_TO_TICKS(100));

    // Example: send a UDP message every 15 seconds
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 15000) {
        lastSend = millis();

        const char* msg = "Hello from mesh!";
        uint8_t broadcastHash[16] = {};  // all zeros = broadcast

        bool ok = meshNode.sendUDP(broadcastHash,
                                    reinterpret_cast<const uint8_t*>(msg),
                                    strlen(msg));
        if (ok) {
            Serial.println("[APP] UDP broadcast sent");
        }
    }

    // Example: TCP request (uncomment to test with a known destination)
    /*
    static unsigned long lastTcp = 0;
    if (millis() - lastTcp > 30000) {
        lastTcp = millis();

        uint8_t targetHash[16] = {0};  // fill with target node hash
        const char* request = "PING";
        mesh::MeshResponse resp = meshNode.sendTCP(
            targetHash,
            reinterpret_cast<const uint8_t*>(request),
            strlen(request),
            5000  // 5 second timeout
        );

        if (resp.success) {
            char buf[200];
            memcpy(buf, resp.responsePacket.payload, resp.payloadLen);
            buf[resp.payloadLen] = '\0';
            Serial.printf("[APP] TCP response: %s\n", buf);
        } else {
            Serial.println("[APP] TCP request timed out");
        }
    }
    */
}
