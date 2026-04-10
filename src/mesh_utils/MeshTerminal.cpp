#include "mesh_utils/MeshTerminal.hpp"
#include "mesh_registry/NodeRegistry.hpp"
#include <Arduino.h>

namespace mesh {

MeshTerminal::MeshTerminal(Mesh& node) : node_(node) {}

void MeshTerminal::processSerial() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        
        // Echo character back to user so they see they are typing
        if (c != '\n' && c != '\r') Serial.print(c);

        if (c == '\n' || c == '\r') {
            if (buffer_.length() > 0) {
                String input = buffer_;
                input.trim();
                buffer_ = ""; // Clear for next
                if (input.length() > 0) {
                    execute(input);
                }
            }
        } else {
            buffer_ += c;
            if (buffer_.length() > 256) buffer_ = ""; // Prevent overflow
        }
    }
}

void MeshTerminal::execute(const String& input) {
    Serial.printf("\n> %s\n", input.c_str());

    if (input == "ls") {
        handleLs();
    } else if (input.startsWith("msg ")) {
        handleMsg(input.substring(4));
    } else if (input.startsWith("tcp ")) {
        handleTcp(input.substring(4));
    } else if (input.startsWith("geo ")) {
        handleGeo(input.substring(4));
    } else if (input.startsWith("broadcast ")) {
        handleBroadcast(input.substring(10));
    } else if (input == "help") {
        Serial.println("┌─────────────────────────────────────────┐");
        Serial.println("│         MESH TERMINAL COMMANDS          │");
        Serial.println("├─────────────────────────────────────────┤");
        Serial.println("│ ls                  List nearby nodes   │");
        Serial.println("│ msg <hash> <text>   Send UDP message    │");
        Serial.println("│ tcp <hash> <text>   Reliable TCP send   │");
        Serial.println("│ broadcast <text>    Send to all nodes   │");
        Serial.println("│ geo <lat> <lon> <text>  Geo-route msg   │");
        Serial.println("│ help                Show this menu      │");
        Serial.println("└─────────────────────────────────────────┘");
    } else {
        Serial.println("Unknown command. Type 'help'.");
    }
}

void MeshTerminal::handleLs() {
    Serial.println("--- DISCOVERED NODES ---");
    Node nodes[MAX_NODES];
    int count = node_.getNodeRegistry().getAll(nodes, MAX_NODES);
    for (int i = 0; i < count; i++) {
        Serial.printf("[%02d] HASH:%02X%02X%02X%02X | MAC:%02X:%02X:%02X:%02X:%02X:%02X | POS:(%.4f, %.4f) | SEEN:%lus\n",
                      i, nodes[i].node_hash[0], nodes[i].node_hash[1], nodes[i].node_hash[2], nodes[i].node_hash[3],
                      nodes[i].mac[0], nodes[i].mac[1], nodes[i].mac[2], nodes[i].mac[3], nodes[i].mac[4], nodes[i].mac[5],
                      nodes[i].lat, nodes[i].lon, (millis() - nodes[i].last_seen) / 1000);
    }
    if (count == 0) Serial.println("No peers found yet.");
    Serial.println("------------------------");
}

void MeshTerminal::handleMsg(const String& args) {
    int firstSpace = args.indexOf(' ');
    if (firstSpace == -1) {
        Serial.println("Usage: msg <hash> <message>");
        return;
    }
    String hashStr = args.substring(0, firstSpace);
    String msg = args.substring(firstSpace + 1); // Greedily capture all remaining text

    uint8_t target[16] = {0};
    hexToBytes(hashStr, target, 16);
    if (node_.sendUDP(target, (const uint8_t*)msg.c_str(), msg.length())) {
        Serial.printf("[CLI] UDP Sent to %s: %s\n", hashStr.c_str(), msg.c_str());
    }
}

void MeshTerminal::handleTcp(const String& args) {
    int firstSpace = args.indexOf(' ');
    if (firstSpace == -1) {
        Serial.println("Usage: tcp <hash> <message>");
        return;
    }
    String hashStr = args.substring(0, firstSpace);
    String msg = args.substring(firstSpace + 1); // Greedily capture all remaining text

    uint8_t target[16] = {0};
    hexToBytes(hashStr, target, 16);
    
    Serial.printf("[CLI] TCP Session with %s: %s\n", hashStr.c_str(), msg.c_str());
    MeshResponse resp = node_.sendTCP(target, (const uint8_t*)msg.c_str(), msg.length());
    if (resp.success) {
        String reply = String((const char*)(resp.responsePacket.payload + 16));
        Serial.printf("[CLI] TCP Result: %s\n", reply.c_str());
    } else {
        Serial.println("[CLI] TCP Failed (Timeout)");
    }
}

void MeshTerminal::handleGeo(const String& args) {
    int firstSpace = args.indexOf(' ');
    int secondSpace = args.indexOf(' ', firstSpace + 1);
    if (firstSpace == -1 || secondSpace == -1) {
        Serial.println("Usage: geo <lat> <lon> <message>");
        return;
    }

    float lat = args.substring(0, firstSpace).toFloat();
    float lon = args.substring(firstSpace + 1, secondSpace).toFloat();
    String msg = args.substring(secondSpace + 1);

    uint8_t target[16] = {0};
    if (node_.sendUDP(target, (const uint8_t*)msg.c_str(), msg.length())) {
        Serial.printf("[CLI] Geo-Routed to %.4f, %.4f: %s\n", lat, lon, msg.c_str());
    }
}

void MeshTerminal::handleBroadcast(const String& args) {
    uint8_t target[16] = {0};
    if (node_.sendUDP(target, (const uint8_t*)args.c_str(), args.length())) {
        Serial.printf("[CLI] Broadcast: %s\n", args.c_str());
    }
}

void MeshTerminal::hexToBytes(String hex, uint8_t* bytes, int maxLen) {
    hex.trim();
    String filtered = "";
    for (size_t i = 0; i < hex.length(); i++) {
        char c = tolower(hex[i]);
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) {
            filtered += c;
        }
    }
    
    int len = filtered.length() / 2;
    if (len > maxLen) len = maxLen;
    for (int i = 0; i < len; i++) {
        String part = filtered.substring(i * 2, i * 2 + 2);
        bytes[i] = (uint8_t)strtol(part.c_str(), NULL, 16);
    }
}

} // namespace mesh
