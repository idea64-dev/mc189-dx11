// The item drop methods of the block classes that need more than a line (Java: getItemDropped, getSeed /
// getCrop, createStackedBlock, BlockOre.quantityDroppedWithBonus).

#include "block/AllBlocks.h"
#include "init/Items.h"
#include "item/ItemStack.h"

namespace mc {

Item* BlockCrops::getSeed() const
{
    return Items::wheat_seeds;
}

Item* BlockCrops::getCrop() const
{
    return Items::wheat;
}

Item* BlockCarrot::getSeed() const
{
    return Items::carrot;
}

Item* BlockCarrot::getCrop() const
{
    return Items::carrot;
}

Item* BlockPotato::getSeed() const
{
    return Items::potato;
}

Item* BlockPotato::getCrop() const
{
    return Items::potato;
}

Item* BlockDoor::getDoorItem() const
{
    if (this == Blocks::iron_door)
        return Items::iron_door;
    if (this == Blocks::spruce_door)
        return Items::spruce_door;
    if (this == Blocks::birch_door)
        return Items::birch_door;
    if (this == Blocks::jungle_door)
        return Items::jungle_door;
    if (this == Blocks::acacia_door)
        return Items::acacia_door;
    if (this == Blocks::dark_oak_door)
        return Items::dark_oak_door;
    return Items::oak_door;
}

namespace {

// Grass, mycelium and farmland drop what plain dirt drops.
Item* dirtDrop(Random& rand, int fortune)
{
    return Blocks::dirt->getItemDropped(Blocks::dirt->getDefaultState()->withProperty(BlockDirt::VARIANT, BlockDirt::DirtType::DIRT), rand, fortune);
}

} // namespace

Item* BlockGrass::getItemDropped(const IBlockState*, Random& rand, int fortune) const
{
    return dirtDrop(rand, fortune);
}

Item* BlockMycelium::getItemDropped(const IBlockState*, Random& rand, int fortune) const
{
    return dirtDrop(rand, fortune);
}

Item* BlockFarmland::getItemDropped(const IBlockState*, Random& rand, int fortune) const
{
    return dirtDrop(rand, fortune);
}

Item* BlockDoublePlant::getItemDropped(const IBlockState* state, Random& rand, int) const
{
    if (state->getValue(HALF) == EnumBlockHalf::UPPER)
        return nullptr;
    const EnumPlantType type = state->getValue(VARIANT);
    if (type == EnumPlantType::FERN)
        return nullptr;
    if (type == EnumPlantType::GRASS)
        return rand.nextInt(8) == 0 ? Items::wheat_seeds : nullptr;
    return Item::getItemFromBlock(this);
}

Item* BlockGravel::getItemDropped(const IBlockState*, Random& rand, int fortune) const
{
    if (fortune > 3)
        fortune = 3;
    return rand.nextInt(10 - fortune * 3) == 0 ? Items::flint : Item::getItemFromBlock(this);
}

Item* BlockOre::getItemDropped(const IBlockState*, Random&, int) const
{
    if (this == Blocks::coal_ore)
        return Items::coal;
    if (this == Blocks::diamond_ore)
        return Items::diamond;
    if (this == Blocks::lapis_ore)
        return Items::dye;
    if (this == Blocks::emerald_ore)
        return Items::emerald;
    if (this == Blocks::quartz_ore)
        return Items::quartz;
    return Item::getItemFromBlock(this);
}

int BlockOre::quantityDroppedWithBonus(int fortune, Random& random) const
{
    if (fortune > 0 && Item::getItemFromBlock(this) != getItemDropped(getBlockState().getValidStates().front(), random, fortune)) {
        int i = random.nextInt(fortune + 2) - 1;
        if (i < 0)
            i = 0;
        return quantityDropped(random) * (i + 1);
    }
    return quantityDropped(random);
}

ItemStack BlockQuartz::createStackedBlock(const IBlockState* state) const
{
    const EnumType type = state->getValue(VARIANT);
    return type != EnumType::LINES_X && type != EnumType::LINES_Z ? Block::createStackedBlock(state)
                                                                   : ItemStack(Item::getItemFromBlock(this), 1, static_cast<int>(EnumType::LINES_Y));
}

ItemStack BlockSilverfish::createStackedBlock(const IBlockState* state) const
{
    switch (state->getValue(VARIANT)) {
    case EnumType::COBBLESTONE: return ItemStack(Blocks::cobblestone);
    case EnumType::STONEBRICK: return ItemStack(Blocks::stonebrick);
    case EnumType::MOSSY_STONEBRICK: return ItemStack(Blocks::stonebrick, 1, BlockStoneBrick::info(BlockStoneBrick::EnumType::MOSSY).meta);
    case EnumType::CRACKED_STONEBRICK: return ItemStack(Blocks::stonebrick, 1, BlockStoneBrick::info(BlockStoneBrick::EnumType::CRACKED).meta);
    case EnumType::CHISELED_STONEBRICK: return ItemStack(Blocks::stonebrick, 1, BlockStoneBrick::info(BlockStoneBrick::EnumType::CHISELED).meta);
    default: return ItemStack(Blocks::stone);
    }
}

Item* BlockTallGrass::getItemDropped(const IBlockState*, Random& rand, int) const
{
    return rand.nextInt(8) == 0 ? Items::wheat_seeds : nullptr;
}

} // namespace mc
