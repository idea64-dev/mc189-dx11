#pragma once

// Original: net/minecraft/client/renderer/chunk/VisGraph.java and SetVisibility.java
// Which faces of a 16^3 section can see each other through its non-opaque blocks (flood fill).

#include "util/BlockPos.h"
#include "util/EnumFacing.h"

#include <bitset>
#include <cstdint>

namespace mc {

class SetVisibility {
public:
    // Java: a set of facings (EnumSet).
    using FacingSet = uint8_t;

    void setManyVisible(FacingSet facings);
    void setVisible(EnumFacing facing, EnumFacing facing2, bool visible);
    void setAllVisible(bool visible);
    bool isVisible(EnumFacing facing, EnumFacing facing2) const { return bits[static_cast<size_t>(ordinal(facing) + ordinal(facing2) * 6)]; }

private:
    std::bitset<36> bits;
};

constexpr SetVisibility::FacingSet facingBit(EnumFacing facing)
{
    return static_cast<SetVisibility::FacingSet>(1u << ordinal(facing));
}

class VisGraph {
public:
    // Java: func_178606_a -- marks an opaque block.
    void setOpaque(const BlockPos& pos);
    SetVisibility computeVisibility();
    // Java: func_178609_b -- the faces reachable from the block.
    SetVisibility::FacingSet getVisibleFacings(const BlockPos& pos);

private:
    static int getIndex(const BlockPos& pos) { return getIndex(pos.getX() & 15, pos.getY() & 15, pos.getZ() & 15); }
    static constexpr int getIndex(int x, int y, int z) { return x << 0 | y << 8 | z << 4; }
    SetVisibility::FacingSet floodFill(int index);
    static void addEdges(int index, SetVisibility::FacingSet& set);
    static int getNeighborIndexAtFace(int index, EnumFacing facing);

    std::bitset<4096> opaque;
    int empty = 4096;
};

} // namespace mc
