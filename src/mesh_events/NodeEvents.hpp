#pragma once

#include "mesh_core/MeshContext.hpp"
#include "mesh_core/MeshEvent.hpp"

namespace mesh {

void onNodeDiscovered(MeshContext* ctx, const Node& node);
void onNodeLost(MeshContext* ctx, const Node& node);
void onNodeUpdated(MeshContext* ctx, const Node& node);

} // namespace mesh
