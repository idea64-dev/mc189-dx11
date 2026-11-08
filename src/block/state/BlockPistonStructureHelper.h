#pragma once

// Original: net/minecraft/block/state/BlockPistonStructureHelper.java -- which blocks a piston moves or breaks.

#include "util/BlockPos.h"
#include "util/EnumFacing.h"

#include <vector>

namespace mc {

class World;

class BlockPistonStructureHelper {
public:
    BlockPistonStructureHelper(World* worldIn, const BlockPos& posIn, EnumFacing pistonFacing, bool extending);

    bool canMove();
    const std::vector<BlockPos>& getBlocksToMove() const { return toMove; }
    const std::vector<BlockPos>& getBlocksToDestroy() const { return toDestroy; }

private:
    bool func_177251_a(const BlockPos& origin);
    void func_177255_a(int p_177255_1_, int p_177255_2_);
    bool func_177250_b(const BlockPos& p_177250_1_);

    World* world;
    BlockPos pistonPos;
    BlockPos blockToMove;
    EnumFacing moveDirection;
    std::vector<BlockPos> toMove;
    std::vector<BlockPos> toDestroy;
};

} // namespace mc
