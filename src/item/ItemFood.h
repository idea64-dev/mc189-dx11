#pragma once

// Original: net/minecraft/item/ItemFood.java, ItemAppleGold.java, ItemFishFood.java, ItemSoup.java,
// ItemSeedFood.java and ItemSeeds.java.

#include "item/Item.h"

namespace mc {

class ItemFood : public Item {
public:
    MC_ITEM_CLASS(ItemFood)

    ItemFood(int amount, float saturation, bool isWolfFood);
    ItemFood(int amount, bool isWolfFood)
        : ItemFood(amount, 0.6f, isWolfFood)
    {
    }

    int getMaxItemUseDuration(const ItemStack&) const override { return 32; }
    EnumAction getItemUseAction(const ItemStack&) const override { return EnumAction::EAT; }
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
    std::shared_ptr<ItemStack> onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn) override;
    virtual int getHealAmount(const ItemStack&) const { return healAmount; }
    virtual float getSaturationModifier(const ItemStack&) const { return saturationModifier; }
    bool isWolfsFavoriteMeat() const { return wolfsFavoriteMeat; }
    using Item::setPotionEffect;
    ItemFood* setPotionEffect(int id, int duration, int amplifier, float probability)
    {
        potionId = id;
        potionDuration = duration;
        potionAmplifier = amplifier;
        potionEffectProbability = probability;
        return this;
    }
    ItemFood* setAlwaysEdible()
    {
        alwaysEdible = true;
        return this;
    }

    // Private in Java; read by the registry dump.
    bool isAlwaysEdible() const { return alwaysEdible; }
    int getPotionId() const { return potionId; }
    int getPotionDuration() const { return potionDuration; }
    int getPotionAmplifier() const { return potionAmplifier; }
    float getPotionEffectProbability() const { return potionEffectProbability; }

    const int itemUseDuration = 32;

protected:
    virtual void onFoodEaten(ItemStack& stack, World* worldIn, EntityPlayer* player);

private:
    const int healAmount;
    const float saturationModifier;
    const bool wolfsFavoriteMeat;
    bool alwaysEdible = false;
    int potionId = 0;
    int potionDuration = 0;
    int potionAmplifier = 0;
    float potionEffectProbability = 0.0f;
};

class ItemAppleGold : public ItemFood {
public:
    MC_ITEM_CLASS(ItemAppleGold)

    ItemAppleGold(int amount, float saturation, bool isWolfFood)
        : ItemFood(amount, saturation, isWolfFood)
    {
        setHasSubtypes(true);
    }

    bool hasEffect(const ItemStack& stack) const override;
    EnumRarity getRarity(const ItemStack& stack) const override;
    void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const override;

protected:
    void onFoodEaten(ItemStack& stack, World* worldIn, EntityPlayer* player) override;
};

class ItemFishFood : public ItemFood {
public:
    MC_ITEM_CLASS(ItemFishFood)

    enum class FishType { COD, SALMON, CLOWNFISH, PUFFERFISH };
    struct FishTypeInfo {
        int meta;
        std::string_view unlocalizedName;
        int uncookedHealAmount;
        float uncookedSaturationModifier;
        int cookedHealAmount;
        float cookedSaturationModifier;
        bool cookable;
    };
    static constexpr FishTypeInfo kFishTypes[4] {
        { 0, "cod", 2, 0.1f, 5, 0.6f, true },
        { 1, "salmon", 2, 0.1f, 6, 0.8f, true },
        { 2, "clownfish", 1, 0.1f, 0, 0.0f, false },
        { 3, "pufferfish", 1, 0.1f, 0, 0.0f, false },
    };
    static const FishTypeInfo& info(FishType type) { return kFishTypes[static_cast<int>(type)]; }
    // Unknown metadata gives COD.
    static FishType byMetadata(int meta) { return meta < 0 || meta >= 4 ? FishType::COD : static_cast<FishType>(meta); }
    static FishType byItemStack(const ItemStack& stack);

    explicit ItemFishFood(bool cookedIn)
        : ItemFood(0, 0.0f, false)
        , cooked(cookedIn)
    {
    }

    int getHealAmount(const ItemStack& stack) const override;
    float getSaturationModifier(const ItemStack& stack) const override;
    std::optional<std::string> getPotionEffect(const ItemStack& stack) const override;
    void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const override;
    std::string getUnlocalizedName(const ItemStack& stack) const override;
    using ItemFood::getUnlocalizedName;

protected:
    void onFoodEaten(ItemStack& stack, World* worldIn, EntityPlayer* player) override;

private:
    const bool cooked;
};

class ItemSoup : public ItemFood {
public:
    MC_ITEM_CLASS(ItemSoup)

    explicit ItemSoup(int healAmount)
        : ItemFood(healAmount, false)
    {
        setMaxStackSize(1);
    }

    // Gives the bowl back.
    std::shared_ptr<ItemStack> onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn) override;
};

class ItemSeedFood : public ItemFood {
public:
    MC_ITEM_CLASS(ItemSeedFood)

    ItemSeedFood(int healAmount, float saturation, Block* cropsIn, Block* soil)
        : ItemFood(healAmount, saturation, false)
        , crops(cropsIn)
        , soilId(soil)
    {
    }

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;

private:
    Block* crops;
    Block* soilId;
};

class ItemSeeds : public Item {
public:
    MC_ITEM_CLASS(ItemSeeds)

    ItemSeeds(Block* cropsIn, Block* soil);

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;

private:
    Block* crops;
    Block* soilBlockID;
};

} // namespace mc
