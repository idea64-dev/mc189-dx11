#pragma once

// Original: net/minecraft/item/ItemPotion.java
// Not ported yet: drinking and throwing (onItemRightClick, onItemUseFinish) and the effect tooltips
// (addInformation) -- stages 9 and 10.

#include "item/Item.h"
#include "potion/Potion.h"

#include <unordered_map>

namespace mc {

class ItemPotion : public Item {
public:
    MC_ITEM_CLASS(ItemPotion)

    ItemPotion();

    // The stack's custom effects, else the effects of its metadata. Java returns null for none; empty here.
    std::vector<PotionEffect> getEffects(const ItemStack& stack) const;
    const std::vector<PotionEffect>& getEffects(int meta) const;
    int getMaxItemUseDuration(const ItemStack&) const override { return 32; }
    EnumAction getItemUseAction(const ItemStack&) const override { return EnumAction::DRINK; }
    static bool isSplash(int meta) { return (meta & 16384) != 0; }
    int getColorFromDamage(int meta) const;
    int getColorFromItemStack(const ItemStack& stack, int renderPass) const override;
    bool isEffectInstant(int meta) const;
    std::string getItemStackDisplayName(const ItemStack& stack) const override;
    bool hasEffect(const ItemStack& stack) const override;
    void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const override;

private:
    mutable std::unordered_map<int, std::vector<PotionEffect>> effectCache;
};

} // namespace mc
