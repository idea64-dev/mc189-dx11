// The world callbacks of the rails: BlockRailBase.java (with BlockRailBase.Rail), BlockRail.java,
// BlockRailPowered.java and BlockRailDetector.java.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"

#include <optional>
#include <vector>

namespace mc {

using D = BlockRailBase::EnumRailDirection;

// --- BlockRailBase ---

bool BlockRailBase::isRailBlock(World* worldIn, const BlockPos& pos)
{
    return isRailBlock(worldIn->getBlockState(pos));
}

bool BlockRailBase::isRailBlock(const IBlockState* state)
{
    const Block* block = state->getBlock();
    return block == Blocks::rail || block == Blocks::golden_rail || block == Blocks::detector_rail || block == Blocks::activator_rail;
}

std::optional<MovingObjectPosition> BlockRailBase::collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::collisionRayTrace(worldIn, pos, start, end);
}

void BlockRailBase::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() == this && isAscending(iblockstate->getValue(getShapeProperty())))
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.625f, 1.0f);
    else
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
}

bool BlockRailBase::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return World::doesBlockHaveSolidTopSurface(worldIn, pos.down());
}

void BlockRailBase::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!worldIn->isRemote) {
        state = func_176564_a(worldIn, pos, state, true);
        if (isPowered)
            onNeighborBlockChange(worldIn, pos, state, this);
    }
}

void BlockRailBase::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    if (!worldIn->isRemote) {
        const EnumRailDirection direction = state->getValue(getShapeProperty());
        bool flag = false;
        if (!World::doesBlockHaveSolidTopSurface(worldIn, pos.down()))
            flag = true;
        if (direction == D::ASCENDING_EAST && !World::doesBlockHaveSolidTopSurface(worldIn, pos.east()))
            flag = true;
        else if (direction == D::ASCENDING_WEST && !World::doesBlockHaveSolidTopSurface(worldIn, pos.west()))
            flag = true;
        else if (direction == D::ASCENDING_NORTH && !World::doesBlockHaveSolidTopSurface(worldIn, pos.north()))
            flag = true;
        else if (direction == D::ASCENDING_SOUTH && !World::doesBlockHaveSolidTopSurface(worldIn, pos.south()))
            flag = true;
        if (flag) {
            dropBlockAsItem(worldIn, pos, state, 0);
            worldIn->setBlockToAir(pos);
        } else {
            onNeighborChangedInternal(worldIn, pos, state, neighborBlock);
        }
    }
}

const IBlockState* BlockRailBase::func_176564_a(World* worldIn, const BlockPos& p_176564_2_, const IBlockState* p_176564_3_, bool p_176564_4_)
{
    if (worldIn->isRemote)
        return p_176564_3_;
    Rail rail(worldIn, p_176564_2_, p_176564_3_);
    return rail.func_180364_a(worldIn->isBlockPowered(p_176564_2_), p_176564_4_).getBlockState();
}

void BlockRailBase::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    Block::breakBlock(worldIn, pos, state);
    if (isAscending(state->getValue(getShapeProperty())))
        worldIn->notifyNeighborsOfStateChange(pos.up(), this);
    if (isPowered) {
        worldIn->notifyNeighborsOfStateChange(pos, this);
        worldIn->notifyNeighborsOfStateChange(pos.down(), this);
    }
}

// --- BlockRailBase.Rail ---

BlockRailBase::Rail::Rail(World* worldIn, const BlockPos& posIn, const IBlockState* stateIn)
    : world(worldIn)
    , pos(posIn)
    , block(static_cast<BlockRailBase*>(stateIn->getBlock()))
    , state(stateIn)
    , isPowered(block->isPowered)
{
    // Java reads the shape through the outer block's property; the properties match by name.
    func_180360_a(stateIn->getValue(block->getShapeProperty()));
}

void BlockRailBase::Rail::func_180360_a(EnumRailDirection p_180360_1_)
{
    field_150657_g.clear();
    switch (p_180360_1_) {
    case D::NORTH_SOUTH:
        field_150657_g.push_back(pos.north());
        field_150657_g.push_back(pos.south());
        break;
    case D::EAST_WEST:
        field_150657_g.push_back(pos.west());
        field_150657_g.push_back(pos.east());
        break;
    case D::ASCENDING_EAST:
        field_150657_g.push_back(pos.west());
        field_150657_g.push_back(pos.east().up());
        break;
    case D::ASCENDING_WEST:
        field_150657_g.push_back(pos.west().up());
        field_150657_g.push_back(pos.east());
        break;
    case D::ASCENDING_NORTH:
        field_150657_g.push_back(pos.north().up());
        field_150657_g.push_back(pos.south());
        break;
    case D::ASCENDING_SOUTH:
        field_150657_g.push_back(pos.north());
        field_150657_g.push_back(pos.south().up());
        break;
    case D::SOUTH_EAST:
        field_150657_g.push_back(pos.east());
        field_150657_g.push_back(pos.south());
        break;
    case D::SOUTH_WEST:
        field_150657_g.push_back(pos.west());
        field_150657_g.push_back(pos.south());
        break;
    case D::NORTH_WEST:
        field_150657_g.push_back(pos.west());
        field_150657_g.push_back(pos.north());
        break;
    case D::NORTH_EAST:
        field_150657_g.push_back(pos.east());
        field_150657_g.push_back(pos.north());
        break;
    }
}

void BlockRailBase::Rail::func_150651_b()
{
    for (int i = 0; i < static_cast<int>(field_150657_g.size()); ++i) {
        std::optional<Rail> rail = findRailAt(field_150657_g[static_cast<size_t>(i)]);
        if (rail && rail->func_150653_a(*this)) {
            field_150657_g[static_cast<size_t>(i)] = rail->pos;
        } else {
            field_150657_g.erase(field_150657_g.begin() + i);
            --i;
        }
    }
}

bool BlockRailBase::Rail::hasRailAt(const BlockPos& p)
{
    return isRailBlock(world, p) || isRailBlock(world, p.up()) || isRailBlock(world, p.down());
}

std::optional<BlockRailBase::Rail> BlockRailBase::Rail::findRailAt(const BlockPos& p)
{
    const IBlockState* iblockstate = world->getBlockState(p);
    if (isRailBlock(iblockstate))
        return Rail(world, p, iblockstate);
    BlockPos lvt_2_1_ = p.up();
    iblockstate = world->getBlockState(lvt_2_1_);
    if (isRailBlock(iblockstate))
        return Rail(world, lvt_2_1_, iblockstate);
    lvt_2_1_ = p.down();
    iblockstate = world->getBlockState(lvt_2_1_);
    if (isRailBlock(iblockstate))
        return Rail(world, lvt_2_1_, iblockstate);
    return std::nullopt;
}

bool BlockRailBase::Rail::func_150653_a(const Rail& p_150653_1_)
{
    return func_180363_c(p_150653_1_.pos);
}

bool BlockRailBase::Rail::func_180363_c(const BlockPos& p_180363_1_)
{
    for (const BlockPos& blockpos : field_150657_g) {
        if (blockpos.getX() == p_180363_1_.getX() && blockpos.getZ() == p_180363_1_.getZ())
            return true;
    }
    return false;
}

int BlockRailBase::Rail::countAdjacentRails()
{
    int i = 0;
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (hasRailAt(pos.offset(enumfacing)))
            ++i;
    }
    return i;
}

bool BlockRailBase::Rail::func_150649_b(const Rail& rail)
{
    return func_150653_a(rail) || field_150657_g.size() != 2;
}

void BlockRailBase::Rail::func_150645_c(const Rail& p_150645_1_)
{
    field_150657_g.push_back(p_150645_1_.pos);
    const BlockPos blockpos = pos.north();
    const BlockPos blockpos1 = pos.south();
    const BlockPos blockpos2 = pos.west();
    const BlockPos blockpos3 = pos.east();
    const bool flag = func_180363_c(blockpos);
    const bool flag1 = func_180363_c(blockpos1);
    const bool flag2 = func_180363_c(blockpos2);
    const bool flag3 = func_180363_c(blockpos3);
    std::optional<EnumRailDirection> direction;
    if (flag || flag1)
        direction = D::NORTH_SOUTH;
    if (flag2 || flag3)
        direction = D::EAST_WEST;
    if (!isPowered) {
        if (flag1 && flag3 && !flag && !flag2)
            direction = D::SOUTH_EAST;
        if (flag1 && flag2 && !flag && !flag3)
            direction = D::SOUTH_WEST;
        if (flag && flag2 && !flag1 && !flag3)
            direction = D::NORTH_WEST;
        if (flag && flag3 && !flag1 && !flag2)
            direction = D::NORTH_EAST;
    }
    if (direction == D::NORTH_SOUTH) {
        if (isRailBlock(world, blockpos.up()))
            direction = D::ASCENDING_NORTH;
        if (isRailBlock(world, blockpos1.up()))
            direction = D::ASCENDING_SOUTH;
    }
    if (direction == D::EAST_WEST) {
        if (isRailBlock(world, blockpos3.up()))
            direction = D::ASCENDING_EAST;
        if (isRailBlock(world, blockpos2.up()))
            direction = D::ASCENDING_WEST;
    }
    if (!direction)
        direction = D::NORTH_SOUTH;
    state = state->withProperty(block->getShapeProperty(), *direction);
    world->setBlockState(pos, state, 3);
}

bool BlockRailBase::Rail::func_180361_d(const BlockPos& p_180361_1_)
{
    std::optional<Rail> rail = findRailAt(p_180361_1_);
    if (!rail)
        return false;
    rail->func_150651_b();
    return rail->func_150649_b(*this);
}

BlockRailBase::Rail& BlockRailBase::Rail::func_180364_a(bool p_180364_1_, bool p_180364_2_)
{
    const BlockPos blockpos = pos.north();
    const BlockPos blockpos1 = pos.south();
    const BlockPos blockpos2 = pos.west();
    const BlockPos blockpos3 = pos.east();
    const bool flag = func_180361_d(blockpos);
    const bool flag1 = func_180361_d(blockpos1);
    const bool flag2 = func_180361_d(blockpos2);
    const bool flag3 = func_180361_d(blockpos3);
    std::optional<EnumRailDirection> direction;
    if ((flag || flag1) && !flag2 && !flag3)
        direction = D::NORTH_SOUTH;
    if ((flag2 || flag3) && !flag && !flag1)
        direction = D::EAST_WEST;
    if (!isPowered) {
        if (flag1 && flag3 && !flag && !flag2)
            direction = D::SOUTH_EAST;
        if (flag1 && flag2 && !flag && !flag3)
            direction = D::SOUTH_WEST;
        if (flag && flag2 && !flag1 && !flag3)
            direction = D::NORTH_WEST;
        if (flag && flag3 && !flag1 && !flag2)
            direction = D::NORTH_EAST;
    }
    if (!direction) {
        if (flag || flag1)
            direction = D::NORTH_SOUTH;
        if (flag2 || flag3)
            direction = D::EAST_WEST;
        if (!isPowered) {
            if (p_180364_1_) {
                if (flag1 && flag3)
                    direction = D::SOUTH_EAST;
                if (flag2 && flag1)
                    direction = D::SOUTH_WEST;
                if (flag3 && flag)
                    direction = D::NORTH_EAST;
                if (flag && flag2)
                    direction = D::NORTH_WEST;
            } else {
                if (flag && flag2)
                    direction = D::NORTH_WEST;
                if (flag3 && flag)
                    direction = D::NORTH_EAST;
                if (flag2 && flag1)
                    direction = D::SOUTH_WEST;
                if (flag1 && flag3)
                    direction = D::SOUTH_EAST;
            }
        }
    }
    if (direction == D::NORTH_SOUTH) {
        if (isRailBlock(world, blockpos.up()))
            direction = D::ASCENDING_NORTH;
        if (isRailBlock(world, blockpos1.up()))
            direction = D::ASCENDING_SOUTH;
    }
    if (direction == D::EAST_WEST) {
        if (isRailBlock(world, blockpos3.up()))
            direction = D::ASCENDING_EAST;
        if (isRailBlock(world, blockpos2.up()))
            direction = D::ASCENDING_WEST;
    }
    if (!direction)
        direction = D::NORTH_SOUTH;
    func_180360_a(*direction);
    state = state->withProperty(block->getShapeProperty(), *direction);
    if (p_180364_2_ || world->getBlockState(pos) != state) {
        world->setBlockState(pos, state, 3);
        for (size_t i = 0; i < field_150657_g.size(); ++i) {
            std::optional<Rail> rail = findRailAt(field_150657_g[i]);
            if (rail) {
                rail->func_150651_b();
                if (rail->func_150649_b(*this))
                    rail->func_150645_c(*this);
            }
        }
    }
    return *this;
}

// --- BlockRail ---

void BlockRail::onNeighborChangedInternal(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    if (neighborBlock->canProvidePower() && Rail(worldIn, pos, state).countAdjacentRails() == 3)
        func_176564_a(worldIn, pos, state, false);
}

// --- BlockRailPowered ---

bool BlockRailPowered::func_176566_a(World* worldIn, const BlockPos& pos, const IBlockState* state, bool p_176566_4_, int p_176566_5_)
{
    if (p_176566_5_ >= 8)
        return false;
    int i = pos.getX();
    int j = pos.getY();
    int k = pos.getZ();
    bool flag = true;
    EnumRailDirection direction = state->getValue(SHAPE);
    switch (direction) {
    case D::NORTH_SOUTH:
        if (p_176566_4_)
            ++k;
        else
            --k;
        break;
    case D::EAST_WEST:
        if (p_176566_4_)
            --i;
        else
            ++i;
        break;
    case D::ASCENDING_EAST:
        if (p_176566_4_) {
            --i;
        } else {
            ++i;
            ++j;
            flag = false;
        }
        direction = D::EAST_WEST;
        break;
    case D::ASCENDING_WEST:
        if (p_176566_4_) {
            --i;
            ++j;
            flag = false;
        } else {
            ++i;
        }
        direction = D::EAST_WEST;
        break;
    case D::ASCENDING_NORTH:
        if (p_176566_4_) {
            ++k;
        } else {
            --k;
            ++j;
            flag = false;
        }
        direction = D::NORTH_SOUTH;
        break;
    case D::ASCENDING_SOUTH:
        if (p_176566_4_) {
            ++k;
            ++j;
            flag = false;
        } else {
            --k;
        }
        direction = D::NORTH_SOUTH;
        break;
    default:
        break;
    }
    return func_176567_a(worldIn, BlockPos(i, j, k), p_176566_4_, p_176566_5_, direction)
        ? true
        : flag && func_176567_a(worldIn, BlockPos(i, j - 1, k), p_176566_4_, p_176566_5_, direction);
}

bool BlockRailPowered::func_176567_a(World* worldIn, const BlockPos& p_176567_2_, bool p_176567_3_, int distance, EnumRailDirection p_176567_5_)
{
    const IBlockState* iblockstate = worldIn->getBlockState(p_176567_2_);
    if (iblockstate->getBlock() != this)
        return false;
    const EnumRailDirection direction = iblockstate->getValue(SHAPE);
    if (p_176567_5_ == D::EAST_WEST && (direction == D::NORTH_SOUTH || direction == D::ASCENDING_NORTH || direction == D::ASCENDING_SOUTH))
        return false;
    if (p_176567_5_ == D::NORTH_SOUTH && (direction == D::EAST_WEST || direction == D::ASCENDING_EAST || direction == D::ASCENDING_WEST))
        return false;
    if (!iblockstate->getValue(POWERED))
        return false;
    return worldIn->isBlockPowered(p_176567_2_) ? true : func_176566_a(worldIn, p_176567_2_, iblockstate, p_176567_3_, distance + 1);
}

void BlockRailPowered::onNeighborChangedInternal(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    const bool flag = state->getValue(POWERED);
    const bool flag1 = worldIn->isBlockPowered(pos) || func_176566_a(worldIn, pos, state, true, 0) || func_176566_a(worldIn, pos, state, false, 0);
    if (flag1 != flag) {
        worldIn->setBlockState(pos, state->withProperty(POWERED, flag1), 3);
        worldIn->notifyNeighborsOfStateChange(pos.down(), this);
        if (isAscending(state->getValue(SHAPE)))
            worldIn->notifyNeighborsOfStateChange(pos.up(), this);
    }
}

// --- BlockRailDetector ---

void BlockRailDetector::onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity*)
{
    if (!worldIn->isRemote) {
        if (!state->getValue(POWERED))
            updatePoweredState(worldIn, pos, state);
    }
}

void BlockRailDetector::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    if (!worldIn->isRemote && state->getValue(POWERED))
        updatePoweredState(worldIn, pos, state);
}

void BlockRailDetector::updatePoweredState(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const bool flag = state->getValue(POWERED);
    // Java: !findMinecarts(worldIn, pos, EntityMinecart.class).isEmpty() over getDectectionBox(pos) -- minecarts
    // come with stage 10.
    [[maybe_unused]] const AxisAlignedBB axisalignedbb = getDectectionBox(pos);
    const bool flag1 = false;
    if (flag1 && !flag) {
        worldIn->setBlockState(pos, state->withProperty(POWERED, true), 3);
        worldIn->notifyNeighborsOfStateChange(pos, this);
        worldIn->notifyNeighborsOfStateChange(pos.down(), this);
        worldIn->markBlockRangeForRenderUpdate(pos, pos);
    }
    if (!flag1 && flag) {
        worldIn->setBlockState(pos, state->withProperty(POWERED, false), 3);
        worldIn->notifyNeighborsOfStateChange(pos, this);
        worldIn->notifyNeighborsOfStateChange(pos.down(), this);
        worldIn->markBlockRangeForRenderUpdate(pos, pos);
    }
    if (flag1)
        worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
    worldIn->updateComparatorOutputLevel(pos, this);
}

void BlockRailDetector::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    BlockRailBase::onBlockAdded(worldIn, pos, state);
    updatePoweredState(worldIn, pos, state);
}

int BlockRailDetector::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    if (worldIn->getBlockState(pos)->getValue(POWERED)) {
        // Java: the success count of a command block minecart on the rail, else the fill level of a minecart
        // with an inventory (Container.calcRedstoneFromInventory) -- minecarts come with stage 10.
    }
    return 0;
}

AxisAlignedBB BlockRailDetector::getDectectionBox(const BlockPos& pos) const
{
    return AxisAlignedBB(static_cast<float>(pos.getX()) + 0.2f, pos.getY(), static_cast<float>(pos.getZ()) + 0.2f, static_cast<float>(pos.getX() + 1) - 0.2f,
        static_cast<float>(pos.getY() + 1) - 0.2f, static_cast<float>(pos.getZ() + 1) - 0.2f);
}

} // namespace mc
