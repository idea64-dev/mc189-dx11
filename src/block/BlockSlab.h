#pragma once

// Original: net/minecraft/block/BlockSlab.java, BlockStoneSlab.java, BlockStoneSlabNew.java, BlockWoodSlab.java
// and their Double/Half subclasses.

#include "block/BlockPlanks.h"
#include "block/BlockSand.h"
#include "init/Blocks.h"
#include "item/Item.h"

namespace mc {

class BlockSlab : public Block {
public:
    enum class EnumBlockHalf { TOP, BOTTOM };
    static const PropertyEnum<EnumBlockHalf> HALF;

    explicit BlockSlab(const Material& materialIn)
        : Block(materialIn)
    {
    }

    bool canSilkHarvest() const override { return false; }
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    bool shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side) override;
    int getDamageValue(World* worldIn, const BlockPos& pos) override;
    void setBlockBoundsForItemRender() override
    {
        if (isDouble())
            setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        else
            setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    }
    bool isOpaqueCube() const override { return isDouble(); }
    int quantityDropped(Random&) const override { return isDouble() ? 2 : 1; }
    bool isFullCube() const override { return isDouble(); }

    virtual std::string getUnlocalizedName(int meta) const = 0;
    using Block::getUnlocalizedName;
    virtual bool isDouble() const = 0;
    virtual const IProperty& getVariantProperty() const = 0;
    // Java: getVariant(ItemStack) comes with the slab items.

protected:
    void construct() override
    {
        Block::construct();
        if (isDouble())
            fullBlock = true;
        else
            setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
        setLightOpacity(255);
    }

    static bool isSlab(const Block* blockIn);
};

MC_ENUM_TRAITS(BlockSlab::EnumBlockHalf, 2, value == BlockSlab::EnumBlockHalf::TOP ? "top" : "bottom")

inline const PropertyEnum<BlockSlab::EnumBlockHalf> BlockSlab::HALF("half");

class BlockStoneSlab : public BlockSlab {
public:
    enum class EnumType { STONE, SAND, WOOD, COBBLESTONE, BRICK, SMOOTHBRICK, NETHERBRICK, QUARTZ };
    struct EnumTypeInfo {
        int meta;
        const MapColor* mapColor;
        std::string_view name;
        std::string_view unlocalizedName;
    };
    static constexpr EnumTypeInfo kTypes[8] {
        { 0, &MapColor::stoneColor, "stone", "stone" },
        { 1, &MapColor::sandColor, "sandstone", "sand" },
        { 2, &MapColor::woodColor, "wood_old", "wood" },
        { 3, &MapColor::stoneColor, "cobblestone", "cobble" },
        { 4, &MapColor::redColor, "brick", "brick" },
        { 5, &MapColor::stoneColor, "stone_brick", "smoothStoneBrick" },
        { 6, &MapColor::netherrackColor, "nether_brick", "netherBrick" },
        { 7, &MapColor::quartzColor, "quartz", "quartz" },
    };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 8 ? 0 : meta); }

    static const PropertyBool SEAMLESS;
    static const PropertyEnum<EnumType> VARIANT;

    BlockStoneSlab()
        : BlockSlab(Material::rock)
    {
    }

    std::string getUnlocalizedName(int meta) const override { return Block::getUnlocalizedName() + "." + std::string(info(byMetadata(meta)).unlocalizedName); }
    const IProperty& getVariantProperty() const override { return VARIANT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const IBlockState* iblockstate = getDefaultState()->withProperty(VARIANT, byMetadata(meta & 7));
        if (isDouble())
            iblockstate = iblockstate->withProperty(SEAMLESS, (meta & 8) != 0);
        else
            iblockstate = iblockstate->withProperty(HALF, (meta & 8) == 0 ? EnumBlockHalf::BOTTOM : EnumBlockHalf::TOP);
        return iblockstate;
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | info(state->getValue(VARIANT)).meta;
        if (isDouble()) {
            if (state->getValue(SEAMLESS))
                i |= 8;
        } else if (state->getValue(HALF) == EnumBlockHalf::TOP) {
            i |= 8;
        }
        return i;
    }
    int damageDropped(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    const MapColor& getMapColor(const IBlockState* state) const override { return *info(state->getValue(VARIANT)).mapColor; }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override
    {
        if (item != Item::getItemFromBlock(Blocks::double_stone_slab))
            addSubBlocks(item, list, { 0, 1, 3, 4, 5, 6, 7 }); // all but WOOD
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(Blocks::stone_slab); }
    Item* getItem(World*, const BlockPos&) override { return Item::getItemFromBlock(Blocks::stone_slab); }

protected:
    void construct() override
    {
        BlockSlab::construct();
        const IBlockState* iblockstate = blockState->getBaseState();
        if (isDouble())
            iblockstate = iblockstate->withProperty(SEAMLESS, false);
        else
            iblockstate = iblockstate->withProperty(HALF, EnumBlockHalf::BOTTOM);
        setDefaultState(iblockstate->withProperty(VARIANT, EnumType::STONE));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override
    {
        return isDouble() ? makeBlockState({ &SEAMLESS, &VARIANT }) : makeBlockState({ &HALF, &VARIANT });
    }
};

class BlockDoubleStoneSlab : public BlockStoneSlab {
public:
    MC_BLOCK_CLASS(BlockDoubleStoneSlab)
    bool isDouble() const override { return true; }
};

class BlockHalfStoneSlab : public BlockStoneSlab {
public:
    MC_BLOCK_CLASS(BlockHalfStoneSlab)
    bool isDouble() const override { return false; }
};

class BlockStoneSlabNew : public BlockSlab {
public:
    enum class EnumType { RED_SANDSTONE };
    static const PropertyBool SEAMLESS;
    static const PropertyEnum<EnumType> VARIANT;

    static EnumType byMetadata(int) { return EnumType::RED_SANDSTONE; }

    BlockStoneSlabNew()
        : BlockSlab(Material::rock)
    {
    }

    std::string getUnlocalizedName(int /*meta*/) const override { return Block::getUnlocalizedName() + ".red_sandstone"; }
    const IProperty& getVariantProperty() const override { return VARIANT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const IBlockState* iblockstate = getDefaultState()->withProperty(VARIANT, byMetadata(meta & 7));
        if (isDouble())
            iblockstate = iblockstate->withProperty(SEAMLESS, (meta & 8) != 0);
        else
            iblockstate = iblockstate->withProperty(HALF, (meta & 8) == 0 ? EnumBlockHalf::BOTTOM : EnumBlockHalf::TOP);
        return iblockstate;
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0; // RED_SANDSTONE's meta
        if (isDouble()) {
            if (state->getValue(SEAMLESS))
                i |= 8;
        } else if (state->getValue(HALF) == EnumBlockHalf::TOP) {
            i |= 8;
        }
        return i;
    }
    const MapColor& getMapColor(const IBlockState*) const override { return *BlockSand::info(BlockSand::EnumType::RED_SAND).mapColor; }
    int damageDropped(const IBlockState*) const override { return 0; }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override
    {
        if (item != Item::getItemFromBlock(Blocks::double_stone_slab2))
            addSubBlocks(item, list, { 0 });
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(Blocks::stone_slab2); }
    Item* getItem(World*, const BlockPos&) override { return Item::getItemFromBlock(Blocks::stone_slab2); }

protected:
    void construct() override
    {
        BlockSlab::construct();
        const IBlockState* iblockstate = blockState->getBaseState();
        if (isDouble())
            iblockstate = iblockstate->withProperty(SEAMLESS, false);
        else
            iblockstate = iblockstate->withProperty(HALF, EnumBlockHalf::BOTTOM);
        setDefaultState(iblockstate->withProperty(VARIANT, EnumType::RED_SANDSTONE));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override
    {
        return isDouble() ? makeBlockState({ &SEAMLESS, &VARIANT }) : makeBlockState({ &HALF, &VARIANT });
    }
};

class BlockDoubleStoneSlabNew : public BlockStoneSlabNew {
public:
    MC_BLOCK_CLASS(BlockDoubleStoneSlabNew)
    bool isDouble() const override { return true; }
};

class BlockHalfStoneSlabNew : public BlockStoneSlabNew {
public:
    MC_BLOCK_CLASS(BlockHalfStoneSlabNew)
    bool isDouble() const override { return false; }
};

class BlockWoodSlab : public BlockSlab {
public:
    static const PropertyEnum<BlockPlanks::EnumType> VARIANT;

    BlockWoodSlab()
        : BlockSlab(Material::wood)
    {
    }

    const MapColor& getMapColor(const IBlockState* state) const override { return BlockPlanks::getMapColor(state->getValue(VARIANT)); }
    std::string getUnlocalizedName(int meta) const override
    {
        return Block::getUnlocalizedName() + "." + std::string(BlockPlanks::info(BlockPlanks::byMetadata(meta)).unlocalizedName);
    }
    const IProperty& getVariantProperty() const override { return VARIANT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const IBlockState* iblockstate = getDefaultState()->withProperty(VARIANT, BlockPlanks::byMetadata(meta & 7));
        if (!isDouble())
            iblockstate = iblockstate->withProperty(HALF, (meta & 8) == 0 ? EnumBlockHalf::BOTTOM : EnumBlockHalf::TOP);
        return iblockstate;
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | BlockPlanks::getMetadata(state->getValue(VARIANT));
        if (!isDouble() && state->getValue(HALF) == EnumBlockHalf::TOP)
            i |= 8;
        return i;
    }
    int damageDropped(const IBlockState* state) const override { return BlockPlanks::getMetadata(state->getValue(VARIANT)); }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override
    {
        if (item != Item::getItemFromBlock(Blocks::double_wooden_slab))
            addSubBlocks(item, list, 0, 6);
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(Blocks::wooden_slab); }
    Item* getItem(World*, const BlockPos&) override { return Item::getItemFromBlock(Blocks::wooden_slab); }

protected:
    void construct() override
    {
        BlockSlab::construct();
        const IBlockState* iblockstate = blockState->getBaseState();
        if (!isDouble())
            iblockstate = iblockstate->withProperty(HALF, EnumBlockHalf::BOTTOM);
        setDefaultState(iblockstate->withProperty(VARIANT, BlockPlanks::EnumType::OAK));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override
    {
        return isDouble() ? makeBlockState({ &VARIANT }) : makeBlockState({ &HALF, &VARIANT });
    }
};

class BlockDoubleWoodSlab : public BlockWoodSlab {
public:
    MC_BLOCK_CLASS(BlockDoubleWoodSlab)
    bool isDouble() const override { return true; }
};

class BlockHalfWoodSlab : public BlockWoodSlab {
public:
    MC_BLOCK_CLASS(BlockHalfWoodSlab)
    bool isDouble() const override { return false; }
};

MC_ENUM_TRAITS(BlockStoneSlab::EnumType, 8, BlockStoneSlab::info(value).name)
MC_ENUM_TRAITS(BlockStoneSlabNew::EnumType, 1, value == BlockStoneSlabNew::EnumType::RED_SANDSTONE ? "red_sandstone" : "")

inline const PropertyBool BlockStoneSlab::SEAMLESS("seamless");
inline const PropertyEnum<BlockStoneSlab::EnumType> BlockStoneSlab::VARIANT("variant");
inline const PropertyBool BlockStoneSlabNew::SEAMLESS("seamless");
inline const PropertyEnum<BlockStoneSlabNew::EnumType> BlockStoneSlabNew::VARIANT("variant");
inline const PropertyEnum<BlockPlanks::EnumType> BlockWoodSlab::VARIANT("variant");

} // namespace mc
