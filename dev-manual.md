# VortexaMesh Developer Manual

Welcome to the **VortexaMesh** developer manual. This document covers the architecture, data structures, and the three primary ways to interact with the mesh framework.

---

## 📑 Table of Contents
1. [Architecture & Workflow](#1-architecture--workflow)
2. [Configuration (MeshConfig)](#2-configuration-meshconfig)
3. [Events Available](#3-events-available)
4. [Core Structures & Enums](#4-core-structures--enums)
5. [Event System & Subscriptions](#5-event-system--subscriptions)
6. [Defining Event Signatures](#6-defining-event-signatures)
7. [Ways of Using Mesh APIs](#7-ways-of-using-mesh-apis)
8. [Self-Healing Mechanism](#8-self-healing-mechanism)
9. [Example Code](#9-example-code)

---

## 1. Architecture & Workflow

VortexaMesh operates as a multi-threaded system pinned to both cores of the ESP32.

### Internal Data Flow:
1. **Initiation:** User calls a messaging function (e.g., `sendUDP`).
2. **Strategy:** Logic decides between `DIRECT` (cached path) or `GEO_FLOOD` (spatial path).
3. **Queueing:** Packet is prioritized and queued.
4. **Processing:** `SenderTask` signs and transmits via ESP-NOW.
5. **Dispatch:** Receiving node verifies signature and passes to `Dispatcher`.
6. **Execution:** **Subscribed functions (multiple handlers allowed)** are executed. If no handler is present, a default logger prints the data to Serial.

---

## 2. Configuration (MeshConfig)

You can initialize the mesh with default settings or provide a custom `MeshConfig` object:

```cpp
mesh::MeshConfig cfg;
cfg.maxPeers = 32;
cfg.networkKeyLen = 16;
memcpy(cfg.networkKey, "MY_SECURE_KEY_12", 16);

meshNode.init(cfg);
```

| Property | Default | Description |
| :--- | :--- | :--- |
| `maxPeers` | 20 | Capacity of the peer registry. |
| `maxRetries` | 3 | Retries before triggering self-healing. |
| `networkKey` | (Empty) | Key for SHA-256 integrity signatures. |
| `ttlDefault` | 10 | Maximum hops allowed. |
| `angleThreshold` | 90.0f | Flooding cone angle. |

---

## 3. Events Available

The framework dispatches numerous events via the `EventBus`. Below is the complete list of available `MeshEventType` enums:

| Triggering Event | Event Name | Sugar Name |
| :--- | :--- | :--- |
| **Data Received** | `PACKET_RECEIVED` | `onPacketReceived` |
| **Data Sent** | `PACKET_SENT` | `onPacketSent` |
| **Data Dropped** | `PACKET_DROPPED` | `onPacketDropped` |
| **Ack Received** | `PACKET_ACK_RECEIVED` | `onPacketAckReceived` |
| **Ack Timeout** | `PACKET_ACK_TIMEOUT` | `onPacketAckTimeout` |
| **Node Found** | `NODE_DISCOVERED` | `onNodeDiscovered` |
| **Node Lost** | `NODE_LOST` | `onNodeLost` |
| **Node Updated** | `NODE_UPDATED` | `onNodeUpdated` |
| **GPS Changed** | `LOCATION_UPDATED` | `onLocationUpdated` |
| **GPS Lost** | `LOCATION_LOST` | `onLocationLost` |
| **Service Add** | `SERVICE_REGISTERED` | `onServiceRegistered` |
| **Service Del** | `SERVICE_UNREGISTERED`| `onServiceUnregistered` |
| **System Boot** | `MESH_STARTED` | `onMeshStarted` |
| **System Halt** | `MESH_STOPPED` | `onMeshStopped` |
| **System Crash** | `MESH_ERROR` | `onMeshError` |

---

## 4. Core Structures & Enums

### `Node` Struct
```cpp
struct Node {
    uint8_t  node_hash[16];
    uint8_t  mac[6];
    float    lat, lon;
    int64_t  last_seen;
};
```

### `PacketHeader` Struct
```cpp
struct PacketHeader {
    uint8_t  version, ttl, flags, priority;
    uint8_t  routing_strategy;
    uint16_t payload_size;
    uint8_t  packet_id[16];
    uint8_t  source_hash[16], dest_hash[16];
    uint8_t  last_hop_mac[6];
    float    dest_lat, dest_lon;
};
```

### `RoutingStrategy` Enum
- `STRAT_DIRECT`: Point-to-point via known MAC.
- `STRAT_GEO_FLOOD`: Directional flood toward coordinates.
- `STRAT_BROADCAST`: Blind flood to all neighbors.

---

## 5. Event System & Subscriptions

VortexaMesh provides two distinct ways to subscribe to events:

### 1) Direct Calls (EventBus)
The raw EventBus allows you to subscribe to *any* event using the `MeshEventType` enum. 

```cpp
meshNode.getEventBus().subscribe(mesh::MeshEventType::PACKET_SENT, [](const mesh::MeshEvent& ev, void* ctx) {
    // Extract data from the union based on the event type
    Serial.printf("Sent packet size: %d\n", ev.packetData.packet.header.payload_size);
}, nullptr);
```

### 2) Sugar Calls (Convenience API)
For the most common events, the framework unwraps the union for you and provides strongly-typed parameters.

```cpp
// 1. Listen for incoming data packets
meshNode.onPacketReceived([](const mesh::Packet& pkt, const uint8_t senderMac[6]) {
    Serial.printf("Packet Received! Size: %d\n", pkt.header.payload_size);
});

// 2. Listen for new nodes joining the mesh
meshNode.onNodeDiscovered([](const mesh::Node& node) {
    Serial.printf("Discovered new node: %02X:%02X...\n", node.mac[0], node.mac[1]);
});

// 3. Listen for local GPS updates
meshNode.onLocationUpdated([](float lat, float lon) {
    Serial.printf("My location updated: %f, %f\n", lat, lon);
});
```

---

## 6. Defining Event Signatures

When using the direct `EventBus`, all events trigger a generic callback signature. You must manually unpack the `MeshEvent` struct based on the event type.

### The Callback Signature
```cpp
void myCustomHandler(const mesh::MeshEvent& ev, void* context) {
    // Implementation
}
```

### Unpacking the `MeshEvent` Union
The `MeshEvent` contains a type and a union of data. You must access the correct struct inside the union depending on the `ev.type`:

| If `ev.type` is... | Read from union... | Example |
| :--- | :--- | :--- |
| `PACKET_*` events | `ev.packetData` | `ev.packetData.packet` or `ev.packetData.sender_mac` |
| `NODE_*` events | `ev.nodeData` | `ev.nodeData.node` |
| `LOCATION_*` events | `ev.locationData` | `ev.locationData.lat` and `ev.locationData.lon` |
| `SERVICE_*` events | `ev.serviceData` | `ev.serviceData.service_id` |
| `MESH_ERROR` | `ev.errorData` | `ev.errorData.error_code` |

**Example of safe unpacking:**
```cpp
meshNode.getEventBus().subscribe(mesh::MeshEventType::NODE_LOST, [](const mesh::MeshEvent& ev, void*) {
    // We know NODE_LOST uses nodeData
    const mesh::Node& lostNode = ev.nodeData.node;
    Serial.printf("Lost connection to node at %f, %f\n", lostNode.lat, lostNode.lon);
}, nullptr);
```

---

## 7. Ways of Using Mesh APIs

### Mode 1: Manual Serial CLI
Connect to Serial (115200) and type commands directly for manual testing:
- `help`: See all commands.
- `ls`: List nearby nodes.
- `msg <hash> <text>`: Send UDP.

### Mode 2: Programmatic Command Execution
Call the terminal engine from your code and get a `String` result. This is useful for building a custom UI or remote logging:
```cpp
mesh::MeshTerminal terminal(meshNode);

// Programmatically execute 'ls' and print the returned string
String output = terminal.execute("ls"); 
Serial.print(output); 
```

### Mode 3: Direct API Function Calls (Recommended)
Call the high-performance functions directly. These functions return immediate statuses.

```cpp
// 1. Broadcast (Returns bool: true if queued)
bool bcastSuccess = meshNode.broadcast((uint8_t*)"Hello All", 9);

// 2. Unicast UDP (Returns bool: true if queued)
bool udpSuccess = meshNode.sendUDP(targetHash, (uint8_t*)"Hello Node", 10);

// 3. Reliable TCP (Blocks and returns a MeshResponse struct)
mesh::MeshResponse res = meshNode.sendTCP(targetHash, (uint8_t*)"Ping", 4);
if (res.success) {
    Serial.printf("Received TCP Ack! Payload Length: %d\n", res.payloadLen);
}

// 4. Geographic Flooding (Returns bool: true if queued)
bool geoSuccess = meshNode.sendGeo(12.9716, 77.5946, (uint8_t*)"Help", 4);
```

---

## 8. Self-Healing Mechanism
VortexaMesh maintains a "Live" network state. If a direct transmission fails after 3 retries:
1. The destination is removed from `NodeRegistry`.
2. Any cached paths in `RouteCache` using that node as a hop are wiped.
3. The next packet will automatically fall back to `STRAT_GEO_FLOOD` to bypass the dead node.

---

## 9. Example Code
You can find full implementation examples by navigating to:
`file -> examples -> MeshFramework -> basic_mesh` in the Arduino Cloud IDE.

Alternatively, you can locate it locally in:
`./examples/basic_mesh.ino`

---

© 2024 VortexaMesh Framework
