// Original: the world side of net/minecraft/block/BlockLeaves.java (with BlockLeavesBase, BlockOldLeaf, BlockNewLeaf),
// BlockLog.java, BlockGrass.java, BlockMycelium.java, BlockDirt.java, BlockFarmland.java, BlockCrops.java,
// BlockPotato.java, BlockStem.java, BlockCocoa.java, BlockCactus.java, BlockReed.java, BlockSapling.java,
// BlockVine.java, BlockSnow.java, BlockSnowBlock.java and BlockSponge.java.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"
#include "core/Java.h"
#include "entity/Entity.h"
#include "item/EnumDyeColor.h"
#include "util/MathHelper.h"
#include "world/EnumSkyBlock.h"
#include "world/GameRules.h"
#include "world/gen/feature/WorldGenerators.h"

#include <algorithm>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>

namespace mc {

// --- leaves ---

bool BlockLeavesBase::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    return !fancyGraphics && worldIn->getBlockState(pos)->getBlock() == this ? false : Block::shouldSideBeRendered(worldIn, pos, side);
}

int BlockLeaves::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int)
{
    return BiomeColorHelper::getFoliageColorAtPos(worldIn, pos);
}

void BlockLeaves::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    const int i = 1;
    const int j = i + 1;
    const int k = pos.getX();
    const int l = pos.getY();
    const int i1 = pos.getZ();
    if (worldIn->isAreaLoaded(BlockPos(k - j, l - j, i1 - j), BlockPos(k + j, l + j, i1 + j))) {
        for (int j1 = -i; j1 <= i; ++j1) {
            for (int k1 = -i; k1 <= i; ++k1) {
                for (int l1 = -i; l1 <= i; ++l1) {
                    const BlockPos blockpos = pos.add(j1, k1, l1);
                    const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
                    if (&iblockstate->getBlock()->getMaterial() == &Material::leaves && !iblockstate->getValue(CHECK_DECAY))
                        worldIn->setBlockState(blockpos, iblockstate->withProperty(CHECK_DECAY, true), 4);
                }
            }
        }
    }
}

void BlockLeaves::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    if (worldIn->isRemote || !state->getValue(CHECK_DECAY) || !state->getValue(DECAYABLE))
        return;
    const int i = 4;
    const int j = i + 1;
    const int k = pos.getX();
    const int l = pos.getY();
    const int i1 = pos.getZ();
    const int j1 = 32;
    const int k1 = j1 * j1;
    const int l1 = j1 / 2;
    if (surroundings.empty())
        surroundings.assign(static_cast<size_t>(j1 * j1 * j1), 0);
    const auto at = [&](int index) -> int& { return surroundings[static_cast<size_t>(index)]; };
    if (worldIn->isAreaLoaded(BlockPos(k - j, l - j, i1 - j), BlockPos(k + j, l + j, i1 + j))) {
        BlockPos mutablePos;
        for (int i2 = -i; i2 <= i; ++i2) {
            for (int j2 = -i; j2 <= i; ++j2) {
                for (int k2 = -i; k2 <= i; ++k2) {
                    const Block* block = worldIn->getBlockState(mutablePos.set(k + i2, l + j2, i1 + k2))->getBlock();
                    int& cell = at((i2 + l1) * k1 + (j2 + l1) * j1 + k2 + l1);
                    if (block != Blocks::log && block != Blocks::log2)
                        cell = &block->getMaterial() == &Material::leaves ? -2 : -1;
                    else
                        cell = 0;
                }
            }
        }
        for (int i3 = 1; i3 <= 4; ++i3) {
            for (int j3 = -i; j3 <= i; ++j3) {
                for (int k3 = -i; k3 <= i; ++k3) {
                    for (int l3 = -i; l3 <= i; ++l3) {
                        if (at((j3 + l1) * k1 + (k3 + l1) * j1 + l3 + l1) == i3 - 1) {
                            const int neighbours[6] { (j3 + l1 - 1) * k1 + (k3 + l1) * j1 + l3 + l1, (j3 + l1 + 1) * k1 + (k3 + l1) * j1 + l3 + l1,
                                (j3 + l1) * k1 + (k3 + l1 - 1) * j1 + l3 + l1, (j3 + l1) * k1 + (k3 + l1 + 1) * j1 + l3 + l1,
                                (j3 + l1) * k1 + (k3 + l1) * j1 + (l3 + l1 - 1), (j3 + l1) * k1 + (k3 + l1) * j1 + l3 + l1 + 1 };
                            for (const int n : neighbours) {
                                if (at(n) == -2)
                                    at(n) = i3;
                            }
                        }
                    }
                }
            }
        }
    }
    const int l2 = at(l1 * k1 + l1 * j1 + l1);
    if (l2 >= 0)
        worldIn->setBlockState(pos, state->withProperty(CHECK_DECAY, false), 4);
    else
        destroy(worldIn, pos);
}

void BlockLeaves::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random& rand)
{
    if (worldIn->isRainingAt(pos.up()) && !World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) && rand.nextInt(15) == 1) {
        const double d0 = static_cast<double>(static_cast<float>(pos.getX()) + rand.nextFloat());
        const double d1 = static_cast<double>(pos.getY()) - 0.05;
        const double d2 = static_cast<double>(static_cast<float>(pos.getZ()) + rand.nextFloat());
        spawnParticle(worldIn, EnumParticleTypes::DRIP_WATER, d0, d1, d2, 0.0, 0.0, 0.0);
    }
}

void BlockLeaves::destroy(World* worldIn, const BlockPos& pos)
{
    dropBlockAsItem(worldIn, pos, worldIn->getBlockState(pos), 0);
    worldIn->setBlockToAir(pos);
}

void BlockLeaves::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float, int fortune)
{
    if (worldIn->isRemote)
        return;
    int i = getSaplingDropChance(state);
    if (fortune > 0) {
        i -= 2 << fortune;
        if (i < 10)
            i = 10;
    }
    if (worldIn->rand.nextInt(i) == 0) {
        Item* item = getItemDropped(state, worldIn->rand, fortune);
        spawnAsEntity(worldIn, pos, ItemStack(item, 1, damageDropped(state)));
    }
    i = 200;
    if (fortune > 0) {
        i -= 10 << fortune;
        if (i < 40)
            i = 40;
    }
    dropApple(worldIn, pos, state, i);
}

int BlockOldLeaf::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() == this) {
        const BlockPlanks::EnumType type = iblockstate->getValue(VARIANT);
        if (type == BlockPlanks::EnumType::SPRUCE)
            return ColorizerFoliage::getFoliageColorPine();
        if (type == BlockPlanks::EnumType::BIRCH)
            return ColorizerFoliage::getFoliageColorBirch();
    }
    return BlockLeaves::colorMultiplier(worldIn, pos, renderPass);
}

void BlockOldLeaf::dropApple(World* worldIn, const BlockPos& pos, const IBlockState* state, int chance)
{
    if (state->getValue(VARIANT) == BlockPlanks::EnumType::OAK && worldIn->rand.nextInt(chance) == 0)
        spawnAsEntity(worldIn, pos, ItemStack(Items::apple, 1, 0));
}

void BlockNewLeaf::dropApple(World* worldIn, const BlockPos& pos, const IBlockState* state, int chance)
{
    if (state->getValue(VARIANT) == BlockPlanks::EnumType::DARK_OAK && worldIn->rand.nextInt(chance) == 0)
        spawnAsEntity(worldIn, pos, ItemStack(Items::apple, 1, 0));
}

int BlockNewLeaf::getDamageValue(World* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    return iblockstate->getBlock()->getMetaFromState(iblockstate) & 3;
}

// --- logs ---

void BlockLog::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    const int i = 4;
    const int j = i + 1;
    if (worldIn->isAreaLoaded(pos.add(-j, -j, -j), pos.add(j, j, j))) {
        for (const BlockPos& blockpos : BlockPos::getAllInBox(pos.add(-i, -i, -i), pos.add(i, i, i))) {
            const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
            if (&iblockstate->getBlock()->getMaterial() == &Material::leaves && !iblockstate->getValue(BlockLeaves::CHECK_DECAY))
                worldIn->setBlockState(blockpos, iblockstate->withProperty(BlockLeaves::CHECK_DECAY, true), 4);
        }
    }
}

const IBlockState* BlockLog::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
    EntityLivingBase* placer)
{
    return BlockRotatedPillar::onBlockPlaced(worldIn, pos, facing, hitX, hitY, hitZ, meta, placer)->withProperty(LOG_AXIS, fromFacingAxis(getAxis(facing)));
}

// --- grass, mycelium, dirt ---

namespace {

bool isSnowAbove(IBlockAccess* worldIn, const BlockPos& pos)
{
    const Block* block = worldIn->getBlockState(pos.up())->getBlock();
    return block == Blocks::snow || block == Blocks::snow_layer;
}

// rand.nextInt(3) - 1, rand.nextInt(5) - 3, rand.nextInt(3) - 1 -- the spreading offset of grass and mycelium.
BlockPos spreadTarget(Random& rand, const BlockPos& pos)
{
    const int dx = rand.nextInt(3) - 1;
    const int dy = rand.nextInt(5) - 3;
    const int dz = rand.nextInt(3) - 1;
    return pos.add(dx, dy, dz);
}

} // namespace

const IBlockState* BlockGrass::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    return state->withProperty(SNOWY, isSnowAbove(worldIn, pos));
}

int BlockGrass::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int)
{
    return BiomeColorHelper::getGrassColorAtPos(worldIn, pos);
}

void BlockGrass::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random& rand)
{
    if (worldIn->isRemote)
        return;
    if (worldIn->getLightFromNeighbors(pos.up()) < 4 && worldIn->getBlockState(pos.up())->getBlock()->getLightOpacity() > 2) {
        worldIn->setBlockState(pos, Blocks::dirt->getDefaultState());
    } else if (worldIn->getLightFromNeighbors(pos.up()) >= 9) {
        for (int i = 0; i < 4; ++i) {
            const BlockPos blockpos = spreadTarget(rand, pos);
            const Block* block = worldIn->getBlockState(blockpos.up())->getBlock();
            const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
            if (iblockstate->getBlock() == Blocks::dirt && iblockstate->getValue(BlockDirt::VARIANT) == BlockDirt::DirtType::DIRT
                && worldIn->getLightFromNeighbors(blockpos.up()) >= 4 && block->getLightOpacity() <= 2)
                worldIn->setBlockState(blockpos, Blocks::grass->getDefaultState());
        }
    }
}

void BlockGrass::grow(World* worldIn, Random& rand, const BlockPos& pos, const IBlockState*)
{
    const BlockPos blockpos = pos.up();
    for (int i = 0; i < 128; ++i) {
        BlockPos blockpos1 = blockpos;
        int j = 0;
        while (true) {
            if (j >= i / 16) {
                if (&worldIn->getBlockState(blockpos1)->getBlock()->getMaterial() == &Material::air) {
                    if (rand.nextInt(8) == 0) {
                        const BlockFlower::EnumFlowerType type = worldIn->getBiomeGenForCoords(blockpos1)->pickRandomFlower(rand, blockpos1);
                        BlockFlower* blockflower = BlockFlower::info(type).blockType == BlockFlower::EnumFlowerColor::YELLOW ? Blocks::yellow_flower : Blocks::red_flower;
                        const IBlockState* iblockstate = blockflower->getDefaultState()->withProperty(blockflower->getTypeProperty(), type);
                        if (blockflower->canBlockStay(worldIn, blockpos1, iblockstate))
                            worldIn->setBlockState(blockpos1, iblockstate, 3);
                    } else {
                        const IBlockState* iblockstate1 = Blocks::tallgrass->getDefaultState()->withProperty(BlockTallGrass::TYPE, BlockTallGrass::EnumType::GRASS);
                        if (Blocks::tallgrass->canBlockStay(worldIn, blockpos1, iblockstate1))
                            worldIn->setBlockState(blockpos1, iblockstate1, 3);
                    }
                }
                break;
            }
            const int dx = rand.nextInt(3) - 1;
            const int a = rand.nextInt(3) - 1;
            const int dy = a * rand.nextInt(3) / 2;
            const int dz = rand.nextInt(3) - 1;
            blockpos1 = blockpos1.add(dx, dy, dz);
            if (worldIn->getBlockState(blockpos1.down())->getBlock() != Blocks::grass || worldIn->getBlockState(blockpos1)->getBlock()->isNormalCube())
                break;
            ++j;
        }
    }
}

const IBlockState* BlockMycelium::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    return state->withProperty(SNOWY, isSnowAbove(worldIn, pos));
}

void BlockMycelium::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random& rand)
{
    if (worldIn->isRemote)
        return;
    if (worldIn->getLightFromNeighbors(pos.up()) < 4 && worldIn->getBlockState(pos.up())->getBlock()->getLightOpacity() > 2) {
        worldIn->setBlockState(pos, Blocks::dirt->getDefaultState()->withProperty(BlockDirt::VARIANT, BlockDirt::DirtType::DIRT));
    } else if (worldIn->getLightFromNeighbors(pos.up()) >= 9) {
        for (int i = 0; i < 4; ++i) {
            const BlockPos blockpos = spreadTarget(rand, pos);
            const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
            const Block* block = worldIn->getBlockState(blockpos.up())->getBlock();
            if (iblockstate->getBlock() == Blocks::dirt && iblockstate->getValue(BlockDirt::VARIANT) == BlockDirt::DirtType::DIRT
                && worldIn->getLightFromNeighbors(blockpos.up()) >= 4 && block->getLightOpacity() <= 2)
                worldIn->setBlockState(blockpos, getDefaultState());
        }
    }
}

void BlockMycelium::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    Block::randomDisplayTick(worldIn, pos, state, rand);
    if (rand.nextInt(10) == 0) {
        const double x = static_cast<double>(static_cast<float>(pos.getX()) + rand.nextFloat());
        const double y = static_cast<double>(static_cast<float>(pos.getY()) + 1.1f);
        const double z = static_cast<double>(static_cast<float>(pos.getZ()) + rand.nextFloat());
        spawnParticle(worldIn, EnumParticleTypes::TOWN_AURA, x, y, z, 0.0, 0.0, 0.0);
    }
}

const IBlockState* BlockDirt::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    if (state->getValue(VARIANT) == DirtType::PODZOL)
        state = state->withProperty(SNOWY, isSnowAbove(worldIn, pos));
    return state;
}

int BlockDirt::getDamageValue(World* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    return iblockstate->getBlock() != this ? 0 : info(iblockstate->getValue(VARIANT)).metadata;
}

// --- farmland ---

std::optional<AxisAlignedBB> BlockFarmland::getCollisionBoundingBox(World*, const BlockPos& pos, const IBlockState*)
{
    return AxisAlignedBB(static_cast<double>(pos.getX()), static_cast<double>(pos.getY()), static_cast<double>(pos.getZ()), static_cast<double>(pos.getX() + 1),
        static_cast<double>(pos.getY() + 1), static_cast<double>(pos.getZ() + 1));
}

void BlockFarmland::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    const int i = state->getValue(MOISTURE);
    if (!hasWater(worldIn, pos) && !worldIn->isRainingAt(pos.up())) {
        if (i > 0)
            worldIn->setBlockState(pos, state->withProperty(MOISTURE, i - 1), 2);
        else if (!hasCrops(worldIn, pos))
            worldIn->setBlockState(pos, Blocks::dirt->getDefaultState());
    } else if (i < 7) {
        worldIn->setBlockState(pos, state->withProperty(MOISTURE, 7), 2);
    }
}

void BlockFarmland::onFallenUpon(World* worldIn, const BlockPos& pos, Entity* entityIn, float fallDistance)
{
    // Java: only for living entities, trampling with chance fallDistance - 0.5 (mobs only with mobGriefing) --
    // entities come with stage 10.
    Block::onFallenUpon(worldIn, pos, entityIn, fallDistance);
}

bool BlockFarmland::hasCrops(World* worldIn, const BlockPos& pos)
{
    const Block* block = worldIn->getBlockState(pos.up())->getBlock();
    return dynamic_cast<const BlockCrops*>(block) || dynamic_cast<const BlockStem*>(block);
}

bool BlockFarmland::hasWater(World* worldIn, const BlockPos& pos)
{
    for (const BlockPos& p : BlockPos::getAllInBox(pos.add(-4, 0, -4), pos.add(4, 1, 4))) {
        if (&worldIn->getBlockState(p)->getBlock()->getMaterial() == &Material::water)
            return true;
    }
    return false;
}

void BlockFarmland::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    Block::onNeighborBlockChange(worldIn, pos, state, neighborBlock);
    if (worldIn->getBlockState(pos.up())->getBlock()->getMaterial().isSolid())
        worldIn->setBlockState(pos, Blocks::dirt->getDefaultState());
}

bool BlockFarmland::shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side)
{
    switch (side) {
    case EnumFacing::UP:
        return true;
    case EnumFacing::NORTH:
    case EnumFacing::SOUTH:
    case EnumFacing::WEST:
    case EnumFacing::EAST: {
        const Block* block = worldIn->getBlockState(pos)->getBlock();
        return !block->isOpaqueCube() && block != Blocks::farmland;
    }
    default:
        return Block::shouldSideBeRendered(worldIn, pos, side);
    }
}

// --- crops ---

void BlockCrops::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    BlockBush::updateTick(worldIn, pos, state, rand);
    if (worldIn->getLightFromNeighbors(pos.up()) >= 9) {
        const int i = state->getValue(AGE);
        if (i < 7) {
            const float f = getGrowthChance(this, worldIn, pos);
            if (rand.nextInt(java::f2i(25.0f / f) + 1) == 0)
                worldIn->setBlockState(pos, state->withProperty(AGE, i + 1), 2);
        }
    }
}

void BlockCrops::grow(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    int i = state->getValue(AGE) + MathHelper::getRandomIntegerInRange(worldIn->rand, 2, 5);
    if (i > 7)
        i = 7;
    worldIn->setBlockState(pos, state->withProperty(AGE, i), 2);
}

float BlockCrops::getGrowthChance(const Block* blockIn, World* worldIn, const BlockPos& pos)
{
    float f = 1.0f;
    const BlockPos blockpos = pos.down();
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            float f1 = 0.0f;
            const IBlockState* iblockstate = worldIn->getBlockState(blockpos.add(i, 0, j));
            if (iblockstate->getBlock() == Blocks::farmland) {
                f1 = 1.0f;
                if (iblockstate->getValue(BlockFarmland::MOISTURE) > 0)
                    f1 = 3.0f;
            }
            if (i != 0 || j != 0)
                f1 /= 4.0f;
            f += f1;
        }
    }
    const BlockPos blockpos1 = pos.north();
    const BlockPos blockpos2 = pos.south();
    const BlockPos blockpos3 = pos.west();
    const BlockPos blockpos4 = pos.east();
    const bool flag = blockIn == worldIn->getBlockState(blockpos3)->getBlock() || blockIn == worldIn->getBlockState(blockpos4)->getBlock();
    const bool flag1 = blockIn == worldIn->getBlockState(blockpos1)->getBlock() || blockIn == worldIn->getBlockState(blockpos2)->getBlock();
    if (flag && flag1) {
        f /= 2.0f;
    } else {
        const bool flag2 = blockIn == worldIn->getBlockState(blockpos3.north())->getBlock() || blockIn == worldIn->getBlockState(blockpos4.north())->getBlock()
            || blockIn == worldIn->getBlockState(blockpos4.south())->getBlock() || blockIn == worldIn->getBlockState(blockpos3.south())->getBlock();
        if (flag2)
            f /= 2.0f;
    }
    return f;
}

bool BlockCrops::canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    return (worldIn->getLight(pos) >= 8 || worldIn->canSeeSky(pos)) && canPlaceBlockOn(worldIn->getBlockState(pos.down())->getBlock());
}

void BlockCrops::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune)
{
    BlockBush::dropBlockAsItemWithChance(worldIn, pos, state, chance, 0);
    if (!worldIn->isRemote) {
        const int i = state->getValue(AGE);
        if (i >= 7) {
            const int j = 3 + fortune;
            for (int k = 0; k < j; ++k) {
                if (worldIn->rand.nextInt(15) <= i)
                    spawnAsEntity(worldIn, pos, ItemStack(getSeed(), 1, 0));
            }
        }
    }
}

void BlockPotato::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune)
{
    BlockCrops::dropBlockAsItemWithChance(worldIn, pos, state, chance, fortune);
    if (!worldIn->isRemote && state->getValue(AGE) >= 7 && worldIn->rand.nextInt(50) == 0)
        spawnAsEntity(worldIn, pos, ItemStack(Items::poisonous_potato));
}

// --- stem ---

const IBlockState* BlockStem::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    state = state->withProperty(FACING, EnumFacing::UP);
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (worldIn->getBlockState(pos.offset(enumfacing))->getBlock() == crop) {
            state = state->withProperty(FACING, enumfacing);
            break;
        }
    }
    return state;
}

void BlockStem::updateTick(World* worldIn, const BlockPos& posIn, const IBlockState* state, Random& rand)
{
    BlockBush::updateTick(worldIn, posIn, state, rand);
    if (worldIn->getLightFromNeighbors(posIn.up()) >= 9) {
        const float f = BlockCrops::getGrowthChance(this, worldIn, posIn);
        if (rand.nextInt(java::f2i(25.0f / f) + 1) == 0) {
            const int i = state->getValue(AGE);
            if (i < 7) {
                state = state->withProperty(AGE, i + 1);
                worldIn->setBlockState(posIn, state, 2);
            } else {
                for (const EnumFacing enumfacing : kPlaneHorizontal) {
                    if (worldIn->getBlockState(posIn.offset(enumfacing))->getBlock() == crop)
                        return;
                }
                const BlockPos pos = posIn.offset(randomFacing(Plane::HORIZONTAL, rand));
                const Block* block = worldIn->getBlockState(pos.down())->getBlock();
                if (&worldIn->getBlockState(pos)->getBlock()->getMaterial() == &Material::air
                    && (block == Blocks::farmland || block == Blocks::dirt || block == Blocks::grass))
                    worldIn->setBlockState(pos, crop->getDefaultState());
            }
        }
    }
}

void BlockStem::growStem(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const int i = state->getValue(AGE) + MathHelper::getRandomIntegerInRange(worldIn->rand, 2, 5);
    worldIn->setBlockState(pos, state->withProperty(AGE, std::min(7, i)), 2);
}

int BlockStem::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int)
{
    return getRenderColor(worldIn->getBlockState(pos));
}

void BlockStem::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    maxY = static_cast<double>(static_cast<float>(worldIn->getBlockState(pos)->getValue(AGE) * 2 + 2) / 16.0f);
    const float f = 0.125f;
    setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, static_cast<float>(maxY), 0.5f + f);
}

void BlockStem::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune)
{
    BlockBush::dropBlockAsItemWithChance(worldIn, pos, state, chance, fortune);
    if (!worldIn->isRemote) {
        Item* item = getSeedItem();
        if (item) {
            const int i = state->getValue(AGE);
            for (int j = 0; j < 3; ++j) {
                if (worldIn->rand.nextInt(15) <= i)
                    spawnAsEntity(worldIn, pos, ItemStack(item));
            }
        }
    }
}

Item* BlockStem::getSeedItem() const
{
    return crop == Blocks::pumpkin ? Items::pumpkin_seeds : (crop == Blocks::melon_block ? Items::melon_seeds : nullptr);
}

// --- cocoa ---

void BlockCocoa::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    if (!canBlockStay(worldIn, pos, state)) {
        dropBlock(worldIn, pos, state);
    } else if (worldIn->rand.nextInt(5) == 0) {
        const int i = state->getValue(AGE);
        if (i < 2)
            worldIn->setBlockState(pos, state->withProperty(AGE, i + 1), 2);
    }
}

bool BlockCocoa::canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos.offset(state->getValue(FACING)));
    // Java reads BlockPlanks.VARIANT, the same property as the old log variant by name.
    return iblockstate->getBlock() == Blocks::log && iblockstate->getValue(BlockOldLog::VARIANT) == BlockPlanks::EnumType::JUNGLE;
}

std::optional<AxisAlignedBB> BlockCocoa::getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return BlockDirectional::getCollisionBoundingBox(worldIn, pos, state);
}

AxisAlignedBB BlockCocoa::getSelectedBoundingBox(World* worldIn, const BlockPos& pos)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return BlockDirectional::getSelectedBoundingBox(worldIn, pos);
}

void BlockCocoa::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const EnumFacing enumfacing = iblockstate->getValue(FACING);
    const int i = iblockstate->getValue(AGE);
    const int j = 4 + i * 2;
    const int k = 5 + i * 2;
    const float f = static_cast<float>(j) / 2.0f;
    switch (enumfacing) {
    case EnumFacing::SOUTH:
        setBlockBounds((8.0f - f) / 16.0f, (12.0f - static_cast<float>(k)) / 16.0f, (15.0f - static_cast<float>(j)) / 16.0f, (8.0f + f) / 16.0f, 0.75f, 0.9375f);
        break;
    case EnumFacing::NORTH:
        setBlockBounds((8.0f - f) / 16.0f, (12.0f - static_cast<float>(k)) / 16.0f, 0.0625f, (8.0f + f) / 16.0f, 0.75f, (1.0f + static_cast<float>(j)) / 16.0f);
        break;
    case EnumFacing::WEST:
        setBlockBounds(0.0625f, (12.0f - static_cast<float>(k)) / 16.0f, (8.0f - f) / 16.0f, (1.0f + static_cast<float>(j)) / 16.0f, 0.75f, (8.0f + f) / 16.0f);
        break;
    case EnumFacing::EAST:
        setBlockBounds((15.0f - static_cast<float>(j)) / 16.0f, (12.0f - static_cast<float>(k)) / 16.0f, (8.0f - f) / 16.0f, 0.9375f, 0.75f, (8.0f + f) / 16.0f);
        break;
    default:
        break;
    }
}

const IBlockState* BlockCocoa::onBlockPlaced(World*, const BlockPos&, EnumFacing facing, float, float, float, int, EntityLivingBase*)
{
    if (!isHorizontal(getAxis(facing)))
        facing = EnumFacing::NORTH;
    return getDefaultState()->withProperty(FACING, getOpposite(facing))->withProperty(AGE, 0);
}

void BlockCocoa::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (!canBlockStay(worldIn, pos, state))
        dropBlock(worldIn, pos, state);
}

void BlockCocoa::dropBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    worldIn->setBlockState(pos, Blocks::air->getDefaultState(), 3);
    dropBlockAsItem(worldIn, pos, state, 0);
}

void BlockCocoa::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float, int)
{
    const int i = state->getValue(AGE);
    const int j = i >= 2 ? 3 : 1;
    for (int k = 0; k < j; ++k)
        spawnAsEntity(worldIn, pos, ItemStack(Items::dye, 1, getDyeDamage(EnumDyeColor::BROWN)));
}

int BlockCocoa::getDamageValue(World*, const BlockPos&)
{
    return getDyeDamage(EnumDyeColor::BROWN);
}

void BlockCocoa::grow(World* worldIn, Random&, const BlockPos& pos, const IBlockState* state)
{
    worldIn->setBlockState(pos, state->withProperty(AGE, state->getValue(AGE) + 1), 2);
}

// --- cactus, reed ---

void BlockCactus::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    const BlockPos blockpos = pos.up();
    if (worldIn->isAirBlock(blockpos)) {
        int i;
        for (i = 1; worldIn->getBlockState(pos.down(i))->getBlock() == this; ++i) {
        }
        if (i < 3) {
            const int j = state->getValue(AGE);
            if (j == 15) {
                worldIn->setBlockState(blockpos, getDefaultState());
                const IBlockState* iblockstate = state->withProperty(AGE, 0);
                worldIn->setBlockState(pos, iblockstate, 4);
                onNeighborBlockChange(worldIn, blockpos, iblockstate, this);
            } else {
                worldIn->setBlockState(pos, state->withProperty(AGE, j + 1), 4);
            }
        }
    }
}

std::optional<AxisAlignedBB> BlockCactus::getCollisionBoundingBox(World*, const BlockPos& pos, const IBlockState*)
{
    const float f = 0.0625f;
    return AxisAlignedBB(static_cast<double>(static_cast<float>(pos.getX()) + f), static_cast<double>(pos.getY()), static_cast<double>(static_cast<float>(pos.getZ()) + f),
        static_cast<double>(static_cast<float>(pos.getX() + 1) - f), static_cast<double>(static_cast<float>(pos.getY() + 1) - f),
        static_cast<double>(static_cast<float>(pos.getZ() + 1) - f));
}

AxisAlignedBB BlockCactus::getSelectedBoundingBox(World*, const BlockPos& pos)
{
    const float f = 0.0625f;
    return AxisAlignedBB(static_cast<double>(static_cast<float>(pos.getX()) + f), static_cast<double>(pos.getY()), static_cast<double>(static_cast<float>(pos.getZ()) + f),
        static_cast<double>(static_cast<float>(pos.getX() + 1) - f), static_cast<double>(pos.getY() + 1), static_cast<double>(static_cast<float>(pos.getZ() + 1) - f));
}

void BlockReed::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    if (worldIn->getBlockState(pos.down())->getBlock() == Blocks::reeds || checkForDrop(worldIn, pos, state)) {
        if (worldIn->isAirBlock(pos.up())) {
            int i;
            for (i = 1; worldIn->getBlockState(pos.down(i))->getBlock() == this; ++i) {
            }
            if (i < 3) {
                const int j = state->getValue(AGE);
                if (j == 15) {
                    worldIn->setBlockState(pos.up(), getDefaultState());
                    worldIn->setBlockState(pos, state->withProperty(AGE, 0), 4);
                } else {
                    worldIn->setBlockState(pos, state->withProperty(AGE, j + 1), 4);
                }
            }
        }
    }
}

void BlockReed::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    checkForDrop(worldIn, pos, state);
}

bool BlockReed::checkForDrop(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (canBlockStay(worldIn, pos))
        return true;
    dropBlockAsItem(worldIn, pos, state, 0);
    worldIn->setBlockToAir(pos);
    return false;
}

int BlockReed::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int)
{
    return worldIn->getBiomeGenForCoords(pos)->getGrassColorAtPos(pos);
}

// --- sapling ---

void BlockSapling::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    if (!worldIn->isRemote) {
        BlockBush::updateTick(worldIn, pos, state, rand);
        if (worldIn->getLightFromNeighbors(pos.up()) >= 9 && rand.nextInt(7) == 0)
            grow(worldIn, pos, state, rand);
    }
}

void BlockSapling::grow(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    if (state->getValue(STAGE) == 0)
        worldIn->setBlockState(pos, state->cycleProperty(STAGE), 4);
    else
        generateTree(worldIn, pos, state, rand);
}

void BlockSapling::generateTree(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    std::unique_ptr<WorldGenerator> worldgenerator;
    if (rand.nextInt(10) == 0)
        worldgenerator = std::make_unique<WorldGenBigTree>(true);
    else
        worldgenerator = std::make_unique<WorldGenTrees>(true);
    int i = 0;
    int j = 0;
    bool flag = false;
    // The two-by-two search of the large trees: the first corner with four saplings of the type.
    const auto findSquare = [&](BlockPlanks::EnumType type) {
        for (i = 0; i >= -1; --i) {
            for (j = 0; j >= -1; --j) {
                if (func_181624_a(worldIn, pos, i, j, type))
                    return true;
            }
        }
        return false;
    };
    switch (state->getValue(TYPE)) {
    case BlockPlanks::EnumType::SPRUCE:
        if (findSquare(BlockPlanks::EnumType::SPRUCE)) {
            worldgenerator = std::make_unique<WorldGenMegaPineTree>(false, rand.nextBoolean());
            flag = true;
        }
        if (!flag) {
            j = 0;
            i = 0;
            worldgenerator = std::make_unique<WorldGenTaiga2>(true);
        }
        break;
    case BlockPlanks::EnumType::BIRCH:
        worldgenerator = std::make_unique<WorldGenForest>(true, false);
        break;
    case BlockPlanks::EnumType::JUNGLE: {
        const IBlockState* iblockstate = Blocks::log->getDefaultState()->withProperty(BlockOldLog::VARIANT, BlockPlanks::EnumType::JUNGLE);
        const IBlockState* iblockstate1 = Blocks::leaves->getDefaultState()
                                              ->withProperty(BlockOldLeaf::VARIANT, BlockPlanks::EnumType::JUNGLE)
                                              ->withProperty(BlockLeaves::CHECK_DECAY, false);
        if (findSquare(BlockPlanks::EnumType::JUNGLE)) {
            worldgenerator = std::make_unique<WorldGenMegaJungle>(true, 10, 20, iblockstate, iblockstate1);
            flag = true;
        }
        if (!flag) {
            j = 0;
            i = 0;
            const int height = 4 + rand.nextInt(7);
            worldgenerator = std::make_unique<WorldGenTrees>(true, height, iblockstate, iblockstate1, false);
        }
        break;
    }
    case BlockPlanks::EnumType::ACACIA:
        worldgenerator = std::make_unique<WorldGenSavannaTree>(true);
        break;
    case BlockPlanks::EnumType::DARK_OAK:
        if (findSquare(BlockPlanks::EnumType::DARK_OAK)) {
            worldgenerator = std::make_unique<WorldGenCanopyTree>(true);
            flag = true;
        }
        if (!flag)
            return;
        break;
    case BlockPlanks::EnumType::OAK:
    default:
        break;
    }
    const IBlockState* iblockstate2 = Blocks::air->getDefaultState();
    if (flag) {
        worldIn->setBlockState(pos.add(i, 0, j), iblockstate2, 4);
        worldIn->setBlockState(pos.add(i + 1, 0, j), iblockstate2, 4);
        worldIn->setBlockState(pos.add(i, 0, j + 1), iblockstate2, 4);
        worldIn->setBlockState(pos.add(i + 1, 0, j + 1), iblockstate2, 4);
    } else {
        worldIn->setBlockState(pos, iblockstate2, 4);
    }
    if (!worldgenerator->generate(worldIn, rand, pos.add(i, 0, j))) {
        if (flag) {
            worldIn->setBlockState(pos.add(i, 0, j), state, 4);
            worldIn->setBlockState(pos.add(i + 1, 0, j), state, 4);
            worldIn->setBlockState(pos.add(i, 0, j + 1), state, 4);
            worldIn->setBlockState(pos.add(i + 1, 0, j + 1), state, 4);
        } else {
            worldIn->setBlockState(pos, state, 4);
        }
    }
}

bool BlockSapling::func_181624_a(World* worldIn, const BlockPos& pos, int x, int z, BlockPlanks::EnumType type)
{
    return isTypeAt(worldIn, pos.add(x, 0, z), type) && isTypeAt(worldIn, pos.add(x + 1, 0, z), type) && isTypeAt(worldIn, pos.add(x, 0, z + 1), type)
        && isTypeAt(worldIn, pos.add(x + 1, 0, z + 1), type);
}

bool BlockSapling::isTypeAt(World* worldIn, const BlockPos& pos, BlockPlanks::EnumType type)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    return iblockstate->getBlock() == this && iblockstate->getValue(TYPE) == type;
}

bool BlockSapling::canUseBonemeal(World* worldIn, Random&, const BlockPos&, const IBlockState*)
{
    return static_cast<double>(worldIn->rand.nextFloat()) < 0.45;
}

// --- vine ---

const IBlockState* BlockVine::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    return state->withProperty(UP, worldIn->getBlockState(pos.up())->getBlock()->isBlockNormalCube());
}

void BlockVine::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    float f1 = 1.0f;
    float f2 = 1.0f;
    float f3 = 1.0f;
    float f4 = 0.0f;
    float f5 = 0.0f;
    float f6 = 0.0f;
    bool flag = false;
    const IBlockState* state = worldIn->getBlockState(pos);
    if (state->getValue(WEST)) {
        f4 = std::max(f4, 0.0625f);
        f1 = 0.0f;
        f2 = 0.0f;
        f5 = 1.0f;
        f3 = 0.0f;
        f6 = 1.0f;
        flag = true;
    }
    if (state->getValue(EAST)) {
        f1 = std::min(f1, 0.9375f);
        f4 = 1.0f;
        f2 = 0.0f;
        f5 = 1.0f;
        f3 = 0.0f;
        f6 = 1.0f;
        flag = true;
    }
    if (state->getValue(NORTH)) {
        f6 = std::max(f6, 0.0625f);
        f3 = 0.0f;
        f1 = 0.0f;
        f4 = 1.0f;
        f2 = 0.0f;
        f5 = 1.0f;
        flag = true;
    }
    if (state->getValue(SOUTH)) {
        f3 = std::min(f3, 0.9375f);
        f6 = 1.0f;
        f1 = 0.0f;
        f4 = 1.0f;
        f2 = 0.0f;
        f5 = 1.0f;
        flag = true;
    }
    if (!flag && canPlaceOn(worldIn->getBlockState(pos.up())->getBlock())) {
        f2 = std::min(f2, 0.9375f);
        f5 = 1.0f;
        f1 = 0.0f;
        f4 = 1.0f;
        f3 = 0.0f;
        f6 = 1.0f;
    }
    setBlockBounds(f1, f2, f3, f4, f5, f6);
}

bool BlockVine::recheckGrownSides(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const IBlockState* iblockstate = state;
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        const PropertyBool& propertybool = getPropertyFor(enumfacing);
        if (state->getValue(propertybool) && !canPlaceOn(worldIn->getBlockState(pos.offset(enumfacing))->getBlock())) {
            const IBlockState* iblockstate1 = worldIn->getBlockState(pos.up());
            if (iblockstate1->getBlock() != this || !iblockstate1->getValue(propertybool))
                state = state->withProperty(propertybool, false);
        }
    }
    if (getNumGrownFaces(state) == 0)
        return false;
    if (iblockstate != state)
        worldIn->setBlockState(pos, state, 2);
    return true;
}

int BlockVine::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int)
{
    return worldIn->getBiomeGenForCoords(pos)->getFoliageColorAtPos(pos);
}

void BlockVine::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (!worldIn->isRemote && !recheckGrownSides(worldIn, pos, state)) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
}

void BlockVine::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    if (worldIn->isRemote || worldIn->rand.nextInt(4) != 0)
        return;
    const int i = 4;
    int j = 5;
    bool flag = false;
    for (int k = -i; k <= i && !flag; ++k) {
        for (int l = -i; l <= i && !flag; ++l) {
            for (int i1 = -1; i1 <= 1; ++i1) {
                if (worldIn->getBlockState(pos.add(k, i1, l))->getBlock() == this) {
                    --j;
                    if (j <= 0) {
                        flag = true;
                        break;
                    }
                }
            }
        }
    }
    const EnumFacing enumfacing1 = randomFacing(rand);
    const BlockPos blockpos1 = pos.up();
    const auto anySide = [](const IBlockState* s) { return s->getValue(NORTH) || s->getValue(EAST) || s->getValue(SOUTH) || s->getValue(WEST); };
    if (enumfacing1 == EnumFacing::UP && pos.getY() < 255 && worldIn->isAirBlock(blockpos1)) {
        if (!flag) {
            const IBlockState* iblockstate2 = state;
            for (const EnumFacing enumfacing3 : kPlaneHorizontal) {
                if (rand.nextBoolean() || !canPlaceOn(worldIn->getBlockState(blockpos1.offset(enumfacing3))->getBlock()))
                    iblockstate2 = iblockstate2->withProperty(getPropertyFor(enumfacing3), false);
            }
            if (anySide(iblockstate2))
                worldIn->setBlockState(blockpos1, iblockstate2, 2);
        }
    } else if (isHorizontal(getAxis(enumfacing1)) && !state->getValue(getPropertyFor(enumfacing1))) {
        if (!flag) {
            const BlockPos blockpos3 = pos.offset(enumfacing1);
            const Block* block1 = worldIn->getBlockState(blockpos3)->getBlock();
            if (&block1->getMaterial() == &Material::air) {
                const EnumFacing enumfacing2 = rotateY(enumfacing1);
                const EnumFacing enumfacing4 = rotateYCCW(enumfacing1);
                const bool flag1 = state->getValue(getPropertyFor(enumfacing2));
                const bool flag2 = state->getValue(getPropertyFor(enumfacing4));
                const BlockPos blockpos4 = blockpos3.offset(enumfacing2);
                const BlockPos blockpos = blockpos3.offset(enumfacing4);
                if (flag1 && canPlaceOn(worldIn->getBlockState(blockpos4)->getBlock()))
                    worldIn->setBlockState(blockpos3, getDefaultState()->withProperty(getPropertyFor(enumfacing2), true), 2);
                else if (flag2 && canPlaceOn(worldIn->getBlockState(blockpos)->getBlock()))
                    worldIn->setBlockState(blockpos3, getDefaultState()->withProperty(getPropertyFor(enumfacing4), true), 2);
                else if (flag1 && worldIn->isAirBlock(blockpos4) && canPlaceOn(worldIn->getBlockState(pos.offset(enumfacing2))->getBlock()))
                    worldIn->setBlockState(blockpos4, getDefaultState()->withProperty(getPropertyFor(getOpposite(enumfacing1)), true), 2);
                else if (flag2 && worldIn->isAirBlock(blockpos) && canPlaceOn(worldIn->getBlockState(pos.offset(enumfacing4))->getBlock()))
                    worldIn->setBlockState(blockpos, getDefaultState()->withProperty(getPropertyFor(getOpposite(enumfacing1)), true), 2);
                else if (canPlaceOn(worldIn->getBlockState(blockpos3.up())->getBlock()))
                    worldIn->setBlockState(blockpos3, getDefaultState(), 2);
            } else if (block1->getMaterial().isOpaque() && block1->isFullCube()) {
                worldIn->setBlockState(pos, state->withProperty(getPropertyFor(enumfacing1), true), 2);
            }
        }
    } else if (pos.getY() > 1) {
        const BlockPos blockpos2 = pos.down();
        const IBlockState* iblockstate = worldIn->getBlockState(blockpos2);
        const Block* block = iblockstate->getBlock();
        if (&block->getMaterial() == &Material::air) {
            const IBlockState* iblockstate1 = state;
            for (const EnumFacing enumfacing : kPlaneHorizontal) {
                if (rand.nextBoolean())
                    iblockstate1 = iblockstate1->withProperty(getPropertyFor(enumfacing), false);
            }
            if (anySide(iblockstate1))
                worldIn->setBlockState(blockpos2, iblockstate1, 2);
        } else if (block == this) {
            const IBlockState* iblockstate3 = iblockstate;
            for (const EnumFacing enumfacing5 : kPlaneHorizontal) {
                const PropertyBool& propertybool = getPropertyFor(enumfacing5);
                if (rand.nextBoolean() && state->getValue(propertybool))
                    iblockstate3 = iblockstate3->withProperty(propertybool, true);
            }
            if (anySide(iblockstate3))
                worldIn->setBlockState(blockpos2, iblockstate3, 2);
        }
    }
}

const IBlockState* BlockVine::onBlockPlaced(World*, const BlockPos&, EnumFacing facing, float, float, float, int, EntityLivingBase*)
{
    const IBlockState* iblockstate =
        getDefaultState()->withProperty(UP, false)->withProperty(NORTH, false)->withProperty(EAST, false)->withProperty(SOUTH, false)->withProperty(WEST, false);
    return isHorizontal(getAxis(facing)) ? iblockstate->withProperty(getPropertyFor(getOpposite(facing)), true) : iblockstate;
}

const PropertyBool& BlockVine::getPropertyFor(EnumFacing side)
{
    switch (side) {
    case EnumFacing::UP: return UP;
    case EnumFacing::NORTH: return NORTH;
    case EnumFacing::SOUTH: return SOUTH;
    case EnumFacing::EAST: return EAST;
    case EnumFacing::WEST: return WEST;
    default: throw std::invalid_argument(std::string(getName(side)) + " is an invalid choice");
    }
}

int BlockVine::getNumGrownFaces(const IBlockState* state)
{
    int i = 0;
    for (const PropertyBool* propertybool : ALL_FACES) {
        if (state->getValue(*propertybool))
            ++i;
    }
    return i;
}

// --- snow ---

bool BlockSnow::isPassable(IBlockAccess* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos)->getValue(LAYERS) < 5;
}

void BlockSnow::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    getBoundsForLayers(worldIn->getBlockState(pos)->getValue(LAYERS));
}

bool BlockSnow::isReplaceable(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos)->getValue(LAYERS) == 1;
}

// --- BlockSoulSand ---

std::optional<AxisAlignedBB> BlockSoulSand::getCollisionBoundingBox(World*, const BlockPos& pos, const IBlockState*)
{
    const float f = 0.125f;
    return AxisAlignedBB(pos.getX(), pos.getY(), pos.getZ(), pos.getX() + 1, static_cast<float>(pos.getY() + 1) - f, pos.getZ() + 1);
}

void BlockSoulSand::onEntityCollidedWithBlock(World*, const BlockPos&, const IBlockState*, Entity* entityIn)
{
    entityIn->motionX *= 0.4;
    entityIn->motionZ *= 0.4;
}

std::optional<AxisAlignedBB> BlockSnow::getCollisionBoundingBox(World*, const BlockPos& pos, const IBlockState* state)
{
    const int i = state->getValue(LAYERS) - 1;
    const float f = 0.125f;
    return AxisAlignedBB(static_cast<double>(pos.getX()) + minX, static_cast<double>(pos.getY()) + minY, static_cast<double>(pos.getZ()) + minZ,
        static_cast<double>(pos.getX()) + maxX, static_cast<double>(static_cast<float>(pos.getY()) + static_cast<float>(i) * f), static_cast<double>(pos.getZ()) + maxZ);
}

void BlockSnow::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    checkAndDropBlock(worldIn, pos, state);
}

bool BlockSnow::checkAndDropBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!canPlaceBlockAt(worldIn, pos)) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
        return false;
    }
    return true;
}

void BlockSnow::harvestBlock(World* worldIn, EntityPlayer*, const BlockPos& pos, const IBlockState* state, TileEntity*)
{
    spawnAsEntity(worldIn, pos, ItemStack(Items::snowball, state->getValue(LAYERS) + 1, 0));
    worldIn->setBlockToAir(pos);
    // Java: and the mining statistic of the player -- stage 8.
}

void BlockSnow::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    if (worldIn->getLightFor(EnumSkyBlock::BLOCK, pos) > 11) {
        dropBlockAsItem(worldIn, pos, worldIn->getBlockState(pos), 0);
        worldIn->setBlockToAir(pos);
    }
}

void BlockSnowBlock::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    if (worldIn->getLightFor(EnumSkyBlock::BLOCK, pos) > 11) {
        dropBlockAsItem(worldIn, pos, worldIn->getBlockState(pos), 0);
        worldIn->setBlockToAir(pos);
    }
}

// --- sponge ---

void BlockSponge::tryAbsorb(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!state->getValue(WET) && absorb(worldIn, pos)) {
        worldIn->setBlockState(pos, state->withProperty(WET, true), 2);
        worldIn->playAuxSFX(2001, pos, Block::getIdFromBlock(Blocks::water));
    }
}

bool BlockSponge::absorb(World* worldIn, const BlockPos& pos)
{
    std::deque<std::pair<BlockPos, int>> queue;
    std::vector<BlockPos> arraylist;
    queue.emplace_back(pos, 0);
    int i = 0;
    while (!queue.empty()) {
        const auto [blockpos, j] = queue.front();
        queue.pop_front();
        for (const EnumFacing enumfacing : kFacings) {
            const BlockPos blockpos1 = blockpos.offset(enumfacing);
            if (&worldIn->getBlockState(blockpos1)->getBlock()->getMaterial() == &Material::water) {
                worldIn->setBlockState(blockpos1, Blocks::air->getDefaultState(), 2);
                arraylist.push_back(blockpos1);
                ++i;
                if (j < 6)
                    queue.emplace_back(blockpos1, j + 1);
            }
        }
        if (i > 64)
            break;
    }
    for (const BlockPos& blockpos2 : arraylist)
        worldIn->notifyNeighborsOfStateChange(blockpos2, Blocks::air);
    return i > 0;
}

void BlockSponge::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    if (!state->getValue(WET))
        return;
    const EnumFacing enumfacing = randomFacing(rand);
    if (enumfacing != EnumFacing::UP && !World::doesBlockHaveSolidTopSurface(worldIn, pos.offset(enumfacing))) {
        double d0 = static_cast<double>(pos.getX());
        double d1 = static_cast<double>(pos.getY());
        double d2 = static_cast<double>(pos.getZ());
        if (enumfacing == EnumFacing::DOWN) {
            d1 = d1 - 0.05;
            d0 += rand.nextDouble();
            d2 += rand.nextDouble();
        } else {
            d1 = d1 + rand.nextDouble() * 0.8;
            if (getAxis(enumfacing) == Axis::X) {
                d2 += rand.nextDouble();
                if (enumfacing == EnumFacing::EAST)
                    ++d0;
                else
                    d0 += 0.05;
            } else {
                d0 += rand.nextDouble();
                if (enumfacing == EnumFacing::SOUTH)
                    ++d2;
                else
                    d2 += 0.05;
            }
        }
        spawnParticle(worldIn, EnumParticleTypes::DRIP_WATER, d0, d1, d2, 0.0, 0.0, 0.0);
    }
}

} // namespace mc
