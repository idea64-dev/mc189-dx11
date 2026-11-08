#pragma once

// Original: net/minecraft/block/BlockRotatedPillar.java, BlockHay.java, BlockLog.java, BlockOldLog.java and
// BlockNewLog.java

#include "block/BlockPlanks.h"

namespace mc {

class BlockRotatedPillar : public Block {
public:
    static const PropertyEnum<Axis> AXIS;

    explicit BlockRotatedPillar(const Material& materialIn)
        : Block(materialIn, materialIn.getMaterialMapColor())
    {
    }
    BlockRotatedPillar(const Material& materialIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
    {
    }
};

class BlockHay : public BlockRotatedPillar {
public:
    MC_BLOCK_CLASS(BlockHay)

    BlockHay()
        : BlockRotatedPillar(Material::grass, MapColor::yellowColor)
    {
    }

    const IBlockState* getStateFromMeta(int meta) const override
    {
        Axis axis = Axis::Y;
        const int i = meta & 12;
        if (i == 4)
            axis = Axis::X;
        else if (i == 8)
            axis = Axis::Z;
        return getDefaultState()->withProperty(AXIS, axis);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        const Axis axis = state->getValue(AXIS);
        if (axis == Axis::X)
            i |= 4;
        else if (axis == Axis::Z)
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        BlockRotatedPillar::construct();
        setDefaultState(blockState->getBaseState()->withProperty(AXIS, Axis::Y));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AXIS }); }
};

class BlockLog : public BlockRotatedPillar {
public:
    enum class EnumAxis { X, Y, Z, NONE };
    static const PropertyEnum<EnumAxis> LOG_AXIS;

    static EnumAxis fromFacingAxis(Axis axis)
    {
        switch (axis) {
        case Axis::X: return EnumAxis::X;
        case Axis::Y: return EnumAxis::Y;
        case Axis::Z: return EnumAxis::Z;
        }
        return EnumAxis::NONE;
    }

    BlockLog()
        : BlockRotatedPillar(Material::wood)
    {
    }

protected:
    void construct() override
    {
        BlockRotatedPillar::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
        setHardness(2.0f);
        setStepSound(soundTypeWood);
    }

    // Java's getStateFromMeta/getMetaFromState axis bits, shared by both logs.
    static EnumAxis axisFromMeta(int meta)
    {
        switch (meta & 12) {
        case 0: return EnumAxis::Y;
        case 4: return EnumAxis::X;
        case 8: return EnumAxis::Z;
        default: return EnumAxis::NONE;
        }
    }
    static int axisMeta(EnumAxis axis)
    {
        switch (axis) {
        case EnumAxis::X: return 4;
        case EnumAxis::Z: return 8;
        case EnumAxis::NONE: return 12;
        default: return 0;
        }
    }
};

class BlockOldLog : public BlockLog {
public:
    MC_BLOCK_CLASS(BlockOldLog)

    static const PropertyEnum<BlockPlanks::EnumType> VARIANT;

    const MapColor& getMapColor(const IBlockState* state) const override
    {
        using Type = BlockPlanks::EnumType;
        const Type type = state->getValue(VARIANT);
        if (state->getValue(LOG_AXIS) == EnumAxis::Y)
            return BlockPlanks::getMapColor(type);
        switch (type) {
        case Type::SPRUCE: return BlockPlanks::getMapColor(Type::DARK_OAK);
        case Type::BIRCH: return MapColor::quartzColor;
        case Type::JUNGLE: return BlockPlanks::getMapColor(Type::SPRUCE);
        default: return BlockPlanks::getMapColor(Type::SPRUCE);
        }
    }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(VARIANT, BlockPlanks::byMetadata((meta & 3) % 4))->withProperty(LOG_AXIS, axisFromMeta(meta));
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        return BlockPlanks::getMetadata(state->getValue(VARIANT)) | axisMeta(state->getValue(LOG_AXIS));
    }
    int damageDropped(const IBlockState* state) const override { return BlockPlanks::getMetadata(state->getValue(VARIANT)); }

protected:
    void construct() override
    {
        BlockLog::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, BlockPlanks::EnumType::OAK)->withProperty(LOG_AXIS, EnumAxis::Y));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT, &LOG_AXIS }); }
};

class BlockNewLog : public BlockLog {
public:
    MC_BLOCK_CLASS(BlockNewLog)

    static const PropertyEnum<BlockPlanks::EnumType> VARIANT;

    const MapColor& getMapColor(const IBlockState* state) const override
    {
        using Type = BlockPlanks::EnumType;
        const Type type = state->getValue(VARIANT);
        if (state->getValue(LOG_AXIS) == EnumAxis::Y)
            return BlockPlanks::getMapColor(type);
        return type == Type::DARK_OAK ? BlockPlanks::getMapColor(Type::DARK_OAK) : MapColor::stoneColor;
    }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        // byMetadata of 6 or 7 falls back to OAK, which this property doesn't allow: Java throws, like this.
        const IBlockState* state = getDefaultState()->withProperty(VARIANT, BlockPlanks::byMetadata((meta & 3) + 4));
        return state->withProperty(LOG_AXIS, axisFromMeta(meta));
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        return (BlockPlanks::getMetadata(state->getValue(VARIANT)) - 4) | axisMeta(state->getValue(LOG_AXIS));
    }
    int damageDropped(const IBlockState* state) const override { return BlockPlanks::getMetadata(state->getValue(VARIANT)) - 4; }

protected:
    void construct() override
    {
        BlockLog::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, BlockPlanks::EnumType::ACACIA)->withProperty(LOG_AXIS, EnumAxis::Y));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT, &LOG_AXIS }); }
};

MC_ENUM_TRAITS(BlockLog::EnumAxis, 4, value == BlockLog::EnumAxis::X ? "x" : value == BlockLog::EnumAxis::Y ? "y" : value == BlockLog::EnumAxis::Z ? "z" : "none")

inline const PropertyEnum<Axis> BlockRotatedPillar::AXIS("axis");
inline const PropertyEnum<BlockLog::EnumAxis> BlockLog::LOG_AXIS("axis");
inline const PropertyEnum<BlockPlanks::EnumType> BlockOldLog::VARIANT("variant", [](BlockPlanks::EnumType t) { return BlockPlanks::getMetadata(t) < 4; });
inline const PropertyEnum<BlockPlanks::EnumType> BlockNewLog::VARIANT("variant", [](BlockPlanks::EnumType t) { return BlockPlanks::getMetadata(t) >= 4; });

} // namespace mc
