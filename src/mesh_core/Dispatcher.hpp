#pragma once

#include "mesh_core/MeshContext.hpp"

namespace mesh {

// The Dispatcher task function: reads packetQueue, verifies, deduplicates,
// processes control/ACK/fragment logic, routes or delivers.
void dispatcherTaskFn(void* param);

class Dispatcher {
public:
    explicit Dispatcher(MeshContext* ctx);

    // Process a single deserialized+verified packet
    void processPacket(Packet& pkt);

private:
    MeshContext* ctx_;

    void handleAck(Packet& pkt);
    void handleControl(Packet& pkt);
    void handleFragment(Packet& pkt);
    void handleData(Packet& pkt);
    void forwardPacket(Packet& pkt);
    void sendAckFor(const Packet& pkt);
    bool isForUs(const Packet& pkt) const;
};

} // namespace mesh
