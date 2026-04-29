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
    // If quiet is true, it returns the output as a String instead of printing to Serial
    String execute(const String& input, bool quiet = false);

private:
    Mesh& node_;
    String buffer_;
    
    String handleLs(bool quiet);
    String handleMsg(const String& args, bool quiet);
    String handleTcp(const String& args, bool quiet);
    String handleGeo(const String& args, bool quiet);
    String handleBroadcast(const String& args, bool quiet);

    static void hexToBytes(String hex, uint8_t* bytes, int len);
};

} // namespace mesh
