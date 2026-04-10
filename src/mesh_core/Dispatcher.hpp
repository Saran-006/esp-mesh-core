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
    void processPacket(Packet& pkt, bool isDuplicate);

private:
    MeshContext* ctx_;

    void handleAck(Packet& pkt);
    void handleControl(Packet& pkt, bool isDuplicate);
    void handleTcpResponse(Packet& pkt, bool isDuplicate);
    void handleFragment(Packet& pkt);
    void handleData(Packet& pkt, bool isDuplicate);
    void forwardPacket(Packet& pkt);
    void sendAckFor(const Packet& pkt);
    bool isForUs(const Packet& pkt) const;
};

} // namespace mesh
