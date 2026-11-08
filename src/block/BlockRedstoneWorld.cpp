// The world callbacks of the redstone blocks: BlockRedstoneWire.java, BlockTorch.java, BlockRedstoneTorch.java,
// BlockRedstoneDiode.java, BlockRedstoneRepeater.java, BlockRedstoneComparator.java, BlockLever.java,
// BlockButton.java, BlockBasePressurePlate.java, BlockPressurePlate.java, BlockPressurePlateWeighted.java,
// BlockTNT.java, BlockTripWire.java, BlockTripWireHook.java, BlockRedstoneLight.java, BlockRedstoneOre.java,
// BlockCompressedPowered.java, BlockDaylightDetector.java, BlockNote.java, BlockMobSpawner.java and
// BlockContainer.java.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"
#include "core/Java.h"
#include "entity/Entity.h"
#include "entity/player/EntityPlayer.h"
#include "tileentity/TileEntities.h"
#include "util/MathHelper.h"
#include "world/EnumSkyBlock.h"
#include "world/WorldProvider.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace mc {

// --- placer helpers (BlockWorldHelpers.h) ---

Entity* placerEntity(EntityLivingBase* placer)
{
    return placer;
}

Entity* playerEntity(EntityPlayer* player)
{
    return player;
}

EnumFacing placerHorizontalFacing(EntityLivingBase* placer)
{
    const Entity* entity = placerEntity(placer);
    return entity ? entity->getHorizontalFacing() : EnumFacing::NORTH;
}

// --- BlockContainer ---

void BlockContainer::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    Block::breakBlock(worldIn, pos, state);
    worldIn->removeTileEntity(pos);
}

bool BlockContainer::onBlockEventReceived(World* worldIn, const BlockPos& pos, const IBlockState* state, int eventID, int eventParam)
{
    Block::onBlockEventReceived(worldIn, pos, state, eventID, eventParam);
    TileEntity* tileentity = worldIn->getTileEntity(pos);
    return tileentity == nullptr ? false : tileentity->receiveClientEvent(eventID, eventParam);
}

bool BlockContainer::isInvalidNeighbor(World* worldIn, const BlockPos& pos, EnumFacing side)
{
    return &worldIn->getBlockState(pos.offset(side))->getBlock()->getMaterial() == &Material::cactus;
}

bool BlockContainer::hasInvalidNeighbor(World* worldIn, const BlockPos& pos)
{
    return isInvalidNeighbor(worldIn, pos, EnumFacing::NORTH) || isInvalidNeighbor(worldIn, pos, EnumFacing::SOUTH) || isInvalidNeighbor(worldIn, pos, EnumFacing::WEST)
        || isInvalidNeighbor(worldIn, pos, EnumFacing::EAST);
}

// --- BlockRedstoneWire ---

const IBlockState* BlockRedstoneWire::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    state = state->withProperty(WEST, getAttachPosition(worldIn, pos, EnumFacing::WEST));
    state = state->withProperty(EAST, getAttachPosition(worldIn, pos, EnumFacing::EAST));
    state = state->withProperty(NORTH, getAttachPosition(worldIn, pos, EnumFacing::NORTH));
    state = state->withProperty(SOUTH, getAttachPosition(worldIn, pos, EnumFacing::SOUTH));
    return state;
}

BlockRedstoneWire::EnumAttachPosition BlockRedstoneWire::getAttachPosition(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing direction)
{
    const BlockPos blockpos = pos.offset(direction);
    const Block* block = worldIn->getBlockState(pos.offset(direction))->getBlock();
    if (!canConnectTo(worldIn->getBlockState(blockpos), direction) && (block->isBlockNormalCube() || !canConnectUpwardsTo(worldIn->getBlockState(blockpos.down())))) {
        const Block* block1 = worldIn->getBlockState(pos.up())->getBlock();
        return !block1->isBlockNormalCube() && block->isBlockNormalCube() && canConnectUpwardsTo(worldIn->getBlockState(blockpos.up())) ? EnumAttachPosition::UP
                                                                                                                                       : EnumAttachPosition::NONE;
    }
    return EnumAttachPosition::SIDE;
}

int BlockRedstoneWire::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    return iblockstate->getBlock() != this ? Block::colorMultiplier(worldIn, pos, renderPass) : colorMultiplier(iblockstate->getValue(POWER));
}

bool BlockRedstoneWire::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) || worldIn->getBlockState(pos.down())->getBlock() == Blocks::glowstone;
}

const IBlockState* BlockRedstoneWire::updateSurroundingRedstone(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    state = calculateCurrentChanges(worldIn, pos, pos, state);
    std::vector<BlockPos> list;
    list.reserve(blocksNeedingUpdate.size());
    for (const auto& entry : blocksNeedingUpdate)
        list.push_back(entry.first);
    blocksNeedingUpdate.clear();
    for (const BlockPos& blockpos : list)
        worldIn->notifyNeighborsOfStateChange(blockpos, this);
    return state;
}

const IBlockState* BlockRedstoneWire::calculateCurrentChanges(World* worldIn, const BlockPos& pos1, const BlockPos& pos2, const IBlockState* state)
{
    const IBlockState* iblockstate = state;
    const int i = state->getValue(POWER);
    int j = 0;
    j = getMaxCurrentStrength(worldIn, pos2, j);
    canProvidePowerFlag = false;
    const int k = worldIn->isBlockIndirectlyGettingPowered(pos1);
    canProvidePowerFlag = true;
    if (k > 0 && k > j - 1)
        j = k;
    int l = 0;
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        const BlockPos blockpos = pos1.offset(enumfacing);
        const bool flag = blockpos.getX() != pos2.getX() || blockpos.getZ() != pos2.getZ();
        if (flag)
            l = getMaxCurrentStrength(worldIn, blockpos, l);
        if (worldIn->getBlockState(blockpos)->getBlock()->isNormalCube() && !worldIn->getBlockState(pos1.up())->getBlock()->isNormalCube()) {
            if (flag && pos1.getY() >= pos2.getY())
                l = getMaxCurrentStrength(worldIn, blockpos.up(), l);
        } else if (!worldIn->getBlockState(blockpos)->getBlock()->isNormalCube() && flag && pos1.getY() <= pos2.getY()) {
            l = getMaxCurrentStrength(worldIn, blockpos.down(), l);
        }
    }
    if (l > j)
        j = l - 1;
    else if (j > 0)
        --j;
    else
        j = 0;
    if (k > j - 1)
        j = k;
    if (i != j) {
        state = state->withProperty(POWER, j);
        if (worldIn->getBlockState(pos1) == iblockstate)
            worldIn->setBlockState(pos1, state, 2);
        blocksNeedingUpdate.put(pos1, true);
        for (const EnumFacing enumfacing1 : kFacings)
            blocksNeedingUpdate.put(pos1.offset(enumfacing1), true);
    }
    return state;
}

void BlockRedstoneWire::notifyWireNeighborsOfStateChange(World* worldIn, const BlockPos& pos)
{
    if (worldIn->getBlockState(pos)->getBlock() == this) {
        worldIn->notifyNeighborsOfStateChange(pos, this);
        for (const EnumFacing enumfacing : kFacings)
            worldIn->notifyNeighborsOfStateChange(pos.offset(enumfacing), this);
    }
}

void BlockRedstoneWire::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!worldIn->isRemote) {
        updateSurroundingRedstone(worldIn, pos, state);
        // Java: EnumFacing.Plane.VERTICAL -- UP, DOWN.
        for (const EnumFacing enumfacing : { EnumFacing::UP, EnumFacing::DOWN })
            worldIn->notifyNeighborsOfStateChange(pos.offset(enumfacing), this);
        for (const EnumFacing enumfacing1 : kPlaneHorizontal)
            notifyWireNeighborsOfStateChange(worldIn, pos.offset(enumfacing1));
        for (const EnumFacing enumfacing2 : kPlaneHorizontal) {
            const BlockPos blockpos = pos.offset(enumfacing2);
            if (worldIn->getBlockState(blockpos)->getBlock()->isNormalCube())
                notifyWireNeighborsOfStateChange(worldIn, blockpos.up());
            else
                notifyWireNeighborsOfStateChange(worldIn, blockpos.down());
        }
    }
}

void BlockRedstoneWire::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    Block::breakBlock(worldIn, pos, state);
    if (!worldIn->isRemote) {
        for (const EnumFacing enumfacing : kFacings)
            worldIn->notifyNeighborsOfStateChange(pos.offset(enumfacing), this);
        updateSurroundingRedstone(worldIn, pos, state);
        for (const EnumFacing enumfacing1 : kPlaneHorizontal)
            notifyWireNeighborsOfStateChange(worldIn, pos.offset(enumfacing1));
        for (const EnumFacing enumfacing2 : kPlaneHorizontal) {
            const BlockPos blockpos = pos.offset(enumfacing2);
            if (worldIn->getBlockState(blockpos)->getBlock()->isNormalCube())
                notifyWireNeighborsOfStateChange(worldIn, blockpos.up());
            else
                notifyWireNeighborsOfStateChange(worldIn, blockpos.down());
        }
    }
}

int BlockRedstoneWire::getMaxCurrentStrength(World* worldIn, const BlockPos& pos, int strength)
{
    if (worldIn->getBlockState(pos)->getBlock() != this)
        return strength;
    const int i = worldIn->getBlockState(pos)->getValue(POWER);
    return i > strength ? i : strength;
}

void BlockRedstoneWire::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (!worldIn->isRemote) {
        if (canPlaceBlockAt(worldIn, pos)) {
            updateSurroundingRedstone(worldIn, pos, state);
        } else {
            dropBlockAsItem(worldIn, pos, state, 0);
            worldIn->setBlockToAir(pos);
        }
    }
}

int BlockRedstoneWire::getStrongPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side)
{
    return !canProvidePowerFlag ? 0 : getWeakPower(worldIn, pos, state, side);
}

int BlockRedstoneWire::getWeakPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side)
{
    if (!canProvidePowerFlag)
        return 0;
    const int i = state->getValue(POWER);
    if (i == 0)
        return 0;
    if (side == EnumFacing::UP)
        return i;
    // Java: an EnumSet<EnumFacing>.
    std::array<bool, 6> enumset {};
    bool empty = true;
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (func_176339_d(worldIn, pos, enumfacing)) {
            enumset[static_cast<size_t>(ordinal(enumfacing))] = true;
            empty = false;
        }
    }
    const auto contains = [&enumset](EnumFacing f) { return enumset[static_cast<size_t>(ordinal(f))]; };
    if (isHorizontal(getAxis(side)) && empty)
        return i;
    if (contains(side) && !contains(rotateYCCW(side)) && !contains(rotateY(side)))
        return i;
    return 0;
}

bool BlockRedstoneWire::func_176339_d(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    const BlockPos blockpos = pos.offset(side);
    const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
    const Block* block = iblockstate->getBlock();
    const bool flag = block->isNormalCube();
    const bool flag1 = worldIn->getBlockState(pos.up())->getBlock()->isNormalCube();
    return !flag1 && flag && canConnectUpwardsTo(worldIn, blockpos.up())
        ? true
        : (canConnectTo(iblockstate, side) ? true
                                           : (block == Blocks::powered_repeater && iblockstate->getValue(BlockRedstoneDiode::FACING) == side
                                                     ? true
                                                     : !flag && canConnectUpwardsTo(worldIn, blockpos.down())));
}

bool BlockRedstoneWire::canConnectUpwardsTo(IBlockAccess* worldIn, const BlockPos& pos)
{
    return canConnectUpwardsTo(worldIn->getBlockState(pos));
}

bool BlockRedstoneWire::canConnectUpwardsTo(const IBlockState* state)
{
    return canConnectTo(state, std::nullopt);
}

bool BlockRedstoneWire::canConnectTo(const IBlockState* blockState, std::optional<EnumFacing> side)
{
    const Block* block = blockState->getBlock();
    if (block == Blocks::redstone_wire)
        return true;
    if (Blocks::unpowered_repeater->isAssociated(block)) {
        const EnumFacing enumfacing = blockState->getValue(BlockRedstoneRepeater::FACING);
        return enumfacing == side || getOpposite(enumfacing) == side;
    }
    return block->canProvidePower() && side.has_value();
}

int BlockRedstoneWire::colorMultiplier(int powerLevel)
{
    const float f = static_cast<float>(powerLevel) / 15.0f;
    float f1 = f * 0.6f + 0.4f;
    if (powerLevel == 0)
        f1 = 0.3f;
    float f2 = f * f * 0.7f - 0.5f;
    float f3 = f * f * 0.6f - 0.7f;
    if (f2 < 0.0f)
        f2 = 0.0f;
    if (f3 < 0.0f)
        f3 = 0.0f;
    const int i = MathHelper::clamp_int(java::f2i(f1 * 255.0f), 0, 255);
    const int j = MathHelper::clamp_int(java::f2i(f2 * 255.0f), 0, 255);
    const int k = MathHelper::clamp_int(java::f2i(f3 * 255.0f), 0, 255);
    return -16777216 | i << 16 | j << 8 | k;
}

void BlockRedstoneWire::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    const int i = state->getValue(POWER);
    if (i != 0) {
        const double d0 = static_cast<double>(pos.getX()) + 0.5 + (static_cast<double>(rand.nextFloat()) - 0.5) * 0.2;
        const double d1 = static_cast<double>(static_cast<float>(pos.getY()) + 0.0625f);
        const double d2 = static_cast<double>(pos.getZ()) + 0.5 + (static_cast<double>(rand.nextFloat()) - 0.5) * 0.2;
        const float f = static_cast<float>(i) / 15.0f;
        const float f1 = f * 0.6f + 0.4f;
        const float f2 = std::max(0.0f, f * f * 0.7f - 0.5f);
        const float f3 = std::max(0.0f, f * f * 0.6f - 0.7f);
        spawnParticle(worldIn, EnumParticleTypes::REDSTONE, d0, d1, d2, f1, f2, f3);
    }
}

// --- BlockTorch ---

bool BlockTorch::canPlaceOn(World* worldIn, const BlockPos& pos)
{
    if (World::doesBlockHaveSolidTopSurface(worldIn, pos))
        return true;
    const Block* block = worldIn->getBlockState(pos)->getBlock();
    return dynamic_cast<const BlockFence*>(block) || block == Blocks::glass || block == Blocks::cobblestone_wall || block == Blocks::stained_glass;
}

bool BlockTorch::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    for (const EnumFacing enumfacing : FACING.getAllowedValues()) {
        if (canPlaceAt(worldIn, pos, enumfacing))
            return true;
    }
    return false;
}

bool BlockTorch::canPlaceAt(World* worldIn, const BlockPos& pos, EnumFacing facing)
{
    const BlockPos blockpos = pos.offset(getOpposite(facing));
    const bool flag = isHorizontal(getAxis(facing));
    return (flag && worldIn->isBlockNormalCube(blockpos, true)) || (facing == EnumFacing::UP && canPlaceOn(worldIn, blockpos));
}

const IBlockState* BlockTorch::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float, float, float, int, EntityLivingBase*)
{
    if (canPlaceAt(worldIn, pos, facing))
        return getDefaultState()->withProperty(FACING, facing);
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (worldIn->isBlockNormalCube(pos.offset(getOpposite(enumfacing)), true))
            return getDefaultState()->withProperty(FACING, enumfacing);
    }
    return getDefaultState();
}

void BlockTorch::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    checkForDrop(worldIn, pos, state);
}

void BlockTorch::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    onNeighborChangeInternal(worldIn, pos, state);
}

bool BlockTorch::onNeighborChangeInternal(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!checkForDrop(worldIn, pos, state))
        return true;
    const EnumFacing enumfacing = state->getValue(FACING);
    const Axis axis = getAxis(enumfacing);
    const EnumFacing enumfacing1 = getOpposite(enumfacing);
    bool flag = false;
    if (isHorizontal(axis) && !worldIn->isBlockNormalCube(pos.offset(enumfacing1), true))
        flag = true;
    else if (isVertical(axis) && !canPlaceOn(worldIn, pos.offset(enumfacing1)))
        flag = true;
    if (flag) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
        return true;
    }
    return false;
}

bool BlockTorch::checkForDrop(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (state->getBlock() == this && canPlaceAt(worldIn, pos, state->getValue(FACING)))
        return true;
    if (worldIn->getBlockState(pos)->getBlock() == this) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
    return false;
}

std::optional<MovingObjectPosition> BlockTorch::collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end)
{
    const EnumFacing enumfacing = worldIn->getBlockState(pos)->getValue(FACING);
    float f = 0.15f;
    if (enumfacing == EnumFacing::EAST) {
        setBlockBounds(0.0f, 0.2f, 0.5f - f, f * 2.0f, 0.8f, 0.5f + f);
    } else if (enumfacing == EnumFacing::WEST) {
        setBlockBounds(1.0f - f * 2.0f, 0.2f, 0.5f - f, 1.0f, 0.8f, 0.5f + f);
    } else if (enumfacing == EnumFacing::SOUTH) {
        setBlockBounds(0.5f - f, 0.2f, 0.0f, 0.5f + f, 0.8f, f * 2.0f);
    } else if (enumfacing == EnumFacing::NORTH) {
        setBlockBounds(0.5f - f, 0.2f, 1.0f - f * 2.0f, 0.5f + f, 0.8f, 1.0f);
    } else {
        f = 0.1f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.6f, 0.5f + f);
    }
    return Block::collisionRayTrace(worldIn, pos, start, end);
}

void BlockTorch::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    const double d0 = static_cast<double>(pos.getX()) + 0.5;
    const double d1 = static_cast<double>(pos.getY()) + 0.7;
    const double d2 = static_cast<double>(pos.getZ()) + 0.5;
    const double d3 = 0.22;
    const double d4 = 0.27;
    if (isHorizontal(getAxis(enumfacing))) {
        const EnumFacing enumfacing1 = getOpposite(enumfacing);
        spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0 + d4 * getFrontOffsetX(enumfacing1), d1 + d3, d2 + d4 * getFrontOffsetZ(enumfacing1), 0.0,
            0.0, 0.0);
        spawnParticle(worldIn, EnumParticleTypes::FLAME, d0 + d4 * getFrontOffsetX(enumfacing1), d1 + d3, d2 + d4 * getFrontOffsetZ(enumfacing1), 0.0, 0.0,
            0.0);
    } else {
        spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0, d1, d2, 0.0, 0.0, 0.0);
        spawnParticle(worldIn, EnumParticleTypes::FLAME, d0, d1, d2, 0.0, 0.0, 0.0);
    }
}

// --- BlockRedstoneTorch ---

std::unordered_map<World*, std::vector<BlockRedstoneTorch::Toggle>>& BlockRedstoneTorch::toggles()
{
    static std::unordered_map<World*, std::vector<Toggle>> map;
    return map;
}

bool BlockRedstoneTorch::isBurnedOut(World* worldIn, const BlockPos& pos, bool turnOff)
{
    std::vector<Toggle>& list = toggles()[worldIn];
    if (turnOff)
        list.push_back(Toggle { pos, worldIn->getTotalWorldTime() });
    int i = 0;
    for (const Toggle& toggle : list) {
        if (toggle.pos == pos) {
            ++i;
            if (i >= 8)
                return true;
        }
    }
    return false;
}

void BlockRedstoneTorch::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (isOn) {
        for (const EnumFacing enumfacing : kFacings)
            worldIn->notifyNeighborsOfStateChange(pos.offset(enumfacing), this);
    }
}

void BlockRedstoneTorch::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (isOn) {
        for (const EnumFacing enumfacing : kFacings)
            worldIn->notifyNeighborsOfStateChange(pos.offset(enumfacing), this);
    }
}

int BlockRedstoneTorch::getWeakPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing side)
{
    return isOn && state->getValue(FACING) != side ? 15 : 0;
}

bool BlockRedstoneTorch::shouldBeOff(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const EnumFacing enumfacing = getOpposite(state->getValue(FACING));
    return worldIn->isSidePowered(pos.offset(enumfacing), enumfacing);
}

void BlockRedstoneTorch::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    const bool flag = shouldBeOff(worldIn, pos, state);
    const auto it = toggles().find(worldIn);
    if (it != toggles().end()) {
        std::vector<Toggle>& list = it->second;
        while (!list.empty() && worldIn->getTotalWorldTime() - list.front().time > 60)
            list.erase(list.begin());
    }
    if (isOn) {
        if (flag) {
            worldIn->setBlockState(pos, Blocks::unlit_redstone_torch->getDefaultState()->withProperty(FACING, state->getValue(FACING)), 3);
            if (isBurnedOut(worldIn, pos, true)) {
                const float r1 = worldIn->rand.nextFloat();
                const float r2 = worldIn->rand.nextFloat();
                worldIn->playSoundEffect(static_cast<float>(pos.getX()) + 0.5f, static_cast<float>(pos.getY()) + 0.5f, static_cast<float>(pos.getZ()) + 0.5f,
                    "random.fizz", 0.5f, 2.6f + (r1 - r2) * 0.8f);
                for (int i = 0; i < 5; ++i) {
                    const double d0 = static_cast<double>(pos.getX()) + rand.nextDouble() * 0.6 + 0.2;
                    const double d1 = static_cast<double>(pos.getY()) + rand.nextDouble() * 0.6 + 0.2;
                    const double d2 = static_cast<double>(pos.getZ()) + rand.nextDouble() * 0.6 + 0.2;
                    spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0, d1, d2, 0.0, 0.0, 0.0);
                }
                worldIn->scheduleUpdate(pos, worldIn->getBlockState(pos)->getBlock(), 160);
            }
        }
    } else if (!flag && !isBurnedOut(worldIn, pos, false)) {
        worldIn->setBlockState(pos, Blocks::redstone_torch->getDefaultState()->withProperty(FACING, state->getValue(FACING)), 3);
    }
}

void BlockRedstoneTorch::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (!onNeighborChangeInternal(worldIn, pos, state)) {
        if (isOn == shouldBeOff(worldIn, pos, state))
            worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
    }
}

int BlockRedstoneTorch::getStrongPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side)
{
    return side == EnumFacing::DOWN ? getWeakPower(worldIn, pos, state, side) : 0;
}

void BlockRedstoneTorch::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    if (isOn) {
        double d0 = static_cast<double>(pos.getX()) + 0.5 + (rand.nextDouble() - 0.5) * 0.2;
        double d1 = static_cast<double>(pos.getY()) + 0.7 + (rand.nextDouble() - 0.5) * 0.2;
        double d2 = static_cast<double>(pos.getZ()) + 0.5 + (rand.nextDouble() - 0.5) * 0.2;
        const EnumFacing enumfacing = state->getValue(FACING);
        if (isHorizontal(getAxis(enumfacing))) {
            const EnumFacing enumfacing1 = getOpposite(enumfacing);
            d0 += 0.27 * getFrontOffsetX(enumfacing1);
            d1 += 0.22;
            d2 += 0.27 * getFrontOffsetZ(enumfacing1);
        }
        spawnParticle(worldIn, EnumParticleTypes::REDSTONE, d0, d1, d2, 0.0, 0.0, 0.0);
    }
}

// --- BlockRedstoneDiode ---

bool BlockRedstoneDiode::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) ? BlockDirectional::canPlaceBlockAt(worldIn, pos) : false;
}

bool BlockRedstoneDiode::canBlockStay(World* worldIn, const BlockPos& pos)
{
    return World::doesBlockHaveSolidTopSurface(worldIn, pos.down());
}

void BlockRedstoneDiode::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    if (!isLocked(worldIn, pos, state)) {
        const bool flag = shouldBePowered(worldIn, pos, state);
        if (isRepeaterPowered && !flag) {
            worldIn->setBlockState(pos, getUnpoweredState(state), 2);
        } else if (!isRepeaterPowered) {
            worldIn->setBlockState(pos, getPoweredState(state), 2);
            if (!flag)
                worldIn->updateBlockTick(pos, getPoweredState(state)->getBlock(), getTickDelay(state), -1);
        }
    }
}

int BlockRedstoneDiode::getWeakPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side)
{
    return !isPowered(state) ? 0 : (state->getValue(FACING) == side ? getActiveSignal(worldIn, pos, state) : 0);
}

void BlockRedstoneDiode::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (canBlockStay(worldIn, pos)) {
        updateState(worldIn, pos, state);
    } else {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
        for (const EnumFacing enumfacing : kFacings)
            worldIn->notifyNeighborsOfStateChange(pos.offset(enumfacing), this);
    }
}

void BlockRedstoneDiode::updateState(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!isLocked(worldIn, pos, state)) {
        const bool flag = shouldBePowered(worldIn, pos, state);
        if (((isRepeaterPowered && !flag) || (!isRepeaterPowered && flag)) && !worldIn->isBlockTickPending(pos, this)) {
            int i = -1;
            if (isFacingTowardsRepeater(worldIn, pos, state))
                i = -3;
            else if (isRepeaterPowered)
                i = -2;
            worldIn->updateBlockTick(pos, this, getDelay(state), i);
        }
    }
}

bool BlockRedstoneDiode::shouldBePowered(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    return calculateInputStrength(worldIn, pos, state) > 0;
}

int BlockRedstoneDiode::calculateInputStrength(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    const BlockPos blockpos = pos.offset(enumfacing);
    const int i = worldIn->getRedstonePower(blockpos, enumfacing);
    if (i >= 15)
        return i;
    const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
    return std::max(i, iblockstate->getBlock() == Blocks::redstone_wire ? iblockstate->getValue(BlockRedstoneWire::POWER) : 0);
}

int BlockRedstoneDiode::getPowerOnSides(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    const EnumFacing enumfacing1 = rotateY(enumfacing);
    const EnumFacing enumfacing2 = rotateYCCW(enumfacing);
    const int a = getPowerOnSide(worldIn, pos.offset(enumfacing1), enumfacing1);
    const int b = getPowerOnSide(worldIn, pos.offset(enumfacing2), enumfacing2);
    return std::max(a, b);
}

int BlockRedstoneDiode::getPowerOnSide(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const Block* block = iblockstate->getBlock();
    return canPowerSide(block) ? (block == Blocks::redstone_wire ? iblockstate->getValue(BlockRedstoneWire::POWER) : worldIn->getStrongPower(pos, side)) : 0;
}

void BlockRedstoneDiode::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase*, const ItemStack&)
{
    if (shouldBePowered(worldIn, pos, state))
        worldIn->scheduleUpdate(pos, this, 1);
}

void BlockRedstoneDiode::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    notifyNeighbors(worldIn, pos, state);
}

void BlockRedstoneDiode::notifyNeighbors(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    const BlockPos blockpos = pos.offset(getOpposite(enumfacing));
    worldIn->notifyBlockOfStateChange(blockpos, this);
    worldIn->notifyNeighborsOfStateExcept(blockpos, this, enumfacing);
}

void BlockRedstoneDiode::onBlockDestroyedByPlayer(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (isRepeaterPowered) {
        for (const EnumFacing enumfacing : kFacings)
            worldIn->notifyNeighborsOfStateChange(pos.offset(enumfacing), this);
    }
    BlockDirectional::onBlockDestroyedByPlayer(worldIn, pos, state);
}

bool BlockRedstoneDiode::isRedstoneRepeaterBlockID(const Block* blockIn)
{
    return Blocks::unpowered_repeater->isAssociated(blockIn) || Blocks::unpowered_comparator->isAssociated(blockIn);
}

bool BlockRedstoneDiode::isAssociated(const Block* other) const
{
    return other == getPoweredState(getDefaultState())->getBlock() || other == getUnpoweredState(getDefaultState())->getBlock();
}

bool BlockRedstoneDiode::isFacingTowardsRepeater(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const EnumFacing enumfacing = getOpposite(state->getValue(FACING));
    const BlockPos blockpos = pos.offset(enumfacing);
    return isRedstoneRepeaterBlockID(worldIn->getBlockState(blockpos)->getBlock()) ? worldIn->getBlockState(blockpos)->getValue(FACING) != enumfacing : false;
}

// --- BlockRedstoneRepeater ---

const IBlockState* BlockRedstoneRepeater::getPoweredState(const IBlockState* unpoweredState) const
{
    const int integer = unpoweredState->getValue(DELAY);
    const bool obool = unpoweredState->getValue(LOCKED);
    const EnumFacing enumfacing = unpoweredState->getValue(FACING);
    return Blocks::powered_repeater->getDefaultState()->withProperty(FACING, enumfacing)->withProperty(DELAY, integer)->withProperty(LOCKED, obool);
}

const IBlockState* BlockRedstoneRepeater::getUnpoweredState(const IBlockState* poweredState) const
{
    const int integer = poweredState->getValue(DELAY);
    const bool obool = poweredState->getValue(LOCKED);
    const EnumFacing enumfacing = poweredState->getValue(FACING);
    return Blocks::unpowered_repeater->getDefaultState()->withProperty(FACING, enumfacing)->withProperty(DELAY, integer)->withProperty(LOCKED, obool);
}

void BlockRedstoneRepeater::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    if (isRepeaterPowered) {
        const EnumFacing enumfacing = state->getValue(FACING);
        const double d0 = static_cast<double>(static_cast<float>(pos.getX()) + 0.5f) + static_cast<double>(rand.nextFloat() - 0.5f) * 0.2;
        const double d1 = static_cast<double>(static_cast<float>(pos.getY()) + 0.4f) + static_cast<double>(rand.nextFloat() - 0.5f) * 0.2;
        const double d2 = static_cast<double>(static_cast<float>(pos.getZ()) + 0.5f) + static_cast<double>(rand.nextFloat() - 0.5f) * 0.2;
        float f = -5.0f;
        if (rand.nextBoolean())
            f = static_cast<float>(state->getValue(DELAY) * 2 - 1);
        f = f / 16.0f;
        const double d3 = static_cast<double>(f * static_cast<float>(getFrontOffsetX(enumfacing)));
        const double d4 = static_cast<double>(f * static_cast<float>(getFrontOffsetZ(enumfacing)));
        spawnParticle(worldIn, EnumParticleTypes::REDSTONE, d0 + d3, d1, d2 + d4, 0.0, 0.0, 0.0);
    }
}

void BlockRedstoneRepeater::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    BlockRedstoneDiode::breakBlock(worldIn, pos, state);
    notifyNeighbors(worldIn, pos, state);
}

// --- BlockRedstoneComparator ---

const IBlockState* BlockRedstoneComparator::getPoweredState(const IBlockState* unpoweredState) const
{
    const bool obool = unpoweredState->getValue(POWERED);
    const Mode mode = unpoweredState->getValue(MODE);
    const EnumFacing enumfacing = unpoweredState->getValue(FACING);
    return Blocks::powered_comparator->getDefaultState()->withProperty(FACING, enumfacing)->withProperty(POWERED, obool)->withProperty(MODE, mode);
}

const IBlockState* BlockRedstoneComparator::getUnpoweredState(const IBlockState* poweredState) const
{
    const bool obool = poweredState->getValue(POWERED);
    const Mode mode = poweredState->getValue(MODE);
    const EnumFacing enumfacing = poweredState->getValue(FACING);
    return Blocks::unpowered_comparator->getDefaultState()->withProperty(FACING, enumfacing)->withProperty(POWERED, obool)->withProperty(MODE, mode);
}

int BlockRedstoneComparator::getActiveSignal(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState*)
{
    const auto* tileentity = dynamic_cast<TileEntityComparator*>(worldIn->getTileEntity(pos));
    return tileentity ? tileentity->getOutputSignal() : 0;
}

int BlockRedstoneComparator::calculateOutput(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    return state->getValue(MODE) == Mode::SUBTRACT ? std::max(calculateInputStrength(worldIn, pos, state) - getPowerOnSides(worldIn, pos, state), 0)
                                                   : calculateInputStrength(worldIn, pos, state);
}

bool BlockRedstoneComparator::shouldBePowered(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const int i = calculateInputStrength(worldIn, pos, state);
    if (i >= 15)
        return true;
    if (i == 0)
        return false;
    const int j = getPowerOnSides(worldIn, pos, state);
    return j == 0 ? true : i >= j;
}

int BlockRedstoneComparator::calculateInputStrength(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    int i = BlockRedstoneDiode::calculateInputStrength(worldIn, pos, state);
    const EnumFacing enumfacing = state->getValue(FACING);
    BlockPos blockpos = pos.offset(enumfacing);
    Block* block = worldIn->getBlockState(blockpos)->getBlock();
    if (block->hasComparatorInputOverride()) {
        i = block->getComparatorInputOverride(worldIn, blockpos);
    } else if (i < 15 && block->isNormalCube()) {
        blockpos = blockpos.offset(enumfacing);
        block = worldIn->getBlockState(blockpos)->getBlock();
        if (block->hasComparatorInputOverride()) {
            i = block->getComparatorInputOverride(worldIn, blockpos);
        } else if (&block->getMaterial() == &Material::air) {
            // Java: findItemFrame -- the single item frame in the block facing this way gives
            // func_174866_q(); item frames come with stage 10.
        }
    }
    return i;
}

void BlockRedstoneComparator::updateState(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!worldIn->isBlockTickPending(pos, this)) {
        const int i = calculateOutput(worldIn, pos, state);
        const auto* tileentity = dynamic_cast<TileEntityComparator*>(worldIn->getTileEntity(pos));
        const int j = tileentity ? tileentity->getOutputSignal() : 0;
        if (i != j || isPowered(state) != shouldBePowered(worldIn, pos, state)) {
            if (isFacingTowardsRepeater(worldIn, pos, state))
                worldIn->updateBlockTick(pos, this, 2, -1);
            else
                worldIn->updateBlockTick(pos, this, 2, 0);
        }
    }
}

void BlockRedstoneComparator::onStateChange(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const int i = calculateOutput(worldIn, pos, state);
    int j = 0;
    if (auto* tileentitycomparator = dynamic_cast<TileEntityComparator*>(worldIn->getTileEntity(pos))) {
        j = tileentitycomparator->getOutputSignal();
        tileentitycomparator->setOutputSignal(i);
    }
    if (j != i || state->getValue(MODE) == Mode::COMPARE) {
        const bool flag1 = shouldBePowered(worldIn, pos, state);
        const bool flag = isPowered(state);
        if (flag && !flag1)
            worldIn->setBlockState(pos, state->withProperty(POWERED, false), 2);
        else if (!flag && flag1)
            worldIn->setBlockState(pos, state->withProperty(POWERED, true), 2);
        notifyNeighbors(worldIn, pos, state);
    }
}

void BlockRedstoneComparator::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    if (isRepeaterPowered)
        worldIn->setBlockState(pos, getUnpoweredState(state)->withProperty(POWERED, true), 4);
    onStateChange(worldIn, pos, state);
}

void BlockRedstoneComparator::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    BlockRedstoneDiode::onBlockAdded(worldIn, pos, state);
    worldIn->setTileEntity(pos, createNewTileEntity(worldIn, 0));
}

void BlockRedstoneComparator::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    BlockRedstoneDiode::breakBlock(worldIn, pos, state);
    worldIn->removeTileEntity(pos);
    notifyNeighbors(worldIn, pos, state);
}

bool BlockRedstoneComparator::onBlockEventReceived(World* worldIn, const BlockPos& pos, const IBlockState* state, int eventID, int eventParam)
{
    BlockRedstoneDiode::onBlockEventReceived(worldIn, pos, state, eventID, eventParam);
    TileEntity* tileentity = worldIn->getTileEntity(pos);
    return tileentity == nullptr ? false : tileentity->receiveClientEvent(eventID, eventParam);
}

std::shared_ptr<TileEntity> BlockRedstoneComparator::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityComparator>();
}

// --- BlockLever ---

bool BlockLever::canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side)
{
    return func_181090_a(worldIn, pos, getOpposite(side));
}

bool BlockLever::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    for (const EnumFacing enumfacing : kFacings) {
        if (func_181090_a(worldIn, pos, enumfacing))
            return true;
    }
    return false;
}

bool BlockLever::func_181090_a(World* worldIn, const BlockPos& pos, EnumFacing side)
{
    return BlockButton::func_181088_a(worldIn, pos, side);
}

const IBlockState* BlockLever::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float, float, float, int, EntityLivingBase* placer)
{
    const IBlockState* iblockstate = getDefaultState()->withProperty(POWERED, false);
    if (func_181090_a(worldIn, pos, getOpposite(facing)))
        return iblockstate->withProperty(FACING, forFacings(facing, placerHorizontalFacing(placer)));
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (enumfacing != facing && func_181090_a(worldIn, pos, getOpposite(enumfacing)))
            return iblockstate->withProperty(FACING, forFacings(enumfacing, placerHorizontalFacing(placer)));
    }
    if (World::doesBlockHaveSolidTopSurface(worldIn, pos.down()))
        return iblockstate->withProperty(FACING, forFacings(EnumFacing::UP, placerHorizontalFacing(placer)));
    return iblockstate;
}

int BlockLever::getMetadataForFacing(EnumFacing facing)
{
    switch (facing) {
    case EnumFacing::DOWN: return 0;
    case EnumFacing::UP: return 5;
    case EnumFacing::NORTH: return 4;
    case EnumFacing::SOUTH: return 3;
    case EnumFacing::WEST: return 2;
    case EnumFacing::EAST: return 1;
    }
    return -1;
}

void BlockLever::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (func_181091_e(worldIn, pos, state) && !func_181090_a(worldIn, pos, getOpposite(info(state->getValue(FACING)).facing))) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
}

bool BlockLever::func_181091_e(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (canPlaceBlockAt(worldIn, pos))
        return true;
    dropBlockAsItem(worldIn, pos, state, 0);
    worldIn->setBlockToAir(pos);
    return false;
}

void BlockLever::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    float f = 0.1875f;
    switch (worldIn->getBlockState(pos)->getValue(FACING)) {
    case EnumOrientation::EAST: setBlockBounds(0.0f, 0.2f, 0.5f - f, f * 2.0f, 0.8f, 0.5f + f); break;
    case EnumOrientation::WEST: setBlockBounds(1.0f - f * 2.0f, 0.2f, 0.5f - f, 1.0f, 0.8f, 0.5f + f); break;
    case EnumOrientation::SOUTH: setBlockBounds(0.5f - f, 0.2f, 0.0f, 0.5f + f, 0.8f, f * 2.0f); break;
    case EnumOrientation::NORTH: setBlockBounds(0.5f - f, 0.2f, 1.0f - f * 2.0f, 0.5f + f, 0.8f, 1.0f); break;
    case EnumOrientation::UP_Z:
    case EnumOrientation::UP_X:
        f = 0.25f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.6f, 0.5f + f);
        break;
    case EnumOrientation::DOWN_X:
    case EnumOrientation::DOWN_Z:
        f = 0.25f;
        setBlockBounds(0.5f - f, 0.4f, 0.5f - f, 0.5f + f, 1.0f, 0.5f + f);
        break;
    }
}

bool BlockLever::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer*, EnumFacing, float, float, float)
{
    if (worldIn->isRemote)
        return true;
    state = state->cycleProperty(POWERED);
    worldIn->setBlockState(pos, state, 3);
    worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "random.click", 0.3f, state->getValue(POWERED) ? 0.6f : 0.5f);
    worldIn->notifyNeighborsOfStateChange(pos, this);
    const EnumFacing enumfacing = info(state->getValue(FACING)).facing;
    worldIn->notifyNeighborsOfStateChange(pos.offset(getOpposite(enumfacing)), this);
    return true;
}

void BlockLever::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (state->getValue(POWERED)) {
        worldIn->notifyNeighborsOfStateChange(pos, this);
        const EnumFacing enumfacing = info(state->getValue(FACING)).facing;
        worldIn->notifyNeighborsOfStateChange(pos.offset(getOpposite(enumfacing)), this);
    }
    Block::breakBlock(worldIn, pos, state);
}

// --- BlockButton ---

bool BlockButton::canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side)
{
    return func_181088_a(worldIn, pos, getOpposite(side));
}

bool BlockButton::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    for (const EnumFacing enumfacing : kFacings) {
        if (func_181088_a(worldIn, pos, enumfacing))
            return true;
    }
    return false;
}

bool BlockButton::func_181088_a(World* worldIn, const BlockPos& pos, EnumFacing side)
{
    const BlockPos blockpos = pos.offset(side);
    return side == EnumFacing::DOWN ? World::doesBlockHaveSolidTopSurface(worldIn, blockpos) : worldIn->getBlockState(blockpos)->getBlock()->isNormalCube();
}

const IBlockState* BlockButton::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float, float, float, int, EntityLivingBase*)
{
    return func_181088_a(worldIn, pos, getOpposite(facing)) ? getDefaultState()->withProperty(FACING, facing)->withProperty(POWERED, false)
                                                            : getDefaultState()->withProperty(FACING, EnumFacing::DOWN)->withProperty(POWERED, false);
}

void BlockButton::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (checkForDrop(worldIn, pos, state) && !func_181088_a(worldIn, pos, getOpposite(state->getValue(FACING)))) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
}

bool BlockButton::checkForDrop(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (canPlaceBlockAt(worldIn, pos))
        return true;
    dropBlockAsItem(worldIn, pos, state, 0);
    worldIn->setBlockToAir(pos);
    return false;
}

void BlockButton::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    updateBlockBounds(worldIn->getBlockState(pos));
}

void BlockButton::updateBlockBounds(const IBlockState* state)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    const bool flag = state->getValue(POWERED);
    const float f2 = static_cast<float>(flag ? 1 : 2) / 16.0f;
    switch (enumfacing) {
    case EnumFacing::EAST: setBlockBounds(0.0f, 0.375f, 0.3125f, f2, 0.625f, 0.6875f); break;
    case EnumFacing::WEST: setBlockBounds(1.0f - f2, 0.375f, 0.3125f, 1.0f, 0.625f, 0.6875f); break;
    case EnumFacing::SOUTH: setBlockBounds(0.3125f, 0.375f, 0.0f, 0.6875f, 0.625f, f2); break;
    case EnumFacing::NORTH: setBlockBounds(0.3125f, 0.375f, 1.0f - f2, 0.6875f, 0.625f, 1.0f); break;
    case EnumFacing::UP: setBlockBounds(0.3125f, 0.0f, 0.375f, 0.6875f, 0.0f + f2, 0.625f); break;
    case EnumFacing::DOWN: setBlockBounds(0.3125f, 1.0f - f2, 0.375f, 0.6875f, 1.0f, 0.625f); break;
    }
}

bool BlockButton::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer*, EnumFacing, float, float, float)
{
    if (state->getValue(POWERED))
        return true;
    worldIn->setBlockState(pos, state->withProperty(POWERED, true), 3);
    worldIn->markBlockRangeForRenderUpdate(pos, pos);
    worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "random.click", 0.3f, 0.6f);
    notifyNeighbors(worldIn, pos, state->getValue(FACING));
    worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
    return true;
}

void BlockButton::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (state->getValue(POWERED))
        notifyNeighbors(worldIn, pos, state->getValue(FACING));
    Block::breakBlock(worldIn, pos, state);
}

void BlockButton::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    if (!worldIn->isRemote) {
        if (state->getValue(POWERED)) {
            if (wooden) {
                checkForArrows(worldIn, pos, state);
            } else {
                worldIn->setBlockState(pos, state->withProperty(POWERED, false));
                notifyNeighbors(worldIn, pos, state->getValue(FACING));
                worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "random.click", 0.3f, 0.5f);
                worldIn->markBlockRangeForRenderUpdate(pos, pos);
            }
        }
    }
}

void BlockButton::onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity*)
{
    if (!worldIn->isRemote) {
        if (wooden) {
            if (!state->getValue(POWERED))
                checkForArrows(worldIn, pos, state);
        }
    }
}

void BlockButton::checkForArrows(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    updateBlockBounds(state);
    // Java: !getEntitiesWithinAABB(EntityArrow.class, the button's box).isEmpty() -- arrows come with stage 10.
    const bool flag = false;
    const bool flag1 = state->getValue(POWERED);
    if (flag && !flag1) {
        worldIn->setBlockState(pos, state->withProperty(POWERED, true));
        notifyNeighbors(worldIn, pos, state->getValue(FACING));
        worldIn->markBlockRangeForRenderUpdate(pos, pos);
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "random.click", 0.3f, 0.6f);
    }
    if (!flag && flag1) {
        worldIn->setBlockState(pos, state->withProperty(POWERED, false));
        notifyNeighbors(worldIn, pos, state->getValue(FACING));
        worldIn->markBlockRangeForRenderUpdate(pos, pos);
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "random.click", 0.3f, 0.5f);
    }
    if (flag)
        worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
}

void BlockButton::notifyNeighbors(World* worldIn, const BlockPos& pos, EnumFacing facing)
{
    worldIn->notifyNeighborsOfStateChange(pos, this);
    worldIn->notifyNeighborsOfStateChange(pos.offset(getOpposite(facing)), this);
}

// --- BlockBasePressurePlate, BlockPressurePlate, BlockPressurePlateWeighted ---

void BlockBasePressurePlate::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    setBlockBoundsBasedOnState0(worldIn->getBlockState(pos));
}

void BlockBasePressurePlate::setBlockBoundsBasedOnState0(const IBlockState* state)
{
    const bool flag = getRedstoneStrength(state) > 0;
    if (flag)
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.03125f, 0.9375f);
    else
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.0625f, 0.9375f);
}

void BlockBasePressurePlate::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (!canBePlacedOn(worldIn, pos.down())) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
}

bool BlockBasePressurePlate::canBePlacedOn(World* worldIn, const BlockPos& pos)
{
    return World::doesBlockHaveSolidTopSurface(worldIn, pos) || dynamic_cast<const BlockFence*>(worldIn->getBlockState(pos)->getBlock());
}

void BlockBasePressurePlate::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    if (!worldIn->isRemote) {
        const int i = getRedstoneStrength(state);
        if (i > 0)
            updateState(worldIn, pos, state, i);
    }
}

void BlockBasePressurePlate::onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity*)
{
    if (!worldIn->isRemote) {
        const int i = getRedstoneStrength(state);
        if (i == 0)
            updateState(worldIn, pos, state, i);
    }
}

void BlockBasePressurePlate::updateState(World* worldIn, const BlockPos& pos, const IBlockState* state, int oldRedstoneStrength)
{
    const int i = computeRedstoneStrength(worldIn, pos);
    const bool flag = oldRedstoneStrength > 0;
    const bool flag1 = i > 0;
    if (oldRedstoneStrength != i) {
        state = setRedstoneStrength(state, i);
        worldIn->setBlockState(pos, state, 2);
        updateNeighbors(worldIn, pos);
        worldIn->markBlockRangeForRenderUpdate(pos, pos);
    }
    if (!flag1 && flag)
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.1, pos.getZ() + 0.5, "random.click", 0.3f, 0.5f);
    else if (flag1 && !flag)
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.1, pos.getZ() + 0.5, "random.click", 0.3f, 0.6f);
    if (flag1)
        worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
}

AxisAlignedBB BlockBasePressurePlate::getSensitiveAABB(const BlockPos& pos) const
{
    return AxisAlignedBB(static_cast<float>(pos.getX()) + 0.125f, pos.getY(), static_cast<float>(pos.getZ()) + 0.125f,
        static_cast<float>(pos.getX() + 1) - 0.125f, pos.getY() + 0.25, static_cast<float>(pos.getZ() + 1) - 0.125f);
}

void BlockBasePressurePlate::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (getRedstoneStrength(state) > 0)
        updateNeighbors(worldIn, pos);
    Block::breakBlock(worldIn, pos, state);
}

void BlockBasePressurePlate::updateNeighbors(World* worldIn, const BlockPos& pos)
{
    worldIn->notifyNeighborsOfStateChange(pos, this);
    worldIn->notifyNeighborsOfStateChange(pos.down(), this);
}

int BlockPressurePlate::computeRedstoneStrength(World* worldIn, const BlockPos& pos)
{
    const AxisAlignedBB axisalignedbb = getSensitiveAABB(pos);
    std::vector<Entity*> list;
    switch (sensitivity) {
    case Sensitivity::EVERYTHING:
        list = worldIn->getEntitiesWithinAABBExcludingEntity(nullptr, axisalignedbb);
        break;
    case Sensitivity::MOBS:
        // Java: getEntitiesWithinAABB(EntityLivingBase.class, axisalignedbb) -- living entities come with stage 8.
        break;
    default:
        return 0;
    }
    for (Entity* entity : list) {
        if (!entity->doesEntityNotTriggerPressurePlate())
            return 15;
    }
    return 0;
}

int BlockPressurePlateWeighted::computeRedstoneStrength(World* worldIn, const BlockPos& pos)
{
    const int i = std::min(static_cast<int>(worldIn->getEntitiesWithinAABB<Entity>(getSensitiveAABB(pos)).size()), field_150068_a);
    if (i > 0) {
        const float f = static_cast<float>(std::min(field_150068_a, i)) / static_cast<float>(field_150068_a);
        return MathHelper::ceiling_float_int(f * 15.0f);
    }
    return 0;
}

// --- BlockTNT ---

void BlockTNT::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    Block::onBlockAdded(worldIn, pos, state);
    if (worldIn->isBlockPowered(pos)) {
        onBlockDestroyedByPlayer(worldIn, pos, state->withProperty(EXPLODE, true));
        worldIn->setBlockToAir(pos);
    }
}

void BlockTNT::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (worldIn->isBlockPowered(pos)) {
        onBlockDestroyedByPlayer(worldIn, pos, state->withProperty(EXPLODE, true));
        worldIn->setBlockToAir(pos);
    }
}

void BlockTNT::onBlockDestroyedByExplosion(World* worldIn, const BlockPos&, Explosion*)
{
    if (!worldIn->isRemote) {
        // Java: spawns an EntityTNTPrimed (fuse 80, placed by explosionIn.getExplosivePlacedBy()) with
        // fuse = rand.nextInt(fuse / 4) + fuse / 8 -- the entity comes with stage 10; the draw stays in step.
        const int fuse = 80;
        [[maybe_unused]] const int newFuse = worldIn->rand.nextInt(fuse / 4) + fuse / 8;
    }
}

void BlockTNT::onBlockDestroyedByPlayer(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    explode(worldIn, pos, state, nullptr);
}

void BlockTNT::explode(World* worldIn, const BlockPos&, const IBlockState* state, EntityLivingBase*)
{
    if (!worldIn->isRemote) {
        if (state->getValue(EXPLODE)) {
            // Java: spawns an EntityTNTPrimed at (x + 0.5, y, z + 0.5) for the igniter and plays
            // "game.tnt.primed" at it -- the entity comes with stage 10.
        }
    }
}

// --- BlockTripWire ---

const IBlockState* BlockTripWire::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    return state->withProperty(NORTH, isConnectedTo(worldIn, pos, state, EnumFacing::NORTH))
        ->withProperty(EAST, isConnectedTo(worldIn, pos, state, EnumFacing::EAST))
        ->withProperty(SOUTH, isConnectedTo(worldIn, pos, state, EnumFacing::SOUTH))
        ->withProperty(WEST, isConnectedTo(worldIn, pos, state, EnumFacing::WEST));
}

void BlockTripWire::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    const bool flag = state->getValue(SUSPENDED);
    const bool flag1 = !World::doesBlockHaveSolidTopSurface(worldIn, pos.down());
    if (flag != flag1) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
}

void BlockTripWire::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const bool flag = iblockstate->getValue(ATTACHED);
    const bool flag1 = iblockstate->getValue(SUSPENDED);
    if (!flag1)
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.09375f, 1.0f);
    else if (!flag)
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    else
        setBlockBounds(0.0f, 0.0625f, 0.0f, 1.0f, 0.15625f, 1.0f);
}

void BlockTripWire::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    state = state->withProperty(SUSPENDED, !World::doesBlockHaveSolidTopSurface(worldIn, pos.down()));
    worldIn->setBlockState(pos, state, 3);
    notifyHook(worldIn, pos, state);
}

void BlockTripWire::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    notifyHook(worldIn, pos, state->withProperty(POWERED, true));
}

void BlockTripWire::notifyHook(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    for (const EnumFacing enumfacing : { EnumFacing::SOUTH, EnumFacing::WEST }) {
        for (int i = 1; i < 42; ++i) {
            const BlockPos blockpos = pos.offset(enumfacing, i);
            const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
            if (iblockstate->getBlock() == Blocks::tripwire_hook) {
                if (iblockstate->getValue(BlockTripWireHook::FACING) == getOpposite(enumfacing))
                    Blocks::tripwire_hook->func_176260_a(worldIn, blockpos, iblockstate, false, true, i, state);
                break;
            }
            if (iblockstate->getBlock() != Blocks::tripwire)
                break;
        }
    }
}

void BlockTripWire::onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity*)
{
    if (!worldIn->isRemote) {
        if (!state->getValue(POWERED))
            updateState(worldIn, pos);
    }
}

void BlockTripWire::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    if (!worldIn->isRemote) {
        if (worldIn->getBlockState(pos)->getValue(POWERED))
            updateState(worldIn, pos);
    }
}

void BlockTripWire::updateState(World* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const bool flag = iblockstate->getValue(POWERED);
    bool flag1 = false;
    const std::vector<Entity*> list = worldIn->getEntitiesWithinAABBExcludingEntity(nullptr,
        AxisAlignedBB(pos.getX() + minX, pos.getY() + minY, pos.getZ() + minZ, pos.getX() + maxX, pos.getY() + maxY, pos.getZ() + maxZ));
    for (Entity* entity : list) {
        if (!entity->doesEntityNotTriggerPressurePlate()) {
            flag1 = true;
            break;
        }
    }
    if (flag1 != flag) {
        iblockstate = iblockstate->withProperty(POWERED, flag1);
        worldIn->setBlockState(pos, iblockstate, 3);
        notifyHook(worldIn, pos, iblockstate);
    }
    if (flag1)
        worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
}

bool BlockTripWire::isConnectedTo(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing direction)
{
    const BlockPos blockpos = pos.offset(direction);
    const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
    const Block* block = iblockstate->getBlock();
    if (block == Blocks::tripwire_hook) {
        const EnumFacing enumfacing = getOpposite(direction);
        return iblockstate->getValue(BlockTripWireHook::FACING) == enumfacing;
    }
    if (block == Blocks::tripwire) {
        const bool flag = state->getValue(SUSPENDED);
        const bool flag1 = iblockstate->getValue(SUSPENDED);
        return flag == flag1;
    }
    return false;
}

// --- BlockTripWireHook ---

const IBlockState* BlockTripWireHook::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    return state->withProperty(SUSPENDED, !World::doesBlockHaveSolidTopSurface(worldIn, pos.down()));
}

bool BlockTripWireHook::canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side)
{
    return isHorizontal(getAxis(side)) && worldIn->getBlockState(pos.offset(getOpposite(side)))->getBlock()->isNormalCube();
}

bool BlockTripWireHook::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (worldIn->getBlockState(pos.offset(enumfacing))->getBlock()->isNormalCube())
            return true;
    }
    return false;
}

const IBlockState* BlockTripWireHook::onBlockPlaced(World*, const BlockPos&, EnumFacing facing, float, float, float, int, EntityLivingBase*)
{
    const IBlockState* iblockstate = getDefaultState()->withProperty(POWERED, false)->withProperty(ATTACHED, false)->withProperty(SUSPENDED, false);
    if (isHorizontal(getAxis(facing)))
        iblockstate = iblockstate->withProperty(FACING, facing);
    return iblockstate;
}

void BlockTripWireHook::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase*, const ItemStack&)
{
    func_176260_a(worldIn, pos, state, false, false, -1, nullptr);
}

void BlockTripWireHook::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    if (neighborBlock != this) {
        if (checkForDrop(worldIn, pos, state)) {
            const EnumFacing enumfacing = state->getValue(FACING);
            if (!worldIn->getBlockState(pos.offset(getOpposite(enumfacing)))->getBlock()->isNormalCube()) {
                dropBlockAsItem(worldIn, pos, state, 0);
                worldIn->setBlockToAir(pos);
            }
        }
    }
}

void BlockTripWireHook::func_176260_a(World* worldIn, const BlockPos& pos, const IBlockState* hookState, bool p_176260_4_, bool p_176260_5_, int p_176260_6_,
    const IBlockState* p_176260_7_)
{
    const EnumFacing enumfacing = hookState->getValue(FACING);
    const bool flag = hookState->getValue(ATTACHED);
    const bool flag1 = hookState->getValue(POWERED);
    const bool flag2 = !World::doesBlockHaveSolidTopSurface(worldIn, pos.down());
    bool flag3 = !p_176260_4_;
    bool flag4 = false;
    int i = 0;
    std::array<const IBlockState*, 42> aiblockstate {};
    for (int j = 1; j < 42; ++j) {
        const BlockPos blockpos = pos.offset(enumfacing, j);
        const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
        if (iblockstate->getBlock() == Blocks::tripwire_hook) {
            if (iblockstate->getValue(FACING) == getOpposite(enumfacing))
                i = j;
            break;
        }
        if (iblockstate->getBlock() != Blocks::tripwire && j != p_176260_6_) {
            aiblockstate[static_cast<size_t>(j)] = nullptr;
            flag3 = false;
        } else {
            if (j == p_176260_6_)
                iblockstate = p_176260_7_ ? p_176260_7_ : iblockstate; // Java: Objects.firstNonNull
            const bool flag5 = !iblockstate->getValue(BlockTripWire::DISARMED);
            const bool flag6 = iblockstate->getValue(BlockTripWire::POWERED);
            const bool flag7 = iblockstate->getValue(BlockTripWire::SUSPENDED);
            flag3 &= flag7 == flag2;
            flag4 |= flag5 && flag6;
            aiblockstate[static_cast<size_t>(j)] = iblockstate;
            if (j == p_176260_6_) {
                worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
                flag3 &= flag5;
            }
        }
    }
    flag3 = flag3 & (i > 1);
    flag4 = flag4 & flag3;
    const IBlockState* iblockstate1 = getDefaultState()->withProperty(ATTACHED, flag3)->withProperty(POWERED, flag4);
    if (i > 0) {
        const BlockPos blockpos1 = pos.offset(enumfacing, i);
        const EnumFacing enumfacing1 = getOpposite(enumfacing);
        worldIn->setBlockState(blockpos1, iblockstate1->withProperty(FACING, enumfacing1), 3);
        func_176262_b(worldIn, blockpos1, enumfacing1);
        func_180694_a(worldIn, blockpos1, flag3, flag4, flag, flag1);
    }
    func_180694_a(worldIn, pos, flag3, flag4, flag, flag1);
    if (!p_176260_4_) {
        worldIn->setBlockState(pos, iblockstate1->withProperty(FACING, enumfacing), 3);
        if (p_176260_5_)
            func_176262_b(worldIn, pos, enumfacing);
    }
    if (flag != flag3) {
        for (int k = 1; k < i; ++k) {
            const BlockPos blockpos2 = pos.offset(enumfacing, k);
            const IBlockState* iblockstate2 = aiblockstate[static_cast<size_t>(k)];
            if (iblockstate2 != nullptr && worldIn->getBlockState(blockpos2)->getBlock() != Blocks::air)
                worldIn->setBlockState(blockpos2, iblockstate2->withProperty(ATTACHED, flag3), 3);
        }
    }
}

void BlockTripWireHook::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    func_176260_a(worldIn, pos, state, false, true, -1, nullptr);
}

void BlockTripWireHook::func_180694_a(World* worldIn, const BlockPos& pos, bool p_180694_3_, bool p_180694_4_, bool p_180694_5_, bool p_180694_6_)
{
    if (p_180694_4_ && !p_180694_6_) {
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.1, pos.getZ() + 0.5, "random.click", 0.4f, 0.6f);
    } else if (!p_180694_4_ && p_180694_6_) {
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.1, pos.getZ() + 0.5, "random.click", 0.4f, 0.5f);
    } else if (p_180694_3_ && !p_180694_5_) {
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.1, pos.getZ() + 0.5, "random.click", 0.4f, 0.7f);
    } else if (!p_180694_3_ && p_180694_5_) {
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.1, pos.getZ() + 0.5, "random.bowhit", 0.4f, 1.2f / (worldIn->rand.nextFloat() * 0.2f + 0.9f));
    }
}

void BlockTripWireHook::func_176262_b(World* worldIn, const BlockPos& pos, EnumFacing facing)
{
    worldIn->notifyNeighborsOfStateChange(pos, this);
    worldIn->notifyNeighborsOfStateChange(pos.offset(getOpposite(facing)), this);
}

bool BlockTripWireHook::checkForDrop(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!canPlaceBlockAt(worldIn, pos)) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
        return false;
    }
    return true;
}

void BlockTripWireHook::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const float f = 0.1875f;
    switch (worldIn->getBlockState(pos)->getValue(FACING)) {
    case EnumFacing::EAST: setBlockBounds(0.0f, 0.2f, 0.5f - f, f * 2.0f, 0.8f, 0.5f + f); break;
    case EnumFacing::WEST: setBlockBounds(1.0f - f * 2.0f, 0.2f, 0.5f - f, 1.0f, 0.8f, 0.5f + f); break;
    case EnumFacing::SOUTH: setBlockBounds(0.5f - f, 0.2f, 0.0f, 0.5f + f, 0.8f, f * 2.0f); break;
    case EnumFacing::NORTH: setBlockBounds(0.5f - f, 0.2f, 1.0f - f * 2.0f, 0.5f + f, 0.8f, 1.0f); break;
    default: break;
    }
}

void BlockTripWireHook::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const bool flag = state->getValue(ATTACHED);
    const bool flag1 = state->getValue(POWERED);
    if (flag || flag1)
        func_176260_a(worldIn, pos, state, true, false, -1, nullptr);
    if (flag1) {
        worldIn->notifyNeighborsOfStateChange(pos, this);
        worldIn->notifyNeighborsOfStateChange(pos.offset(getOpposite(state->getValue(FACING))), this);
    }
    Block::breakBlock(worldIn, pos, state);
}

// --- BlockRedstoneLight ---

void BlockRedstoneLight::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (!worldIn->isRemote) {
        if (isOn && !worldIn->isBlockPowered(pos))
            worldIn->setBlockState(pos, Blocks::redstone_lamp->getDefaultState(), 2);
        else if (!isOn && worldIn->isBlockPowered(pos))
            worldIn->setBlockState(pos, Blocks::lit_redstone_lamp->getDefaultState(), 2);
    }
}

void BlockRedstoneLight::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState*, Block*)
{
    if (!worldIn->isRemote) {
        if (isOn && !worldIn->isBlockPowered(pos))
            worldIn->scheduleUpdate(pos, this, 4);
        else if (!isOn && worldIn->isBlockPowered(pos))
            worldIn->setBlockState(pos, Blocks::lit_redstone_lamp->getDefaultState(), 2);
    }
}

void BlockRedstoneLight::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    if (!worldIn->isRemote) {
        if (isOn && !worldIn->isBlockPowered(pos))
            worldIn->setBlockState(pos, Blocks::redstone_lamp->getDefaultState(), 2);
    }
}

// --- BlockRedstoneOre ---

void BlockRedstoneOre::onBlockClicked(World* worldIn, const BlockPos& pos, EntityPlayer* playerIn)
{
    activate(worldIn, pos);
    Block::onBlockClicked(worldIn, pos, playerIn);
}

void BlockRedstoneOre::onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, Entity* entityIn)
{
    activate(worldIn, pos);
    Block::onEntityCollidedWithBlock(worldIn, pos, entityIn);
}

bool BlockRedstoneOre::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
    float hitY, float hitZ)
{
    activate(worldIn, pos);
    return Block::onBlockActivated(worldIn, pos, state, playerIn, side, hitX, hitY, hitZ);
}

void BlockRedstoneOre::activate(World* worldIn, const BlockPos& pos)
{
    spawnParticles(worldIn, pos);
    if (this == Blocks::redstone_ore)
        worldIn->setBlockState(pos, Blocks::lit_redstone_ore->getDefaultState());
}

void BlockRedstoneOre::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    if (this == Blocks::lit_redstone_ore)
        worldIn->setBlockState(pos, Blocks::redstone_ore->getDefaultState());
}

void BlockRedstoneOre::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune)
{
    Block::dropBlockAsItemWithChance(worldIn, pos, state, chance, fortune);
    if (getItemDropped(state, worldIn->rand, fortune) != Item::getItemFromBlock(this)) {
        // Java: dropXpOnBlockBreak(worldIn, pos, i) -- experience orbs come with stage 10; the draw stays in step.
        [[maybe_unused]] const int i = 1 + worldIn->rand.nextInt(5);
    }
}

void BlockRedstoneOre::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    if (isOn)
        spawnParticles(worldIn, pos);
}

void BlockRedstoneOre::spawnParticles(World* worldIn, const BlockPos& pos)
{
    Random& random = worldIn->rand;
    const double d0 = 0.0625;
    for (int i = 0; i < 6; ++i) {
        double d1 = static_cast<float>(pos.getX()) + random.nextFloat();
        double d2 = static_cast<float>(pos.getY()) + random.nextFloat();
        double d3 = static_cast<float>(pos.getZ()) + random.nextFloat();
        if (i == 0 && !worldIn->getBlockState(pos.up())->getBlock()->isOpaqueCube())
            d2 = pos.getY() + d0 + 1.0;
        if (i == 1 && !worldIn->getBlockState(pos.down())->getBlock()->isOpaqueCube())
            d2 = pos.getY() - d0;
        if (i == 2 && !worldIn->getBlockState(pos.south())->getBlock()->isOpaqueCube())
            d3 = pos.getZ() + d0 + 1.0;
        if (i == 3 && !worldIn->getBlockState(pos.north())->getBlock()->isOpaqueCube())
            d3 = pos.getZ() - d0;
        if (i == 4 && !worldIn->getBlockState(pos.east())->getBlock()->isOpaqueCube())
            d1 = pos.getX() + d0 + 1.0;
        if (i == 5 && !worldIn->getBlockState(pos.west())->getBlock()->isOpaqueCube())
            d1 = pos.getX() - d0;
        // Java compares d2 with 0, not with y.
        if (d1 < pos.getX() || d1 > pos.getX() + 1 || d2 < 0.0 || d2 > pos.getY() + 1 || d3 < pos.getZ() || d3 > pos.getZ() + 1)
            spawnParticle(worldIn, EnumParticleTypes::REDSTONE, d1, d2, d3, 0.0, 0.0, 0.0);
    }
}

// --- BlockDaylightDetector ---

void BlockDaylightDetector::updatePower(World* worldIn, const BlockPos& pos)
{
    if (!worldIn->provider->getHasNoSky()) {
        const IBlockState* iblockstate = worldIn->getBlockState(pos);
        int i = worldIn->getLightFor(EnumSkyBlock::SKY, pos) - worldIn->getSkylightSubtracted();
        float f = worldIn->getCelestialAngleRadians(1.0f);
        const float pi = static_cast<float>(3.141592653589793);
        const float f1 = f < pi ? 0.0f : pi * 2.0f;
        f = f + (f1 - f) * 0.2f;
        i = java::round(static_cast<float>(i) * MathHelper::cos(f));
        i = MathHelper::clamp_int(i, 0, 15);
        if (inverted)
            i = 15 - i;
        if (iblockstate->getValue(POWER) != i)
            worldIn->setBlockState(pos, iblockstate->withProperty(POWER, i), 3);
    }
}

std::shared_ptr<TileEntity> BlockDaylightDetector::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityDaylightDetector>();
}

// --- BlockNote ---

namespace {

std::string getInstrument(int id)
{
    static const char* const INSTRUMENTS[] = { "harp", "bd", "snare", "hat", "bassattack" };
    if (id < 0 || id >= 5)
        id = 0;
    return INSTRUMENTS[id];
}

} // namespace

void BlockNote::triggerNote(TileEntityNote* te, World* worldIn, const BlockPos& pos)
{
    if (&worldIn->getBlockState(pos.up())->getBlock()->getMaterial() == &Material::air) {
        const Material* material = &worldIn->getBlockState(pos.down())->getBlock()->getMaterial();
        int i = 0;
        if (material == &Material::rock)
            i = 1;
        if (material == &Material::sand)
            i = 2;
        if (material == &Material::glass)
            i = 3;
        if (material == &Material::wood)
            i = 4;
        worldIn->addBlockEvent(pos, Blocks::noteblock, i, te->note);
    }
}

void BlockNote::changePitch(TileEntityNote* te)
{
    te->note = static_cast<int8_t>((te->note + 1) % 25);
    te->markDirty();
}

void BlockNote::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState*, Block*)
{
    const bool flag = worldIn->isBlockPowered(pos);
    if (auto* tileentitynote = dynamic_cast<TileEntityNote*>(worldIn->getTileEntity(pos))) {
        if (tileentitynote->previousRedstoneState != flag) {
            if (flag)
                triggerNote(tileentitynote, worldIn, pos);
            tileentitynote->previousRedstoneState = flag;
        }
    }
}

bool BlockNote::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState*, EntityPlayer*, EnumFacing, float, float, float)
{
    if (worldIn->isRemote)
        return true;
    if (auto* tileentitynote = dynamic_cast<TileEntityNote*>(worldIn->getTileEntity(pos))) {
        changePitch(tileentitynote);
        triggerNote(tileentitynote, worldIn, pos);
        // Java: playerIn.triggerAchievement(StatList.field_181735_S) -- stats come with stage 8.
    }
    return true;
}

void BlockNote::onBlockClicked(World* worldIn, const BlockPos& pos, EntityPlayer*)
{
    if (!worldIn->isRemote) {
        if (auto* tileentitynote = dynamic_cast<TileEntityNote*>(worldIn->getTileEntity(pos))) {
            triggerNote(tileentitynote, worldIn, pos);
            // Java: playerIn.triggerAchievement(StatList.field_181734_R) -- stats come with stage 8.
        }
    }
}

std::shared_ptr<TileEntity> BlockNote::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityNote>();
}

bool BlockNote::onBlockEventReceived(World* worldIn, const BlockPos& pos, const IBlockState*, int eventID, int eventParam)
{
    const float f = static_cast<float>(std::pow(2.0, static_cast<double>(eventParam - 12) / 12.0));
    worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "note." + getInstrument(eventID), 3.0f, f);
    spawnParticle(worldIn, EnumParticleTypes::NOTE, pos.getX() + 0.5, pos.getY() + 1.2, pos.getZ() + 0.5, static_cast<double>(eventParam) / 24.0, 0.0, 0.0);
    return true;
}

// --- BlockMobSpawner ---

std::shared_ptr<TileEntity> BlockMobSpawner::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityMobSpawner>();
}

void BlockMobSpawner::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune)
{
    BlockContainer::dropBlockAsItemWithChance(worldIn, pos, state, chance, fortune);
    // Java: dropXpOnBlockBreak(worldIn, pos, i) -- experience orbs come with stage 10; the draws stay in step.
    const int a = worldIn->rand.nextInt(15);
    const int b = worldIn->rand.nextInt(15);
    [[maybe_unused]] const int i = 15 + a + b;
}

} // namespace mc
