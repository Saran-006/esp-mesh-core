#pragma once

#include "mesh_core/Mesh.hpp"
#include <Arduino.h>

namespace mesh {

class MeshTerminal {
public:
    MeshTerminal(Mesh& node);
    
    // Checks for Serial input and executes commands if found
    void processSerial();

    // Directly execute a command string
    void execute(const String& input);

private:
    Mesh& node_;
    String buffer_;
    
    void handleLs();
    void handleMsg(const String& args);
    void handleTcp(const String& args);
    void handleGeo(const String& args);
    void handleBroadcast(const String& args);

    static void hexToBytes(String hex, uint8_t* bytes, int len);
};

} // namespace mesh
