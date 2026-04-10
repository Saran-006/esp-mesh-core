#include "mesh_core/Mesh.hpp"
#include "mesh_core/Logger.hpp"
#include "mesh_core/Security.hpp"
#include "mesh_core/Dispatcher.hpp"
#include "mesh_core/RequestManager.hpp"
#include "mesh_transport/ESPNOWTransport.hpp"
#include "mesh_registry/NodeRegistry.hpp"
#include "mesh_registry/DedupCache.hpp"
#include "mesh_routing/DirectionalRouter.hpp"
#include "mesh_routing/RouteCache.hpp"
#include "mesh_reliability/AckManager.hpp"
#include "mesh_reliability/FragmentManager.hpp"
#include "mesh_tasks/ReceiverTask.hpp"
#include "mesh_tasks/SenderTask.hpp"
#include "mesh_tasks/DiscoveryTask.hpp"
#include "mesh_tasks/LocationTask.hpp"
#include "mesh_tasks/HealthTask.hpp"
#include "mesh_utils/Hash.hpp"
#include "mesh_utils/UUID.hpp"
#include <nvs_flash.h>
#include <cstring>
#include <Arduino.h>

static const char* TAG = "Mesh";

namespace mesh {

Mesh::Mesh()
    : transport_(nullptr), peerManager_(nullptr), nodeRegistry_(nullptr),
      dedupCache_(nullptr), router_(nullptr), ackManager_(nullptr),
      fragmentManager_(nullptr), dispatcher_(nullptr), routeCache_(nullptr),
      requestManager_(nullptr), locationProvider_(nullptr),
      receiverTaskHandle_(nullptr), dispatcherTaskHandle_(nullptr),
      senderTaskHandle_(nullptr), discoveryTaskHandle_(nullptr),
      locationTaskHandle_(nullptr), healthTaskHandle_(nullptr) {
    memset(&ctx_, 0, sizeof(ctx_));
}

Mesh::~Mesh() {
    ctx_.running = false;
    vTaskDelay(pdMS_TO_TICKS(500)); // let tasks exit

    delete transport_;
    delete peerManager_;
    delete nodeRegistry_;
    delete dedupCache_;
    delete router_;
    delete ackManager_;
    delete fragmentManager_;
    delete dispatcher_;
    delete routeCache_;
    delete requestManager_;
}

// ---- Fluent configuration ----

Mesh& Mesh::setMaxPeers(int n) {
    config_.maxPeers = n;
    return *this;
}

Mesh& Mesh::setMaxRetries(int n) {
    config_.maxRetries = n;
    return *this;
}

Mesh& Mesh::setQueueSize(int n) {
    config_.outgoingQueueSize = n;
    config_.eventQueueSize = n;
    return *this;
}

Mesh& Mesh::setDirectionAngle(float degrees) {
    config_.angleThreshold = degrees;
    return *this;
}

Mesh& Mesh::setDistanceTolerance(float meters) {
    config_.distanceTolerance = meters;
    return *this;
}

Mesh& Mesh::setNetworkKey(const uint8_t* key, size_t len) {
    size_t copyLen = len > 32 ? 32 : len;
    memcpy(config_.networkKey, key, copyLen);
    config_.networkKeyLen = copyLen;
    return *this;
}

Mesh& Mesh::setLocationProvider(ILocationProvider* provider) {
    locationProvider_ = provider;
    return *this;
}

// ---- Lifecycle ----

void Mesh::init() {
    LOG_INFO(TAG, "Initializing mesh...");

    // Initialize NVS
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Create subsystems
    transport_      = new ESPNOWTransport();
    peerManager_    = new PeerManager();
    nodeRegistry_   = new NodeRegistry();
    dedupCache_     = new DedupCache(config_.dedupSize);
    router_         = new DirectionalRouter(nodeRegistry_, config_.angleThreshold,
                                            config_.distanceTolerance);
    ackManager_     = new AckManager(config_.maxRetries, config_.retryBackoffMs);
    fragmentManager_ = new FragmentManager();
    routeCache_     = new RouteCache();
    requestManager_ = new RequestManager();

    // Initialize event bus
    eventBus_.init(config_.eventQueueSize);

    // Create queues
    createQueues();

    // Initialize transport (WiFi STA + ESP-NOW)
    transport_->init(ctx_.rawIncomingQueue);

    // Initialize self node
    initSelfNode();

    // Initialize location provider
    if (locationProvider_) {
        locationProvider_->init();
    }

    // Wire up context
    ctx_.config          = &config_;
    ctx_.selfNode        = &selfNode_;
    ctx_.transport       = transport_;
    ctx_.peerManager     = peerManager_;
    ctx_.nodeRegistry    = nodeRegistry_;
    ctx_.dedupCache      = dedupCache_;
    ctx_.router          = router_;
    ctx_.ackManager      = ackManager_;
    ctx_.fragmentManager = fragmentManager_;
    ctx_.locationProvider = locationProvider_;
    ctx_.eventBus        = &eventBus_;
    ctx_.routeCache      = routeCache_;
    ctx_.requestManager  = requestManager_;
    ctx_.running         = false;

    // Create dispatcher (uses context)
    dispatcher_ = new Dispatcher(&ctx_);

    LOG_INFO(TAG, "Mesh initialized");
}

void Mesh::start() {
    ctx_.running = true;
    createTasks();

    MeshEvent evt;
    evt.type = MeshEventType::MESH_STARTED;
    eventBus_.post(evt);

    LOG_INFO(TAG, "Mesh started — node hash: %02X%02X%02X%02X...",
             selfNode_.node_hash[0], selfNode_.node_hash[1],
             selfNode_.node_hash[2], selfNode_.node_hash[3]);
}

void Mesh::initSelfNode() {
    uint8_t mac[6];
    transport_->getOwnMac(mac);
    memcpy(selfNode_.mac, mac, 6);
    Hash::nodeHashFromMac(mac, selfNode_.node_hash);
    selfNode_.lat = 0;
    selfNode_.lon = 0;
    selfNode_.last_seen = millis();

    LOG_INFO(TAG, "Self: MAC=%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void Mesh::createQueues() {
    ctx_.rawIncomingQueue   = xQueueCreate(config_.eventQueueSize, sizeof(RawIncoming));
    ctx_.packetQueue        = xQueueCreate(config_.eventQueueSize, sizeof(Packet));
    ctx_.outgoingQueueHigh  = xQueueCreate(config_.outgoingQueueSize / 2 + 1, sizeof(Packet));
    ctx_.outgoingQueueMed   = xQueueCreate(config_.outgoingQueueSize / 2 + 1, sizeof(Packet));
    ctx_.outgoingQueueLow   = xQueueCreate(config_.outgoingQueueSize, sizeof(Packet));

    if (!ctx_.rawIncomingQueue || !ctx_.packetQueue ||
        !ctx_.outgoingQueueHigh || !ctx_.outgoingQueueMed || !ctx_.outgoingQueueLow) {
        LOG_ERROR(TAG, "Failed to create queues!");
    }
}

void Mesh::createTasks() {
    // ReceiverTask — HIGH priority (5)
    xTaskCreatePinnedToCore(receiverTaskFn, "mesh_recv", 4096, &ctx_,
                            5, &receiverTaskHandle_, 1);

    // DispatcherTask — MEDIUM priority (3)
    xTaskCreatePinnedToCore(dispatcherTaskFn, "mesh_disp", 8192, &ctx_,
                            3, &dispatcherTaskHandle_, 1);

    // SenderTask — MEDIUM priority (3)
    xTaskCreatePinnedToCore(senderTaskFn, "mesh_send", 8192, &ctx_,
                            3, &senderTaskHandle_, 1);

    // DiscoveryTask — MEDIUM priority (2)
    xTaskCreatePinnedToCore(discoveryTaskFn, "mesh_disc", 4096, &ctx_,
                            2, &discoveryTaskHandle_, 0);

    // LocationTask — LOW priority (1)
    if (locationProvider_) {
        xTaskCreatePinnedToCore(locationTaskFn, "mesh_loc", 4096, &ctx_,
                                1, &locationTaskHandle_, 0);
    }

    // HealthTask — LOW priority (1)
    xTaskCreatePinnedToCore(healthTaskFn, "mesh_health", 4096, &ctx_,
                            1, &healthTaskHandle_, 0);

    LOG_INFO(TAG, "All tasks created");
}

// ---- UDP Send (fire-and-forget) ----

bool Mesh::sendUDP(const uint8_t* destHash, const uint8_t* data, size_t len,
                   bool ackRequired) {
    // Check if fragmentation is needed
    if (len > MAX_SINGLE_PAYLOAD) {
        // CRITICAL: Allocate on HEAP, not stack!
        // Packet[32] = 8000 bytes, which overflows the 8KB loop() stack.
        Packet* fragments = new Packet[32];
        uint8_t pktId[16];
        UUID::generate(pktId);

        uint8_t flags = FLAG_DATA | FLAG_ROUTE_RECORD;
        if (ackRequired) flags |= FLAG_ACK_REQUIRED;

        // Get dest coordinates from registry if available
        float destLat = 0, destLon = 0;
        const Node* destNode = nodeRegistry_->findByHash(destHash);
        if (destNode) {
            destLat = destNode->lat;
            destLon = destNode->lon;
        }

        int fragCount = fragmentManager_->fragment(
            selfNode_.node_hash, destHash, pktId, flags,
            static_cast<uint8_t>(Priority::PRIO_LOW), destLat, destLon,
            config_.ttlDefault, data, len, fragments, 32);

        if (fragCount <= 0) {
            LOG_ERROR(TAG, "Fragmentation failed");
            delete[] fragments;
            return false;
        }

        for (int i = 0; i < fragCount; i++) {
            enqueueOutgoing(&ctx_, fragments[i]);
        }
        delete[] fragments;
        return true;
    }

    // Single packet — use heap to keep loop() stack safe
    Packet* pkt = new Packet();
    memset(pkt, 0, sizeof(Packet));
    pkt->header.version = 1;
    pkt->header.ttl = config_.ttlDefault;
    pkt->header.flags = FLAG_DATA | FLAG_ROUTE_RECORD;
    if (ackRequired) pkt->header.flags |= FLAG_ACK_REQUIRED;
    pkt->header.priority = static_cast<uint8_t>(Priority::PRIO_LOW);

    UUID::generate(pkt->header.packet_id);
    memcpy(pkt->header.source_hash, selfNode_.node_hash, 16);
    memcpy(pkt->header.dest_hash, destHash, 16);

    // Lookup dest coords
    const Node* destNode = nodeRegistry_->findByHash(destHash);
    if (destNode) {
        pkt->header.dest_lat = destNode->lat;
        pkt->header.dest_lon = destNode->lon;
    }

    size_t copyLen = len > MAX_SINGLE_PAYLOAD ? MAX_SINGLE_PAYLOAD : len;
    memcpy(pkt->payload, data, copyLen);
    pkt->header.payload_size = copyLen;

    bool result = enqueueOutgoing(&ctx_, *pkt);
    delete pkt;
    return result;
}

// ---- TCP Send (blocking request-response) ----

MeshResponse Mesh::sendTCP(const uint8_t* destHash, const uint8_t* data, size_t len,
                           int timeoutMs) {
    MeshResponse failResp;
    failResp.success = false;
    failResp.payloadLen = 0;

    if (len > MAX_SINGLE_PAYLOAD - 16) {
        // TCP mode requires space for request_id in response payload
        LOG_ERROR(TAG, "TCP payload too large (max %d)", (int)(MAX_SINGLE_PAYLOAD - 16));
        return failResp;
    }

    // Build request packet
    Packet pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.version = 1;
    pkt.header.ttl = config_.ttlDefault;
    pkt.header.flags = FLAG_DATA | FLAG_TCP_REQUEST | FLAG_ACK_REQUIRED | FLAG_ROUTE_RECORD;
    pkt.header.priority = static_cast<uint8_t>(Priority::PRIO_MEDIUM);

    UUID::generate(pkt.header.packet_id);
    memcpy(pkt.header.source_hash, selfNode_.node_hash, 16);
    memcpy(pkt.header.dest_hash, destHash, 16);

    const Node* destNode = nodeRegistry_->findByHash(destHash);
    if (destNode) {
        pkt.header.dest_lat = destNode->lat;
        pkt.header.dest_lon = destNode->lon;
    }

    memcpy(pkt.payload, data, len);
    pkt.header.payload_size = len;

    // Register the request BEFORE enqueuing
    if (!requestManager_->registerRequest(pkt.header.packet_id)) {
        LOG_ERROR(TAG, "Failed to register TCP request");
        return failResp;
    }

    // Enqueue the packet
    if (!enqueueOutgoing(&ctx_, pkt)) {
        requestManager_->cancelRequest(pkt.header.packet_id);
        LOG_ERROR(TAG, "Failed to enqueue TCP request");
        return failResp;
    }

    // Block the calling task until response arrives or timeout
    return requestManager_->sendAndWait(pkt.header.packet_id, timeoutMs);
}

} // namespace mesh
