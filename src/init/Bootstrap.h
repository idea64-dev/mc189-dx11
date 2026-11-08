#pragma once

// Original: net/minecraft/init/Bootstrap.java
// register() fills the block (and, as they are ported, item and stat) registries once. Dispenser behaviours
// come with the dispenser (stage 9).

namespace mc {

class Bootstrap {
public:
    static bool isRegistered() { return alreadyRegistered; }
    static void registerAll();

private:
    static inline bool alreadyRegistered = false;
};

} // namespace mc
