#pragma once

#include "mesh_core/MeshContext.hpp"
#include "mesh_core/MeshEvent.hpp"

namespace mesh {

void onPacketReceived(MeshContext* ctx, const Packet& pkt, const uint8_t senderMac[6]);
void onPacketSent(MeshContext* ctx, const Packet& pkt);
void onPacketDropped(MeshContext* ctx, const Packet& pkt, const char* reason);

} // namespace mesh
