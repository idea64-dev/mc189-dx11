// Original: net/minecraft/block/BlockLiquid.java, BlockDynamicLiquid.java and BlockStaticLiquid.java -- the world
// side: flow, mixing, lava fire spread and the client effects.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"
#include "util/MathHelper.h"
#include "world/GameRules.h"
#include "world/WorldProvider.h"

#include <stdexcept>

namespace mc {

// --- BlockLiquid ---

int BlockLiquid::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int)
{
    return &blockMaterial == &Material::water ? BiomeColorHelper::getWaterColorAtPos(worldIn, pos) : 16777215;
}

float BlockLiquid::getLiquidHeightPercent(int meta)
{
    if (meta >= 8)
        meta = 0;
    return static_cast<float>(meta + 1) / 9.0f;
}

int BlockLiquid::getLevel(IBlockAccess* worldIn, const BlockPos& pos)
{
    const IBlockState* state = worldIn->getBlockState(pos);
    return &state->getBlock()->getMaterial() == &blockMaterial ? state->getValue(LEVEL) : -1;
}

int BlockLiquid::getEffectiveFlowDecay(IBlockAccess* worldIn, const BlockPos& pos)
{
    const int i = getLevel(worldIn, pos);
    return i >= 8 ? 0 : i;
}

bool BlockLiquid::isBlockSolid(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    const Material& material = worldIn->getBlockState(pos)->getBlock()->getMaterial();
    return &material == &blockMaterial ? false : (side == EnumFacing::UP ? true : (&material == &Material::ice ? false : Block::isBlockSolid(worldIn, pos, side)));
}

bool BlockLiquid::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    return &worldIn->getBlockState(pos)->getBlock()->getMaterial() == &blockMaterial ? false
                                                                                   : (side == EnumFacing::UP ? true : Block::shouldSideBeRendered(worldIn, pos, side));
}

bool BlockLiquid::shouldRenderSides(IBlockAccess* blockAccess, const BlockPos& pos)
{
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            const Block* block = blockAccess->getBlockState(pos.add(i, 0, j))->getBlock();
            if (&block->getMaterial() != &blockMaterial && !block->isFullBlock())
                return true;
        }
    }
    return false;
}

Vec3 BlockLiquid::getFlowVector(IBlockAccess* worldIn, const BlockPos& pos)
{
    Vec3 vec3(0.0, 0.0, 0.0);
    const int i = getEffectiveFlowDecay(worldIn, pos);
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        const BlockPos blockpos = pos.offset(enumfacing);
        int j = getEffectiveFlowDecay(worldIn, blockpos);
        if (j < 0) {
            if (!worldIn->getBlockState(blockpos)->getBlock()->getMaterial().blocksMovement()) {
                j = getEffectiveFlowDecay(worldIn, blockpos.down());
                if (j >= 0) {
                    const int k = j - (i - 8);
                    vec3 = vec3.addVector(static_cast<double>((blockpos.getX() - pos.getX()) * k), static_cast<double>((blockpos.getY() - pos.getY()) * k),
                        static_cast<double>((blockpos.getZ() - pos.getZ()) * k));
                }
            }
        } else if (j >= 0) {
            const int l = j - i;
            vec3 = vec3.addVector(static_cast<double>((blockpos.getX() - pos.getX()) * l), static_cast<double>((blockpos.getY() - pos.getY()) * l),
                static_cast<double>((blockpos.getZ() - pos.getZ()) * l));
        }
    }
    if (worldIn->getBlockState(pos)->getValue(LEVEL) >= 8) {
        for (const EnumFacing enumfacing1 : kPlaneHorizontal) {
            const BlockPos blockpos1 = pos.offset(enumfacing1);
            if (isBlockSolid(worldIn, blockpos1, enumfacing1) || isBlockSolid(worldIn, blockpos1.up(), enumfacing1)) {
                vec3 = vec3.normalize().addVector(0.0, -6.0, 0.0);
                break;
            }
        }
    }
    return vec3.normalize();
}

Vec3 BlockLiquid::modifyAcceleration(World* worldIn, const BlockPos& pos, Entity*, const Vec3& motion)
{
    return motion.add(getFlowVector(worldIn, pos));
}

int BlockLiquid::tickRate(World* worldIn)
{
    return &blockMaterial == &Material::water ? 5 : (&blockMaterial == &Material::lava ? (worldIn->provider->getHasNoSky() ? 10 : 30) : 0);
}

int BlockLiquid::getMixedBrightnessForBlock(IBlockAccess* worldIn, const BlockPos& pos)
{
    const int i = worldIn->getCombinedLight(pos, 0);
    const int j = worldIn->getCombinedLight(pos.up(), 0);
    const int k = i & 255;
    const int l = j & 255;
    const int i1 = i >> 16 & 255;
    const int j1 = j >> 16 & 255;
    return (k > l ? k : l) | (i1 > j1 ? i1 : j1) << 16;
}

void BlockLiquid::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    const double d0 = static_cast<double>(pos.getX());
    const double d1 = static_cast<double>(pos.getY());
    const double d2 = static_cast<double>(pos.getZ());
    if (&blockMaterial == &Material::water) {
        const int i = state->getValue(LEVEL);
        if (i > 0 && i < 8) {
            if (rand.nextInt(64) == 0) {
                const float volume = rand.nextFloat() * 0.25f + 0.75f;
                const float pitch = rand.nextFloat() * 1.0f + 0.5f;
                worldIn->playSound(d0 + 0.5, d1 + 0.5, d2 + 0.5, "liquid.water", volume, pitch, false);
            }
        } else if (rand.nextInt(10) == 0) {
            const double x = d0 + static_cast<double>(rand.nextFloat());
            const double y = d1 + static_cast<double>(rand.nextFloat());
            const double z = d2 + static_cast<double>(rand.nextFloat());
            spawnParticle(worldIn, EnumParticleTypes::SUSPENDED, x, y, z, 0.0, 0.0, 0.0);
        }
    }
    if (&blockMaterial == &Material::lava && &worldIn->getBlockState(pos.up())->getBlock()->getMaterial() == &Material::air
        && !worldIn->getBlockState(pos.up())->getBlock()->isOpaqueCube()) {
        if (rand.nextInt(100) == 0) {
            const double d8 = d0 + static_cast<double>(rand.nextFloat());
            const double d4 = d1 + maxY;
            const double d6 = d2 + static_cast<double>(rand.nextFloat());
            spawnParticle(worldIn, EnumParticleTypes::LAVA, d8, d4, d6, 0.0, 0.0, 0.0);
            const float volume = 0.2f + rand.nextFloat() * 0.2f;
            const float pitch = 0.9f + rand.nextFloat() * 0.15f;
            worldIn->playSound(d8, d4, d6, "liquid.lavapop", volume, pitch, false);
        }
        if (rand.nextInt(200) == 0) {
            const float volume = 0.2f + rand.nextFloat() * 0.2f;
            const float pitch = 0.9f + rand.nextFloat() * 0.15f;
            worldIn->playSound(d0, d1, d2, "liquid.lava", volume, pitch, false);
        }
    }
    if (rand.nextInt(10) == 0 && World::doesBlockHaveSolidTopSurface(worldIn, pos.down())) {
        const Material& material = worldIn->getBlockState(pos.down(2))->getBlock()->getMaterial();
        if (!material.blocksMovement() && !material.isLiquid()) {
            const double d3 = d0 + static_cast<double>(rand.nextFloat());
            const double d5 = d1 - 1.05;
            const double d7 = d2 + static_cast<double>(rand.nextFloat());
            spawnParticle(worldIn, &blockMaterial == &Material::water ? EnumParticleTypes::DRIP_WATER : EnumParticleTypes::DRIP_LAVA, d3, d5, d7, 0.0, 0.0, 0.0);
        }
    }
}

double BlockLiquid::getFlowDirection(IBlockAccess* worldIn, const BlockPos& pos, const Material& materialIn)
{
    const Vec3 vec3 = getFlowingBlock(materialIn)->getFlowVector(worldIn, pos);
    return vec3.xCoord == 0.0 && vec3.zCoord == 0.0 ? -1000.0 : MathHelper::atan2(vec3.zCoord, vec3.xCoord) - (3.141592653589793 / 2.0);
}

void BlockLiquid::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    checkForMixing(worldIn, pos, state);
}

void BlockLiquid::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    checkForMixing(worldIn, pos, state);
}

bool BlockLiquid::checkForMixing(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (&blockMaterial == &Material::lava) {
        bool flag = false;
        for (const EnumFacing enumfacing : kFacings) {
            if (enumfacing != EnumFacing::DOWN && &worldIn->getBlockState(pos.offset(enumfacing))->getBlock()->getMaterial() == &Material::water) {
                flag = true;
                break;
            }
        }
        if (flag) {
            const int level = state->getValue(LEVEL);
            if (level == 0) {
                worldIn->setBlockState(pos, Blocks::obsidian->getDefaultState());
                triggerMixEffects(worldIn, pos);
                return true;
            }
            if (level <= 4) {
                worldIn->setBlockState(pos, Blocks::cobblestone->getDefaultState());
                triggerMixEffects(worldIn, pos);
                return true;
            }
        }
    }
    return false;
}

void BlockLiquid::triggerMixEffects(World* worldIn, const BlockPos& pos)
{
    const double d0 = static_cast<double>(pos.getX());
    const double d1 = static_cast<double>(pos.getY());
    const double d2 = static_cast<double>(pos.getZ());
    const float a = worldIn->rand.nextFloat();
    const float b = worldIn->rand.nextFloat();
    worldIn->playSoundEffect(d0 + 0.5, d1 + 0.5, d2 + 0.5, "random.fizz", 0.5f, 2.6f + (a - b) * 0.8f);
    for (int i = 0; i < 8; ++i) {
        const double x = d0 + mathRandom();
        const double z = d2 + mathRandom();
        spawnParticle(worldIn, EnumParticleTypes::SMOKE_LARGE, x, d1 + 1.2, z, 0.0, 0.0, 0.0);
    }
}

BlockDynamicLiquid* BlockLiquid::getFlowingBlock(const Material& materialIn)
{
    if (&materialIn == &Material::water)
        return Blocks::flowing_water;
    if (&materialIn == &Material::lava)
        return Blocks::flowing_lava;
    throw std::invalid_argument("Invalid material");
}

BlockStaticLiquid* BlockLiquid::getStaticBlock(const Material& materialIn)
{
    if (&materialIn == &Material::water)
        return Blocks::water;
    if (&materialIn == &Material::lava)
        return Blocks::lava;
    throw std::invalid_argument("Invalid material");
}

// --- BlockDynamicLiquid ---

void BlockDynamicLiquid::placeStaticBlock(World* worldIn, const BlockPos& pos, const IBlockState* currentState)
{
    worldIn->setBlockState(pos, getStaticBlock(blockMaterial)->getDefaultState()->withProperty(LEVEL, currentState->getValue(LEVEL)), 2);
}

void BlockDynamicLiquid::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    int i = state->getValue(LEVEL);
    int j = 1;
    if (&blockMaterial == &Material::lava && !worldIn->provider->doesWaterVaporize())
        j = 2;
    int k = tickRate(worldIn);
    if (i > 0) {
        int l = -100;
        adjacentSourceBlocks = 0;
        for (const EnumFacing enumfacing : kPlaneHorizontal)
            l = checkAdjacentBlock(worldIn, pos.offset(enumfacing), l);
        int i1 = l + j;
        if (i1 >= 8 || l < 0)
            i1 = -1;
        if (getLevel(worldIn, pos.up()) >= 0) {
            const int j1 = getLevel(worldIn, pos.up());
            i1 = j1 >= 8 ? j1 : j1 + 8;
        }
        if (adjacentSourceBlocks >= 2 && &blockMaterial == &Material::water) {
            const IBlockState* iblockstate1 = worldIn->getBlockState(pos.down());
            if (iblockstate1->getBlock()->getMaterial().isSolid())
                i1 = 0;
            else if (&iblockstate1->getBlock()->getMaterial() == &blockMaterial && iblockstate1->getValue(LEVEL) == 0)
                i1 = 0;
        }
        if (&blockMaterial == &Material::lava && i < 8 && i1 < 8 && i1 > i && rand.nextInt(4) != 0)
            k *= 4;
        if (i1 == i) {
            placeStaticBlock(worldIn, pos, state);
        } else {
            i = i1;
            if (i1 < 0) {
                worldIn->setBlockToAir(pos);
            } else {
                state = state->withProperty(LEVEL, i1);
                worldIn->setBlockState(pos, state, 2);
                worldIn->scheduleUpdate(pos, this, k);
                worldIn->notifyNeighborsOfStateChange(pos, this);
            }
        }
    } else {
        placeStaticBlock(worldIn, pos, state);
    }
    const IBlockState* iblockstate = worldIn->getBlockState(pos.down());
    if (canFlowInto(worldIn, pos.down(), iblockstate)) {
        if (&blockMaterial == &Material::lava && &worldIn->getBlockState(pos.down())->getBlock()->getMaterial() == &Material::water) {
            worldIn->setBlockState(pos.down(), Blocks::stone->getDefaultState());
            triggerMixEffects(worldIn, pos.down());
            return;
        }
        if (i >= 8)
            tryFlowInto(worldIn, pos.down(), iblockstate, i);
        else
            tryFlowInto(worldIn, pos.down(), iblockstate, i + 8);
    } else if (i >= 0 && (i == 0 || isBlocked(worldIn, pos.down(), iblockstate))) {
        const std::vector<EnumFacing> set = getPossibleFlowDirections(worldIn, pos);
        int k1 = i + j;
        if (i >= 8)
            k1 = 1;
        if (k1 >= 8)
            return;
        for (const EnumFacing enumfacing1 : set)
            tryFlowInto(worldIn, pos.offset(enumfacing1), worldIn->getBlockState(pos.offset(enumfacing1)), k1);
    }
}

void BlockDynamicLiquid::tryFlowInto(World* worldIn, const BlockPos& pos, const IBlockState* state, int level)
{
    if (canFlowInto(worldIn, pos, state)) {
        if (state->getBlock() != Blocks::air) {
            if (&blockMaterial == &Material::lava)
                triggerMixEffects(worldIn, pos);
            else
                state->getBlock()->dropBlockAsItem(worldIn, pos, state, 0);
        }
        worldIn->setBlockState(pos, getDefaultState()->withProperty(LEVEL, level), 3);
    }
}

int BlockDynamicLiquid::func_176374_a(World* worldIn, const BlockPos& pos, int distance, EnumFacing calculateFlowCost)
{
    int i = 1000;
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (enumfacing != calculateFlowCost) {
            const BlockPos blockpos = pos.offset(enumfacing);
            const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
            if (!isBlocked(worldIn, blockpos, iblockstate)
                && (&iblockstate->getBlock()->getMaterial() != &blockMaterial || iblockstate->getValue(LEVEL) > 0)) {
                if (!isBlocked(worldIn, blockpos.down(), iblockstate))
                    return distance;
                if (distance < 4) {
                    const int j = func_176374_a(worldIn, blockpos, distance + 1, getOpposite(enumfacing));
                    if (j < i)
                        i = j;
                }
            }
        }
    }
    return i;
}

std::vector<EnumFacing> BlockDynamicLiquid::getPossibleFlowDirections(World* worldIn, const BlockPos& pos)
{
    int i = 1000;
    bool set[6] {};
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        const BlockPos blockpos = pos.offset(enumfacing);
        const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
        if (!isBlocked(worldIn, blockpos, iblockstate) && (&iblockstate->getBlock()->getMaterial() != &blockMaterial || iblockstate->getValue(LEVEL) > 0)) {
            int j;
            if (isBlocked(worldIn, blockpos.down(), worldIn->getBlockState(blockpos.down())))
                j = func_176374_a(worldIn, blockpos, 1, getOpposite(enumfacing));
            else
                j = 0;
            if (j < i) {
                for (bool& b : set)
                    b = false;
            }
            if (j <= i) {
                set[ordinal(enumfacing)] = true;
                i = j;
            }
        }
    }
    std::vector<EnumFacing> result;
    for (const EnumFacing facing : kFacings) {
        if (set[ordinal(facing)])
            result.push_back(facing);
    }
    return result;
}

bool BlockDynamicLiquid::isBlocked(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    const Block* block = worldIn->getBlockState(pos)->getBlock();
    if (dynamic_cast<const BlockDoor*>(block) || block == Blocks::standing_sign || block == Blocks::ladder || block == Blocks::reeds)
        return true;
    return &block->getMaterial() == &Material::portal ? true : block->getMaterial().blocksMovement();
}

int BlockDynamicLiquid::checkAdjacentBlock(World* worldIn, const BlockPos& pos, int currentMinLevel)
{
    int i = getLevel(worldIn, pos);
    if (i < 0)
        return currentMinLevel;
    if (i == 0)
        ++adjacentSourceBlocks;
    if (i >= 8)
        i = 0;
    return currentMinLevel >= 0 && i >= currentMinLevel ? currentMinLevel : i;
}

bool BlockDynamicLiquid::canFlowInto(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const Material& material = state->getBlock()->getMaterial();
    return &material != &blockMaterial && &material != &Material::lava && !isBlocked(worldIn, pos, state);
}

void BlockDynamicLiquid::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!checkForMixing(worldIn, pos, state))
        worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
}

// --- BlockStaticLiquid ---

void BlockStaticLiquid::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (!checkForMixing(worldIn, pos, state)) {
        // Java: updateLiquid
        BlockDynamicLiquid* blockdynamicliquid = getFlowingBlock(blockMaterial);
        worldIn->setBlockState(pos, blockdynamicliquid->getDefaultState()->withProperty(LEVEL, state->getValue(LEVEL)), 2);
        worldIn->scheduleUpdate(pos, blockdynamicliquid, tickRate(worldIn));
    }
}

void BlockStaticLiquid::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random& rand)
{
    if (&blockMaterial == &Material::lava && worldIn->getGameRules().getBoolean("doFireTick")) {
        const int i = rand.nextInt(3);
        if (i > 0) {
            BlockPos blockpos = pos;
            for (int j = 0; j < i; ++j) {
                const int dx = rand.nextInt(3) - 1;
                const int dz = rand.nextInt(3) - 1;
                blockpos = blockpos.add(dx, 1, dz);
                const Block* block = worldIn->getBlockState(blockpos)->getBlock();
                if (&block->getMaterial() == &Material::air) {
                    if (isSurroundingBlockFlammable(worldIn, blockpos)) {
                        worldIn->setBlockState(blockpos, Blocks::fire->getDefaultState());
                        return;
                    }
                } else if (block->getMaterial().blocksMovement()) {
                    return;
                }
            }
        } else {
            for (int k = 0; k < 3; ++k) {
                const int dx = rand.nextInt(3) - 1;
                const int dz = rand.nextInt(3) - 1;
                const BlockPos blockpos1 = pos.add(dx, 0, dz);
                if (worldIn->isAirBlock(blockpos1.up()) && getCanBlockBurn(worldIn, blockpos1))
                    worldIn->setBlockState(blockpos1.up(), Blocks::fire->getDefaultState());
            }
        }
    }
}

bool BlockStaticLiquid::isSurroundingBlockFlammable(World* worldIn, const BlockPos& pos)
{
    for (const EnumFacing enumfacing : kFacings) {
        if (getCanBlockBurn(worldIn, pos.offset(enumfacing)))
            return true;
    }
    return false;
}

bool BlockStaticLiquid::getCanBlockBurn(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos)->getBlock()->getMaterial().getCanBurn();
}

} // namespace mc
