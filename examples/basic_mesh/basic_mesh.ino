/*
 * MeshFramework — Basic Mesh Example
 *
 * An interactive "Mesh Terminal" for the ESP32.
 * This sketch demonstrates how to initialize the mesh and hand control 
 * over to the interactive CLI (Serial Monitor).
 *
 * AVAILABLE COMMANDS:
 *   - ls              : List all nearby nodes (Hash, MAC, GPS, Last Seen)
 *   - msg <hash> <txt>: Send an unacknowledged UDP message
 *   - tcp <hash> <txt>: Perform a reliable, multi-hop TCP handshake
 *   - geo <lat> <lon> <txt>: Route a message towards specific coordinates
 *   - broadcast <txt> : Send to every node in range
 */

#include <MeshFramework.hpp>

// ---- Configuration ----
static const uint8_t NETWORK_KEY[] = "MySecretMeshKey!";

// ---- Hardware Pins (ESP32) ----
#define GPS_RX_PIN        16
#define GPS_TX_PIN        17
#define INTERNAL_LED      2   // Built-in LED for traffic heartbeat
#define EXTERNAL_LED      4   // For remote control showcase
// WARNING: Do NOT use GPIO 1 or GPIO 3 — they are Serial TX/RX!

// ---- Global instances ----
mesh::Mesh         meshNode;
mesh::MeshTerminal terminal(meshNode);
mesh::GPSLocationProvider gps(GPS_RX_PIN, GPS_TX_PIN, 9600, 2);

// ---- Event: New Node Discovered ----
void onNodeFound(const mesh::MeshEvent& evt, void* ctx) {
    const mesh::Node& node = evt.nodeData.node;
    Serial.printf("\n[APP] *** NEW NODE FOUND: %02X%02X%02X%02X ***\n", 
                  node.node_hash[0], node.node_hash[1], node.node_hash[2], node.node_hash[3]);
}

// ---- Event: Data Packet Received ----
void onDataReceived(const mesh::MeshEvent& evt, void* ctx) {
    const mesh::Packet& pkt = evt.packetData.packet;
    bool isDup = evt.packetData.isDuplicate;
    bool isMe = evt.packetData.isForUs;

    // 1. DATA HEARTBEAT: Always blink internal LED for ANY traffic we touch
    digitalWrite(INTERNAL_LED, HIGH);

    // Determine type string for logging
    String typeStr = isMe ? "" : "[ROUTE]";
    if (pkt.header.flags & mesh::FLAG_TCP_REQUEST) typeStr += "TCP";
    else if (pkt.header.flags & mesh::FLAG_TCP_RESPONSE) typeStr += "TCP_RESP";
    else typeStr += "UDP";
    
    if (isDup) typeStr += "DUP";

    // Get short ID (first 2 bytes of packet_id)
    char idStr[5];
    snprintf(idStr, 5, "%02X%02X", pkt.header.packet_id[0], pkt.header.packet_id[1]);

    // Handle payload offsets
    // TCP responses have [16-byte request ID] [Data...]
    // Standard data is raw [Data...] (no control byte for user payloads)
    size_t offset = 0;
    if (pkt.header.flags & mesh::FLAG_TCP_RESPONSE) {
        offset = 16; // Skip the 16-byte request ID
    }
    
    if (pkt.header.payload_size > offset) {
        String data = String((const char*)(pkt.payload + offset));
        
        // Only log if it's for us or we want to see the "Through-traffic" console noise
        Serial.printf("[%s] %s %s\n", typeStr.c_str(), idStr, data.c_str());

        // 2. REMOTE CONTROL: Toggle External LED ONLY if it's addressed to us
        if (isMe) {
            if (data == "LED_ON") {
                digitalWrite(EXTERNAL_LED, HIGH);
                Serial.println("[APP] Correct address: External LED -> ON");
            } 
            else if (data == "LED_OFF") {
                digitalWrite(EXTERNAL_LED, LOW);
                Serial.println("[APP] Correct address: External LED -> OFF");
            }
        } else {
            // Log that we are just forwarding it
            // Serial.println("[APP] Routing packet for someone else...");
        }
    }

    delay(20); 
    digitalWrite(INTERNAL_LED, LOW); 
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(INTERNAL_LED, OUTPUT);
    digitalWrite(INTERNAL_LED, LOW);
    
    pinMode(EXTERNAL_LED, OUTPUT);
    digitalWrite(EXTERNAL_LED, LOW);

    Serial.println("\n========================================");
    Serial.println("  ESP32 Mesh Framework — CLI Terminal   ");
    Serial.println("========================================\n");
    Serial.println("Type 'ls' to see nodes or 'help' for commands.\n");

    // Configure mesh parameters
    meshNode.setMaxPeers(20)
            .setNetworkKey(NETWORK_KEY, 16)
            .setLocationProvider(&gps);

    // Initialize (Flash, WiFi, Radio)
    meshNode.init();

    // Subscribe to internal events
    meshNode.getEventBus().subscribe(mesh::MeshEventType::NODE_DISCOVERED, onNodeFound);
    meshNode.getEventBus().subscribe(mesh::MeshEventType::PACKET_RECEIVED, onDataReceived);

    // Start the background tasks
    meshNode.start();

    Serial.println("[OK] Mesh active. Type 'help' for commands.\n");
}

void loop() {
    // Add debug tracing to find the crash source
    // Serial.println("loop: processOne");
    meshNode.getEventBus().processOne(0);
    // Serial.println("loop: processSerial");
    terminal.processSerial();
}
