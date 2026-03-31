#pragma once

namespace mesh {

class ILocationProvider {
public:
    virtual ~ILocationProvider() = default;
    virtual bool  init() = 0;
    virtual void  update() = 0;
    virtual bool  hasValidFix() const = 0;
    virtual float getLatitude() const = 0;
    virtual float getLongitude() const = 0;
};

} // namespace mesh
