#pragma once

// Original: net/minecraft/item/Item.java (with Item.ToolMaterial), EnumAction.java and EnumRarity.java.
// The world callbacks (onItemUse, onItemRightClick, onItemUseFinish, ...) take the stack the way Java does. The
// ones that may hand back a different stack (right click, use finish) take and return shared pointers, so the
// callers can still compare identities. Not ported yet: addInformation (tooltips, stage 11) and
// getItemAttributeModifiers (entity/EntityItemHooks.h has it).
//
// Unlike blocks, item constructors make no virtual calls, so items are built in one step. They are owned by
// Item::create (registration) and live for the rest of the program.

#include "util/Registry.h"
#include "util/ResourceLocation.h"
#include "util/Vec3.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Declares getClassName() for an item subclass.
#define MC_ITEM_CLASS(NAME)     std::string_view getClassName() const override { return #NAME; }

namespace mc {

class Block;
class CreativeTabs;
class Entity;
class EntityLivingBase;
class EntityPlayer;
class ItemStack;
class NBTTagCompound;
class Random;
class World;
struct BlockPos;
struct MovingObjectPosition;
enum class EnumFacing;

enum class EnumAction { NONE, EAT, DRINK, BLOCK, BOW };

constexpr std::string_view getActionName(EnumAction action)
{
    constexpr std::string_view names[5] { "NONE", "EAT", "DRINK", "BLOCK", "BOW" };
    return names[static_cast<int>(action)];
}

enum class EnumRarity { COMMON, UNCOMMON, RARE, EPIC };

struct EnumRarityInfo {
    std::string_view enumName;
    char rarityColor; // EnumChatFormatting code: WHITE, YELLOW, AQUA, LIGHT_PURPLE
    std::string_view rarityName;
};

inline constexpr EnumRarityInfo kRarities[4] {
    { "COMMON", 'f', "Common" },
    { "UNCOMMON", 'e', "Uncommon" },
    { "RARE", 'b', "Rare" },
    { "EPIC", 'd', "Epic" },
};

constexpr const EnumRarityInfo& info(EnumRarity rarity) { return kRarities[static_cast<int>(rarity)]; }

class Item {
public:
    class ToolMaterial {
    public:
        static const ToolMaterial WOOD, STONE, IRON, EMERALD, GOLD;

        int getMaxUses() const { return maxUses; }
        float getEfficiencyOnProperMaterial() const { return efficiencyOnProperMaterial; }
        float getDamageVsEntity() const { return damageVsEntity; }
        int getHarvestLevel() const { return harvestLevel; }
        int getEnchantability() const { return enchantability; }
        const Item* getRepairItem() const;
        // Java: name() / toString()
        std::string_view name() const { return enumName; }

    private:
        constexpr ToolMaterial(std::string_view nameIn, int harvestLevelIn, int maxUsesIn, float efficiency, float damage, int enchantabilityIn)
            : enumName(nameIn)
            , harvestLevel(harvestLevelIn)
            , maxUses(maxUsesIn)
            , efficiencyOnProperMaterial(efficiency)
            , damageVsEntity(damage)
            , enchantability(enchantabilityIn)
        {
        }

        std::string_view enumName;
        int harvestLevel;
        int maxUses;
        float efficiencyOnProperMaterial;
        float damageVsEntity;
        int enchantability;
    };

    static RegistryNamespaced<ResourceLocation, Item> itemRegistry;
    // Java: the shared itemRand used by item callbacks.
    static Random& itemRand();

    Item() = default;
    virtual ~Item() = default;
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    static int getIdFromItem(const Item* item);
    static Item* getItemById(int id);
    static Item* getItemFromBlock(const Block* block);
    // By registry name, or by numeric id; null when neither matches.
    static Item* getByNameOrId(std::string_view id);
    // The registry name, nullopt if unregistered.
    static std::optional<ResourceLocation> getNameForItem(const Item* item) { return itemRegistry.getNameForObject(item); }

    virtual bool updateItemStackNBT(NBTTagCompound&) const { return false; }
    Item* setMaxStackSize(int size)
    {
        maxStackSize = size;
        return this;
    }
    virtual float getStrVsBlock(const ItemStack& stack, const Block* block) const;
    int getItemStackLimit() const { return maxStackSize; }
    virtual int getMetadata(int damage) const;
    bool getHasSubtypes() const { return hasSubtypes; }
    int getMaxDamage() const { return maxDamage; }
    bool isDamageable() const { return maxDamage > 0 && !hasSubtypes; }
    virtual bool canHarvestBlock(const Block* block) const;
    Item* setFull3D()
    {
        bFull3D = true;
        return this;
    }
    virtual bool isFull3D() const { return bFull3D; }
    virtual bool shouldRotateAroundWhenRendering() const { return false; }
    Item* setUnlocalizedName(std::string name)
    {
        unlocalizedName = std::move(name);
        return this;
    }
    std::string getUnlocalizedNameInefficiently(const ItemStack& stack) const;
    virtual std::string getUnlocalizedName() const { return "item." + unlocalizedName; }
    virtual std::string getUnlocalizedName(const ItemStack& stack) const;
    Item* setContainerItem(Item* item)
    {
        containerItem = item;
        return this;
    }
    virtual bool getShareTag() const { return true; }
    Item* getContainerItem() const { return containerItem; }
    bool hasContainerItem() const { return containerItem != nullptr; }
    virtual int getColorFromItemStack(const ItemStack& stack, int renderPass) const;
    virtual bool isMap() const { return false; }
    virtual EnumAction getItemUseAction(const ItemStack& stack) const;
    virtual int getMaxItemUseDuration(const ItemStack& stack) const;
    // Java's null is nullopt.
    virtual std::optional<std::string> getPotionEffect(const ItemStack& stack) const;
    bool isPotionIngredient(const ItemStack& stack) const { return getPotionEffect(stack).has_value(); }
    virtual std::string getItemStackDisplayName(const ItemStack& stack) const;
    virtual bool hasEffect(const ItemStack& stack) const;
    virtual EnumRarity getRarity(const ItemStack& stack) const;
    virtual bool isItemTool(const ItemStack& stack) const;
    virtual int getItemEnchantability() const { return 0; }
    virtual void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const;
    virtual const CreativeTabs* getCreativeTab() const { return tabToDisplayOn; }
    Item* setCreativeTab(const CreativeTabs* tab)
    {
        tabToDisplayOn = tab;
        return this;
    }
    virtual bool canItemEditBlocks() const { return false; }
    virtual bool getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const;

    // --- world callbacks (Java defaults) ---
    virtual bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ);
    // Returns the stack the player holds afterwards: the same one, or a new one.
    virtual std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn);
    virtual std::shared_ptr<ItemStack> onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn);
    virtual bool hitEntity(ItemStack& stack, EntityLivingBase* target, EntityLivingBase* attacker);
    virtual bool onBlockDestroyed(ItemStack& stack, World* worldIn, Block* blockIn, const BlockPos& pos, EntityLivingBase* playerIn);
    virtual bool itemInteractionForEntity(ItemStack& stack, EntityPlayer* playerIn, EntityLivingBase* target);
    virtual void onUpdate(ItemStack&, World*, Entity*, int, bool) { }
    virtual void onCreated(ItemStack&, World*, EntityPlayer*) { }
    virtual void onPlayerStoppedUsing(ItemStack&, World*, EntityPlayer*, int) { }
    // Java: getClass().getSimpleName(), for the registry dump.
    virtual std::string_view getClassName() const { return "Item"; }

    // Java: Item.registerItems(), called from Bootstrap after the blocks.
    static void registerItems();

    // Protected in Java; used by the registration code, which isn't part of the class here.
    Item* setHasSubtypes(bool value)
    {
        hasSubtypes = value;
        return this;
    }
    Item* setMaxDamage(int value)
    {
        maxDamage = value;
        return this;
    }
    Item* setPotionEffect(std::string effect)
    {
        potionEffect = std::move(effect);
        return this;
    }

protected:
    // Java: the block the player looks at within 5 blocks; nullopt for none.
    std::optional<MovingObjectPosition> getMovingObjectPositionFromPlayer(World* worldIn, EntityPlayer* playerIn, bool useLiquids) const;

    int maxStackSize = 64;
    bool bFull3D = false;
    bool hasSubtypes = false;

private:
    static void registerItemBlock(Block* block);
    static void registerItemBlock(Block* block, Item* item);
    static void registerItem(int id, const std::string& textualID, Item* item);
    static void registerItem(int id, const ResourceLocation& textualID, Item* item);

    const CreativeTabs* tabToDisplayOn = nullptr;
    int maxDamage = 0;
    Item* containerItem = nullptr;
    std::optional<std::string> potionEffect;
    // Java's null: items registered without a name show "item.null".
    std::string unlocalizedName = "null";
};

} // namespace mc
