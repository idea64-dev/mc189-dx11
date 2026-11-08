#include "block/state/BlockPistonStructureHelper.h"

#include "block/AllBlocks.h"
#include "world/World.h"

#include <algorithm>

namespace mc {

BlockPistonStructureHelper::BlockPistonStructureHelper(World* worldIn, const BlockPos& posIn, EnumFacing pistonFacing, bool extending)
    : world(worldIn)
    , pistonPos(posIn)
    , blockToMove(extending ? posIn.offset(pistonFacing) : posIn.offset(pistonFacing, 2))
    , moveDirection(extending ? pistonFacing : getOpposite(pistonFacing))
{
}

bool BlockPistonStructureHelper::canMove()
{
    toMove.clear();
    toDestroy.clear();
    Block* block = world->getBlockState(blockToMove)->getBlock();
    if (!BlockPistonBase::canPush(block, world, blockToMove, moveDirection, false)) {
        if (block->getMobilityFlag() != 1)
            return false;
        toDestroy.push_back(blockToMove);
        return true;
    }
    if (!func_177251_a(blockToMove))
        return false;
    // toMove may grow while this runs, as in Java.
    for (size_t i = 0; i < toMove.size(); ++i) {
        const BlockPos blockpos = toMove[i];
        if (world->getBlockState(blockpos)->getBlock() == Blocks::slime_block && !func_177250_b(blockpos))
            return false;
    }
    return true;
}

bool BlockPistonStructureHelper::func_177251_a(const BlockPos& origin)
{
    Block* block = world->getBlockState(origin)->getBlock();
    if (&block->getMaterial() == &Material::air)
        return true;
    if (!BlockPistonBase::canPush(block, world, origin, moveDirection, false))
        return true;
    if (origin == pistonPos)
        return true;
    if (std::find(toMove.begin(), toMove.end(), origin) != toMove.end())
        return true;
    int i = 1;
    if (i + static_cast<int>(toMove.size()) > 12)
        return false;
    while (block == Blocks::slime_block) {
        const BlockPos blockpos = origin.offset(getOpposite(moveDirection), i);
        block = world->getBlockState(blockpos)->getBlock();
        if (&block->getMaterial() == &Material::air || !BlockPistonBase::canPush(block, world, blockpos, moveDirection, false) || blockpos == pistonPos)
            break;
        ++i;
        if (i + static_cast<int>(toMove.size()) > 12)
            return false;
    }
    int i1 = 0;
    for (int j = i - 1; j >= 0; --j) {
        toMove.push_back(origin.offset(getOpposite(moveDirection), j));
        ++i1;
    }
    int j1 = 1;
    while (true) {
        const BlockPos blockpos1 = origin.offset(moveDirection, j1);
        const auto found = std::find(toMove.begin(), toMove.end(), blockpos1);
        const int k = found == toMove.end() ? -1 : static_cast<int>(found - toMove.begin());
        if (k > -1) {
            func_177255_a(i1, k);
            for (int l = 0; l <= k + i1; ++l) {
                const BlockPos blockpos2 = toMove[static_cast<size_t>(l)];
                if (world->getBlockState(blockpos2)->getBlock() == Blocks::slime_block && !func_177250_b(blockpos2))
                    return false;
            }
            return true;
        }
        block = world->getBlockState(blockpos1)->getBlock();
        if (&block->getMaterial() == &Material::air)
            return true;
        if (!BlockPistonBase::canPush(block, world, blockpos1, moveDirection, true) || blockpos1 == pistonPos)
            return false;
        if (block->getMobilityFlag() == 1) {
            toDestroy.push_back(blockpos1);
            return true;
        }
        if (toMove.size() >= 12)
            return false;
        toMove.push_back(blockpos1);
        ++i1;
        ++j1;
    }
}

void BlockPistonStructureHelper::func_177255_a(int p_177255_1_, int p_177255_2_)
{
    const auto size = static_cast<std::ptrdiff_t>(toMove.size());
    std::vector<BlockPos> list(toMove.begin(), toMove.begin() + p_177255_2_);
    std::vector<BlockPos> list1(toMove.begin() + (size - p_177255_1_), toMove.end());
    std::vector<BlockPos> list2(toMove.begin() + p_177255_2_, toMove.begin() + (size - p_177255_1_));
    toMove.clear();
    toMove.insert(toMove.end(), list.begin(), list.end());
    toMove.insert(toMove.end(), list1.begin(), list1.end());
    toMove.insert(toMove.end(), list2.begin(), list2.end());
}

bool BlockPistonStructureHelper::func_177250_b(const BlockPos& p_177250_1_)
{
    for (const EnumFacing enumfacing : kFacings) {
        if (getAxis(enumfacing) != getAxis(moveDirection) && !func_177251_a(p_177250_1_.offset(enumfacing)))
            return false;
    }
    return true;
}

} // namespace mc
