#pragma once

// Original: net/minecraft/world/EnumSkyBlock.java

namespace mc {

enum class EnumSkyBlock { SKY, BLOCK };

// Java: defaultLightValue -- what an unloaded or out-of-world position reports.
constexpr int defaultLightValue(EnumSkyBlock type)
{
    return type == EnumSkyBlock::SKY ? 15 : 0;
}

} // namespace mc
