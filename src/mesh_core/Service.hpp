#pragma once

#include "Packet.hpp"
#include <cstdint>

namespace mesh {

class Service {
public:
    virtual ~Service() = default;
    virtual uint8_t     serviceId()   const = 0;
    virtual const char* serviceName() const = 0;
    virtual void onPacketReceived(const Packet& packet) = 0;
};

} // namespace mesh
