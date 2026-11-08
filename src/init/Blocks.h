#pragma once

// Original: net/minecraft/init/Blocks.java -- the registered blocks by name, looked up after registration.

namespace mc {

class Block;

namespace Blocks {

inline Block* air = nullptr;
inline Block* stone = nullptr;

// Java: the static initializer (getRegisteredBlock for every field). Called by Bootstrap.
void init();

} // namespace Blocks
} // namespace mc
