#pragma once

// Original: net/minecraft/block/BlockDoublePlant.java

#include "block/BlockBush.h"
#include "block/BlockDirectional.h"

namespace mc {

class BlockDoublePlant : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockDoublePlant)

    enum class EnumBlockHalf { UPPER, LOWER };
    enum class EnumPlantType { SUNFLOWER, SYRINGA, GRASS, FERN, ROSE, PAEONIA };
    struct EnumPlantTypeInfo {
        int meta;
        std::string_view name;
        std::string_view unlocalizedName;
    };
    static constexpr EnumPlantTypeInfo kPlantTypes[6] {
        { 0, "sunflower", "sunflower" },
        { 1, "syringa", "syringa" },
        { 2, "double_grass", "grass" },
        { 3, "double_fern", "fern" },
        { 4, "double_rose", "rose" },
        { 5, "paeonia", "paeonia" },
    };
    static const EnumPlantTypeInfo& info(EnumPlantType type) { return kPlantTypes[static_cast<int>(type)]; }
    static EnumPlantType byMetadata(int meta) { return static_cast<EnumPlantType>(meta < 0 || meta >= 6 ? 0 : meta); }

    static const PropertyEnum<EnumPlantType> VARIANT;
    static const PropertyEnum<EnumBlockHalf> HALF;
    static inline const PropertyDirection& FACING = BlockDirectional::FACING;

    BlockDoublePlant()
        : BlockBush(Material::vine)
    {
    }

    int damageDropped(const IBlockState* state) const override
    {
        return state->getValue(HALF) != EnumBlockHalf::UPPER && state->getValue(VARIANT) != EnumPlantType::GRASS ? info(state->getValue(VARIANT)).meta : 0;
    }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return (meta & 8) > 0 ? getDefaultState()->withProperty(HALF, EnumBlockHalf::UPPER)
                              : getDefaultState()->withProperty(HALF, EnumBlockHalf::LOWER)->withProperty(VARIANT, byMetadata(meta & 7));
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        return state->getValue(HALF) == EnumBlockHalf::UPPER ? 8 | getHorizontalIndex(state->getValue(FACING)) : info(state->getValue(VARIANT)).meta;
    }
    EnumOffsetType getOffsetType() const override { return EnumOffsetType::XZ; }

protected:
    void construct() override
    {
        BlockBush::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, EnumPlantType::SUNFLOWER)->withProperty(HALF, EnumBlockHalf::LOWER)->withProperty(FACING, EnumFacing::NORTH));
        setHardness(0.0f);
        setStepSound(soundTypeGrass);
        setUnlocalizedName("doublePlant");
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &HALF, &VARIANT, &FACING }); }
};

MC_ENUM_TRAITS(BlockDoublePlant::EnumBlockHalf, 2, value == BlockDoublePlant::EnumBlockHalf::UPPER ? "upper" : "lower")
MC_ENUM_TRAITS(BlockDoublePlant::EnumPlantType, 6, BlockDoublePlant::info(value).name)

inline const PropertyEnum<BlockDoublePlant::EnumPlantType> BlockDoublePlant::VARIANT("variant");
inline const PropertyEnum<BlockDoublePlant::EnumBlockHalf> BlockDoublePlant::HALF("half");

} // namespace mc
