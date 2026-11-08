#pragma once

// Original: the remaining classes of net/minecraft/item -- ItemBow, ItemFishingRod, ItemFlintAndSteel,
// ItemCarrotOnAStick, ItemReed, ItemDoor, ItemMinecart, ItemBucket, ItemBucketMilk, ItemHangingEntity, ItemSign,
// ItemBed, ItemSaddle, ItemRedstone, ItemSnowball, ItemBoat, ItemBook, ItemEgg, ItemMapBase, ItemMap,
// ItemEmptyMap, ItemEnderPearl, ItemEnderEye, ItemGlassBottle, ItemFireball, ItemLead, ItemNameTag,
// ItemArmorStand, ItemMonsterPlacer, ItemSkull, ItemFirework, ItemFireworkCharge, ItemEnchantedBook, ItemDye,
// ItemCoal, ItemRecord, ItemSimpleFoiled, ItemExpBottle, ItemEditableBook and ItemWritableBook.
// Their world callbacks are ported; the entities they would spawn (arrows, throwables, minecarts, boats, paintings,
// armor stands, mobs, fishing hooks, fireworks) come with stage 10 and are marked where they are created -- the
// checks, sounds, stack changes and Random draws around them are kept. Not ported yet: maps (ItemMap,
// ItemEmptyMap, stage 12), book screens and contents (ItemWritableBook, ItemEditableBook, stages 11 and 12),
// the mob items of saddles, name tags and dyes on sheep (stage 10), and tooltips (stage 11).

#include "item/Item.h"
#include "util/WeightedRandomChestContent.h"

#include <memory>

namespace mc {

struct BlockPos;
class EntityPlayer;
class Enchantment;
class NBTBase;
class NBTTagList;
class World;
struct EnchantmentData;
enum class EnumFacing;

class ItemBow : public Item {
public:
    MC_ITEM_CLASS(ItemBow)

    static constexpr const char* bowPullIconNameArray[3] { "pulling_0", "pulling_1", "pulling_2" };

    ItemBow();

    int getMaxItemUseDuration(const ItemStack&) const override { return 72000; }
    EnumAction getItemUseAction(const ItemStack&) const override { return EnumAction::BOW; }
    int getItemEnchantability() const override { return 1; }
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
    void onPlayerStoppedUsing(ItemStack& stack, World* worldIn, EntityPlayer* playerIn, int timeLeft) override;
};

class ItemFishingRod : public Item {
public:
    MC_ITEM_CLASS(ItemFishingRod)

    ItemFishingRod();

    bool isFull3D() const override { return true; }
    bool shouldRotateAroundWhenRendering() const override { return true; }
    int getItemEnchantability() const override { return 1; }
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
};

class ItemFlintAndSteel : public Item {
public:
    MC_ITEM_CLASS(ItemFlintAndSteel)

    ItemFlintAndSteel();

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
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

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;

private:
    Block* block;
};

class ItemDoor : public Item {
public:
    MC_ITEM_CLASS(ItemDoor)

    explicit ItemDoor(Block* blockIn);

    // Places both halves with flags 2 and notifies the neighbours; the hinge follows the neighbouring doors
    // and solid blocks.
    static void placeDoor(World* worldIn, const BlockPos& pos, EnumFacing facing, Block* door);
    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;

private:
    Block* block;
};

class ItemMinecart : public Item {
public:
    MC_ITEM_CLASS(ItemMinecart)

    // Java: EntityMinecart.EnumMinecartType, until the minecart entities exist.
    enum class EnumMinecartType { RIDEABLE, CHEST, FURNACE, TNT, SPAWNER, HOPPER, COMMAND_BLOCK };

    explicit ItemMinecart(EnumMinecartType type);

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;

    const EnumMinecartType minecartType;
};

class ItemBucket : public Item {
public:
    MC_ITEM_CLASS(ItemBucket)

    explicit ItemBucket(Block* containedBlock);

    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
    // Pours the contained liquid at pos; false when the bucket is empty or the place is taken.
    bool tryPlaceContainedLiquid(World* worldIn, const BlockPos& pos);

private:
    std::shared_ptr<ItemStack> fillBucket(const std::shared_ptr<ItemStack>& emptyBuckets, EntityPlayer* player, Item* fullBucket);

    Block* isFull;
};

class ItemBucketMilk : public Item {
public:
    MC_ITEM_CLASS(ItemBucketMilk)

    ItemBucketMilk();

    int getMaxItemUseDuration(const ItemStack&) const override { return 32; }
    EnumAction getItemUseAction(const ItemStack&) const override { return EnumAction::DRINK; }
    std::shared_ptr<ItemStack> onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn) override;
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
};

class ItemHangingEntity : public Item {
public:
    MC_ITEM_CLASS(ItemHangingEntity)

    // Java: the EntityHanging subclass to create.
    enum class HangingType { PAINTING, ITEM_FRAME };

    explicit ItemHangingEntity(HangingType type);

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;

private:
    [[maybe_unused]] HangingType hangingEntityClass; // the entity to create (stage 10)
};

class ItemSign : public Item {
public:
    MC_ITEM_CLASS(ItemSign)

    ItemSign();

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
};

class ItemBed : public Item {
public:
    MC_ITEM_CLASS(ItemBed)

    ItemBed();

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
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

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
};

class ItemSnowball : public Item {
public:
    MC_ITEM_CLASS(ItemSnowball)

    ItemSnowball();

    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
};

class ItemBoat : public Item {
public:
    MC_ITEM_CLASS(ItemBoat)

    ItemBoat();

    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
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

    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
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

    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
};

class ItemEnderEye : public Item {
public:
    MC_ITEM_CLASS(ItemEnderEye)

    ItemEnderEye();

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
};

class ItemGlassBottle : public Item {
public:
    MC_ITEM_CLASS(ItemGlassBottle)

    ItemGlassBottle();

    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
};

class ItemFireball : public Item {
public:
    MC_ITEM_CLASS(ItemFireball)

    ItemFireball();

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
};

class ItemLead : public Item {
public:
    MC_ITEM_CLASS(ItemLead)

    ItemLead();

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
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

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
};

class ItemMonsterPlacer : public Item {
public:
    MC_ITEM_CLASS(ItemMonsterPlacer)

    ItemMonsterPlacer();

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;

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
    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
};

class ItemFirework : public Item {
public:
    MC_ITEM_CLASS(ItemFirework)

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
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
    // Java: getRandom -- a random enchanted book (EnchantmentHelper.addRandomEnchantment at level 30) as a loot
    // entry.
    WeightedRandomChestContent getRandom(Random& rand) const;
    WeightedRandomChestContent getRandom(Random& rand, int minChance, int maxChance, int weight) const;
    // Stores the enchantment, raising the level of one already stored.
    void addEnchantment(ItemStack& stack, const EnchantmentData& enchantment) const;
    ItemStack getEnchantedItemStack(const EnchantmentData& data) const;
    // A book for every level of the enchantment.
    void getAll(const Enchantment* enchantment, std::vector<ItemStack>& list) const;
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
    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
    // Grows the IGrowable block at target (server side); true if it could grow.
    static bool applyBonemeal(ItemStack& stack, World* worldIn, const BlockPos& target);
    // The client effect 2005: green sparkles over the block (draws from itemRand).
    static void spawnBonemealParticles(World* worldIn, const BlockPos& pos, int amount);
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
    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
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
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
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
