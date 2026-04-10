#pragma once

#include "mesh_core/MeshConfig.hpp"
#include "mesh_core/Node.hpp"
#include "mesh_core/ILocationProvider.hpp"
#include "mesh_core/PeerManager.hpp"
#include "mesh_core/EventBus.hpp"
#include "mesh_core/MeshContext.hpp"
#include "mesh_core/RequestManager.hpp"

namespace mesh {

// Forward declarations
class ESPNOWTransport;
class NodeRegistry;
class DedupCache;
class DirectionalRouter;
class AckManager;
class FragmentManager;
class Dispatcher;
class RouteCache;
class RequestManager;

class Mesh {
public:
    Mesh();
    ~Mesh();

    // ---- Fluent configuration (call BEFORE start) ----
    Mesh& setMaxPeers(int n);
    Mesh& setMaxRetries(int n);
    Mesh& setQueueSize(int n);
    Mesh& setDirectionAngle(float degrees);
    Mesh& setDistanceTolerance(float meters);
    Mesh& setNetworkKey(const uint8_t* key, size_t len);
    Mesh& setLocationProvider(ILocationProvider* provider);

    // ---- Lifecycle ----
    void init();
    void start();

    // ---- Send data (UDP mode: fire-and-forget) ----
    bool sendUDP(const uint8_t* destHash, const uint8_t* data, size_t len,
                 bool ackRequired = false);

    // ---- Send data (TCP mode: blocks caller until response or timeout) ----
    MeshResponse sendTCP(const uint8_t* destHash, const uint8_t* data, size_t len,
                         int timeoutMs = 10000);

    // Legacy alias
    bool sendData(const uint8_t* destHash, const uint8_t* data, size_t len,
                  bool ackRequired = false) {
        return sendUDP(destHash, data, len, ackRequired);
    }

    // ---- Accessors ----
    MeshContext*  getContext()  { return &ctx_; }
    MeshConfig&   getConfig()  { return config_; }
    const Node&   getSelf()    const { return selfNode_; }
    NodeRegistry& getNodeRegistry() { return *nodeRegistry_; }
    EventBus&     getEventBus(){ return eventBus_; }

private:
    MeshConfig         config_;
    Node               selfNode_;
    MeshContext         ctx_;
    EventBus           eventBus_;

    // Subsystem instances (allocated on init)
    ESPNOWTransport*   transport_;
    PeerManager*       peerManager_;
    NodeRegistry*      nodeRegistry_;
    DedupCache*        dedupCache_;
    DirectionalRouter* router_;
    AckManager*        ackManager_;
    FragmentManager*   fragmentManager_;
    Dispatcher*        dispatcher_;
    RouteCache*        routeCache_;
    RequestManager*    requestManager_;

    ILocationProvider* locationProvider_;

    // FreeRTOS task handles
    TaskHandle_t receiverTaskHandle_;
    TaskHandle_t dispatcherTaskHandle_;
    TaskHandle_t senderTaskHandle_;
    TaskHandle_t discoveryTaskHandle_;
    TaskHandle_t locationTaskHandle_;
    TaskHandle_t healthTaskHandle_;

    void createQueues();
    void createTasks();
    void initSelfNode();
};

} // namespace mesh
