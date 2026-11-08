#pragma once

// Original: net/minecraft/block/BlockPistonBase.java, BlockPistonExtension.java and BlockPistonMoving.java
// Pushing comes with the world (stage 9).

#include "block/BlockContainer.h"

#include <optional>

namespace mc {

class BlockPistonBase : public Block {
public:
    MC_BLOCK_CLASS(BlockPistonBase)

    static const PropertyDirection FACING;
    static const PropertyBool EXTENDED;

    explicit BlockPistonBase(bool isStickyIn)
        : Block(Material::piston)
        , isStickyArg(isStickyIn)
    {
    }

    // Java returns null for 6 and 7.
    static std::optional<EnumFacing> getFacing(int meta)
    {
        const int i = meta & 7;
        return i > 5 ? std::nullopt : std::optional<EnumFacing>(getFront(i));
    }

    bool isOpaqueCube() const override { return false; }
    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f); }
    bool isFullCube() const override { return false; }
    const IBlockState* getStateForEntityRender(const IBlockState*) const override { return getDefaultState()->withProperty(FACING, EnumFacing::UP); }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return withFacing(getDefaultState(), getFacing(meta))->withProperty(EXTENDED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getIndex(state->getValue(FACING));
        if (state->getValue(EXTENDED))
            i |= 8;
        return i;
    }

    // withProperty(FACING, null) throws IllegalArgumentException in Java.
    static const IBlockState* withFacing(const IBlockState* state, std::optional<EnumFacing> facing)
    {
        if (!facing)
            throw std::invalid_argument("Cannot set property facing to null");
        return state->withProperty(FACING, *facing);
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(EXTENDED, false));
        isSticky = isStickyArg;
        setStepSound(soundTypePiston);
        setHardness(0.5f);
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &EXTENDED }); }

private:
    bool isSticky = false;
    bool isStickyArg;
};

class BlockPistonExtension : public Block {
public:
    MC_BLOCK_CLASS(BlockPistonExtension)

    enum class EnumPistonType { DEFAULT, STICKY };
    static const PropertyDirection FACING;
    static const PropertyEnum<EnumPistonType> TYPE;
    static const PropertyBool SHORT;

    BlockPistonExtension()
        : Block(Material::piston)
    {
    }

    static std::optional<EnumFacing> getFacing(int meta) { return BlockPistonBase::getFacing(meta); }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    int quantityDropped(Random&) const override { return 0; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const auto facing = getFacing(meta);
        if (!facing)
            throw std::invalid_argument("Cannot set property facing to null");
        return getDefaultState()->withProperty(FACING, *facing)->withProperty(TYPE, (meta & 8) > 0 ? EnumPistonType::STICKY : EnumPistonType::DEFAULT);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getIndex(state->getValue(FACING));
        if (state->getValue(TYPE) == EnumPistonType::STICKY)
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(TYPE, EnumPistonType::DEFAULT)->withProperty(SHORT, false));
        setStepSound(soundTypePiston);
        setHardness(0.5f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &TYPE, &SHORT }); }
};

class BlockPistonMoving : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockPistonMoving)

    static inline const PropertyDirection& FACING = BlockPistonExtension::FACING;
    static inline const PropertyEnum<BlockPistonExtension::EnumPistonType>& TYPE = BlockPistonExtension::TYPE;

    BlockPistonMoving()
        : BlockContainer(Material::piston)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const auto facing = BlockPistonExtension::getFacing(meta);
        if (!facing)
            throw std::invalid_argument("Cannot set property facing to null");
        return getDefaultState()->withProperty(FACING, *facing)->withProperty(TYPE, (meta & 8) > 0 ? BlockPistonExtension::EnumPistonType::STICKY : BlockPistonExtension::EnumPistonType::DEFAULT);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getIndex(state->getValue(FACING));
        if (state->getValue(TYPE) == BlockPistonExtension::EnumPistonType::STICKY)
            i |= 8;
        return i;
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(TYPE, BlockPistonExtension::EnumPistonType::DEFAULT));
        setHardness(-1.0f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &TYPE }); }
};

MC_ENUM_TRAITS(BlockPistonExtension::EnumPistonType, 2, value == BlockPistonExtension::EnumPistonType::DEFAULT ? "normal" : "sticky")

inline const PropertyDirection BlockPistonBase::FACING("facing");
inline const PropertyBool BlockPistonBase::EXTENDED("extended");
inline const PropertyDirection BlockPistonExtension::FACING("facing");
inline const PropertyEnum<BlockPistonExtension::EnumPistonType> BlockPistonExtension::TYPE("type");
inline const PropertyBool BlockPistonExtension::SHORT("short");

} // namespace mc
