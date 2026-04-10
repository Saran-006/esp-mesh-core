#pragma once

#include "Packet.hpp"
#include "Node.hpp"
#include <cstdint>
#include <cstring>

namespace mesh {

enum class MeshEventType : uint8_t {
    PACKET_RECEIVED,
    PACKET_SENT,
    PACKET_DROPPED,
    PACKET_ACK_RECEIVED,
    PACKET_ACK_TIMEOUT,

    NODE_DISCOVERED,
    NODE_LOST,
    NODE_UPDATED,

    LOCATION_UPDATED,
    LOCATION_LOST,

    SERVICE_REGISTERED,
    SERVICE_UNREGISTERED,

    MESH_STARTED,
    MESH_STOPPED,
    MESH_ERROR
};

struct MeshEvent {
    MeshEventType type;
    union {
        struct {
            Packet  packet;
            uint8_t sender_mac[6];
            bool    isDuplicate;
            bool    isForUs;
        } packetData;
        struct {
            Node node;
        } nodeData;
        struct {
            float lat;
            float lon;
            bool  valid;
        } locationData;
        struct {
            uint8_t service_id;
        } serviceData;
        struct {
            int error_code;
        } errorData;
    };

    MeshEvent() : type(MeshEventType::MESH_ERROR) {
        memset(&errorData, 0, sizeof(errorData));
    }
};

} // namespace mesh
