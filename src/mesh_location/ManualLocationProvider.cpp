#include "mesh_location/ManualLocationProvider.hpp"

namespace mesh {

ManualLocationProvider::ManualLocationProvider(float lat, float lon)
    : lat_(lat), lon_(lon) {}

bool ManualLocationProvider::init() {
    return true;
}

void ManualLocationProvider::update() {
    // Nothing to do for manual
}

bool ManualLocationProvider::hasValidFix() const {
    return true; // manual is always valid
}

float ManualLocationProvider::getLatitude() const {
    return lat_;
}

float ManualLocationProvider::getLongitude() const {
    return lon_;
}

void ManualLocationProvider::setLocation(float lat, float lon) {
    lat_ = lat;
    lon_ = lon;
}

} // namespace mesh
