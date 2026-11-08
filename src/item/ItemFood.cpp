#include "item/ItemFood.h"

#include "creativetab/CreativeTabs.h"
#include "item/ItemStack.h"
#include "potion/PotionHelper.h"

namespace mc {

ItemFood::ItemFood(int amount, float saturation, bool isWolfFood)
    : healAmount(amount)
    , saturationModifier(saturation)
    , wolfsFavoriteMeat(isWolfFood)
{
    setCreativeTab(&CreativeTabs::tabFood);
}

// --- ItemAppleGold ---

bool ItemAppleGold::hasEffect(const ItemStack& stack) const
{
    return stack.getMetadata() > 0;
}

EnumRarity ItemAppleGold::getRarity(const ItemStack& stack) const
{
    return stack.getMetadata() == 0 ? EnumRarity::RARE : EnumRarity::EPIC;
}

void ItemAppleGold::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    subItems.emplace_back(item, 1, 0);
    subItems.emplace_back(item, 1, 1);
}

// --- ItemFishFood ---

ItemFishFood::FishType ItemFishFood::byItemStack(const ItemStack& stack)
{
    return dynamic_cast<const ItemFishFood*>(stack.getItem()) ? byMetadata(stack.getMetadata()) : FishType::COD;
}

int ItemFishFood::getHealAmount(const ItemStack& stack) const
{
    const FishTypeInfo& type = info(byItemStack(stack));
    return cooked && type.cookable ? type.cookedHealAmount : type.uncookedHealAmount;
}

float ItemFishFood::getSaturationModifier(const ItemStack& stack) const
{
    const FishTypeInfo& type = info(byItemStack(stack));
    return cooked && type.cookable ? type.cookedSaturationModifier : type.uncookedSaturationModifier;
}

std::optional<std::string> ItemFishFood::getPotionEffect(const ItemStack& stack) const
{
    if (byItemStack(stack) == FishType::PUFFERFISH)
        return std::string(PotionHelper::pufferfishEffect);
    return std::nullopt;
}

void ItemFishFood::getSubItems(Item*, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (const FishTypeInfo& type : kFishTypes) {
        if (!cooked || type.cookable)
            subItems.emplace_back(const_cast<ItemFishFood*>(this), 1, type.meta);
    }
}

std::string ItemFishFood::getUnlocalizedName(const ItemStack& stack) const
{
    const FishTypeInfo& type = info(byItemStack(stack));
    return getUnlocalizedName() + "." + std::string(type.unlocalizedName) + "." + (cooked && type.cookable ? "cooked" : "raw");
}

// --- ItemSeeds ---

ItemSeeds::ItemSeeds(Block* cropsIn, Block* soil)
    : crops(cropsIn)
    , soilBlockID(soil)
{
    setCreativeTab(&CreativeTabs::tabMaterials);
}

} // namespace mc
