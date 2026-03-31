#pragma once

#include "mesh_core/MeshContext.hpp"
#include "mesh_core/MeshEvent.hpp"

namespace mesh {

void onLocationUpdated(MeshContext* ctx, float lat, float lon);
void onLocationLost(MeshContext* ctx);

} // namespace mesh
