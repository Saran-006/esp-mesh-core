#include "mesh_location/GPSLocationProvider.hpp"
#include "mesh_core/Logger.hpp"

static const char* TAG = "GPS";

namespace mesh {

GPSLocationProvider::GPSLocationProvider(int rxPin, int txPin,
                                         unsigned long baudRate, int uartNum)
    : serial_(uartNum), rxPin_(rxPin), txPin_(txPin),
      baudRate_(baudRate), initialized_(false) {}

bool GPSLocationProvider::init() {
    serial_.begin(baudRate_, SERIAL_8N1, rxPin_, txPin_);
    initialized_ = true;
    LOG_INFO(TAG, "GPS UART initialized (RX=%d TX=%d baud=%lu)", rxPin_, txPin_, baudRate_);
    return true;
}

void GPSLocationProvider::update() {
    if (!initialized_) return;

    // Read all available bytes from GPS serial (non-blocking)
    while (serial_.available() > 0) {
        char c = serial_.read();
        gps_.encode(c);
    }
}

bool GPSLocationProvider::hasValidFix() const {
    return gps_.location.isValid() && gps_.location.isUpdated();
}

float GPSLocationProvider::getLatitude() const {
    return static_cast<float>(gps_.location.lat());
}

float GPSLocationProvider::getLongitude() const {
    return static_cast<float>(gps_.location.lng());
}

} // namespace mesh
