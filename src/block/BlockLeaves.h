#pragma once

// Original: net/minecraft/block/BlockLeavesBase.java, BlockLeaves.java, BlockOldLeaf.java and BlockNewLeaf.java
// Decay comes with world ticks (stage 9); fancy graphics switching with the world renderer (stage 7).

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

protected:
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

protected:
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

protected:
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
