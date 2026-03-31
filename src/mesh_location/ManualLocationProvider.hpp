#pragma once

#include "mesh_core/ILocationProvider.hpp"

namespace mesh {

class ManualLocationProvider : public ILocationProvider {
public:
    ManualLocationProvider(float lat, float lon);
    ~ManualLocationProvider() override = default;

    bool  init() override;
    void  update() override;
    bool  hasValidFix() const override;
    float getLatitude() const override;
    float getLongitude() const override;

    void setLocation(float lat, float lon);

private:
    float lat_;
    float lon_;
};

} // namespace mesh
