#pragma once

#include "mesh_core/MeshContext.hpp"
#include "mesh_core/MeshEvent.hpp"

namespace mesh {

void onServiceRegistered(MeshContext* ctx, uint8_t serviceId);
void onServiceUnregistered(MeshContext* ctx, uint8_t serviceId);

} // namespace mesh
