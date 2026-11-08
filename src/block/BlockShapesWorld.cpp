// The world callbacks of the shaped blocks: BlockDoor.java, BlockFence.java, BlockFenceGate.java, BlockTrapDoor.java,
// BlockWall.java, BlockPane.java, BlockLadder.java, BlockCarpet.java, BlockCake.java, BlockStairs.java,
// BlockSlab.java and BlockBed.java.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"
#include "entity/Entity.h"

#include <algorithm>
#include <array>
#include <optional>

namespace mc {

// --- BlockDoor ---

bool BlockDoor::isPassable(IBlockAccess* worldIn, const BlockPos& pos)
{
    return isOpen(combineMetadata(worldIn, pos));
}

AxisAlignedBB BlockDoor::getSelectedBoundingBox(World* worldIn, const BlockPos& pos)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::getSelectedBoundingBox(worldIn, pos);
}

std::optional<AxisAlignedBB> BlockDoor::getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::getCollisionBoundingBox(worldIn, pos, state);
}

void BlockDoor::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    setBoundBasedOnMeta(combineMetadata(worldIn, pos));
}

void BlockDoor::setBoundBasedOnMeta(int combinedMeta)
{
    const float f = 0.1875f;
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f);
    const EnumFacing enumfacing = getFacing(combinedMeta);
    const bool flag = isOpen(combinedMeta);
    const bool flag1 = isHingeLeft(combinedMeta);
    if (flag) {
        if (enumfacing == EnumFacing::EAST) {
            if (!flag1)
                setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
            else
                setBlockBounds(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
        } else if (enumfacing == EnumFacing::SOUTH) {
            if (!flag1)
                setBlockBounds(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            else
                setBlockBounds(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
        } else if (enumfacing == EnumFacing::WEST) {
            if (!flag1)
                setBlockBounds(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
            else
                setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
        } else if (enumfacing == EnumFacing::NORTH) {
            if (!flag1)
                setBlockBounds(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
            else
                setBlockBounds(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        }
    } else if (enumfacing == EnumFacing::EAST) {
        setBlockBounds(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
    } else if (enumfacing == EnumFacing::SOUTH) {
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
    } else if (enumfacing == EnumFacing::WEST) {
        setBlockBounds(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    } else if (enumfacing == EnumFacing::NORTH) {
        setBlockBounds(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
    }
}

bool BlockDoor::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing, float, float, float)
{
    if (&getMaterial() == &Material::iron)
        return true;
    const BlockPos blockpos = state->getValue(HALF) == EnumDoorHalf::LOWER ? pos : pos.down();
    const IBlockState* iblockstate = pos == blockpos ? state : worldIn->getBlockState(blockpos);
    if (iblockstate->getBlock() != this)
        return false;
    state = iblockstate->cycleProperty(OPEN);
    worldIn->setBlockState(blockpos, state, 2);
    worldIn->markBlockRangeForRenderUpdate(blockpos, pos);
    worldIn->playAuxSFXAtEntity(playerIn, state->getValue(OPEN) ? 1003 : 1006, pos, 0);
    return true;
}

void BlockDoor::toggleDoor(World* worldIn, const BlockPos& pos, bool open)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() == this) {
        const BlockPos blockpos = iblockstate->getValue(HALF) == EnumDoorHalf::LOWER ? pos : pos.down();
        const IBlockState* iblockstate1 = pos == blockpos ? iblockstate : worldIn->getBlockState(blockpos);
        if (iblockstate1->getBlock() == this && iblockstate1->getValue(OPEN) != open) {
            worldIn->setBlockState(blockpos, iblockstate1->withProperty(OPEN, open), 2);
            worldIn->markBlockRangeForRenderUpdate(blockpos, pos);
            worldIn->playAuxSFXAtEntity(nullptr, open ? 1003 : 1006, pos, 0);
        }
    }
}

void BlockDoor::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    if (state->getValue(HALF) == EnumDoorHalf::UPPER) {
        const BlockPos blockpos = pos.down();
        const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
        if (iblockstate->getBlock() != this)
            worldIn->setBlockToAir(pos);
        else if (neighborBlock != this)
            onNeighborBlockChange(worldIn, blockpos, iblockstate, neighborBlock);
    } else {
        bool flag1 = false;
        const BlockPos blockpos1 = pos.up();
        const IBlockState* iblockstate1 = worldIn->getBlockState(blockpos1);
        if (iblockstate1->getBlock() != this) {
            worldIn->setBlockToAir(pos);
            flag1 = true;
        }
        if (!World::doesBlockHaveSolidTopSurface(worldIn, pos.down())) {
            worldIn->setBlockToAir(pos);
            flag1 = true;
            if (iblockstate1->getBlock() == this)
                worldIn->setBlockToAir(blockpos1);
        }
        if (flag1) {
            if (!worldIn->isRemote)
                dropBlockAsItem(worldIn, pos, state, 0);
        } else {
            const bool flag = worldIn->isBlockPowered(pos) || worldIn->isBlockPowered(blockpos1);
            if ((flag || neighborBlock->canProvidePower()) && neighborBlock != this && flag != iblockstate1->getValue(POWERED)) {
                worldIn->setBlockState(blockpos1, iblockstate1->withProperty(POWERED, flag), 2);
                if (flag != state->getValue(OPEN)) {
                    worldIn->setBlockState(pos, state->withProperty(OPEN, flag), 2);
                    worldIn->markBlockRangeForRenderUpdate(pos, pos);
                    worldIn->playAuxSFXAtEntity(nullptr, flag ? 1003 : 1006, pos, 0);
                }
            }
        }
    }
}

std::optional<MovingObjectPosition> BlockDoor::collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::collisionRayTrace(worldIn, pos, start, end);
}

bool BlockDoor::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return pos.getY() >= 255 ? false
                             : World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) && Block::canPlaceBlockAt(worldIn, pos) && Block::canPlaceBlockAt(worldIn, pos.up());
}

int BlockDoor::combineMetadata(IBlockAccess* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const int i = iblockstate->getBlock()->getMetaFromState(iblockstate);
    const bool flag = isTop(i);
    const IBlockState* iblockstate1 = worldIn->getBlockState(pos.down());
    const int j = iblockstate1->getBlock()->getMetaFromState(iblockstate1);
    const int k = flag ? j : i;
    const IBlockState* iblockstate2 = worldIn->getBlockState(pos.up());
    const int l = iblockstate2->getBlock()->getMetaFromState(iblockstate2);
    const int i1 = flag ? i : l;
    const bool flag1 = (i1 & 1) != 0;
    const bool flag2 = (i1 & 2) != 0;
    return removeHalfBit(k) | (flag ? 8 : 0) | (flag1 ? 16 : 0) | (flag2 ? 32 : 0);
}

const IBlockState* BlockDoor::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    if (state->getValue(HALF) == EnumDoorHalf::LOWER) {
        const IBlockState* iblockstate = worldIn->getBlockState(pos.up());
        if (iblockstate->getBlock() == this)
            state = state->withProperty(HINGE, iblockstate->getValue(HINGE))->withProperty(POWERED, iblockstate->getValue(POWERED));
    } else {
        const IBlockState* iblockstate1 = worldIn->getBlockState(pos.down());
        if (iblockstate1->getBlock() == this)
            state = state->withProperty(FACING, iblockstate1->getValue(FACING))->withProperty(OPEN, iblockstate1->getValue(OPEN));
    }
    return state;
}

bool BlockDoor::isOpen(IBlockAccess* worldIn, const BlockPos& pos)
{
    return isOpen(combineMetadata(worldIn, pos));
}

EnumFacing BlockDoor::getFacing(IBlockAccess* worldIn, const BlockPos& pos)
{
    return getFacing(combineMetadata(worldIn, pos));
}

// --- BlockFence ---

void BlockFence::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    const bool flag = canConnectTo(worldIn, pos.north());
    const bool flag1 = canConnectTo(worldIn, pos.south());
    const bool flag2 = canConnectTo(worldIn, pos.west());
    const bool flag3 = canConnectTo(worldIn, pos.east());
    float f = 0.375f;
    float f1 = 0.625f;
    float f2 = 0.375f;
    float f3 = 0.625f;
    if (flag)
        f2 = 0.0f;
    if (flag1)
        f3 = 1.0f;
    if (flag || flag1) {
        setBlockBounds(f, 0.0f, f2, f1, 1.5f, f3);
        Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    }
    f2 = 0.375f;
    f3 = 0.625f;
    if (flag2)
        f = 0.0f;
    if (flag3)
        f1 = 1.0f;
    if (flag2 || flag3 || (!flag && !flag1)) {
        setBlockBounds(f, 0.0f, f2, f1, 1.5f, f3);
        Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    }
    if (flag)
        f2 = 0.0f;
    if (flag1)
        f3 = 1.0f;
    setBlockBounds(f, 0.0f, f2, f1, 1.0f, f3);
}

void BlockFence::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const bool flag = canConnectTo(worldIn, pos.north());
    const bool flag1 = canConnectTo(worldIn, pos.south());
    const bool flag2 = canConnectTo(worldIn, pos.west());
    const bool flag3 = canConnectTo(worldIn, pos.east());
    float f = 0.375f;
    float f1 = 0.625f;
    float f2 = 0.375f;
    float f3 = 0.625f;
    if (flag)
        f2 = 0.0f;
    if (flag1)
        f3 = 1.0f;
    if (flag2)
        f = 0.0f;
    if (flag3)
        f1 = 1.0f;
    setBlockBounds(f, 0.0f, f2, f1, 1.0f, f3);
}

bool BlockFence::canConnectTo(IBlockAccess* worldIn, const BlockPos& pos)
{
    const Block* block = worldIn->getBlockState(pos)->getBlock();
    if (block == Blocks::barrier)
        return false;
    if ((!dynamic_cast<const BlockFence*>(block) || &block->getMaterial() != &getMaterial()) && !dynamic_cast<const BlockFenceGate*>(block))
        return block->getMaterial().isOpaque() && block->isFullCube() ? &block->getMaterial() != &Material::gourd : false;
    return true;
}

const IBlockState* BlockFence::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    return state->withProperty(NORTH, canConnectTo(worldIn, pos.north()))
        ->withProperty(EAST, canConnectTo(worldIn, pos.east()))
        ->withProperty(SOUTH, canConnectTo(worldIn, pos.south()))
        ->withProperty(WEST, canConnectTo(worldIn, pos.west()));
}

// --- BlockFenceGate ---

const IBlockState* BlockFenceGate::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    const Axis axis = getAxis(state->getValue(FACING));
    if ((axis == Axis::Z
            && (worldIn->getBlockState(pos.west())->getBlock() == Blocks::cobblestone_wall || worldIn->getBlockState(pos.east())->getBlock() == Blocks::cobblestone_wall))
        || (axis == Axis::X
            && (worldIn->getBlockState(pos.north())->getBlock() == Blocks::cobblestone_wall || worldIn->getBlockState(pos.south())->getBlock() == Blocks::cobblestone_wall)))
        state = state->withProperty(IN_WALL, true);
    return state;
}

bool BlockFenceGate::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos.down())->getBlock()->getMaterial().isSolid() ? BlockDirectional::canPlaceBlockAt(worldIn, pos) : false;
}

std::optional<AxisAlignedBB> BlockFenceGate::getCollisionBoundingBox(World*, const BlockPos& pos, const IBlockState* state)
{
    if (state->getValue(OPEN))
        return std::nullopt;
    const Axis axis = getAxis(state->getValue(FACING));
    return axis == Axis::Z ? AxisAlignedBB(pos.getX(), pos.getY(), static_cast<float>(pos.getZ()) + 0.375f, pos.getX() + 1, static_cast<float>(pos.getY()) + 1.5f,
                                 static_cast<float>(pos.getZ()) + 0.625f)
                           : AxisAlignedBB(static_cast<float>(pos.getX()) + 0.375f, pos.getY(), pos.getZ(), static_cast<float>(pos.getX()) + 0.625f,
                                 static_cast<float>(pos.getY()) + 1.5f, pos.getZ() + 1);
}

void BlockFenceGate::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    if (getAxis(worldIn->getBlockState(pos)->getValue(FACING)) == Axis::Z)
        setBlockBounds(0.0f, 0.0f, 0.375f, 1.0f, 1.0f, 0.625f);
    else
        setBlockBounds(0.375f, 0.0f, 0.0f, 0.625f, 1.0f, 1.0f);
}

bool BlockFenceGate::isPassable(IBlockAccess* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos)->getValue(OPEN);
}

const IBlockState* BlockFenceGate::onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, placerHorizontalFacing(placer))->withProperty(OPEN, false)->withProperty(POWERED, false)->withProperty(IN_WALL, false);
}

bool BlockFenceGate::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing, float, float, float)
{
    if (state->getValue(OPEN)) {
        state = state->withProperty(OPEN, false);
        worldIn->setBlockState(pos, state, 2);
    } else {
        // Java: EnumFacing.fromAngle(playerIn.rotationYaw) -- without the player (stage 8) the gate keeps its facing.
        if (const Entity* player = playerEntity(playerIn)) {
            const EnumFacing enumfacing = fromAngle(player->rotationYaw);
            if (state->getValue(FACING) == getOpposite(enumfacing))
                state = state->withProperty(FACING, enumfacing);
        }
        state = state->withProperty(OPEN, true);
        worldIn->setBlockState(pos, state, 2);
    }
    worldIn->playAuxSFXAtEntity(playerIn, state->getValue(OPEN) ? 1003 : 1006, pos, 0);
    return true;
}

void BlockFenceGate::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    if (!worldIn->isRemote) {
        const bool flag = worldIn->isBlockPowered(pos);
        if (flag || neighborBlock->canProvidePower()) {
            if (flag && !state->getValue(OPEN) && !state->getValue(POWERED)) {
                worldIn->setBlockState(pos, state->withProperty(OPEN, true)->withProperty(POWERED, true), 2);
                worldIn->playAuxSFXAtEntity(nullptr, 1003, pos, 0);
            } else if (!flag && state->getValue(OPEN) && state->getValue(POWERED)) {
                worldIn->setBlockState(pos, state->withProperty(OPEN, false)->withProperty(POWERED, false), 2);
                worldIn->playAuxSFXAtEntity(nullptr, 1006, pos, 0);
            } else if (flag != state->getValue(POWERED)) {
                worldIn->setBlockState(pos, state->withProperty(POWERED, flag), 2);
            }
        }
    }
}

// --- BlockTrapDoor ---

bool BlockTrapDoor::isPassable(IBlockAccess* worldIn, const BlockPos& pos)
{
    return !worldIn->getBlockState(pos)->getValue(OPEN);
}

AxisAlignedBB BlockTrapDoor::getSelectedBoundingBox(World* worldIn, const BlockPos& pos)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::getSelectedBoundingBox(worldIn, pos);
}

std::optional<AxisAlignedBB> BlockTrapDoor::getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::getCollisionBoundingBox(worldIn, pos, state);
}

void BlockTrapDoor::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    setBounds(worldIn->getBlockState(pos));
}

void BlockTrapDoor::setBounds(const IBlockState* state)
{
    if (state->getBlock() == this) {
        const bool flag = state->getValue(HALF) == DoorHalf::TOP;
        const bool obool = state->getValue(OPEN);
        const EnumFacing enumfacing = state->getValue(FACING);
        if (flag)
            setBlockBounds(0.0f, 0.8125f, 0.0f, 1.0f, 1.0f, 1.0f);
        else
            setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.1875f, 1.0f);
        if (obool) {
            if (enumfacing == EnumFacing::NORTH)
                setBlockBounds(0.0f, 0.0f, 0.8125f, 1.0f, 1.0f, 1.0f);
            if (enumfacing == EnumFacing::SOUTH)
                setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.1875f);
            if (enumfacing == EnumFacing::WEST)
                setBlockBounds(0.8125f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            if (enumfacing == EnumFacing::EAST)
                setBlockBounds(0.0f, 0.0f, 0.0f, 0.1875f, 1.0f, 1.0f);
        }
    }
}

bool BlockTrapDoor::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing, float, float, float)
{
    if (&getMaterial() == &Material::iron)
        return true;
    state = state->cycleProperty(OPEN);
    worldIn->setBlockState(pos, state, 2);
    worldIn->playAuxSFXAtEntity(playerIn, state->getValue(OPEN) ? 1003 : 1006, pos, 0);
    return true;
}

void BlockTrapDoor::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    if (!worldIn->isRemote) {
        const BlockPos blockpos = pos.offset(getOpposite(state->getValue(FACING)));
        if (!isValidSupportBlock(worldIn->getBlockState(blockpos)->getBlock())) {
            worldIn->setBlockToAir(pos);
            dropBlockAsItem(worldIn, pos, state, 0);
        } else {
            const bool flag = worldIn->isBlockPowered(pos);
            if (flag || neighborBlock->canProvidePower()) {
                const bool flag1 = state->getValue(OPEN);
                if (flag1 != flag) {
                    worldIn->setBlockState(pos, state->withProperty(OPEN, flag), 2);
                    worldIn->playAuxSFXAtEntity(nullptr, flag ? 1003 : 1006, pos, 0);
                }
            }
        }
    }
}

std::optional<MovingObjectPosition> BlockTrapDoor::collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::collisionRayTrace(worldIn, pos, start, end);
}

const IBlockState* BlockTrapDoor::onBlockPlaced(World*, const BlockPos&, EnumFacing facing, float, float hitY, float, int, EntityLivingBase*)
{
    const IBlockState* iblockstate = getDefaultState();
    if (isHorizontal(getAxis(facing))) {
        iblockstate = iblockstate->withProperty(FACING, facing)->withProperty(OPEN, false);
        iblockstate = iblockstate->withProperty(HALF, hitY > 0.5f ? DoorHalf::TOP : DoorHalf::BOTTOM);
    }
    return iblockstate;
}

bool BlockTrapDoor::canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side)
{
    return !isVertical(getAxis(side)) && isValidSupportBlock(worldIn->getBlockState(pos.offset(getOpposite(side)))->getBlock());
}

bool BlockTrapDoor::isValidSupportBlock(const Block* blockIn)
{
    return (blockIn->getMaterial().isOpaque() && blockIn->isFullCube()) || blockIn == Blocks::glowstone || dynamic_cast<const BlockSlab*>(blockIn)
        || dynamic_cast<const BlockStairs*>(blockIn);
}

// --- BlockWall ---

void BlockWall::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const bool flag = canConnectTo(worldIn, pos.north());
    const bool flag1 = canConnectTo(worldIn, pos.south());
    const bool flag2 = canConnectTo(worldIn, pos.west());
    const bool flag3 = canConnectTo(worldIn, pos.east());
    float f = 0.25f;
    float f1 = 0.75f;
    float f2 = 0.25f;
    float f3 = 0.75f;
    float f4 = 1.0f;
    if (flag)
        f2 = 0.0f;
    if (flag1)
        f3 = 1.0f;
    if (flag2)
        f = 0.0f;
    if (flag3)
        f1 = 1.0f;
    if (flag && flag1 && !flag2 && !flag3) {
        f4 = 0.8125f;
        f = 0.3125f;
        f1 = 0.6875f;
    } else if (!flag && !flag1 && flag2 && flag3) {
        f4 = 0.8125f;
        f2 = 0.3125f;
        f3 = 0.6875f;
    }
    setBlockBounds(f, 0.0f, f2, f1, f4, f3);
}

std::optional<AxisAlignedBB> BlockWall::getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    maxY = 1.5;
    return Block::getCollisionBoundingBox(worldIn, pos, state);
}

bool BlockWall::canConnectTo(IBlockAccess* worldIn, const BlockPos& pos)
{
    const Block* block = worldIn->getBlockState(pos)->getBlock();
    if (block == Blocks::barrier)
        return false;
    if (block != this && !dynamic_cast<const BlockFenceGate*>(block))
        return block->getMaterial().isOpaque() && block->isFullCube() ? &block->getMaterial() != &Material::gourd : false;
    return true;
}

bool BlockWall::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    return side == EnumFacing::DOWN ? Block::shouldSideBeRendered(worldIn, pos, side) : true;
}

const IBlockState* BlockWall::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    return state->withProperty(UP, !worldIn->isAirBlock(pos.up()))
        ->withProperty(NORTH, canConnectTo(worldIn, pos.north()))
        ->withProperty(EAST, canConnectTo(worldIn, pos.east()))
        ->withProperty(SOUTH, canConnectTo(worldIn, pos.south()))
        ->withProperty(WEST, canConnectTo(worldIn, pos.west()));
}

// --- BlockPane ---

const IBlockState* BlockPane::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    return state->withProperty(NORTH, canPaneConnectToBlock(worldIn->getBlockState(pos.north())->getBlock()))
        ->withProperty(SOUTH, canPaneConnectToBlock(worldIn->getBlockState(pos.south())->getBlock()))
        ->withProperty(WEST, canPaneConnectToBlock(worldIn->getBlockState(pos.west())->getBlock()))
        ->withProperty(EAST, canPaneConnectToBlock(worldIn->getBlockState(pos.east())->getBlock()));
}

bool BlockPane::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    return worldIn->getBlockState(pos)->getBlock() == this ? false : Block::shouldSideBeRendered(worldIn, pos, side);
}

void BlockPane::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    const bool flag = canPaneConnectToBlock(worldIn->getBlockState(pos.north())->getBlock());
    const bool flag1 = canPaneConnectToBlock(worldIn->getBlockState(pos.south())->getBlock());
    const bool flag2 = canPaneConnectToBlock(worldIn->getBlockState(pos.west())->getBlock());
    const bool flag3 = canPaneConnectToBlock(worldIn->getBlockState(pos.east())->getBlock());
    if ((!flag2 || !flag3) && (flag2 || flag3 || flag || flag1)) {
        if (flag2) {
            setBlockBounds(0.0f, 0.0f, 0.4375f, 0.5f, 1.0f, 0.5625f);
            Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
        } else if (flag3) {
            setBlockBounds(0.5f, 0.0f, 0.4375f, 1.0f, 1.0f, 0.5625f);
            Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
        }
    } else {
        setBlockBounds(0.0f, 0.0f, 0.4375f, 1.0f, 1.0f, 0.5625f);
        Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    }
    if ((!flag || !flag1) && (flag2 || flag3 || flag || flag1)) {
        if (flag) {
            setBlockBounds(0.4375f, 0.0f, 0.0f, 0.5625f, 1.0f, 0.5f);
            Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
        } else if (flag1) {
            setBlockBounds(0.4375f, 0.0f, 0.5f, 0.5625f, 1.0f, 1.0f);
            Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
        }
    } else {
        setBlockBounds(0.4375f, 0.0f, 0.0f, 0.5625f, 1.0f, 1.0f);
        Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    }
}

void BlockPane::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    float f = 0.4375f;
    float f1 = 0.5625f;
    float f2 = 0.4375f;
    float f3 = 0.5625f;
    const bool flag = canPaneConnectToBlock(worldIn->getBlockState(pos.north())->getBlock());
    const bool flag1 = canPaneConnectToBlock(worldIn->getBlockState(pos.south())->getBlock());
    const bool flag2 = canPaneConnectToBlock(worldIn->getBlockState(pos.west())->getBlock());
    const bool flag3 = canPaneConnectToBlock(worldIn->getBlockState(pos.east())->getBlock());
    if ((!flag2 || !flag3) && (flag2 || flag3 || flag || flag1)) {
        if (flag2)
            f = 0.0f;
        else if (flag3)
            f1 = 1.0f;
    } else {
        f = 0.0f;
        f1 = 1.0f;
    }
    if ((!flag || !flag1) && (flag2 || flag3 || flag || flag1)) {
        if (flag)
            f2 = 0.0f;
        else if (flag1)
            f3 = 1.0f;
    } else {
        f2 = 0.0f;
        f3 = 1.0f;
    }
    setBlockBounds(f, 0.0f, f2, f1, 1.0f, f3);
}

void BlockStainedGlassPane::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (!worldIn->isRemote)
        BlockBeacon::updateColorAsync(worldIn, pos);
}

void BlockStainedGlassPane::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (!worldIn->isRemote)
        BlockBeacon::updateColorAsync(worldIn, pos);
}

bool BlockPane::canPaneConnectToBlock(const Block* blockIn) const
{
    return blockIn->isFullBlock() || blockIn == this || blockIn == Blocks::glass || blockIn == Blocks::stained_glass || blockIn == Blocks::stained_glass_pane
        || dynamic_cast<const BlockPane*>(blockIn);
}

// --- BlockLadder ---

std::optional<AxisAlignedBB> BlockLadder::getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::getCollisionBoundingBox(worldIn, pos, state);
}

AxisAlignedBB BlockLadder::getSelectedBoundingBox(World* worldIn, const BlockPos& pos)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::getSelectedBoundingBox(worldIn, pos);
}

void BlockLadder::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() == this) {
        const float f = 0.125f;
        switch (iblockstate->getValue(FACING)) {
        case EnumFacing::NORTH: setBlockBounds(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f); break;
        case EnumFacing::SOUTH: setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f); break;
        case EnumFacing::WEST: setBlockBounds(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f); break;
        default: setBlockBounds(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f); break;
        }
    }
}

bool BlockLadder::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos.west())->getBlock()->isNormalCube()
        ? true
        : (worldIn->getBlockState(pos.east())->getBlock()->isNormalCube()
                  ? true
                  : (worldIn->getBlockState(pos.north())->getBlock()->isNormalCube() ? true : worldIn->getBlockState(pos.south())->getBlock()->isNormalCube()));
}

const IBlockState* BlockLadder::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float, float, float, int, EntityLivingBase*)
{
    if (isHorizontal(getAxis(facing)) && canBlockStay(worldIn, pos, facing))
        return getDefaultState()->withProperty(FACING, facing);
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (canBlockStay(worldIn, pos, enumfacing))
            return getDefaultState()->withProperty(FACING, enumfacing);
    }
    return getDefaultState();
}

void BlockLadder::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    if (!canBlockStay(worldIn, pos, enumfacing)) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
    Block::onNeighborBlockChange(worldIn, pos, state, neighborBlock);
}

bool BlockLadder::canBlockStay(World* worldIn, const BlockPos& pos, EnumFacing facing)
{
    return worldIn->getBlockState(pos.offset(getOpposite(facing)))->getBlock()->isNormalCube();
}

// --- BlockCarpet ---

bool BlockCarpet::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return Block::canPlaceBlockAt(worldIn, pos) && canBlockStay(worldIn, pos);
}

void BlockCarpet::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    checkForDrop(worldIn, pos, state);
}

bool BlockCarpet::checkForDrop(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!canBlockStay(worldIn, pos)) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
        return false;
    }
    return true;
}

bool BlockCarpet::canBlockStay(World* worldIn, const BlockPos& pos)
{
    return !worldIn->isAirBlock(pos.down());
}

bool BlockCarpet::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    return side == EnumFacing::UP ? true : Block::shouldSideBeRendered(worldIn, pos, side);
}

// --- BlockCake ---

void BlockCake::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const float f = 0.0625f;
    const float f1 = static_cast<float>(1 + worldIn->getBlockState(pos)->getValue(BITES) * 2) / 16.0f;
    const float f2 = 0.5f;
    setBlockBounds(f1, 0.0f, f, 1.0f - f, f2, 1.0f - f);
}

std::optional<AxisAlignedBB> BlockCake::getCollisionBoundingBox(World*, const BlockPos& pos, const IBlockState* state)
{
    const float f = 0.0625f;
    const float f1 = static_cast<float>(1 + state->getValue(BITES) * 2) / 16.0f;
    const float f2 = 0.5f;
    return AxisAlignedBB(static_cast<float>(pos.getX()) + f1, pos.getY(), static_cast<float>(pos.getZ()) + f, static_cast<float>(pos.getX() + 1) - f,
        static_cast<float>(pos.getY()) + f2, static_cast<float>(pos.getZ() + 1) - f);
}

AxisAlignedBB BlockCake::getSelectedBoundingBox(World* worldIn, const BlockPos& pos)
{
    return *getCollisionBoundingBox(worldIn, pos, worldIn->getBlockState(pos));
}

bool BlockCake::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return Block::canPlaceBlockAt(worldIn, pos) ? canBlockStay(worldIn, pos) : false;
}

void BlockCake::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState*, Block*)
{
    if (!canBlockStay(worldIn, pos))
        worldIn->setBlockToAir(pos);
}

bool BlockCake::canBlockStay(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos.down())->getBlock()->getMaterial().isSolid();
}

int BlockCake::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    return (7 - worldIn->getBlockState(pos)->getValue(BITES)) * 2;
}

// --- BlockStairs ---

namespace {

// Java: field_150150_a -- the octants each facing/half combination leaves out of the ray trace.
constexpr std::array<std::array<int, 2>, 8> kSkippedOctants { { { 4, 5 }, { 5, 7 }, { 6, 7 }, { 4, 6 }, { 0, 1 }, { 1, 3 }, { 2, 3 }, { 0, 2 } } };

} // namespace

void BlockStairs::setBlockBoundsBasedOnState(IBlockAccess*, const BlockPos&)
{
    if (hasRaytraced) {
        setBlockBounds(0.5f * static_cast<float>(rayTracePass % 2), 0.5f * static_cast<float>(rayTracePass / 4 % 2), 0.5f * static_cast<float>(rayTracePass / 2 % 2),
            0.5f + 0.5f * static_cast<float>(rayTracePass % 2), 0.5f + 0.5f * static_cast<float>(rayTracePass / 4 % 2),
            0.5f + 0.5f * static_cast<float>(rayTracePass / 2 % 2));
    } else {
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }
}

void BlockStairs::setBaseCollisionBounds(IBlockAccess* worldIn, const BlockPos& pos)
{
    if (worldIn->getBlockState(pos)->getValue(HALF) == EnumHalf::TOP)
        setBlockBounds(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);
    else
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
}

bool BlockStairs::isBlockStairs(const Block* blockIn)
{
    return dynamic_cast<const BlockStairs*>(blockIn) != nullptr;
}

bool BlockStairs::isSameStair(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const Block* block = iblockstate->getBlock();
    return isBlockStairs(block) && iblockstate->getValue(HALF) == state->getValue(HALF) && iblockstate->getValue(FACING) == state->getValue(FACING);
}

int BlockStairs::func_176307_f(IBlockAccess* blockAccess, const BlockPos& pos)
{
    const IBlockState* iblockstate = blockAccess->getBlockState(pos);
    const EnumFacing enumfacing = iblockstate->getValue(FACING);
    const EnumHalf half = iblockstate->getValue(HALF);
    const bool flag = half == EnumHalf::TOP;
    if (enumfacing == EnumFacing::EAST) {
        const IBlockState* iblockstate1 = blockAccess->getBlockState(pos.east());
        if (isBlockStairs(iblockstate1->getBlock()) && half == iblockstate1->getValue(HALF)) {
            const EnumFacing enumfacing1 = iblockstate1->getValue(FACING);
            if (enumfacing1 == EnumFacing::NORTH && !isSameStair(blockAccess, pos.south(), iblockstate))
                return flag ? 1 : 2;
            if (enumfacing1 == EnumFacing::SOUTH && !isSameStair(blockAccess, pos.north(), iblockstate))
                return flag ? 2 : 1;
        }
    } else if (enumfacing == EnumFacing::WEST) {
        const IBlockState* iblockstate2 = blockAccess->getBlockState(pos.west());
        if (isBlockStairs(iblockstate2->getBlock()) && half == iblockstate2->getValue(HALF)) {
            const EnumFacing enumfacing2 = iblockstate2->getValue(FACING);
            if (enumfacing2 == EnumFacing::NORTH && !isSameStair(blockAccess, pos.south(), iblockstate))
                return flag ? 2 : 1;
            if (enumfacing2 == EnumFacing::SOUTH && !isSameStair(blockAccess, pos.north(), iblockstate))
                return flag ? 1 : 2;
        }
    } else if (enumfacing == EnumFacing::SOUTH) {
        const IBlockState* iblockstate3 = blockAccess->getBlockState(pos.south());
        if (isBlockStairs(iblockstate3->getBlock()) && half == iblockstate3->getValue(HALF)) {
            const EnumFacing enumfacing3 = iblockstate3->getValue(FACING);
            if (enumfacing3 == EnumFacing::WEST && !isSameStair(blockAccess, pos.east(), iblockstate))
                return flag ? 2 : 1;
            if (enumfacing3 == EnumFacing::EAST && !isSameStair(blockAccess, pos.west(), iblockstate))
                return flag ? 1 : 2;
        }
    } else if (enumfacing == EnumFacing::NORTH) {
        const IBlockState* iblockstate4 = blockAccess->getBlockState(pos.north());
        if (isBlockStairs(iblockstate4->getBlock()) && half == iblockstate4->getValue(HALF)) {
            const EnumFacing enumfacing4 = iblockstate4->getValue(FACING);
            if (enumfacing4 == EnumFacing::WEST && !isSameStair(blockAccess, pos.east(), iblockstate))
                return flag ? 1 : 2;
            if (enumfacing4 == EnumFacing::EAST && !isSameStair(blockAccess, pos.west(), iblockstate))
                return flag ? 2 : 1;
        }
    }
    return 0;
}

int BlockStairs::func_176305_g(IBlockAccess* blockAccess, const BlockPos& pos)
{
    const IBlockState* iblockstate = blockAccess->getBlockState(pos);
    const EnumFacing enumfacing = iblockstate->getValue(FACING);
    const EnumHalf half = iblockstate->getValue(HALF);
    const bool flag = half == EnumHalf::TOP;
    if (enumfacing == EnumFacing::EAST) {
        const IBlockState* iblockstate1 = blockAccess->getBlockState(pos.west());
        if (isBlockStairs(iblockstate1->getBlock()) && half == iblockstate1->getValue(HALF)) {
            const EnumFacing enumfacing1 = iblockstate1->getValue(FACING);
            if (enumfacing1 == EnumFacing::NORTH && !isSameStair(blockAccess, pos.north(), iblockstate))
                return flag ? 1 : 2;
            if (enumfacing1 == EnumFacing::SOUTH && !isSameStair(blockAccess, pos.south(), iblockstate))
                return flag ? 2 : 1;
        }
    } else if (enumfacing == EnumFacing::WEST) {
        const IBlockState* iblockstate2 = blockAccess->getBlockState(pos.east());
        if (isBlockStairs(iblockstate2->getBlock()) && half == iblockstate2->getValue(HALF)) {
            const EnumFacing enumfacing2 = iblockstate2->getValue(FACING);
            if (enumfacing2 == EnumFacing::NORTH && !isSameStair(blockAccess, pos.north(), iblockstate))
                return flag ? 2 : 1;
            if (enumfacing2 == EnumFacing::SOUTH && !isSameStair(blockAccess, pos.south(), iblockstate))
                return flag ? 1 : 2;
        }
    } else if (enumfacing == EnumFacing::SOUTH) {
        const IBlockState* iblockstate3 = blockAccess->getBlockState(pos.north());
        if (isBlockStairs(iblockstate3->getBlock()) && half == iblockstate3->getValue(HALF)) {
            const EnumFacing enumfacing3 = iblockstate3->getValue(FACING);
            if (enumfacing3 == EnumFacing::WEST && !isSameStair(blockAccess, pos.west(), iblockstate))
                return flag ? 2 : 1;
            if (enumfacing3 == EnumFacing::EAST && !isSameStair(blockAccess, pos.east(), iblockstate))
                return flag ? 1 : 2;
        }
    } else if (enumfacing == EnumFacing::NORTH) {
        const IBlockState* iblockstate4 = blockAccess->getBlockState(pos.south());
        if (isBlockStairs(iblockstate4->getBlock()) && half == iblockstate4->getValue(HALF)) {
            const EnumFacing enumfacing4 = iblockstate4->getValue(FACING);
            if (enumfacing4 == EnumFacing::WEST && !isSameStair(blockAccess, pos.west(), iblockstate))
                return flag ? 1 : 2;
            if (enumfacing4 == EnumFacing::EAST && !isSameStair(blockAccess, pos.east(), iblockstate))
                return flag ? 2 : 1;
        }
    }
    return 0;
}

bool BlockStairs::func_176306_h(IBlockAccess* blockAccess, const BlockPos& pos)
{
    const IBlockState* iblockstate = blockAccess->getBlockState(pos);
    const EnumFacing enumfacing = iblockstate->getValue(FACING);
    const EnumHalf half = iblockstate->getValue(HALF);
    const bool flag = half == EnumHalf::TOP;
    float f = 0.5f;
    float f1 = 1.0f;
    if (flag) {
        f = 0.0f;
        f1 = 0.5f;
    }
    float f2 = 0.0f;
    float f3 = 1.0f;
    float f4 = 0.0f;
    float f5 = 0.5f;
    bool flag1 = true;
    if (enumfacing == EnumFacing::EAST) {
        f2 = 0.5f;
        f5 = 1.0f;
        const IBlockState* iblockstate1 = blockAccess->getBlockState(pos.east());
        if (isBlockStairs(iblockstate1->getBlock()) && half == iblockstate1->getValue(HALF)) {
            const EnumFacing enumfacing1 = iblockstate1->getValue(FACING);
            if (enumfacing1 == EnumFacing::NORTH && !isSameStair(blockAccess, pos.south(), iblockstate)) {
                f5 = 0.5f;
                flag1 = false;
            } else if (enumfacing1 == EnumFacing::SOUTH && !isSameStair(blockAccess, pos.north(), iblockstate)) {
                f4 = 0.5f;
                flag1 = false;
            }
        }
    } else if (enumfacing == EnumFacing::WEST) {
        f3 = 0.5f;
        f5 = 1.0f;
        const IBlockState* iblockstate2 = blockAccess->getBlockState(pos.west());
        if (isBlockStairs(iblockstate2->getBlock()) && half == iblockstate2->getValue(HALF)) {
            const EnumFacing enumfacing2 = iblockstate2->getValue(FACING);
            if (enumfacing2 == EnumFacing::NORTH && !isSameStair(blockAccess, pos.south(), iblockstate)) {
                f5 = 0.5f;
                flag1 = false;
            } else if (enumfacing2 == EnumFacing::SOUTH && !isSameStair(blockAccess, pos.north(), iblockstate)) {
                f4 = 0.5f;
                flag1 = false;
            }
        }
    } else if (enumfacing == EnumFacing::SOUTH) {
        f4 = 0.5f;
        f5 = 1.0f;
        const IBlockState* iblockstate3 = blockAccess->getBlockState(pos.south());
        if (isBlockStairs(iblockstate3->getBlock()) && half == iblockstate3->getValue(HALF)) {
            const EnumFacing enumfacing3 = iblockstate3->getValue(FACING);
            if (enumfacing3 == EnumFacing::WEST && !isSameStair(blockAccess, pos.east(), iblockstate)) {
                f3 = 0.5f;
                flag1 = false;
            } else if (enumfacing3 == EnumFacing::EAST && !isSameStair(blockAccess, pos.west(), iblockstate)) {
                f2 = 0.5f;
                flag1 = false;
            }
        }
    } else if (enumfacing == EnumFacing::NORTH) {
        const IBlockState* iblockstate4 = blockAccess->getBlockState(pos.north());
        if (isBlockStairs(iblockstate4->getBlock()) && half == iblockstate4->getValue(HALF)) {
            const EnumFacing enumfacing4 = iblockstate4->getValue(FACING);
            if (enumfacing4 == EnumFacing::WEST && !isSameStair(blockAccess, pos.east(), iblockstate)) {
                f3 = 0.5f;
                flag1 = false;
            } else if (enumfacing4 == EnumFacing::EAST && !isSameStair(blockAccess, pos.west(), iblockstate)) {
                f2 = 0.5f;
                flag1 = false;
            }
        }
    }
    setBlockBounds(f2, f, f4, f3, f1, f5);
    return flag1;
}

bool BlockStairs::func_176304_i(IBlockAccess* blockAccess, const BlockPos& pos)
{
    const IBlockState* iblockstate = blockAccess->getBlockState(pos);
    const EnumFacing enumfacing = iblockstate->getValue(FACING);
    const EnumHalf half = iblockstate->getValue(HALF);
    const bool flag = half == EnumHalf::TOP;
    float f = 0.5f;
    float f1 = 1.0f;
    if (flag) {
        f = 0.0f;
        f1 = 0.5f;
    }
    float f2 = 0.0f;
    float f3 = 0.5f;
    float f4 = 0.5f;
    float f5 = 1.0f;
    bool flag1 = false;
    if (enumfacing == EnumFacing::EAST) {
        const IBlockState* iblockstate1 = blockAccess->getBlockState(pos.west());
        if (isBlockStairs(iblockstate1->getBlock()) && half == iblockstate1->getValue(HALF)) {
            const EnumFacing enumfacing1 = iblockstate1->getValue(FACING);
            if (enumfacing1 == EnumFacing::NORTH && !isSameStair(blockAccess, pos.north(), iblockstate)) {
                f4 = 0.0f;
                f5 = 0.5f;
                flag1 = true;
            } else if (enumfacing1 == EnumFacing::SOUTH && !isSameStair(blockAccess, pos.south(), iblockstate)) {
                f4 = 0.5f;
                f5 = 1.0f;
                flag1 = true;
            }
        }
    } else if (enumfacing == EnumFacing::WEST) {
        const IBlockState* iblockstate2 = blockAccess->getBlockState(pos.east());
        if (isBlockStairs(iblockstate2->getBlock()) && half == iblockstate2->getValue(HALF)) {
            f2 = 0.5f;
            f3 = 1.0f;
            const EnumFacing enumfacing2 = iblockstate2->getValue(FACING);
            if (enumfacing2 == EnumFacing::NORTH && !isSameStair(blockAccess, pos.north(), iblockstate)) {
                f4 = 0.0f;
                f5 = 0.5f;
                flag1 = true;
            } else if (enumfacing2 == EnumFacing::SOUTH && !isSameStair(blockAccess, pos.south(), iblockstate)) {
                f4 = 0.5f;
                f5 = 1.0f;
                flag1 = true;
            }
        }
    } else if (enumfacing == EnumFacing::SOUTH) {
        const IBlockState* iblockstate3 = blockAccess->getBlockState(pos.north());
        if (isBlockStairs(iblockstate3->getBlock()) && half == iblockstate3->getValue(HALF)) {
            f4 = 0.0f;
            f5 = 0.5f;
            const EnumFacing enumfacing3 = iblockstate3->getValue(FACING);
            if (enumfacing3 == EnumFacing::WEST && !isSameStair(blockAccess, pos.west(), iblockstate)) {
                flag1 = true;
            } else if (enumfacing3 == EnumFacing::EAST && !isSameStair(blockAccess, pos.east(), iblockstate)) {
                f2 = 0.5f;
                f3 = 1.0f;
                flag1 = true;
            }
        }
    } else if (enumfacing == EnumFacing::NORTH) {
        const IBlockState* iblockstate4 = blockAccess->getBlockState(pos.south());
        if (isBlockStairs(iblockstate4->getBlock()) && half == iblockstate4->getValue(HALF)) {
            const EnumFacing enumfacing4 = iblockstate4->getValue(FACING);
            if (enumfacing4 == EnumFacing::WEST && !isSameStair(blockAccess, pos.west(), iblockstate)) {
                flag1 = true;
            } else if (enumfacing4 == EnumFacing::EAST && !isSameStair(blockAccess, pos.east(), iblockstate)) {
                f2 = 0.5f;
                f3 = 1.0f;
                flag1 = true;
            }
        }
    }
    if (flag1)
        setBlockBounds(f2, f, f4, f3, f1, f5);
    return flag1;
}

void BlockStairs::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    setBaseCollisionBounds(worldIn, pos);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    const bool flag = func_176306_h(worldIn, pos);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    if (flag && func_176304_i(worldIn, pos))
        Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void BlockStairs::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    modelBlock->randomDisplayTick(worldIn, pos, state, rand);
}

void BlockStairs::onBlockClicked(World* worldIn, const BlockPos& pos, EntityPlayer* playerIn)
{
    modelBlock->onBlockClicked(worldIn, pos, playerIn);
}

void BlockStairs::onBlockDestroyedByPlayer(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    modelBlock->onBlockDestroyedByPlayer(worldIn, pos, state);
}

int BlockStairs::getMixedBrightnessForBlock(IBlockAccess* worldIn, const BlockPos& pos)
{
    return modelBlock->getMixedBrightnessForBlock(worldIn, pos);
}

int BlockStairs::tickRate(World* worldIn)
{
    return modelBlock->tickRate(worldIn);
}

AxisAlignedBB BlockStairs::getSelectedBoundingBox(World* worldIn, const BlockPos& pos)
{
    return modelBlock->getSelectedBoundingBox(worldIn, pos);
}

Vec3 BlockStairs::modifyAcceleration(World* worldIn, const BlockPos& pos, Entity* entityIn, const Vec3& motion)
{
    return modelBlock->modifyAcceleration(worldIn, pos, entityIn, motion);
}

bool BlockStairs::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return modelBlock->canPlaceBlockAt(worldIn, pos);
}

void BlockStairs::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    onNeighborBlockChange(worldIn, pos, modelState, Blocks::air);
    modelBlock->onBlockAdded(worldIn, pos, modelState);
}

void BlockStairs::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    modelBlock->breakBlock(worldIn, pos, modelState);
}

void BlockStairs::onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, Entity* entityIn)
{
    modelBlock->onEntityCollidedWithBlock(worldIn, pos, entityIn);
}

void BlockStairs::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    modelBlock->updateTick(worldIn, pos, state, rand);
}

bool BlockStairs::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState*, EntityPlayer* playerIn, EnumFacing, float, float, float)
{
    return modelBlock->onBlockActivated(worldIn, pos, modelState, playerIn, EnumFacing::DOWN, 0.0f, 0.0f, 0.0f);
}

void BlockStairs::onBlockDestroyedByExplosion(World* worldIn, const BlockPos& pos, Explosion* explosionIn)
{
    modelBlock->onBlockDestroyedByExplosion(worldIn, pos, explosionIn);
}

const IBlockState* BlockStairs::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
    EntityLivingBase* placer)
{
    const IBlockState* iblockstate = Block::onBlockPlaced(worldIn, pos, facing, hitX, hitY, hitZ, meta, placer);
    iblockstate = iblockstate->withProperty(FACING, placerHorizontalFacing(placer))->withProperty(SHAPE, EnumShape::STRAIGHT);
    return facing != EnumFacing::DOWN && (facing == EnumFacing::UP || static_cast<double>(hitY) <= 0.5) ? iblockstate->withProperty(HALF, EnumHalf::BOTTOM)
                                                                                                       : iblockstate->withProperty(HALF, EnumHalf::TOP);
}

std::optional<MovingObjectPosition> BlockStairs::collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end)
{
    std::array<std::optional<MovingObjectPosition>, 8> amovingobjectposition {};
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const int i = getHorizontalIndex(iblockstate->getValue(FACING));
    const bool flag = iblockstate->getValue(HALF) == EnumHalf::TOP;
    const std::array<int, 2>& aint = kSkippedOctants[static_cast<size_t>(i + (flag ? 4 : 0))];
    hasRaytraced = true;
    for (int j = 0; j < 8; ++j) {
        rayTracePass = j;
        if (!std::binary_search(aint.begin(), aint.end(), j))
            amovingobjectposition[static_cast<size_t>(j)] = Block::collisionRayTrace(worldIn, pos, start, end);
    }
    for (const int k : aint)
        amovingobjectposition[static_cast<size_t>(k)].reset();
    std::optional<MovingObjectPosition> movingobjectposition1;
    double d1 = 0.0;
    for (const std::optional<MovingObjectPosition>& movingobjectposition : amovingobjectposition) {
        if (movingobjectposition) {
            const double d0 = movingobjectposition->hitVec.squareDistanceTo(end);
            if (d0 > d1) {
                movingobjectposition1 = movingobjectposition;
                d1 = d0;
            }
        }
    }
    return movingobjectposition1;
}

const IBlockState* BlockStairs::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    if (func_176306_h(worldIn, pos)) {
        switch (func_176305_g(worldIn, pos)) {
        case 0: state = state->withProperty(SHAPE, EnumShape::STRAIGHT); break;
        case 1: state = state->withProperty(SHAPE, EnumShape::INNER_RIGHT); break;
        case 2: state = state->withProperty(SHAPE, EnumShape::INNER_LEFT); break;
        }
    } else {
        switch (func_176307_f(worldIn, pos)) {
        case 0: state = state->withProperty(SHAPE, EnumShape::STRAIGHT); break;
        case 1: state = state->withProperty(SHAPE, EnumShape::OUTER_RIGHT); break;
        case 2: state = state->withProperty(SHAPE, EnumShape::OUTER_LEFT); break;
        }
    }
    return state;
}

// --- BlockSlab ---

void BlockSlab::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    if (isDouble()) {
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    } else {
        const IBlockState* iblockstate = worldIn->getBlockState(pos);
        if (iblockstate->getBlock() == this) {
            if (iblockstate->getValue(HALF) == EnumBlockHalf::TOP)
                setBlockBounds(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);
            else
                setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
        }
    }
}

void BlockSlab::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
}

const IBlockState* BlockSlab::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
    EntityLivingBase* placer)
{
    const IBlockState* iblockstate = Block::onBlockPlaced(worldIn, pos, facing, hitX, hitY, hitZ, meta, placer)->withProperty(HALF, EnumBlockHalf::BOTTOM);
    return isDouble() ? iblockstate
                      : (facing != EnumFacing::DOWN && (facing == EnumFacing::UP || static_cast<double>(hitY) <= 0.5) ? iblockstate
                                                                                                                    : iblockstate->withProperty(HALF, EnumBlockHalf::TOP));
}

bool BlockSlab::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    if (isDouble())
        return Block::shouldSideBeRendered(worldIn, pos, side);
    if (side != EnumFacing::UP && side != EnumFacing::DOWN && !Block::shouldSideBeRendered(worldIn, pos, side))
        return false;
    const BlockPos blockpos = pos.offset(getOpposite(side));
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const IBlockState* iblockstate1 = worldIn->getBlockState(blockpos);
    const bool flag = isSlab(iblockstate->getBlock()) && iblockstate->getValue(HALF) == EnumBlockHalf::TOP;
    const bool flag1 = isSlab(iblockstate1->getBlock()) && iblockstate1->getValue(HALF) == EnumBlockHalf::TOP;
    return flag1 ? (side == EnumFacing::DOWN ? true
                                             : (side == EnumFacing::UP && Block::shouldSideBeRendered(worldIn, pos, side) ? true : !isSlab(iblockstate->getBlock()) || !flag))
                 : (side == EnumFacing::UP ? true
                                           : (side == EnumFacing::DOWN && Block::shouldSideBeRendered(worldIn, pos, side) ? true : !isSlab(iblockstate->getBlock()) || flag));
}

bool BlockSlab::isSlab(const Block* blockIn)
{
    return blockIn == Blocks::stone_slab || blockIn == Blocks::wooden_slab || blockIn == Blocks::stone_slab2;
}

int BlockSlab::getDamageValue(World* worldIn, const BlockPos& pos)
{
    return Block::getDamageValue(worldIn, pos) & 7;
}

// --- BlockBed ---

void BlockBed::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    if (state->getValue(PART) == EnumPartType::HEAD) {
        if (worldIn->getBlockState(pos.offset(getOpposite(enumfacing)))->getBlock() != this)
            worldIn->setBlockToAir(pos);
    } else if (worldIn->getBlockState(pos.offset(enumfacing))->getBlock() != this) {
        worldIn->setBlockToAir(pos);
        if (!worldIn->isRemote)
            dropBlockAsItem(worldIn, pos, state, 0);
    }
}

std::optional<BlockPos> BlockBed::getSafeExitLocation(World* worldIn, const BlockPos& pos, int tries)
{
    const EnumFacing enumfacing = worldIn->getBlockState(pos)->getValue(FACING);
    const int i = pos.getX();
    const int j = pos.getY();
    const int k = pos.getZ();
    for (int l = 0; l <= 1; ++l) {
        const int i1 = i - getFrontOffsetX(enumfacing) * l - 1;
        const int j1 = k - getFrontOffsetZ(enumfacing) * l - 1;
        const int k1 = i1 + 2;
        const int l1 = j1 + 2;
        for (int i2 = i1; i2 <= k1; ++i2) {
            for (int j2 = j1; j2 <= l1; ++j2) {
                const BlockPos blockpos(i2, j, j2);
                if (hasRoomForPlayer(worldIn, blockpos)) {
                    if (tries <= 0)
                        return blockpos;
                    --tries;
                }
            }
        }
    }
    return std::nullopt;
}

bool BlockBed::hasRoomForPlayer(World* worldIn, const BlockPos& pos)
{
    return World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) && !worldIn->getBlockState(pos)->getBlock()->getMaterial().isSolid()
        && !worldIn->getBlockState(pos.up())->getBlock()->getMaterial().isSolid();
}

void BlockBed::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int)
{
    if (state->getValue(PART) == EnumPartType::FOOT)
        BlockDirectional::dropBlockAsItemWithChance(worldIn, pos, state, chance, 0);
}

const IBlockState* BlockBed::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    if (state->getValue(PART) == EnumPartType::FOOT) {
        const IBlockState* iblockstate = worldIn->getBlockState(pos.offset(state->getValue(FACING)));
        if (iblockstate->getBlock() == this)
            state = state->withProperty(OCCUPIED, iblockstate->getValue(OCCUPIED));
    }
    return state;
}

} // namespace mc
