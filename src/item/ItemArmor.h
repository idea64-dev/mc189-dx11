#pragma once

// Original: net/minecraft/item/ItemArmor.java (with ItemArmor.ArmorMaterial).
// Not ported yet: the dispenser behavior the constructor registers (BlockDispenser.dispenseBehaviorRegistry).

#include "item/Item.h"

#include <array>

namespace mc {

class ItemArmor : public Item {
public:
    MC_ITEM_CLASS(ItemArmor)

    class ArmorMaterial {
    public:
        static const ArmorMaterial LEATHER, CHAIN, IRON, GOLD, DIAMOND;

        int getDurability(int armorType) const;
        int getDamageReductionAmount(int armorType) const { return damageReductionAmountArray[static_cast<size_t>(armorType)]; }
        int getEnchantability() const { return enchantability; }
        const Item* getRepairItem() const;
        std::string_view getName() const { return materialName; }
        // Java: name()
        std::string_view name() const { return enumName; }

    private:
        constexpr ArmorMaterial(std::string_view enumNameIn, std::string_view nameIn, int maxDamage, std::array<int, 4> reductionAmounts, int enchantabilityIn)
            : enumName(enumNameIn)
            , materialName(nameIn)
            , maxDamageFactor(maxDamage)
            , damageReductionAmountArray(reductionAmounts)
            , enchantability(enchantabilityIn)
        {
        }

        std::string_view enumName;
        std::string_view materialName;
        int maxDamageFactor;
        std::array<int, 4> damageReductionAmountArray;
        int enchantability;
    };

    static constexpr const char* EMPTY_SLOT_NAMES[4] { "minecraft:items/empty_armor_slot_helmet", "minecraft:items/empty_armor_slot_chestplate",
        "minecraft:items/empty_armor_slot_leggings", "minecraft:items/empty_armor_slot_boots" };

    ItemArmor(const ArmorMaterial& materialIn, int renderIndexIn, int armorTypeIn);

    int getColorFromItemStack(const ItemStack& stack, int renderPass) const override;
    int getItemEnchantability() const override { return material.getEnchantability(); }
    const ArmorMaterial& getArmorMaterial() const { return material; }
    bool hasColor(const ItemStack& stack) const;
    // -1 for materials other than leather.
    int getColor(const ItemStack& stack) const;
    void removeColor(ItemStack& stack) const;
    // Throws std::logic_error for materials other than leather (Java: UnsupportedOperationException).
    void setColor(ItemStack& stack, int color) const;
    bool getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const override;
    // Puts the armor on when that slot is empty.
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;

    const int armorType;
    const int damageReduceAmount;
    const int renderIndex;

private:
    const ArmorMaterial& material;
};

} // namespace mc
