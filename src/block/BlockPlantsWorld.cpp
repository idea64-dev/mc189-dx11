// The world callbacks of the plants (net/minecraft/block/BlockBush.java and its subclasses, BlockCactus.java,
// BlockReed.java, BlockVine.java), the snow layer and pumpkin placement checks and the chest's facing helper --
// what world generation asks of blocks.

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/ItemStack.h"
#include "util/Random.h"
#include "util/Vec3.h"
#include "world/World.h"
#include "world/biome/BiomeGenBase.h"
#include "world/gen/feature/WorldGenerators.h"

namespace mc {

// --- BlockBush ---

bool BlockBush::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return Block::canPlaceBlockAt(worldIn, pos) && canPlaceBlockOn(worldIn->getBlockState(pos.down())->getBlock());
}

bool BlockBush::canPlaceBlockOn(const Block* ground) const
{
    return ground == Blocks::grass || ground == Blocks::dirt || ground == Blocks::farmland;
}

void BlockBush::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    Block::onNeighborBlockChange(worldIn, pos, state, neighborBlock);
    checkAndDropBlock(worldIn, pos, state);
}

void BlockBush::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random&)
{
    checkAndDropBlock(worldIn, pos, state);
}

void BlockBush::checkAndDropBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!canBlockStay(worldIn, pos, state)) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockState(pos, Blocks::air->getDefaultState(), 3);
    }
}

bool BlockBush::canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    return canPlaceBlockOn(worldIn->getBlockState(pos.down())->getBlock());
}

// --- BlockTallGrass ---

bool BlockTallGrass::canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    return canPlaceBlockOn(worldIn->getBlockState(pos.down())->getBlock());
}

int BlockTallGrass::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int)
{
    return worldIn->getBiomeGenForCoords(pos)->getGrassColorAtPos(pos);
}

int BlockTallGrass::getDamageValue(World* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    return iblockstate->getBlock()->getMetaFromState(iblockstate);
}

void BlockTallGrass::grow(World* worldIn, Random&, const BlockPos& pos, const IBlockState* state)
{
    BlockDoublePlant::EnumPlantType type = BlockDoublePlant::EnumPlantType::GRASS;
    if (state->getValue(TYPE) == EnumType::FERN)
        type = BlockDoublePlant::EnumPlantType::FERN;
    if (Blocks::double_plant->canPlaceBlockAt(worldIn, pos))
        Blocks::double_plant->placeAt(worldIn, pos, type, 2);
}

// --- BlockDeadBush ---

bool BlockDeadBush::canPlaceBlockOn(const Block* ground) const
{
    return ground == Blocks::sand || ground == Blocks::hardened_clay || ground == Blocks::stained_hardened_clay || ground == Blocks::dirt;
}

// --- BlockLilyPad ---

void BlockLilyPad::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    // Java: boats pass through -- entities come with stage 10.
    BlockBush::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
}

bool BlockLilyPad::canPlaceBlockOn(const Block* ground) const
{
    return ground == Blocks::water;
}

bool BlockLilyPad::canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (pos.getY() >= 0 && pos.getY() < 256) {
        const IBlockState* iblockstate = worldIn->getBlockState(pos.down());
        return &iblockstate->getBlock()->getMaterial() == &Material::water && iblockstate->getValue(BlockLiquid::LEVEL) == 0;
    }
    return false;
}

// --- BlockMushroom ---

void BlockMushroom::updateTick(World* worldIn, const BlockPos& posIn, const IBlockState*, Random& rand)
{
    BlockPos pos = posIn;
    if (rand.nextInt(25) == 0) {
        int i = 5;
        for (const BlockPos& blockpos : BlockPos::getAllInBox(pos.add(-4, -1, -4), pos.add(4, 1, 4))) {
            if (worldIn->getBlockState(blockpos)->getBlock() == this) {
                --i;
                if (i <= 0)
                    return;
            }
        }
        const auto randomNear = [&rand](const BlockPos& p) {
            const int dx = rand.nextInt(3) - 1;
            const int a = rand.nextInt(2);
            const int dy = a - rand.nextInt(2);
            const int dz = rand.nextInt(3) - 1;
            return p.add(dx, dy, dz);
        };
        BlockPos blockpos1 = randomNear(pos);
        for (int k = 0; k < 4; ++k) {
            if (worldIn->isAirBlock(blockpos1) && canBlockStay(worldIn, blockpos1, getDefaultState()))
                pos = blockpos1;
            blockpos1 = randomNear(pos);
        }
        if (worldIn->isAirBlock(blockpos1) && canBlockStay(worldIn, blockpos1, getDefaultState()))
            worldIn->setBlockState(blockpos1, getDefaultState(), 2);
    }
}

bool BlockMushroom::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return BlockBush::canPlaceBlockAt(worldIn, pos) && canBlockStay(worldIn, pos, getDefaultState());
}

bool BlockMushroom::canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (pos.getY() >= 0 && pos.getY() < 256) {
        const IBlockState* iblockstate = worldIn->getBlockState(pos.down());
        if (iblockstate->getBlock() == Blocks::mycelium)
            return true;
        if (iblockstate->getBlock() == Blocks::dirt && iblockstate->getValue(BlockDirt::VARIANT) == BlockDirt::DirtType::PODZOL)
            return true;
        return worldIn->getLight(pos) < 13 && canPlaceBlockOn(iblockstate->getBlock());
    }
    return false;
}

bool BlockMushroom::generateBigMushroom(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    worldIn->setBlockToAir(pos);
    std::unique_ptr<WorldGenerator> worldgenerator;
    if (this == Blocks::brown_mushroom)
        worldgenerator = std::make_unique<WorldGenBigMushroom>(Blocks::brown_mushroom_block);
    else if (this == Blocks::red_mushroom)
        worldgenerator = std::make_unique<WorldGenBigMushroom>(Blocks::red_mushroom_block);
    if (worldgenerator && worldgenerator->generate(worldIn, rand, pos))
        return true;
    worldIn->setBlockState(pos, state, 3);
    return false;
}

// --- BlockNetherWart ---

bool BlockNetherWart::canPlaceBlockOn(const Block* ground) const
{
    return ground == Blocks::soul_sand;
}

bool BlockNetherWart::canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    return canPlaceBlockOn(worldIn->getBlockState(pos.down())->getBlock());
}

void BlockNetherWart::updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    const int i = state->getValue(AGE);
    if (i < 3 && rand.nextInt(10) == 0) {
        state = state->withProperty(AGE, i + 1);
        worldIn->setBlockState(pos, state, 2);
    }
    BlockBush::updateTick(worldIn, pos, state, rand);
}

void BlockNetherWart::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float, int fortune)
{
    if (!worldIn->isRemote) {
        int i = 1;
        if (state->getValue(AGE) >= 3) {
            i = 2 + worldIn->rand.nextInt(3);
            if (fortune > 0)
                i += worldIn->rand.nextInt(fortune + 1);
        }
        for (int j = 0; j < i; ++j)
            spawnAsEntity(worldIn, pos, ItemStack(Items::nether_wart));
    }
}

Item* BlockNetherWart::getItem(World*, const BlockPos&)
{
    return Items::nether_wart;
}

// --- BlockDoublePlant ---

bool BlockDoublePlant::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return BlockBush::canPlaceBlockAt(worldIn, pos) && worldIn->isAirBlock(pos.up());
}

bool BlockDoublePlant::isReplaceable(World* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() != this)
        return true;
    const EnumPlantType type = getActualState(iblockstate, worldIn, pos)->getValue(VARIANT);
    return type == EnumPlantType::FERN || type == EnumPlantType::GRASS;
}

bool BlockDoublePlant::canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (state->getValue(HALF) == EnumBlockHalf::UPPER)
        return worldIn->getBlockState(pos.down())->getBlock() == this;
    const IBlockState* iblockstate = worldIn->getBlockState(pos.up());
    return iblockstate->getBlock() == this && BlockBush::canBlockStay(worldIn, pos, iblockstate);
}

void BlockDoublePlant::placeAt(World* worldIn, const BlockPos& lowerPos, EnumPlantType variant, int flags)
{
    worldIn->setBlockState(lowerPos, getDefaultState()->withProperty(HALF, EnumBlockHalf::LOWER)->withProperty(VARIANT, variant), flags);
    worldIn->setBlockState(lowerPos.up(), getDefaultState()->withProperty(HALF, EnumBlockHalf::UPPER), flags);
}

// --- BlockCactus ---

bool BlockCactus::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return Block::canPlaceBlockAt(worldIn, pos) ? canBlockStay(worldIn, pos) : false;
}

void BlockCactus::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState*, Block*)
{
    if (!canBlockStay(worldIn, pos))
        worldIn->destroyBlock(pos, true);
}

bool BlockCactus::canBlockStay(World* worldIn, const BlockPos& pos)
{
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (worldIn->getBlockState(pos.offset(enumfacing))->getBlock()->getMaterial().isSolid())
            return false;
    }
    const Block* block = worldIn->getBlockState(pos.down())->getBlock();
    return block == Blocks::cactus || block == Blocks::sand;
}

// --- BlockReed ---

bool BlockReed::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    const Block* block = worldIn->getBlockState(pos.down())->getBlock();
    if (block == this)
        return true;
    if (block != Blocks::grass && block != Blocks::dirt && block != Blocks::sand)
        return false;
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (&worldIn->getBlockState(pos.offset(enumfacing).down())->getBlock()->getMaterial() == &Material::water)
            return true;
    }
    return false;
}

// --- BlockVine ---

bool BlockVine::canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side)
{
    switch (side) {
    case EnumFacing::UP:
        return canPlaceOn(worldIn->getBlockState(pos.up())->getBlock());
    case EnumFacing::NORTH:
    case EnumFacing::SOUTH:
    case EnumFacing::EAST:
    case EnumFacing::WEST:
        return canPlaceOn(worldIn->getBlockState(pos.offset(getOpposite(side)))->getBlock());
    default:
        return false;
    }
}

bool BlockVine::canPlaceOn(const Block* blockIn)
{
    return blockIn->isFullCube() && blockIn->getMaterial().blocksMovement();
}

// --- BlockSnow ---

bool BlockSnow::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos.down());
    const Block* block = iblockstate->getBlock();
    if (block == Blocks::ice || block == Blocks::packed_ice)
        return false;
    if (&block->getMaterial() == &Material::leaves)
        return true;
    if (block == this && iblockstate->getValue(LAYERS) >= 7)
        return true;
    return block->isOpaqueCube() && block->getMaterial().blocksMovement();
}

// --- BlockPumpkin ---

bool BlockPumpkin::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos)->getBlock()->getMaterial().isReplaceable() && World::doesBlockHaveSolidTopSurface(worldIn, pos.down());
}

// --- BlockChest ---

const IBlockState* BlockChest::correctFacing(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    std::optional<EnumFacing> enumfacing;
    for (const EnumFacing enumfacing1 : kPlaneHorizontal) {
        const IBlockState* iblockstate = worldIn->getBlockState(pos.offset(enumfacing1));
        if (iblockstate->getBlock() == this)
            return state;
        if (iblockstate->getBlock()->isFullBlock()) {
            if (enumfacing) {
                enumfacing.reset();
                break;
            }
            enumfacing = enumfacing1;
        }
    }
    if (enumfacing)
        return state->withProperty(FACING, getOpposite(*enumfacing));
    EnumFacing enumfacing2 = state->getValue(FACING);
    if (worldIn->getBlockState(pos.offset(enumfacing2))->getBlock()->isFullBlock())
        enumfacing2 = getOpposite(enumfacing2);
    if (worldIn->getBlockState(pos.offset(enumfacing2))->getBlock()->isFullBlock())
        enumfacing2 = rotateY(enumfacing2);
    if (worldIn->getBlockState(pos.offset(enumfacing2))->getBlock()->isFullBlock())
        enumfacing2 = getOpposite(enumfacing2);
    return state->withProperty(FACING, enumfacing2);
}

} // namespace mc
