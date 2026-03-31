#pragma once

namespace mesh {

// Forward declarations
class AckManager;
class FragmentManager;

struct ReliabilityLayer {
    AckManager*      ackManager;
    FragmentManager* fragmentManager;
};

} // namespace mesh
