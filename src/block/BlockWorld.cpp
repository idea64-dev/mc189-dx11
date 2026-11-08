// Original: net/minecraft/block/Block.java -- the default world callbacks.

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "item/Item.h"
#include "item/ItemBlock.h"
#include "item/ItemStack.h"
#include "util/Random.h"
#include "util/Vec3.h"
#include "world/GameRules.h"
#include "world/IBlockAccess.h"
#include "world/World.h"

namespace mc {

const IBlockState* Block::getActualState(const IBlockState* state, IBlockAccess*, const BlockPos&)
{
    return state;
}

bool Block::isPassable(IBlockAccess*, const BlockPos&)
{
    return !blockMaterial.blocksMovement();
}

bool Block::isReplaceable(World*, const BlockPos&)
{
    return false;
}

float Block::getBlockHardness(World*, const BlockPos&)
{
    return blockHardness;
}

int Block::getMixedBrightnessForBlock(IBlockAccess* worldIn, const BlockPos& posIn)
{
    BlockPos pos = posIn;
    const Block* block = worldIn->getBlockState(pos)->getBlock();
    const int i = worldIn->getCombinedLight(pos, block->getLightValue());
    if (i == 0 && dynamic_cast<const BlockSlab*>(block)) {
        pos = pos.down();
        block = worldIn->getBlockState(pos)->getBlock();
        return worldIn->getCombinedLight(pos, block->getLightValue());
    }
    return i;
}

bool Block::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    return side == EnumFacing::DOWN && minY > 0.0 ? true
        : (side == EnumFacing::UP && maxY < 1.0 ? true
        : (side == EnumFacing::NORTH && minZ > 0.0 ? true
        : (side == EnumFacing::SOUTH && maxZ < 1.0 ? true
        : (side == EnumFacing::WEST && minX > 0.0 ? true
        : (side == EnumFacing::EAST && maxX < 1.0 ? true : !worldIn->getBlockState(pos)->getBlock()->isOpaqueCube())))));
}

bool Block::isBlockSolid(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing)
{
    return worldIn->getBlockState(pos)->getBlock()->getMaterial().isSolid();
}

AxisAlignedBB Block::getSelectedBoundingBox(World*, const BlockPos& pos)
{
    return AxisAlignedBB(static_cast<double>(pos.getX()) + minX, static_cast<double>(pos.getY()) + minY, static_cast<double>(pos.getZ()) + minZ,
        static_cast<double>(pos.getX()) + maxX, static_cast<double>(pos.getY()) + maxY, static_cast<double>(pos.getZ()) + maxZ);
}

void Block::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity*)
{
    const auto axisalignedbb = getCollisionBoundingBox(worldIn, pos, state);
    if (axisalignedbb && mask.intersectsWith(*axisalignedbb))
        list.push_back(*axisalignedbb);
}

std::optional<AxisAlignedBB> Block::getCollisionBoundingBox(World*, const BlockPos& pos, const IBlockState*)
{
    return AxisAlignedBB(static_cast<double>(pos.getX()) + minX, static_cast<double>(pos.getY()) + minY, static_cast<double>(pos.getZ()) + minZ,
        static_cast<double>(pos.getX()) + maxX, static_cast<double>(pos.getY()) + maxY, static_cast<double>(pos.getZ()) + maxZ);
}

void Block::randomTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& random)
{
    updateTick(worldIn, pos, state, random);
}

void Block::updateTick(World*, const BlockPos&, const IBlockState*, Random&)
{
}

void Block::randomDisplayTick(World*, const BlockPos&, const IBlockState*, Random&)
{
}

void Block::onBlockDestroyedByPlayer(World*, const BlockPos&, const IBlockState*)
{
}

void Block::onNeighborBlockChange(World*, const BlockPos&, const IBlockState*, Block*)
{
}

int Block::tickRate(World*)
{
    return 10;
}

void Block::onBlockAdded(World*, const BlockPos&, const IBlockState*)
{
}

void Block::breakBlock(World*, const BlockPos&, const IBlockState*)
{
}

float Block::getPlayerRelativeBlockHardness(EntityPlayer*, World* worldIn, const BlockPos& pos)
{
    // Java: divides the player's dig speed by the hardness, by 30 when the player can harvest the block and by
    // 100 otherwise -- the player comes with stage 8.
    const float f = getBlockHardness(worldIn, pos);
    return f < 0.0f ? 0.0f : 1.0f / f / 30.0f;
}

void Block::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune)
{
    if (!worldIn->isRemote) {
        const int i = quantityDroppedWithBonus(fortune, worldIn->rand);
        for (int j = 0; j < i; ++j) {
            if (worldIn->rand.nextFloat() <= chance) {
                Item* item = getItemDropped(state, worldIn->rand, fortune);
                if (item)
                    spawnAsEntity(worldIn, pos, ItemStack(item, 1, damageDropped(state)));
            }
        }
    }
}

void Block::spawnAsEntity(World* worldIn, const BlockPos&, ItemStack)
{
    if (!worldIn->isRemote && worldIn->getGameRules().getBoolean("doTileDrops")) {
        // Java: an EntityItem at a random spot in the block -- entities come with stage 10; the draws keep rand
        // in step.
        worldIn->rand.nextFloat();
        worldIn->rand.nextFloat();
        worldIn->rand.nextFloat();
    }
}

std::optional<MovingObjectPosition> Block::collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& startIn, const Vec3& endIn)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    const Vec3 start = startIn.addVector(static_cast<double>(-pos.getX()), static_cast<double>(-pos.getY()), static_cast<double>(-pos.getZ()));
    const Vec3 end = endIn.addVector(static_cast<double>(-pos.getX()), static_cast<double>(-pos.getY()), static_cast<double>(-pos.getZ()));
    std::optional<Vec3> hits[6] { start.getIntermediateWithXValue(end, minX), start.getIntermediateWithXValue(end, maxX),
        start.getIntermediateWithYValue(end, minY), start.getIntermediateWithYValue(end, maxY), start.getIntermediateWithZValue(end, minZ),
        start.getIntermediateWithZValue(end, maxZ) };
    const auto insideYZ = [this](const Vec3& p) { return p.yCoord >= minY && p.yCoord <= maxY && p.zCoord >= minZ && p.zCoord <= maxZ; };
    const auto insideXZ = [this](const Vec3& p) { return p.xCoord >= minX && p.xCoord <= maxX && p.zCoord >= minZ && p.zCoord <= maxZ; };
    const auto insideXY = [this](const Vec3& p) { return p.xCoord >= minX && p.xCoord <= maxX && p.yCoord >= minY && p.yCoord <= maxY; };
    for (int i = 0; i < 6; ++i) {
        if (hits[i] && !(i < 2 ? insideYZ(*hits[i]) : i < 4 ? insideXZ(*hits[i]) : insideXY(*hits[i])))
            hits[i].reset();
    }
    int best = -1;
    for (int i = 0; i < 6; ++i) {
        if (hits[i] && (best < 0 || start.squareDistanceTo(*hits[i]) < start.squareDistanceTo(*hits[best])))
            best = i;
    }
    if (best < 0)
        return std::nullopt;
    constexpr EnumFacing faces[6] { EnumFacing::WEST, EnumFacing::EAST, EnumFacing::DOWN, EnumFacing::UP, EnumFacing::NORTH, EnumFacing::SOUTH };
    return MovingObjectPosition(hits[best]->addVector(static_cast<double>(pos.getX()), static_cast<double>(pos.getY()), static_cast<double>(pos.getZ())),
        faces[best], pos);
}

bool Block::canReplace(World* worldIn, const BlockPos& pos, EnumFacing side, const ItemStack*)
{
    return canPlaceBlockOnSide(worldIn, pos, side);
}

bool Block::canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing)
{
    return canPlaceBlockAt(worldIn, pos);
}

bool Block::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos)->getBlock()->blockMaterial.isReplaceable();
}

bool Block::onBlockActivated(World*, const BlockPos&, const IBlockState*, EntityPlayer*, EnumFacing, float, float, float)
{
    return false;
}

const IBlockState* Block::onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int meta, EntityLivingBase*)
{
    return getStateFromMeta(meta);
}

Vec3 Block::modifyAcceleration(World*, const BlockPos&, Entity*, const Vec3& motion)
{
    return motion;
}

int Block::colorMultiplier(IBlockAccess*, const BlockPos&, int)
{
    return 16777215;
}

void Block::harvestBlock(World* worldIn, EntityPlayer*, const BlockPos& pos, const IBlockState* state, TileEntity*)
{
    // Java: the mining statistic, exhaustion, silk touch and fortune from the player -- stage 8.
    dropBlockAsItem(worldIn, pos, state, 0);
}

void Block::onFallenUpon(World*, const BlockPos&, Entity*, float)
{
    // Java: entityIn.fall(fallDistance, 1.0F) -- entities, stage 10.
}

void Block::onLanded(World*, Entity*)
{
    // Java: entityIn.motionY = 0.0D -- entities, stage 10.
}

Item* Block::getItem(World*, const BlockPos&)
{
    return Item::getItemFromBlock(this);
}

int Block::getDamageValue(World* worldIn, const BlockPos& pos)
{
    Item* item = getItem(worldIn, pos);
    return item && item->getHasSubtypes() ? damageDropped(worldIn->getBlockState(pos)) : 0;
}

Block* Block::getBlockFromItem(const Item* item)
{
    const auto* itemBlock = dynamic_cast<const ItemBlock*>(item);
    return itemBlock ? itemBlock->getBlock() : nullptr;
}

} // namespace mc
