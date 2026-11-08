#include "item/ItemBlock.h"

#include "block/BlockDoublePlant.h"
#include "block/BlockLeaves.h"
#include "block/BlockSlab.h"
#include "client/resources/Locale.h"
#include "creativetab/CreativeTabs.h"
#include "init/Blocks.h"
#include "item/EnumDyeColor.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "world/Colorizer.h"

namespace mc {

// --- ItemBlock ---

std::string ItemBlock::getUnlocalizedName(const ItemStack&) const
{
    return block->getUnlocalizedName();
}

std::string ItemBlock::getUnlocalizedName() const
{
    return block->getUnlocalizedName();
}

const CreativeTabs* ItemBlock::getCreativeTab() const
{
    return block->getCreativeTabToDisplayOn();
}

void ItemBlock::getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const
{
    block->getSubBlocks(item, tab, subItems);
}

// --- ItemMultiTexture ---

ItemMultiTexture::ItemMultiTexture(Block* blockIn, Block* block2, std::vector<std::string> namesByMeta)
    : ItemMultiTexture(blockIn, block2, [names = std::move(namesByMeta)](const ItemStack& stack) {
        int i = stack.getMetadata();
        if (i < 0 || i >= static_cast<int>(names.size()))
            i = 0;
        return names[static_cast<size_t>(i)];
    })
{
}

std::string ItemMultiTexture::getUnlocalizedName(const ItemStack& stack) const
{
    return ItemBlock::getUnlocalizedName() + "." + nameFunction(stack);
}

// --- ItemColored ---

int ItemColored::getColorFromItemStack(const ItemStack& stack, int) const
{
    return coloredBlock->getRenderColor(coloredBlock->getStateFromMeta(stack.getMetadata()));
}

std::string ItemColored::getUnlocalizedName(const ItemStack& stack) const
{
    if (!hasSubtypeNames)
        return ItemBlock::getUnlocalizedName(stack);
    const int i = stack.getMetadata();
    return i >= 0 && i < static_cast<int>(subtypeNames.size()) ? ItemBlock::getUnlocalizedName(stack) + "." + subtypeNames[static_cast<size_t>(i)]
                                                               : ItemBlock::getUnlocalizedName(stack);
}

// --- ItemCloth ---

std::string ItemCloth::getUnlocalizedName(const ItemStack& stack) const
{
    return ItemBlock::getUnlocalizedName() + "." + std::string(info(dyeColorByMetadata(stack.getMetadata())).unlocalizedName);
}

// --- ItemSlab ---

std::string ItemSlab::getUnlocalizedName(const ItemStack& stack) const
{
    return singleSlab->getUnlocalizedName(stack.getMetadata());
}

// --- ItemLeaves ---

ItemLeaves::ItemLeaves(BlockLeaves* blockIn)
    : ItemBlock(blockIn)
    , leaves(blockIn)
{
    setMaxDamage(0);
    setHasSubtypes(true);
}

int ItemLeaves::getColorFromItemStack(const ItemStack& stack, int) const
{
    return leaves->getRenderColor(leaves->getStateFromMeta(stack.getMetadata()));
}

std::string ItemLeaves::getUnlocalizedName(const ItemStack& stack) const
{
    return ItemBlock::getUnlocalizedName() + "." + std::string(BlockPlanks::info(leaves->getWoodType(stack.getMetadata())).unlocalizedName);
}

// --- ItemLilyPad ---

int ItemLilyPad::getColorFromItemStack(const ItemStack& stack, int) const
{
    return Blocks::waterlily->getRenderColor(Blocks::waterlily->getStateFromMeta(stack.getMetadata()));
}

// --- ItemDoublePlant ---

int ItemDoublePlant::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    const BlockDoublePlant::EnumPlantType type = BlockDoublePlant::byMetadata(stack.getMetadata());
    return type != BlockDoublePlant::EnumPlantType::GRASS && type != BlockDoublePlant::EnumPlantType::FERN ? ItemMultiTexture::getColorFromItemStack(stack, renderPass)
                                                                                                           : ColorizerGrass::getGrassColor(0.5, 1.0);
}

// --- ItemBanner ---

namespace {

EnumDyeColor getBaseColor(const ItemStack& stack)
{
    const auto& tag = stack.getTagCompound();
    const auto nbttagcompound = tag && tag->hasKey("BlockEntityTag", 10) ? tag->getCompoundTag("BlockEntityTag") : nullptr;
    if (nbttagcompound && nbttagcompound->hasKey("Base"))
        return dyeColorByDyeDamage(nbttagcompound->getInteger("Base"));
    return dyeColorByDyeDamage(stack.getMetadata());
}

} // namespace

ItemBanner::ItemBanner()
    : ItemBlock(Blocks::standing_banner)
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabDecorations);
    setHasSubtypes(true);
    setMaxDamage(0);
}

std::string ItemBanner::getItemStackDisplayName(const ItemStack& stack) const
{
    return StatCollector::translateToLocal("item.banner." + std::string(info(getBaseColor(stack)).unlocalizedName) + ".name");
}

int ItemBanner::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    if (renderPass == 0)
        return 16777215;
    return info(getBaseColor(stack)).mapColor->colorValue;
}

void ItemBanner::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (const EnumDyeColor color : enumValues<EnumDyeColor, 16>()) {
        // TileEntityBanner.setBaseColorAndPatterns(tag, dyeDamage, null) sets Base only.
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        nbttagcompound->setInteger("Base", getDyeDamage(color));
        auto nbttagcompound1 = std::make_shared<NBTTagCompound>();
        nbttagcompound1->setTag("BlockEntityTag", nbttagcompound);
        ItemStack itemstack(item, 1, getDyeDamage(color));
        itemstack.setTagCompound(nbttagcompound1);
        subItems.push_back(std::move(itemstack));
    }
}

const CreativeTabs* ItemBanner::getCreativeTab() const
{
    return &CreativeTabs::tabDecorations;
}

} // namespace mc
