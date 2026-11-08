#pragma once

// Original: net/minecraft/item/ItemTool.java, ItemSpade.java, ItemPickaxe.java, ItemAxe.java, ItemSword.java,
// ItemHoe.java and ItemShears.java.
// The attack damage attribute modifiers are in entity/EntityItemHooks.h.

#include "item/Item.h"

#include <unordered_set>

namespace mc {

class IBlockState;

class ItemTool : public Item {
public:
    MC_ITEM_CLASS(ItemTool)

    float getStrVsBlock(const ItemStack& stack, const Block* block) const override;
    bool hitEntity(ItemStack& stack, EntityLivingBase* target, EntityLivingBase* attacker) override;
    bool onBlockDestroyed(ItemStack& stack, World* worldIn, Block* blockIn, const BlockPos& pos, EntityLivingBase* playerIn) override;
    bool isFull3D() const override { return true; }
    const ToolMaterial& getToolMaterial() const { return toolMaterial; }
    int getItemEnchantability() const override { return toolMaterial.getEnchantability(); }
    std::string_view getToolMaterialName() const { return toolMaterial.name(); }
    bool getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const override;
    float getEfficiencyOnProperMaterial() const { return efficiencyOnProperMaterial; }
    // Private in Java; read by the registry dump.
    float getDamageVsEntityValue() const { return damageVsEntity; }

protected:
    ItemTool(float attackDamage, const ToolMaterial& material, const std::unordered_set<const Block*>& effectiveBlocksIn);

    float efficiencyOnProperMaterial = 4.0f;
    const ToolMaterial& toolMaterial;

private:
    const std::unordered_set<const Block*>& effectiveBlocks;
    float damageVsEntity;
};

class ItemSpade : public ItemTool {
public:
    MC_ITEM_CLASS(ItemSpade)

    explicit ItemSpade(const ToolMaterial& material);

    bool canHarvestBlock(const Block* block) const override;
};

class ItemPickaxe : public ItemTool {
public:
    MC_ITEM_CLASS(ItemPickaxe)

    explicit ItemPickaxe(const ToolMaterial& material);

    bool canHarvestBlock(const Block* block) const override;
    float getStrVsBlock(const ItemStack& stack, const Block* block) const override;
};

class ItemAxe : public ItemTool {
public:
    MC_ITEM_CLASS(ItemAxe)

    explicit ItemAxe(const ToolMaterial& material);

    float getStrVsBlock(const ItemStack& stack, const Block* block) const override;
};

class ItemSword : public Item {
public:
    MC_ITEM_CLASS(ItemSword)

    explicit ItemSword(const ToolMaterial& materialIn);

    float getDamageVsEntity() const { return material.getDamageVsEntity(); }
    float getStrVsBlock(const ItemStack& stack, const Block* block) const override;
    bool hitEntity(ItemStack& stack, EntityLivingBase* target, EntityLivingBase* attacker) override;
    bool onBlockDestroyed(ItemStack& stack, World* worldIn, Block* blockIn, const BlockPos& pos, EntityLivingBase* playerIn) override;
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
    bool isFull3D() const override { return true; }
    EnumAction getItemUseAction(const ItemStack&) const override { return EnumAction::BLOCK; }
    int getMaxItemUseDuration(const ItemStack&) const override { return 72000; }
    bool canHarvestBlock(const Block* block) const override;
    int getItemEnchantability() const override { return material.getEnchantability(); }
    std::string_view getToolMaterialName() const { return material.name(); }
    bool getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const override;

private:
    [[maybe_unused]] float attackDamage; // attribute modifiers (stage 10)
    const ToolMaterial& material;
};

class ItemHoe : public Item {
public:
    MC_ITEM_CLASS(ItemHoe)

    explicit ItemHoe(const ToolMaterial& material);

    bool isFull3D() const override { return true; }
    std::string_view getMaterialName() const { return theToolMaterial.name(); }
    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;

protected:
    bool useHoe(ItemStack& stack, EntityPlayer* player, World* worldIn, const BlockPos& target, const IBlockState* newState);

    const ToolMaterial& theToolMaterial;
};

class ItemShears : public Item {
public:
    MC_ITEM_CLASS(ItemShears)

    ItemShears();

    bool canHarvestBlock(const Block* block) const override;
    float getStrVsBlock(const ItemStack& stack, const Block* block) const override;
    bool onBlockDestroyed(ItemStack& stack, World* worldIn, Block* blockIn, const BlockPos& pos, EntityLivingBase* playerIn) override;
};

} // namespace mc
