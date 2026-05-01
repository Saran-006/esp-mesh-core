# VortexaMesh — Decentralized ESP32 Mesh Framework

A robust, FreeRTOS-based mesh networking framework for ESP32 devices utilizing the raw ESP-NOW protocol. VortexaMesh goes beyond simple packet delivery — it provides intelligent geographic routing, automatic self-healing, hardware-accelerated security, and a clean event-driven API that lets you build complex mesh applications with minimal boilerplate.

> **Quick Mental Model:** You send via the API. The internal engine handles routing, reliability, and transport. You react to network activity using event callbacks. That's it.

---

## Features

- **"Throw-and-Play" Self-Organization**: Nodes automatically discover each other on boot using beacon broadcasts and build a live peer registry — no manual configuration needed.
- **Progress-Based Geographic Routing**: Integrates GPS data (via `GPSLocationProvider`) to route packets directionally. Only nodes that are physically closer to the destination forward the packet, eliminating the "border waste" found in traditional flooding.
- **Symmetric Reverse Routing**: Automatically caches the exact path a TCP response traveled and reuses it for all subsequent messages, drastically reducing network flood volume.
- **Active Self-Healing**: A background `HealthTask` monitors node responsiveness. Dead nodes are pruned from the registry and route cache, and the framework automatically reroutes traffic via geo-flooding.
- **Dual-Core Task Segregation**: Radio-intensive tasks (Receiver, Dispatcher, Sender) are pinned to Core 1; logic tasks (Discovery, Health, Location) run on Core 0 — preventing watchdog panics.
- **Packet Fragmentation**: Automatically fragments and reassembles large payloads, overcoming ESP-NOW's 250-byte hardware limit. The routing strategy is preserved across all fragments.
- **Reliability Layers**: Supports fire-and-forget UDP-style and blocking TCP-style request/response with `AckManager` retries.
- **Hardware-Accelerated Security**: SHA-256 HMAC packet signing via `mbedtls` on every frame.
- **Full Event System**: A typed `EventBus` with sugar-coated convenience methods (`onPacketReceived`, `onNodeLost`, etc.) for all 15 network events.
- **Three Integration Modes**: Use the Serial CLI for debugging, call `terminal.execute()` for programmatic control, or call the direct API (`sendUDP`, `sendTCP`, `sendGeo`, `sendBroadcast`) for maximum performance.

---

## Architecture

```
User Code
   │
   ▼
[Direct API]  ──or──  [CLI Terminal]
   │
   ▼
 Queue  (High / Med / Low priority)
   │
   ▼
SenderTask (Core 1) ── SHA-256 Sign ── ESP-NOW
                                          │
                                    (Over the air)
                                          │
                                    ReceiverTask (Core 1)
                                          │
                                     Dispatcher
                                          │
                                       EventBus
                                     /    |    \
                              Handler1  Handler2  Handler3
```

### Routing Decision Logic
```
Send Packet
     │
     ▼
Cached direct path? ──YES──► STRAT_DIRECT
     │
     NO
     ▼
Destination coordinates known? ──YES──► STRAT_GEO_FLOOD (progress-filtered)
     │
     NO
     ▼
STRAT_BROADCAST (discovery fallback only)
```

---

## Quick Start

```cpp
#include <MeshFramework.hpp>

mesh::Mesh meshNode;
// GPS is optional — remove if not using geographic routing
mesh::GPSLocationProvider gps(16, 17, 9600, 2);

void setup() {
    Serial.begin(115200);

    // Option A: Default config — just works, no setup needed
    meshNode.init();

    // Option B: Custom config
    // mesh::MeshConfig cfg;
    // cfg.maxPeers = 32;
    // memcpy(cfg.networkKey, "MySecretMeshKey!", 16);
    // cfg.networkKeyLen = 16;
    // meshNode.init(cfg);

    // React to incoming data
    meshNode.onPacketReceived([](const mesh::Packet& pkt, const uint8_t mac[6]) {
        Serial.printf("Got %d bytes!\n", pkt.header.payload_size);
    });

    meshNode.onNodeDiscovered([](const mesh::Node& node) {
        Serial.printf("Peer found: %02X%02X... at (%.4f, %.4f)\n",
            node.node_hash[0], node.node_hash[1], node.lat, node.lon);
    });

    meshNode.start();
}

void loop() {
    // Required: drives all event callbacks
    meshNode.getEventBus().processOne(0);
}
```

---

## Three Ways to Use VortexaMesh

### Mode 1 — Direct API Calls (Recommended)
Call functions directly from your code for maximum performance. All functions return typed statuses.

| Method | Returns | Description |
| :--- | :--- | :--- |
| `sendBroadcast(data, len)` | `bool` | Sends to all nodes in range. |
| `sendUDP(hash, data, len)` | `bool` | Unicast, fire-and-forget. |
| `sendTCP(hash, data, len)` | `MeshResponse` | Blocking request-response. |
| `sendGeo(lat, lon, data, len)` | `bool` | Geographic target flood. |

```cpp
// Step 1: Get a peer's hash by index (call this after nodes have had time to discover)
//         getNodeHash(index, outHash) — same data the 'ls' command shows
uint8_t targetHash[16];
if (meshNode.getNodeHash(0, targetHash)) {        // index 0 = first peer

    // UDP unicast — fire and forget
    bool ok = meshNode.sendUDP(targetHash, (uint8_t*)"Hello", 5);

    // TCP — blocks until response or timeout
    auto res = meshNode.sendTCP(targetHash, (uint8_t*)"Ping", 4);
    if (res.success) Serial.printf("Reply: %d bytes\n", res.payloadLen);
}

// Broadcast to everyone — no hash needed
bool ok = meshNode.sendBroadcast((uint8_t*)"Hello All", 9);

// Geographic flood — no hash needed, targets a physical location
meshNode.sendGeo(12.9716, 77.5946, (uint8_t*)"Alert", 5);

// Iterate all known peers (same as 'ls' internally)
mesh::Node peers[64];
int count = meshNode.getNodes(peers, 64);
for (int i = 0; i < count; i++) {
    // peers[i].node_hash, .mac, .lat, .lon, .last_seen
    Serial.printf("Peer %d: %02X%02X...\n", i, peers[i].node_hash[0], peers[i].node_hash[1]);
}
```

---

### Mode 2 — Programmatic Command Pass
Call the terminal engine from your code and capture the output as a `String`. Useful for dashboards, remote monitoring, or custom UIs.

```cpp
mesh::MeshTerminal terminal(meshNode);

// Pass a command string and get the result back as a String
String nodeList = terminal.execute("ls");
Serial.print(nodeList);

String result = terminal.execute("msg AABBCCDD Hello");
// result contains success/error response as a string
```

---

### Mode 3 — Serial CLI
Type commands directly into the Arduino IDE Serial Monitor (115200 baud) for live debugging and manual control.

```cpp
mesh::MeshTerminal terminal(meshNode);

void loop() {
    // Both lines are required when using the Serial CLI:
    meshNode.getEventBus().processOne(0); // drives event callbacks
    terminal.processSerial();             // reads and executes Serial input
}
```

| Command | Description |
| :--- | :--- |
| `help` | Show all available commands. |
| `ls` | List all discovered nodes. |
| `msg <hash> <text>` | Send a UDP message. |
| `tcp <hash> <text>` | Send a reliable TCP request. |
| `geo <lat> <lon> <text>` | Send a geographically routed message. |
| `broadcast <text>` | Broadcast to all nodes. |

---


## Documentation

See **[dev-manual.md](./dev-manual.md)** for the full developer reference, including:
- Detailed routing decision logic and source file map
- All 15 event types with sugar methods and data sources
- Error handling guide
- Event signature unpacking reference
- Framework extension points

---

## Hardware

- **MCU:** ESP32 Dev Module (or compatible)
- **GPS:** NEO-6M or any UART GPS module
- **Power:** LM2596S buck converter for stable off-grid operation

---

## Dependencies

- ESP32 Arduino Core
- `mbedtls` (bundled with ESP32 Arduino Core)

---

## License

MIT License
