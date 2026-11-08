// The world callbacks of the pistons: BlockPistonBase.java, BlockPistonExtension.java and BlockPistonMoving.java.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"
#include "block/state/BlockPistonStructureHelper.h"
#include "entity/Entity.h"
#include "tileentity/TileEntityPiston.h"
#include "util/MathHelper.h"
#include "world/border/WorldBorder.h"

#include <vector>

namespace mc {

// --- BlockPistonBase ---

void BlockPistonBase::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack&)
{
    worldIn->setBlockState(pos, state->withProperty(FACING, getFacingFromEntity(worldIn, pos, placer)), 2);
    if (!worldIn->isRemote)
        checkForMove(worldIn, pos, state);
}

void BlockPistonBase::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (!worldIn->isRemote)
        checkForMove(worldIn, pos, state);
}

void BlockPistonBase::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!worldIn->isRemote && worldIn->getTileEntity(pos) == nullptr)
        checkForMove(worldIn, pos, state);
}

const IBlockState* BlockPistonBase::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, getFacingFromEntity(worldIn, pos, placer))->withProperty(EXTENDED, false);
}

void BlockPistonBase::checkForMove(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    const bool flag = shouldBeExtended(worldIn, pos, enumfacing);
    if (flag && !state->getValue(EXTENDED)) {
        if (BlockPistonStructureHelper(worldIn, pos, enumfacing, true).canMove())
            worldIn->addBlockEvent(pos, this, 0, getIndex(enumfacing));
    } else if (!flag && state->getValue(EXTENDED)) {
        worldIn->setBlockState(pos, state->withProperty(EXTENDED, false), 2);
        worldIn->addBlockEvent(pos, this, 1, getIndex(enumfacing));
    }
}

bool BlockPistonBase::shouldBeExtended(World* worldIn, const BlockPos& pos, EnumFacing facing)
{
    for (const EnumFacing enumfacing : kFacings) {
        if (enumfacing != facing && worldIn->isSidePowered(pos.offset(enumfacing), enumfacing))
            return true;
    }
    if (worldIn->isSidePowered(pos, EnumFacing::DOWN))
        return true;
    const BlockPos blockpos = pos.up();
    for (const EnumFacing enumfacing1 : kFacings) {
        if (enumfacing1 != EnumFacing::DOWN && worldIn->isSidePowered(blockpos.offset(enumfacing1), enumfacing1))
            return true;
    }
    return false;
}

bool BlockPistonBase::onBlockEventReceived(World* worldIn, const BlockPos& pos, const IBlockState* state, int eventID, int eventParam)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    if (!worldIn->isRemote) {
        const bool flag = shouldBeExtended(worldIn, pos, enumfacing);
        if (flag && eventID == 1) {
            worldIn->setBlockState(pos, state->withProperty(EXTENDED, true), 2);
            return false;
        }
        if (!flag && eventID == 0)
            return false;
    }
    if (eventID == 0) {
        if (!doMove(worldIn, pos, enumfacing, true))
            return false;
        worldIn->setBlockState(pos, state->withProperty(EXTENDED, true), 2);
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "tile.piston.out", 0.5f, worldIn->rand.nextFloat() * 0.25f + 0.6f);
    } else if (eventID == 1) {
        if (auto* tileentity1 = dynamic_cast<TileEntityPiston*>(worldIn->getTileEntity(pos.offset(enumfacing))))
            tileentity1->clearPistonTileEntity();
        worldIn->setBlockState(pos,
            Blocks::piston_extension->getDefaultState()
                ->withProperty(BlockPistonMoving::FACING, enumfacing)
                ->withProperty(BlockPistonMoving::TYPE, isSticky ? BlockPistonExtension::EnumPistonType::STICKY : BlockPistonExtension::EnumPistonType::DEFAULT),
            3);
        worldIn->setTileEntity(pos, BlockPistonMoving::newTileEntity(getStateFromMeta(eventParam), enumfacing, false, true));
        if (isSticky) {
            const BlockPos blockpos = pos.add(getFrontOffsetX(enumfacing) * 2, getFrontOffsetY(enumfacing) * 2, getFrontOffsetZ(enumfacing) * 2);
            Block* block = worldIn->getBlockState(blockpos)->getBlock();
            bool flag1 = false;
            if (block == Blocks::piston_extension) {
                if (auto* tileentitypiston = dynamic_cast<TileEntityPiston*>(worldIn->getTileEntity(blockpos))) {
                    if (tileentitypiston->getFacing() == enumfacing && tileentitypiston->isExtending()) {
                        tileentitypiston->clearPistonTileEntity();
                        flag1 = true;
                    }
                }
            }
            if (!flag1 && &block->getMaterial() != &Material::air && canPush(block, worldIn, blockpos, getOpposite(enumfacing), false)
                && (block->getMobilityFlag() == 0 || block == Blocks::piston || block == Blocks::sticky_piston))
                doMove(worldIn, pos, enumfacing, false);
        } else {
            worldIn->setBlockToAir(pos.offset(enumfacing));
        }
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "tile.piston.in", 0.5f, worldIn->rand.nextFloat() * 0.15f + 0.6f);
    }
    return true;
}

void BlockPistonBase::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() == this && iblockstate->getValue(EXTENDED)) {
        switch (iblockstate->getValue(FACING)) {
        case EnumFacing::DOWN: setBlockBounds(0.0f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f); break;
        case EnumFacing::UP: setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f); break;
        case EnumFacing::NORTH: setBlockBounds(0.0f, 0.0f, 0.25f, 1.0f, 1.0f, 1.0f); break;
        case EnumFacing::SOUTH: setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.75f); break;
        case EnumFacing::WEST: setBlockBounds(0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f); break;
        case EnumFacing::EAST: setBlockBounds(0.0f, 0.0f, 0.0f, 0.75f, 1.0f, 1.0f); break;
        }
    } else {
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }
}

void BlockPistonBase::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
}

std::optional<AxisAlignedBB> BlockPistonBase::getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return Block::getCollisionBoundingBox(worldIn, pos, state);
}

EnumFacing BlockPistonBase::getFacingFromEntity(World*, const BlockPos& clickedBlock, EntityLivingBase* entityIn)
{
    const Entity* entity = placerEntity(entityIn);
    if (!entity) {
        // Java reads the placer's position, eye height and facing -- the placer comes with stage 8.
        return getOpposite(placerHorizontalFacing(entityIn));
    }
    if (MathHelper::abs(static_cast<float>(entity->posX) - static_cast<float>(clickedBlock.getX())) < 2.0f
        && MathHelper::abs(static_cast<float>(entity->posZ) - static_cast<float>(clickedBlock.getZ())) < 2.0f) {
        const double d0 = entity->posY + static_cast<double>(entity->getEyeHeight());
        if (d0 - clickedBlock.getY() > 2.0)
            return EnumFacing::UP;
        if (clickedBlock.getY() - d0 > 0.0)
            return EnumFacing::DOWN;
    }
    return getOpposite(entity->getHorizontalFacing());
}

bool BlockPistonBase::canPush(Block* blockIn, World* worldIn, const BlockPos& pos, EnumFacing direction, bool allowDestroy)
{
    if (blockIn == Blocks::obsidian)
        return false;
    if (!worldIn->getWorldBorder().contains(pos))
        return false;
    if (pos.getY() >= 0 && (direction != EnumFacing::DOWN || pos.getY() != 0)) {
        if (pos.getY() <= worldIn->getHeight() - 1 && (direction != EnumFacing::UP || pos.getY() != worldIn->getHeight() - 1)) {
            if (blockIn != Blocks::piston && blockIn != Blocks::sticky_piston) {
                if (blockIn->getBlockHardness(worldIn, pos) == -1.0f)
                    return false;
                if (blockIn->getMobilityFlag() == 2)
                    return false;
                if (blockIn->getMobilityFlag() == 1)
                    return allowDestroy;
            } else if (worldIn->getBlockState(pos)->getValue(EXTENDED)) {
                return false;
            }
            // Java: !(blockIn instanceof ITileEntityProvider)
            return !blockIn->hasTileEntity();
        }
        return false;
    }
    return false;
}

bool BlockPistonBase::doMove(World* worldIn, const BlockPos& pos, EnumFacing direction, bool extending)
{
    if (!extending)
        worldIn->setBlockToAir(pos.offset(direction));
    BlockPistonStructureHelper blockpistonstructurehelper(worldIn, pos, direction, extending);
    if (!blockpistonstructurehelper.canMove())
        return false;
    const std::vector<BlockPos>& list = blockpistonstructurehelper.getBlocksToMove();
    const std::vector<BlockPos>& list1 = blockpistonstructurehelper.getBlocksToDestroy();
    int i = static_cast<int>(list.size() + list1.size());
    std::vector<Block*> ablock(static_cast<size_t>(i));
    const EnumFacing enumfacing = extending ? direction : getOpposite(direction);
    for (int j = static_cast<int>(list1.size()) - 1; j >= 0; --j) {
        const BlockPos blockpos = list1[static_cast<size_t>(j)];
        Block* block = worldIn->getBlockState(blockpos)->getBlock();
        block->dropBlockAsItem(worldIn, blockpos, worldIn->getBlockState(blockpos), 0);
        worldIn->setBlockToAir(blockpos);
        --i;
        ablock[static_cast<size_t>(i)] = block;
    }
    for (int k = static_cast<int>(list.size()) - 1; k >= 0; --k) {
        BlockPos blockpos2 = list[static_cast<size_t>(k)];
        const IBlockState* iblockstate = worldIn->getBlockState(blockpos2);
        Block* block1 = iblockstate->getBlock();
        block1->getMetaFromState(iblockstate);
        worldIn->setBlockToAir(blockpos2);
        blockpos2 = blockpos2.offset(enumfacing);
        worldIn->setBlockState(blockpos2, Blocks::piston_extension->getDefaultState()->withProperty(FACING, direction), 4);
        worldIn->setTileEntity(blockpos2, BlockPistonMoving::newTileEntity(iblockstate, direction, extending, false));
        --i;
        ablock[static_cast<size_t>(i)] = block1;
    }
    const BlockPos blockpos1 = pos.offset(direction);
    if (extending) {
        const auto type = isSticky ? BlockPistonExtension::EnumPistonType::STICKY : BlockPistonExtension::EnumPistonType::DEFAULT;
        const IBlockState* iblockstate1 = Blocks::piston_head->getDefaultState()->withProperty(BlockPistonExtension::FACING, direction)->withProperty(BlockPistonExtension::TYPE, type);
        const IBlockState* iblockstate2 =
            Blocks::piston_extension->getDefaultState()->withProperty(BlockPistonMoving::FACING, direction)->withProperty(BlockPistonMoving::TYPE, type);
        worldIn->setBlockState(blockpos1, iblockstate2, 4);
        worldIn->setTileEntity(blockpos1, BlockPistonMoving::newTileEntity(iblockstate1, direction, true, false));
    }
    for (int l = static_cast<int>(list1.size()) - 1; l >= 0; --l)
        worldIn->notifyNeighborsOfStateChange(list1[static_cast<size_t>(l)], ablock[static_cast<size_t>(i++)]);
    for (int i1 = static_cast<int>(list.size()) - 1; i1 >= 0; --i1)
        worldIn->notifyNeighborsOfStateChange(list[static_cast<size_t>(i1)], ablock[static_cast<size_t>(i++)]);
    if (extending) {
        worldIn->notifyNeighborsOfStateChange(blockpos1, Blocks::piston_head);
        worldIn->notifyNeighborsOfStateChange(pos, this);
    }
    return true;
}

// --- BlockPistonExtension ---

void BlockPistonExtension::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    Block::breakBlock(worldIn, pos, state);
    const EnumFacing enumfacing = getOpposite(state->getValue(FACING));
    const BlockPos blockpos = pos.offset(enumfacing);
    const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
    if ((iblockstate->getBlock() == Blocks::piston || iblockstate->getBlock() == Blocks::sticky_piston) && iblockstate->getValue(BlockPistonBase::EXTENDED)) {
        iblockstate->getBlock()->dropBlockAsItem(worldIn, blockpos, iblockstate, 0);
        worldIn->setBlockToAir(blockpos);
    }
}

void BlockPistonExtension::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    applyHeadBounds(state);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    applyCoreBounds(state);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void BlockPistonExtension::applyCoreBounds(const IBlockState* state)
{
    switch (state->getValue(FACING)) {
    case EnumFacing::DOWN: setBlockBounds(0.375f, 0.25f, 0.375f, 0.625f, 1.0f, 0.625f); break;
    case EnumFacing::UP: setBlockBounds(0.375f, 0.0f, 0.375f, 0.625f, 0.75f, 0.625f); break;
    case EnumFacing::NORTH: setBlockBounds(0.25f, 0.375f, 0.25f, 0.75f, 0.625f, 1.0f); break;
    case EnumFacing::SOUTH: setBlockBounds(0.25f, 0.375f, 0.0f, 0.75f, 0.625f, 0.75f); break;
    case EnumFacing::WEST: setBlockBounds(0.375f, 0.25f, 0.25f, 0.625f, 0.75f, 1.0f); break;
    case EnumFacing::EAST: setBlockBounds(0.0f, 0.375f, 0.25f, 0.75f, 0.625f, 0.75f); break;
    }
}

void BlockPistonExtension::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    applyHeadBounds(worldIn->getBlockState(pos));
}

void BlockPistonExtension::applyHeadBounds(const IBlockState* state)
{
    switch (state->getValue(FACING)) {
    case EnumFacing::DOWN: setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f); break;
    case EnumFacing::UP: setBlockBounds(0.0f, 0.75f, 0.0f, 1.0f, 1.0f, 1.0f); break;
    case EnumFacing::NORTH: setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.25f); break;
    case EnumFacing::SOUTH: setBlockBounds(0.0f, 0.0f, 0.75f, 1.0f, 1.0f, 1.0f); break;
    case EnumFacing::WEST: setBlockBounds(0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f); break;
    case EnumFacing::EAST: setBlockBounds(0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f); break;
    }
}

void BlockPistonExtension::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    const BlockPos blockpos = pos.offset(getOpposite(enumfacing));
    const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
    if (iblockstate->getBlock() != Blocks::piston && iblockstate->getBlock() != Blocks::sticky_piston)
        worldIn->setBlockToAir(pos);
    else
        iblockstate->getBlock()->onNeighborBlockChange(worldIn, blockpos, iblockstate, neighborBlock);
}

Item* BlockPistonExtension::getItem(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos)->getValue(TYPE) == EnumPistonType::STICKY ? Item::getItemFromBlock(Blocks::sticky_piston)
                                                                                 : Item::getItemFromBlock(Blocks::piston);
}

// --- BlockPistonMoving ---

std::shared_ptr<TileEntity> BlockPistonMoving::newTileEntity(const IBlockState* state, EnumFacing facing, bool extending, bool renderHead)
{
    return std::make_shared<TileEntityPiston>(state, facing, extending, renderHead);
}

void BlockPistonMoving::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (auto* tileentity = dynamic_cast<TileEntityPiston*>(worldIn->getTileEntity(pos)))
        tileentity->clearPistonTileEntity();
    else
        BlockContainer::breakBlock(worldIn, pos, state);
}

void BlockPistonMoving::onBlockDestroyedByPlayer(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const BlockPos blockpos = pos.offset(getOpposite(state->getValue(FACING)));
    const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
    if (dynamic_cast<const BlockPistonBase*>(iblockstate->getBlock()) && iblockstate->getValue(BlockPistonBase::EXTENDED))
        worldIn->setBlockToAir(blockpos);
}

bool BlockPistonMoving::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState*, EntityPlayer*, EnumFacing, float, float, float)
{
    if (!worldIn->isRemote && worldIn->getTileEntity(pos) == nullptr) {
        worldIn->setBlockToAir(pos);
        return true;
    }
    return false;
}

void BlockPistonMoving::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState*, float, int)
{
    if (!worldIn->isRemote) {
        if (TileEntityPiston* tileentitypiston = getTileEntity(worldIn, pos)) {
            const IBlockState* iblockstate = tileentitypiston->getPistonState();
            iblockstate->getBlock()->dropBlockAsItem(worldIn, pos, iblockstate, 0);
        }
    }
}

void BlockPistonMoving::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState*, Block*)
{
    if (!worldIn->isRemote)
        worldIn->getTileEntity(pos);
}

std::optional<AxisAlignedBB> BlockPistonMoving::getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    TileEntityPiston* tileentitypiston = getTileEntity(worldIn, pos);
    if (tileentitypiston == nullptr)
        return std::nullopt;
    float f = tileentitypiston->getProgress(0.0f);
    if (tileentitypiston->isExtending())
        f = 1.0f - f;
    return getBoundingBox(worldIn, pos, tileentitypiston->getPistonState(), f, tileentitypiston->getFacing());
}

void BlockPistonMoving::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    TileEntityPiston* tileentitypiston = getTileEntity(worldIn, pos);
    if (tileentitypiston != nullptr) {
        const IBlockState* iblockstate = tileentitypiston->getPistonState();
        Block* block = iblockstate->getBlock();
        if (block == this || &block->getMaterial() == &Material::air)
            return;
        float f = tileentitypiston->getProgress(0.0f);
        if (tileentitypiston->isExtending())
            f = 1.0f - f;
        block->setBlockBoundsBasedOnState(worldIn, pos);
        if (block == Blocks::piston || block == Blocks::sticky_piston)
            f = 0.0f;
        const EnumFacing enumfacing = tileentitypiston->getFacing();
        minX = block->getBlockBoundsMinX() - static_cast<double>(static_cast<float>(getFrontOffsetX(enumfacing)) * f);
        minY = block->getBlockBoundsMinY() - static_cast<double>(static_cast<float>(getFrontOffsetY(enumfacing)) * f);
        minZ = block->getBlockBoundsMinZ() - static_cast<double>(static_cast<float>(getFrontOffsetZ(enumfacing)) * f);
        maxX = block->getBlockBoundsMaxX() - static_cast<double>(static_cast<float>(getFrontOffsetX(enumfacing)) * f);
        maxY = block->getBlockBoundsMaxY() - static_cast<double>(static_cast<float>(getFrontOffsetY(enumfacing)) * f);
        maxZ = block->getBlockBoundsMaxZ() - static_cast<double>(static_cast<float>(getFrontOffsetZ(enumfacing)) * f);
    }
}

std::optional<AxisAlignedBB> BlockPistonMoving::getBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* extendingBlock, float progress,
    EnumFacing direction)
{
    if (extendingBlock->getBlock() != this && &extendingBlock->getBlock()->getMaterial() != &Material::air) {
        const std::optional<AxisAlignedBB> axisalignedbb = extendingBlock->getBlock()->getCollisionBoundingBox(worldIn, pos, extendingBlock);
        if (!axisalignedbb)
            return std::nullopt;
        double d0 = axisalignedbb->minX;
        double d1 = axisalignedbb->minY;
        double d2 = axisalignedbb->minZ;
        double d3 = axisalignedbb->maxX;
        double d4 = axisalignedbb->maxY;
        double d5 = axisalignedbb->maxZ;
        if (getFrontOffsetX(direction) < 0)
            d0 -= static_cast<double>(static_cast<float>(getFrontOffsetX(direction)) * progress);
        else
            d3 -= static_cast<double>(static_cast<float>(getFrontOffsetX(direction)) * progress);
        if (getFrontOffsetY(direction) < 0)
            d1 -= static_cast<double>(static_cast<float>(getFrontOffsetY(direction)) * progress);
        else
            d4 -= static_cast<double>(static_cast<float>(getFrontOffsetY(direction)) * progress);
        if (getFrontOffsetZ(direction) < 0)
            d2 -= static_cast<double>(static_cast<float>(getFrontOffsetZ(direction)) * progress);
        else
            d5 -= static_cast<double>(static_cast<float>(getFrontOffsetZ(direction)) * progress);
        return AxisAlignedBB(d0, d1, d2, d3, d4, d5);
    }
    return std::nullopt;
}

TileEntityPiston* BlockPistonMoving::getTileEntity(IBlockAccess* worldIn, const BlockPos& pos)
{
    return dynamic_cast<TileEntityPiston*>(worldIn->getTileEntity(pos));
}

} // namespace mc
