#pragma once

// Original: net/minecraft/util/ITickable.java

namespace mc {

class ITickable {
public:
    virtual ~ITickable() = default;
    virtual void update() = 0;
};

} // namespace mc
