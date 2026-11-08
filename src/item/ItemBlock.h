#pragma once

// Original: net/minecraft/item/ItemBlock.java and its subclasses ItemMultiTexture, ItemColored, ItemCloth,
// ItemSlab, ItemLeaves, ItemPiston, ItemSnow, ItemLilyPad, ItemAnvilBlock, ItemDoublePlant and ItemBanner.
// Not ported yet: ItemBanner.addInformation (tooltips, stage 11).

#include "item/Item.h"

#include <functional>
#include <initializer_list>

namespace mc {

class BlockLeaves;
class BlockSlab;

class ItemBlock : public Item {
public:
    MC_ITEM_CLASS(ItemBlock)

    explicit ItemBlock(Block* blockIn)
        : block(blockIn)
    {
    }

    std::string getUnlocalizedName(const ItemStack& stack) const override;
    std::string getUnlocalizedName() const override;
    const CreativeTabs* getCreativeTab() const override;
    void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const override;
    Block* getBlock() const { return block; }
    // Places the block against the clicked side (or into a replaceable block).
    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
    // Copies the stack's BlockEntityTag into the placed tile entity; true if that changed it.
    static bool setTileEntityNBT(World* worldIn, EntityPlayer* player, const BlockPos& pos, ItemStack& stack);
    // Java: canPlaceBlockOnSide -- the client's check before it sends a placement.
    virtual bool canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side, EntityPlayer* player, ItemStack* stack);

protected:
    Block* const block;
};

class ItemMultiTexture : public ItemBlock {
public:
    MC_ITEM_CLASS(ItemMultiTexture)

    using NameFunction = std::function<std::string(const ItemStack&)>;

    ItemMultiTexture(Block* blockIn, Block* block2, NameFunction nameFunctionIn)
        : ItemBlock(blockIn)
        , theBlock(block2)
        , nameFunction(std::move(nameFunctionIn))
    {
        setMaxDamage(0);
        setHasSubtypes(true);
    }

    // Names by metadata, entry 0 for out-of-range metadata.
    ItemMultiTexture(Block* blockIn, Block* block2, std::vector<std::string> namesByMeta);

    int getMetadata(int damage) const override { return damage; }
    std::string getUnlocalizedName(const ItemStack& stack) const override;
    using ItemBlock::getUnlocalizedName;

protected:
    Block* const theBlock;
    const NameFunction nameFunction;
};

class ItemColored : public ItemBlock {
public:
    MC_ITEM_CLASS(ItemColored)

    ItemColored(Block* blockIn, bool hasSubtypesIn)
        : ItemBlock(blockIn)
        , coloredBlock(blockIn)
    {
        if (hasSubtypesIn) {
            setMaxDamage(0);
            setHasSubtypes(true);
        }
    }

    int getColorFromItemStack(const ItemStack& stack, int renderPass) const override;
    int getMetadata(int damage) const override { return damage; }
    ItemColored* setSubtypeNames(std::vector<std::string> names)
    {
        subtypeNames = std::move(names);
        hasSubtypeNames = true;
        return this;
    }
    std::string getUnlocalizedName(const ItemStack& stack) const override;
    using ItemBlock::getUnlocalizedName;

private:
    Block* const coloredBlock;
    std::vector<std::string> subtypeNames;
    bool hasSubtypeNames = false;
};

class ItemCloth : public ItemBlock {
public:
    MC_ITEM_CLASS(ItemCloth)

    explicit ItemCloth(Block* blockIn)
        : ItemBlock(blockIn)
    {
        setMaxDamage(0);
        setHasSubtypes(true);
    }

    int getMetadata(int damage) const override { return damage; }
    std::string getUnlocalizedName(const ItemStack& stack) const override;
    using ItemBlock::getUnlocalizedName;
};

class ItemSlab : public ItemBlock {
public:
    MC_ITEM_CLASS(ItemSlab)

    ItemSlab(Block* blockIn, BlockSlab* singleSlabIn, BlockSlab* doubleSlabIn)
        : ItemBlock(blockIn)
        , singleSlab(singleSlabIn)
        , doubleSlab(doubleSlabIn)
    {
        setMaxDamage(0);
        setHasSubtypes(true);
    }

    int getMetadata(int damage) const override { return damage; }
    std::string getUnlocalizedName(const ItemStack& stack) const override;
    using ItemBlock::getUnlocalizedName;
    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
    bool canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side, EntityPlayer* player, ItemStack* stack) override;

private:
    // Java: singleSlab.getVariant(stack) -- here the raw value of the variant property for the stack's metadata.
    int getVariant(const ItemStack& stack) const;
    bool tryPlace(ItemStack& stack, World* worldIn, const BlockPos& pos, int variantInStack);
    // Turns the slab at pos into the double slab of that variant.
    bool placeDouble(ItemStack& stack, World* worldIn, const BlockPos& pos, int variant);

    BlockSlab* const singleSlab;
    BlockSlab* const doubleSlab;
};

class ItemLeaves : public ItemBlock {
public:
    MC_ITEM_CLASS(ItemLeaves)

    explicit ItemLeaves(BlockLeaves* blockIn);

    int getMetadata(int damage) const override { return damage | 4; }
    int getColorFromItemStack(const ItemStack& stack, int renderPass) const override;
    std::string getUnlocalizedName(const ItemStack& stack) const override;
    using ItemBlock::getUnlocalizedName;

private:
    BlockLeaves* const leaves;
};

class ItemPiston : public ItemBlock {
public:
    MC_ITEM_CLASS(ItemPiston)

    explicit ItemPiston(Block* blockIn)
        : ItemBlock(blockIn)
    {
    }

    int getMetadata(int) const override { return 7; }
};

class ItemSnow : public ItemBlock {
public:
    MC_ITEM_CLASS(ItemSnow)

    explicit ItemSnow(Block* blockIn)
        : ItemBlock(blockIn)
    {
        setMaxDamage(0);
        setHasSubtypes(true);
    }

    int getMetadata(int damage) const override { return damage; }
    // Adds a layer to the snow there, else places snow.
    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
};

class ItemLilyPad : public ItemColored {
public:
    MC_ITEM_CLASS(ItemLilyPad)

    explicit ItemLilyPad(Block* blockIn)
        : ItemColored(blockIn, false)
    {
    }

    int getColorFromItemStack(const ItemStack& stack, int renderPass) const override;
    // Places the lily pad on the still water looked at.
    std::shared_ptr<ItemStack> onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn) override;
};

class ItemAnvilBlock : public ItemMultiTexture {
public:
    MC_ITEM_CLASS(ItemAnvilBlock)

    explicit ItemAnvilBlock(Block* blockIn)
        : ItemMultiTexture(blockIn, blockIn, std::vector<std::string> { "intact", "slightlyDamaged", "veryDamaged" })
    {
    }

    int getMetadata(int damage) const override { return damage << 2; }
};

class ItemDoublePlant : public ItemMultiTexture {
public:
    MC_ITEM_CLASS(ItemDoublePlant)

    ItemDoublePlant(Block* blockIn, Block* block2, NameFunction nameFunctionIn)
        : ItemMultiTexture(blockIn, block2, std::move(nameFunctionIn))
    {
    }

    int getColorFromItemStack(const ItemStack& stack, int renderPass) const override;
};

class ItemBanner : public ItemBlock {
public:
    MC_ITEM_CLASS(ItemBanner)

    ItemBanner();

    bool onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY,
        float hitZ) override;
    std::string getItemStackDisplayName(const ItemStack& stack) const override;
    int getColorFromItemStack(const ItemStack& stack, int renderPass) const override;
    void getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const override;
    const CreativeTabs* getCreativeTab() const override;
};

} // namespace mc
