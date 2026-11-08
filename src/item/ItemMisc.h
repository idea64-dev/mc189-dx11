#pragma once

// Original: the remaining classes of net/minecraft/item -- ItemBow, ItemFishingRod, ItemFlintAndSteel,
// ItemCarrotOnAStick, ItemReed, ItemDoor, ItemMinecart, ItemBucket, ItemBucketMilk, ItemHangingEntity, ItemSign,
// ItemBed, ItemSaddle, ItemRedstone, ItemSnowball, ItemBoat, ItemBook, ItemEgg, ItemMapBase, ItemMap,
// ItemEmptyMap, ItemEnderPearl, ItemEnderEye, ItemGlassBottle, ItemFireball, ItemLead, ItemNameTag,
// ItemArmorStand, ItemMonsterPlacer, ItemSkull, ItemFirework, ItemFireworkCharge, ItemEnchantedBook, ItemDye,
// ItemCoal, ItemRecord, ItemSimpleFoiled, ItemExpBottle, ItemEditableBook and ItemWritableBook.
// What they do in the world -- placing, throwing, spawning, filling, shooting, map updates, bonemeal, book
// editing, tooltips -- comes with the world, entities and screens (stages 5, 9 and 10). Constructor arguments
// those need are kept.

#include "item/Item.h"

#include <memory>

namespace mc {

class NBTBase;
class NBTTagList;

class ItemBow : public Item {
public:
    MC_ITEM_CLASS(ItemBow)

    static constexpr const char* bowPullIconNameArray[3] { "pulling_0", "pulling_1", "pulling_2" };

    ItemBow();

    int getMaxItemUseDuration(const ItemStack&) const override { return 72000; }
    EnumAction getItemUseAction(const ItemStack&) const override { return EnumAction::BOW; }
    int getItemEnchantability() const override { return 1; }
};

class ItemFishingRod : public Item {
public:
    MC_ITEM_CLASS(ItemFishingRod)

    ItemFishingRod();

    bool isFull3D() const override { return true; }
    bool shouldRotateAroundWhenRendering() const override { return true; }
    int getItemEnchantability() const override { return 1; }
};

class ItemFlintAndSteel : public Item {
public:
    MC_ITEM_CLASS(ItemFlintAndSteel)

    ItemFlintAndSteel();
};

class ItemCarrotOnAStick : public Item {
public:
    MC_ITEM_CLASS(ItemCarrotOnAStick)

    ItemCarrotOnAStick();

    bool isFull3D() const override { return true; }
    bool shouldRotateAroundWhenRendering() const override { return true; }
};

// An item that places a block (string, cake, repeater, cauldron...).
class ItemReed : public Item {
public:
    MC_ITEM_CLASS(ItemReed)

    explicit ItemReed(Block* blockIn)
        : block(blockIn)
    {
    }

private:
    [[maybe_unused]] Block* block;
};

class ItemDoor : public Item {
public:
    MC_ITEM_CLASS(ItemDoor)

    explicit ItemDoor(Block* blockIn);

private:
    [[maybe_unused]] Block* block;
};

class ItemMinecart : public Item {
public:
    MC_ITEM_CLASS(ItemMinecart)

    // Java: EntityMinecart.EnumMinecartType, until the minecart entities exist.
    enum class EnumMinecartType { RIDEABLE, CHEST, FURNACE, TNT, SPAWNER, HOPPER, COMMAND_BLOCK };

    explicit ItemMinecart(EnumMinecartType type);

    const EnumMinecartType minecartType;
};

class ItemBucket : public Item {
public:
    MC_ITEM_CLASS(ItemBucket)

    explicit ItemBucket(Block* containedBlock);

private:
    [[maybe_unused]] Block* isFull;
};

class ItemBucketMilk : public Item {
public:
    MC_ITEM_CLASS(ItemBucketMilk)

    ItemBucketMilk();

    int getMaxItemUseDuration(const ItemStack&) const override { return 32; }
    EnumAction getItemUseAction(const ItemStack&) const override { return EnumAction::DRINK; }
};

class ItemHangingEntity : public Item {
public:
    MC_ITEM_CLASS(ItemHangingEntity)

    // Java: the EntityHanging subclass to create.
    enum class HangingType { PAINTING, ITEM_FRAME };

    explicit ItemHangingEntity(HangingType type);

private:
    [[maybe_unused]] HangingType hangingEntityClass;
};

class ItemSign : public Item {
public:
    MC_ITEM_CLASS(ItemSign)

    ItemSign();
};

class ItemBed : public Item {
public:
    MC_ITEM_CLASS(ItemBed)

    ItemBed();
};

class ItemSaddle : public Item {
public:
    MC_ITEM_CLASS(ItemSaddle)

    ItemSaddle();
};

class ItemRedstone : public Item {
public:
    MC_ITEM_CLASS(ItemRedstone)

    ItemRedstone();
};

class ItemSnowball : public Item {
public:
    MC_ITEM_CLASS(ItemSnowball)

    ItemSnowball();
};

class ItemBoat : public Item {
public:
    MC_ITEM_CLASS(ItemBoat)

    ItemBoat();
};

class ItemBook : public Item {
public:
    MC_ITEM_CLASS(ItemBook)

    bool isItemTool(const ItemStack& stack) const override;
    int getItemEnchantability() const override { return 1; }
};

class ItemEgg : public Item {
public:
    MC_ITEM_CLASS(ItemEgg)

    ItemEgg();
};

class ItemMapBase : public Item {
public:
    MC_ITEM_CLASS(ItemMapBase)

    bool isMap() const override { return true; }
};

class ItemMap : public ItemMapBase {
public:
    MC_ITEM_CLASS(ItemMap)

    ItemMap() { setHasSubtypes(true); }
};

class ItemEmptyMap : public ItemMapBase {
public:
    MC_ITEM_CLASS(ItemEmptyMap)

    ItemEmptyMap();
};

class ItemEnderPearl : public Item {
public:
    MC_ITEM_CLASS(ItemEnderPearl)

    ItemEnderPearl();
};

class ItemEnderEye : public Item {
public:
    MC_ITEM_CLASS(ItemEnderEye)

    ItemEnderEye();
};

class ItemGlassBottle : public Item {
public:
    MC_ITEM_CLASS(ItemGlassBottle)

    ItemGlassBottle();
};

class ItemFireball : public Item {
public:
    MC_ITEM_CLASS(ItemFireball)

    ItemFireball();
};

class ItemLead : public Item {
public:
    MC_ITEM_CLASS(ItemLead)

    ItemLead();
};

class ItemNameTag : public Item {
public:
    MC_ITEM_CLASS(ItemNameTag)

    ItemNameTag();
};

class ItemArmorStand : public Item {
public:
    MC_ITEM_CLASS(ItemArmorStand)

    ItemArmorStand();
};

class ItemMonsterPlacer : public Item {
public:
    MC_ITEM_CLASS(ItemMonsterPlacer)

    ItemMonsterPlacer();

    std::string getItemStackDisplayName(const ItemStack& stack) const override;
    int getColorFromItemStack(const ItemStack& stack, int renderPass) const override;
    void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const override;
};

class ItemSkull : public Item {
public:
    MC_ITEM_CLASS(ItemSkull)

    static constexpr const char* skullTypes[5] { "skeleton", "wither", "zombie", "char", "creeper" };

    ItemSkull();

    void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const override;
    int getMetadata(int damage) const override { return damage; }
    std::string getUnlocalizedName(const ItemStack& stack) const override;
    using Item::getUnlocalizedName;
    std::string getItemStackDisplayName(const ItemStack& stack) const override;
    // Turns a SkullOwner name into a profile compound.
    bool updateItemStackNBT(NBTTagCompound& nbt) const override;
};

class ItemFirework : public Item {
public:
    MC_ITEM_CLASS(ItemFirework)
};

class ItemFireworkCharge : public Item {
public:
    MC_ITEM_CLASS(ItemFireworkCharge)

    int getColorFromItemStack(const ItemStack& stack, int renderPass) const override;
    // Null when the stack has no tag or no such key.
    static std::shared_ptr<NBTBase> getExplosionTag(const ItemStack& stack, const std::string& key);
};

class ItemEnchantedBook : public Item {
public:
    MC_ITEM_CLASS(ItemEnchantedBook)

    bool hasEffect(const ItemStack&) const override { return true; }
    bool isItemTool(const ItemStack&) const override { return false; }
    EnumRarity getRarity(const ItemStack& stack) const override;
    // The StoredEnchantments list, a new empty list when absent.
    std::shared_ptr<NBTTagList> getEnchantments(const ItemStack& stack) const;
    // Java: addEnchantment, getEnchantedItemStack, getAll and getRandom -- with enchantments (stage 10).
};

class ItemDye : public Item {
public:
    MC_ITEM_CLASS(ItemDye)

    static constexpr int dyeColors[16] { 1973019, 11743532, 3887386, 5320730, 2437522, 8073150, 2651799, 11250603, 4408131, 14188952, 4312372, 14602026,
        6719955, 12801229, 15435844, 15790320 };

    ItemDye();

    std::string getUnlocalizedName(const ItemStack& stack) const override;
    using Item::getUnlocalizedName;
    void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const override;
};

class ItemCoal : public Item {
public:
    MC_ITEM_CLASS(ItemCoal)

    ItemCoal();

    std::string getUnlocalizedName(const ItemStack& stack) const override;
    using Item::getUnlocalizedName;
    void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const override;
};

class ItemRecord : public Item {
public:
    MC_ITEM_CLASS(ItemRecord)

    explicit ItemRecord(std::string name);

    std::string getRecordNameLocal() const;
    EnumRarity getRarity(const ItemStack&) const override { return EnumRarity::RARE; }
    // By "records.<name>"; null when unknown.
    static ItemRecord* getRecord(const std::string& name);

    const std::string recordName;
};

class ItemSimpleFoiled : public Item {
public:
    MC_ITEM_CLASS(ItemSimpleFoiled)

    bool hasEffect(const ItemStack&) const override { return true; }
};

class ItemExpBottle : public Item {
public:
    MC_ITEM_CLASS(ItemExpBottle)

    ItemExpBottle();

    bool hasEffect(const ItemStack&) const override { return true; }
};

class ItemWritableBook : public Item {
public:
    MC_ITEM_CLASS(ItemWritableBook)

    ItemWritableBook() { setMaxStackSize(1); }

    // Java takes a nullable compound.
    static bool isNBTValid(const NBTTagCompound* nbt);
};

class ItemEditableBook : public Item {
public:
    MC_ITEM_CLASS(ItemEditableBook)

    ItemEditableBook() { setMaxStackSize(1); }

    static bool validBookTagContents(const NBTTagCompound* nbt);
    static int getGeneration(const ItemStack& book);
    std::string getItemStackDisplayName(const ItemStack& stack) const override;
    bool hasEffect(const ItemStack&) const override { return true; }
};

} // namespace mc
