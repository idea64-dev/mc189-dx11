// Original: net/minecraft/block/BlockFire.java, BlockPortal.java (with Size), BlockEndPortal.java,
// BlockEndPortalFrame.java, BlockBreakable.java, BlockIce.java and BlockStainedGlass.java -- the world side.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"
#include "tileentity/TileEntities.h"
#include "world/EnumDifficulty.h"
#include "world/EnumSkyBlock.h"
#include "world/GameRules.h"
#include "world/WorldProvider.h"

#include <algorithm>

namespace mc {

// --- BlockFire ---

const IBlockState* BlockFire::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    const int i = pos.getX();
    const int j = pos.getY();
    const int k = pos.getZ();
    if (!World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) && !Blocks::fire->canCatchFire(worldIn, pos.down())) {
        const bool flag = (i + j + k & 1) == 1;
        const bool flag1 = (i / 2 + j / 2 + k / 2 & 1) == 1;
        int l = 0;
        if (canCatchFire(worldIn, pos.up()))
            l = flag ? 1 : 2;
        return state->withProperty(NORTH, canCatchFire(worldIn, pos.north()))
            ->withProperty(EAST, canCatchFire(worldIn, pos.east()))
            ->withProperty(SOUTH, canCatchFire(worldIn, pos.south()))
            ->withProperty(WEST, canCatchFire(worldIn, pos.west()))
            ->withProperty(UPPER, l)
            ->withProperty(FLIP, flag1)
            ->withProperty(ALT, flag);
    }
    return getDefaultState();
}

void BlockFire::init()
{
    BlockFire* fire = Blocks::fire;
    fire->setFireInfo(Blocks::planks, 5, 20);
    fire->setFireInfo(Blocks::double_wooden_slab, 5, 20);
    fire->setFireInfo(Blocks::wooden_slab, 5, 20);
    fire->setFireInfo(Blocks::oak_fence_gate, 5, 20);
    fire->setFireInfo(Blocks::spruce_fence_gate, 5, 20);
    fire->setFireInfo(Blocks::birch_fence_gate, 5, 20);
    fire->setFireInfo(Blocks::jungle_fence_gate, 5, 20);
    fire->setFireInfo(Blocks::dark_oak_fence_gate, 5, 20);
    fire->setFireInfo(Blocks::acacia_fence_gate, 5, 20);
    fire->setFireInfo(Blocks::oak_fence, 5, 20);
    fire->setFireInfo(Blocks::spruce_fence, 5, 20);
    fire->setFireInfo(Blocks::birch_fence, 5, 20);
    fire->setFireInfo(Blocks::jungle_fence, 5, 20);
    fire->setFireInfo(Blocks::dark_oak_fence, 5, 20);
    fire->setFireInfo(Blocks::acacia_fence, 5, 20);
    fire->setFireInfo(Blocks::oak_stairs, 5, 20);
    fire->setFireInfo(Blocks::birch_stairs, 5, 20);
    fire->setFireInfo(Blocks::spruce_stairs, 5, 20);
    fire->setFireInfo(Blocks::jungle_stairs, 5, 20);
    fire->setFireInfo(Blocks::log, 5, 5);
    fire->setFireInfo(Blocks::log2, 5, 5);
    fire->setFireInfo(Blocks::leaves, 30, 60);
    fire->setFireInfo(Blocks::leaves2, 30, 60);
    fire->setFireInfo(Blocks::bookshelf, 30, 20);
    fire->setFireInfo(Blocks::tnt, 15, 100);
    fire->setFireInfo(Blocks::tallgrass, 60, 100);
    fire->setFireInfo(Blocks::double_plant, 60, 100);
    fire->setFireInfo(Blocks::yellow_flower, 60, 100);
    fire->setFireInfo(Blocks::red_flower, 60, 100);
    fire->setFireInfo(Blocks::deadbush, 60, 100);
    fire->setFireInfo(Blocks::wool, 30, 60);
    fire->setFireInfo(Blocks::vine, 15, 100);
    fire->setFireInfo(Blocks::coal_block, 5, 5);
    fire->setFireInfo(Blocks::hay_block, 60, 20);
    fire->setFireInfo(Blocks::carpet, 60, 20);
}

void BlockFire::setFireInfo(const Block* blockIn, int encouragement, int flammability)
{
    encouragements[blockIn] = encouragement;
    flammabilities[blockIn] = flammability;
}

void BlockFire::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    if (!worldIn->getGameRules().getBoolean("doFireTick"))
        return;
    if (!canPlaceBlockAt(worldIn, pos))
        worldIn->setBlockToAir(pos);
    const Block* block = worldIn->getBlockState(pos.down())->getBlock();
    bool flag = block == Blocks::netherrack;
    if (dynamic_cast<const WorldProviderEnd*>(worldIn->provider.get()) && block == Blocks::bedrock)
        flag = true;
    if (!flag && worldIn->isRaining() && canDie(worldIn, pos)) {
        worldIn->setBlockToAir(pos);
        return;
    }
    const int i = state->getValue(AGE);
    if (i < 15) {
        state = state->withProperty(AGE, i + rand.nextInt(3) / 2);
        worldIn->setBlockState(pos, state, 4);
    }
    worldIn->scheduleUpdate(pos, this, tickRate(worldIn) + rand.nextInt(10));
    if (!flag) {
        if (!canNeighborCatchFire(worldIn, pos)) {
            if (!World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) || i > 3)
                worldIn->setBlockToAir(pos);
            return;
        }
        if (!canCatchFire(worldIn, pos.down()) && i == 15 && rand.nextInt(4) == 0) {
            worldIn->setBlockToAir(pos);
            return;
        }
    }
    const bool flag1 = worldIn->isBlockinHighHumidity(pos);
    int j = 0;
    if (flag1)
        j = -50;
    catchOnFire(worldIn, pos.east(), 300 + j, rand, i);
    catchOnFire(worldIn, pos.west(), 300 + j, rand, i);
    catchOnFire(worldIn, pos.down(), 250 + j, rand, i);
    catchOnFire(worldIn, pos.up(), 250 + j, rand, i);
    catchOnFire(worldIn, pos.north(), 300 + j, rand, i);
    catchOnFire(worldIn, pos.south(), 300 + j, rand, i);
    for (int k = -1; k <= 1; ++k) {
        for (int l = -1; l <= 1; ++l) {
            for (int i1 = -1; i1 <= 4; ++i1) {
                if (k != 0 || i1 != 0 || l != 0) {
                    int j1 = 100;
                    if (i1 > 1)
                        j1 += (i1 - 1) * 100;
                    const BlockPos blockpos = pos.add(k, i1, l);
                    const int k1 = getNeighborEncouragement(worldIn, blockpos);
                    if (k1 > 0) {
                        int l1 = (k1 + 40 + getDifficultyId(worldIn->getDifficulty()) * 7) / (i + 30);
                        if (flag1)
                            l1 /= 2;
                        if (l1 > 0 && rand.nextInt(j1) <= l1 && (!worldIn->isRaining() || !canDie(worldIn, blockpos))) {
                            int i2 = i + rand.nextInt(5) / 4;
                            if (i2 > 15)
                                i2 = 15;
                            worldIn->setBlockState(blockpos, state->withProperty(AGE, i2), 3);
                        }
                    }
                }
            }
        }
    }
}

bool BlockFire::canDie(World* worldIn, const BlockPos& pos)
{
    return worldIn->isRainingAt(pos) || worldIn->isRainingAt(pos.west()) || worldIn->isRainingAt(pos.east()) || worldIn->isRainingAt(pos.north())
        || worldIn->isRainingAt(pos.south());
}

int BlockFire::getFlammability(const Block* blockIn) const
{
    if (flammabilities.empty())
        init();
    const auto it = flammabilities.find(blockIn);
    return it == flammabilities.end() ? 0 : it->second;
}

int BlockFire::getEncouragement(const Block* blockIn) const
{
    // Java fills the table in Bootstrap (BlockFire.init); here on first use if nobody did.
    if (encouragements.empty())
        init();
    const auto it = encouragements.find(blockIn);
    return it == encouragements.end() ? 0 : it->second;
}

void BlockFire::catchOnFire(World* worldIn, const BlockPos& pos, int chance, Random& random, int age)
{
    const int i = getFlammability(worldIn->getBlockState(pos)->getBlock());
    if (random.nextInt(chance) < i) {
        const IBlockState* iblockstate = worldIn->getBlockState(pos);
        if (random.nextInt(age + 10) < 5 && !worldIn->isRainingAt(pos)) {
            int j = age + random.nextInt(5) / 4;
            if (j > 15)
                j = 15;
            worldIn->setBlockState(pos, getDefaultState()->withProperty(AGE, j), 3);
        } else {
            worldIn->setBlockToAir(pos);
        }
        if (iblockstate->getBlock() == Blocks::tnt)
            Blocks::tnt->onBlockDestroyedByPlayer(worldIn, pos, iblockstate->withProperty(BlockTNT::EXPLODE, true));
    }
}

bool BlockFire::canNeighborCatchFire(World* worldIn, const BlockPos& pos)
{
    for (const EnumFacing enumfacing : kFacings) {
        if (canCatchFire(worldIn, pos.offset(enumfacing)))
            return true;
    }
    return false;
}

int BlockFire::getNeighborEncouragement(World* worldIn, const BlockPos& pos)
{
    if (!worldIn->isAirBlock(pos))
        return 0;
    int i = 0;
    for (const EnumFacing enumfacing : kFacings)
        i = std::max(getEncouragement(worldIn->getBlockState(pos.offset(enumfacing))->getBlock()), i);
    return i;
}

bool BlockFire::canCatchFire(IBlockAccess* worldIn, const BlockPos& pos)
{
    return getEncouragement(worldIn->getBlockState(pos)->getBlock()) > 0;
}

bool BlockFire::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) || canNeighborCatchFire(worldIn, pos);
}

void BlockFire::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState*, Block*)
{
    if (!World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) && !canNeighborCatchFire(worldIn, pos))
        worldIn->setBlockToAir(pos);
}

void BlockFire::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (worldIn->provider->getDimensionId() > 0 || !Blocks::portal->func_176548_d(worldIn, pos)) {
        if (!World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) && !canNeighborCatchFire(worldIn, pos))
            worldIn->setBlockToAir(pos);
        else
            worldIn->scheduleUpdate(pos, this, tickRate(worldIn) + worldIn->rand.nextInt(10));
    }
}

void BlockFire::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random& rand)
{
    if (rand.nextInt(24) == 0) {
        const float volume = 1.0f + rand.nextFloat();
        const float pitch = rand.nextFloat() * 0.7f + 0.3f;
        worldIn->playSound(static_cast<double>(static_cast<float>(pos.getX()) + 0.5f), static_cast<double>(static_cast<float>(pos.getY()) + 0.5f),
            static_cast<double>(static_cast<float>(pos.getZ()) + 0.5f), "fire.fire", volume, pitch, false);
    }
    const double x = static_cast<double>(pos.getX());
    const double y = static_cast<double>(pos.getY());
    const double z = static_cast<double>(pos.getZ());
    const auto smoke = [worldIn](double a, double b, double c) { spawnParticle(worldIn, EnumParticleTypes::SMOKE_LARGE, a, b, c, 0.0, 0.0, 0.0); };
    if (!World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) && !Blocks::fire->canCatchFire(worldIn, pos.down())) {
        if (Blocks::fire->canCatchFire(worldIn, pos.west())) {
            for (int j = 0; j < 2; ++j) {
                const double d3 = x + rand.nextDouble() * 0.10000000149011612;
                const double d8 = y + rand.nextDouble();
                const double d13 = z + rand.nextDouble();
                smoke(d3, d8, d13);
            }
        }
        if (Blocks::fire->canCatchFire(worldIn, pos.east())) {
            for (int k = 0; k < 2; ++k) {
                const double d4 = static_cast<double>(pos.getX() + 1) - rand.nextDouble() * 0.10000000149011612;
                const double d9 = y + rand.nextDouble();
                const double d14 = z + rand.nextDouble();
                smoke(d4, d9, d14);
            }
        }
        if (Blocks::fire->canCatchFire(worldIn, pos.north())) {
            for (int l = 0; l < 2; ++l) {
                const double d5 = x + rand.nextDouble();
                const double d10 = y + rand.nextDouble();
                const double d15 = z + rand.nextDouble() * 0.10000000149011612;
                smoke(d5, d10, d15);
            }
        }
        if (Blocks::fire->canCatchFire(worldIn, pos.south())) {
            for (int i1 = 0; i1 < 2; ++i1) {
                const double d6 = x + rand.nextDouble();
                const double d11 = y + rand.nextDouble();
                const double d16 = static_cast<double>(pos.getZ() + 1) - rand.nextDouble() * 0.10000000149011612;
                smoke(d6, d11, d16);
            }
        }
        if (Blocks::fire->canCatchFire(worldIn, pos.up())) {
            for (int j1 = 0; j1 < 2; ++j1) {
                const double d7 = x + rand.nextDouble();
                const double d12 = static_cast<double>(pos.getY() + 1) - rand.nextDouble() * 0.10000000149011612;
                const double d17 = z + rand.nextDouble();
                smoke(d7, d12, d17);
            }
        }
    } else {
        for (int i = 0; i < 3; ++i) {
            const double d0 = x + rand.nextDouble();
            const double d1 = y + rand.nextDouble() * 0.5 + 0.5;
            const double d2 = z + rand.nextDouble();
            smoke(d0, d1, d2);
        }
    }
}

// --- BlockPortal ---

void BlockPortal::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    BlockBreakable::updateTick(worldIn, pos, state, rand);
    if (worldIn->provider->isSurfaceWorld() && worldIn->getGameRules().getBoolean("doMobSpawning")
        && rand.nextInt(2000) < getDifficultyId(worldIn->getDifficulty())) {
        const int i = pos.getY();
        BlockPos blockpos;
        for (blockpos = pos; !World::doesBlockHaveSolidTopSurface(worldIn, blockpos) && blockpos.getY() > 0; blockpos = blockpos.down()) {
        }
        if (i > 0 && !worldIn->getBlockState(blockpos.up())->getBlock()->isNormalCube()) {
            // Java: ItemMonsterPlacer.spawnCreature(world, 57 (zombie pigman), ...) with the portal cooldown --
            // entities come with stage 10.
        }
    }
}

void BlockPortal::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const Axis axis = worldIn->getBlockState(pos)->getValue(AXIS);
    float f = 0.125f;
    float f1 = 0.125f;
    if (axis == Axis::X)
        f = 0.5f;
    if (axis == Axis::Z)
        f1 = 0.5f;
    setBlockBounds(0.5f - f, 0.0f, 0.5f - f1, 0.5f + f, 1.0f, 0.5f + f1);
}

bool BlockPortal::func_176548_d(World* worldIn, const BlockPos& pos)
{
    Size size(worldIn, pos, Axis::X);
    if (size.func_150860_b() && size.field_150864_e == 0) {
        size.func_150859_c();
        return true;
    }
    Size size1(worldIn, pos, Axis::Z);
    if (size1.func_150860_b() && size1.field_150864_e == 0) {
        size1.func_150859_c();
        return true;
    }
    return false;
}

void BlockPortal::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    const Axis axis = state->getValue(AXIS);
    if (axis == Axis::X || axis == Axis::Z) {
        const Size size(worldIn, pos, axis);
        if (!size.func_150860_b() || size.field_150864_e < size.field_150868_h * size.field_150862_g)
            worldIn->setBlockState(pos, Blocks::air->getDefaultState());
    }
}

bool BlockPortal::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    std::optional<Axis> axis;
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() == this) {
        axis = iblockstate->getValue(AXIS);
        if (*axis == Axis::Z && side != EnumFacing::EAST && side != EnumFacing::WEST)
            return false;
        if (*axis == Axis::X && side != EnumFacing::SOUTH && side != EnumFacing::NORTH)
            return false;
    }
    const bool flag = worldIn->getBlockState(pos.west())->getBlock() == this && worldIn->getBlockState(pos.west(2))->getBlock() != this;
    const bool flag1 = worldIn->getBlockState(pos.east())->getBlock() == this && worldIn->getBlockState(pos.east(2))->getBlock() != this;
    const bool flag2 = worldIn->getBlockState(pos.north())->getBlock() == this && worldIn->getBlockState(pos.north(2))->getBlock() != this;
    const bool flag3 = worldIn->getBlockState(pos.south())->getBlock() == this && worldIn->getBlockState(pos.south(2))->getBlock() != this;
    const bool flag4 = flag || flag1 || axis == Axis::X;
    const bool flag5 = flag2 || flag3 || axis == Axis::Z;
    return flag4 && side == EnumFacing::WEST ? true : (flag4 && side == EnumFacing::EAST ? true : (flag5 && side == EnumFacing::NORTH ? true : flag5 && side == EnumFacing::SOUTH));
}

void BlockPortal::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random& rand)
{
    if (rand.nextInt(100) == 0) {
        const float pitch = rand.nextFloat() * 0.4f + 0.8f;
        worldIn->playSound(static_cast<double>(pos.getX()) + 0.5, static_cast<double>(pos.getY()) + 0.5, static_cast<double>(pos.getZ()) + 0.5, "portal.portal",
            0.5f, pitch, false);
    }
    for (int i = 0; i < 4; ++i) {
        double d0 = static_cast<double>(static_cast<float>(pos.getX()) + rand.nextFloat());
        const double d1 = static_cast<double>(static_cast<float>(pos.getY()) + rand.nextFloat());
        double d2 = static_cast<double>(static_cast<float>(pos.getZ()) + rand.nextFloat());
        double d3 = (static_cast<double>(rand.nextFloat()) - 0.5) * 0.5;
        const double d4 = (static_cast<double>(rand.nextFloat()) - 0.5) * 0.5;
        double d5 = (static_cast<double>(rand.nextFloat()) - 0.5) * 0.5;
        const int j = rand.nextInt(2) * 2 - 1;
        if (worldIn->getBlockState(pos.west())->getBlock() != this && worldIn->getBlockState(pos.east())->getBlock() != this) {
            d0 = static_cast<double>(pos.getX()) + 0.5 + 0.25 * static_cast<double>(j);
            d3 = static_cast<double>(rand.nextFloat() * 2.0f * static_cast<float>(j));
        } else {
            d2 = static_cast<double>(pos.getZ()) + 0.5 + 0.25 * static_cast<double>(j);
            d5 = static_cast<double>(rand.nextFloat() * 2.0f * static_cast<float>(j));
        }
        spawnParticle(worldIn, EnumParticleTypes::PORTAL, d0, d1, d2, d3, d4, d5);
    }
}

BlockPortal::Size::Size(World* worldIn, BlockPos pos, Axis axisIn)
    : world(worldIn)
    , axis(axisIn)
    , field_150866_c(axisIn == Axis::X ? EnumFacing::WEST : EnumFacing::SOUTH)
    , field_150863_d(axisIn == Axis::X ? EnumFacing::EAST : EnumFacing::NORTH)
{
    for (const BlockPos blockpos = pos; pos.getY() > blockpos.getY() - 21 && pos.getY() > 0 && func_150857_a(worldIn->getBlockState(pos.down())->getBlock());
         pos = pos.down()) {
    }
    const int i = func_180120_a(pos, field_150863_d) - 1;
    if (i >= 0) {
        field_150861_f = pos.offset(field_150863_d, i);
        field_150868_h = func_180120_a(*field_150861_f, field_150866_c);
        if (field_150868_h < 2 || field_150868_h > 21) {
            field_150861_f.reset();
            field_150868_h = 0;
        }
    }
    if (field_150861_f)
        field_150862_g = func_150858_a();
}

int BlockPortal::Size::func_180120_a(const BlockPos& pos, EnumFacing facing)
{
    int i;
    for (i = 0; i < 22; ++i) {
        const BlockPos blockpos = pos.offset(facing, i);
        if (!func_150857_a(world->getBlockState(blockpos)->getBlock()) || world->getBlockState(blockpos.down())->getBlock() != Blocks::obsidian)
            break;
    }
    const Block* block = world->getBlockState(pos.offset(facing, i))->getBlock();
    return block == Blocks::obsidian ? i : 0;
}

int BlockPortal::Size::func_150858_a()
{
    for (field_150862_g = 0; field_150862_g < 21; ++field_150862_g) {
        bool stop = false;
        for (int i = 0; i < field_150868_h; ++i) {
            const BlockPos blockpos = field_150861_f->offset(field_150866_c, i).up(field_150862_g);
            const Block* block = world->getBlockState(blockpos)->getBlock();
            if (!func_150857_a(block)) {
                stop = true;
                break;
            }
            if (block == Blocks::portal)
                ++field_150864_e;
            if (i == 0) {
                if (world->getBlockState(blockpos.offset(field_150863_d))->getBlock() != Blocks::obsidian) {
                    stop = true;
                    break;
                }
            } else if (i == field_150868_h - 1) {
                if (world->getBlockState(blockpos.offset(field_150866_c))->getBlock() != Blocks::obsidian) {
                    stop = true;
                    break;
                }
            }
        }
        if (stop)
            break;
    }
    for (int j = 0; j < field_150868_h; ++j) {
        if (world->getBlockState(field_150861_f->offset(field_150866_c, j).up(field_150862_g))->getBlock() != Blocks::obsidian) {
            field_150862_g = 0;
            break;
        }
    }
    if (field_150862_g <= 21 && field_150862_g >= 3)
        return field_150862_g;
    field_150861_f.reset();
    field_150868_h = 0;
    field_150862_g = 0;
    return 0;
}

bool BlockPortal::Size::func_150857_a(const Block* block)
{
    return &block->getMaterial() == &Material::air || block == Blocks::fire || block == Blocks::portal;
}

bool BlockPortal::Size::func_150860_b() const
{
    return field_150861_f && field_150868_h >= 2 && field_150868_h <= 21 && field_150862_g >= 3 && field_150862_g <= 21;
}

void BlockPortal::Size::func_150859_c()
{
    for (int i = 0; i < field_150868_h; ++i) {
        const BlockPos blockpos = field_150861_f->offset(field_150866_c, i);
        for (int j = 0; j < field_150862_g; ++j)
            world->setBlockState(blockpos.up(j), Blocks::portal->getDefaultState()->withProperty(BlockPortal::AXIS, axis), 2);
    }
}

// --- BlockEndPortal / BlockEndPortalFrame ---

std::shared_ptr<TileEntity> BlockEndPortal::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityEndPortal>();
}

void BlockEndPortal::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random& rand)
{
    const double d0 = static_cast<double>(static_cast<float>(pos.getX()) + rand.nextFloat());
    const double d1 = static_cast<double>(static_cast<float>(pos.getY()) + 0.8f);
    const double d2 = static_cast<double>(static_cast<float>(pos.getZ()) + rand.nextFloat());
    spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0, d1, d2, 0.0, 0.0, 0.0);
}

void BlockEndPortalFrame::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.8125f, 1.0f);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    if (worldIn->getBlockState(pos)->getValue(EYE)) {
        setBlockBounds(0.3125f, 0.8125f, 0.3125f, 0.6875f, 1.0f, 0.6875f);
        Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    }
    setBlockBoundsForItemRender();
}

int BlockEndPortalFrame::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos)->getValue(EYE) ? 15 : 0;
}

// --- BlockBreakable and subclasses ---

bool BlockBreakable::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const Block* block = iblockstate->getBlock();
    if (this == Blocks::glass || this == Blocks::stained_glass) {
        if (worldIn->getBlockState(pos.offset(getOpposite(side))) != iblockstate)
            return true;
        if (block == this)
            return false;
    }
    return !ignoreSimilarity && block == this ? false : Block::shouldSideBeRendered(worldIn, pos, side);
}

void BlockIce::harvestBlock(World* worldIn, EntityPlayer*, const BlockPos& pos, const IBlockState* state, TileEntity*)
{
    // Java: the mining statistic, exhaustion and silk touch come from the player (stage 8); without silk touch:
    if (worldIn->provider->doesWaterVaporize()) {
        worldIn->setBlockToAir(pos);
        return;
    }
    dropBlockAsItem(worldIn, pos, state, 0);
    const Material& material = worldIn->getBlockState(pos.down())->getBlock()->getMaterial();
    if (material.blocksMovement() || material.isLiquid())
        worldIn->setBlockState(pos, Blocks::flowing_water->getDefaultState());
}

void BlockIce::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    if (worldIn->getLightFor(EnumSkyBlock::BLOCK, pos) > 11 - getLightOpacity()) {
        if (worldIn->provider->doesWaterVaporize()) {
            worldIn->setBlockToAir(pos);
        } else {
            dropBlockAsItem(worldIn, pos, worldIn->getBlockState(pos), 0);
            worldIn->setBlockState(pos, Blocks::water->getDefaultState());
        }
    }
}

void BlockStainedGlass::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (!worldIn->isRemote)
        BlockBeacon::updateColorAsync(worldIn, pos);
}

void BlockStainedGlass::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (!worldIn->isRemote)
        BlockBeacon::updateColorAsync(worldIn, pos);
}

} // namespace mc
