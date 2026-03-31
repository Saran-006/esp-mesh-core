#pragma once

#include "mesh_core/ILocationProvider.hpp"
#include <TinyGPS++.h>
#include <HardwareSerial.h>

namespace mesh {

class GPSLocationProvider : public ILocationProvider {
public:
    // rxPin, txPin = UART pins; baudRate = GPS serial speed; uartNum = HardwareSerial index (1 or 2)
    GPSLocationProvider(int rxPin, int txPin, unsigned long baudRate = 9600, int uartNum = 1);
    ~GPSLocationProvider() override = default;

    bool  init() override;
    void  update() override;
    bool  hasValidFix() const override;
    float getLatitude() const override;
    float getLongitude() const override;

private:
    HardwareSerial serial_;
    mutable TinyGPSPlus gps_;
    int            rxPin_;
    int            txPin_;
    unsigned long  baudRate_;
    bool           initialized_;
};

} // namespace mesh
