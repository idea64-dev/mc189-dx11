#pragma once

// Original: net/minecraft/block/BlockLeavesBase.java, BlockLeaves.java, BlockOldLeaf.java and BlockNewLeaf.java

#include "block/BlockPlanks.h"
#include "util/Random.h"
#include "world/Colorizer.h"

namespace mc {

class BlockLeavesBase : public Block {
public:
    BlockLeavesBase(const Material& materialIn, bool fancyGraphicsIn)
        : Block(materialIn)
        , fancyGraphicsArg(fancyGraphicsIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side) override;

protected:
    void construct() override
    {
        Block::construct();
        // Assigned after Block's constructor, which already asked the overridden isOpaqueCube() -- with false here.
        fancyGraphics = fancyGraphicsArg;
    }

    bool fancyGraphics = false;

private:
    bool fancyGraphicsArg;
};

class BlockLeaves : public BlockLeavesBase {
public:
    static const PropertyBool DECAYABLE;
    static const PropertyBool CHECK_DECAY;

    BlockLeaves()
        : BlockLeavesBase(Material::leaves, false)
    {
    }

    int getBlockColor() const override { return ColorizerFoliage::getFoliageColor(0.5, 1.0); }
    int getRenderColor(const IBlockState*) const override { return ColorizerFoliage::getFoliageColorBasic(); }
    int quantityDropped(Random& random) const override { return random.nextInt(20) == 0 ? 1 : 0; }
    bool isOpaqueCube() const override { return !fancyGraphics; }
    EnumWorldBlockLayer getBlockLayer() const override { return isTransparent ? EnumWorldBlockLayer::CUTOUT_MIPPED : EnumWorldBlockLayer::SOLID; }
    bool isVisuallyOpaque() const override { return false; }
    virtual BlockPlanks::EnumType getWoodType(int meta) const = 0;
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(Blocks::sapling); }
    int colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    void setGraphicsLevel(bool fancy)
    {
        isTransparent = fancy;
        fancyGraphics = fancy;
        iconIndex = fancy ? 0 : 1;
    }

protected:
    virtual void dropApple(World*, const BlockPos&, const IBlockState*, int) { }
    virtual int getSaplingDropChance(const IBlockState*) const { return 20; }

    void construct() override
    {
        BlockLeavesBase::construct();
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabDecorations);
        setHardness(0.2f);
        setLightOpacity(1);
        setStepSound(soundTypeGrass);
    }

    int iconIndex = 0;
    bool isTransparent = false;

private:
    void destroy(World* worldIn, const BlockPos& pos);

    std::vector<int> surroundings;
};

class BlockOldLeaf : public BlockLeaves {
public:
    MC_BLOCK_CLASS(BlockOldLeaf)

    static const PropertyEnum<BlockPlanks::EnumType> VARIANT;

    int getRenderColor(const IBlockState* state) const override
    {
        if (state->getBlock() != this)
            return BlockLeaves::getRenderColor(state);
        const BlockPlanks::EnumType type = state->getValue(VARIANT);
        return type == BlockPlanks::EnumType::SPRUCE ? ColorizerFoliage::getFoliageColorPine()
                                                      : (type == BlockPlanks::EnumType::BIRCH ? ColorizerFoliage::getFoliageColorBirch() : BlockLeaves::getRenderColor(state));
    }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(VARIANT, getWoodType(meta))->withProperty(DECAYABLE, (meta & 4) == 0)->withProperty(CHECK_DECAY, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | BlockPlanks::getMetadata(state->getValue(VARIANT));
        if (!state->getValue(DECAYABLE))
            i |= 4;
        if (state->getValue(CHECK_DECAY))
            i |= 8;
        return i;
    }
    BlockPlanks::EnumType getWoodType(int meta) const override { return BlockPlanks::byMetadata((meta & 3) % 4); }
    int damageDropped(const IBlockState* state) const override { return BlockPlanks::getMetadata(state->getValue(VARIANT)); }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, { 0, 1, 2, 3 }); }
    ItemStack createStackedBlock(const IBlockState* state) const override { return ItemStack(Item::getItemFromBlock(this), 1, BlockPlanks::getMetadata(state->getValue(VARIANT))); }
    int colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass) override;
    // Java: harvestBlock drops the leaves themselves for shears -- the player comes with stage 8.

protected:
    void dropApple(World* worldIn, const BlockPos& pos, const IBlockState* state, int chance) override;
    int getSaplingDropChance(const IBlockState* state) const override { return state->getValue(VARIANT) == BlockPlanks::EnumType::JUNGLE ? 40 : 20; }
    void construct() override
    {
        BlockLeaves::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, BlockPlanks::EnumType::OAK)->withProperty(CHECK_DECAY, true)->withProperty(DECAYABLE, true));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT, &CHECK_DECAY, &DECAYABLE }); }
};

class BlockNewLeaf : public BlockLeaves {
public:
    MC_BLOCK_CLASS(BlockNewLeaf)

    static const PropertyEnum<BlockPlanks::EnumType> VARIANT;

    int damageDropped(const IBlockState* state) const override { return BlockPlanks::getMetadata(state->getValue(VARIANT)); }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(VARIANT, getWoodType(meta))->withProperty(DECAYABLE, (meta & 4) == 0)->withProperty(CHECK_DECAY, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | (BlockPlanks::getMetadata(state->getValue(VARIANT)) - 4);
        if (!state->getValue(DECAYABLE))
            i |= 4;
        if (state->getValue(CHECK_DECAY))
            i |= 8;
        return i;
    }
    BlockPlanks::EnumType getWoodType(int meta) const override { return BlockPlanks::byMetadata((meta & 3) + 4); }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, { 0, 1 }); }
    ItemStack createStackedBlock(const IBlockState* state) const override { return ItemStack(Item::getItemFromBlock(this), 1, BlockPlanks::getMetadata(state->getValue(VARIANT)) - 4); }
    int getDamageValue(World* worldIn, const BlockPos& pos) override;
    // Java: harvestBlock drops the leaves themselves for shears -- the player comes with stage 8.

protected:
    void dropApple(World* worldIn, const BlockPos& pos, const IBlockState* state, int chance) override;
    void construct() override
    {
        BlockLeaves::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, BlockPlanks::EnumType::ACACIA)->withProperty(CHECK_DECAY, true)->withProperty(DECAYABLE, true));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT, &CHECK_DECAY, &DECAYABLE }); }
};

inline const PropertyBool BlockLeaves::DECAYABLE("decayable");
inline const PropertyBool BlockLeaves::CHECK_DECAY("check_decay");
inline const PropertyEnum<BlockPlanks::EnumType> BlockOldLeaf::VARIANT("variant", [](BlockPlanks::EnumType t) { return BlockPlanks::getMetadata(t) < 4; });
inline const PropertyEnum<BlockPlanks::EnumType> BlockNewLeaf::VARIANT("variant", [](BlockPlanks::EnumType t) { return BlockPlanks::getMetadata(t) >= 4; });

} // namespace mc
