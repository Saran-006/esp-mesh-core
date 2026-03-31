// MeshFramework.hpp — Umbrella header
// Include this single header to access the entire mesh framework.

#pragma once

// Core
#include "mesh_core/Mesh.hpp"
#include "mesh_core/MeshConfig.hpp"
#include "mesh_core/MeshEvent.hpp"
#include "mesh_core/MeshContext.hpp"
#include "mesh_core/Packet.hpp"
#include "mesh_core/Node.hpp"
#include "mesh_core/Service.hpp"
#include "mesh_core/ILocationProvider.hpp"
#include "mesh_core/Security.hpp"
#include "mesh_core/PeerManager.hpp"
#include "mesh_core/Reliability.hpp"
#include "mesh_core/Logger.hpp"
#include "mesh_core/EventBus.hpp"
#include "mesh_core/Dispatcher.hpp"
#include "mesh_core/RequestManager.hpp"

// Transport
#include "mesh_transport/ESPNOWTransport.hpp"

// Registry
#include "mesh_registry/NodeRegistry.hpp"
#include "mesh_registry/DedupCache.hpp"

// Routing
#include "mesh_routing/DirectionalRouter.hpp"
#include "mesh_routing/RouteCache.hpp"

// Reliability
#include "mesh_reliability/AckManager.hpp"
#include "mesh_reliability/FragmentManager.hpp"

// Location Providers
#include "mesh_location/GPSLocationProvider.hpp"
#include "mesh_location/ManualLocationProvider.hpp"

// Events
#include "mesh_events/PacketEvents.hpp"
#include "mesh_events/NodeEvents.hpp"
#include "mesh_events/LocationEvents.hpp"
#include "mesh_events/ServiceEvents.hpp"

// Utils
#include "mesh_utils/Hash.hpp"
#include "mesh_utils/UUID.hpp"
#include "mesh_utils/Serializer.hpp"
