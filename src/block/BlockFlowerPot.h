#pragma once

// Original: net/minecraft/block/BlockFlowerPot.java (the plant comes from the tile entity, stage 9)

#include "block/BlockContainer.h"

namespace mc {

class BlockFlowerPot : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockFlowerPot)

    enum class EnumFlowerType {
        EMPTY, POPPY, BLUE_ORCHID, ALLIUM, HOUSTONIA, RED_TULIP, ORANGE_TULIP, WHITE_TULIP, PINK_TULIP, OXEYE_DAISY, DANDELION,
        OAK_SAPLING, SPRUCE_SAPLING, BIRCH_SAPLING, JUNGLE_SAPLING, ACACIA_SAPLING, DARK_OAK_SAPLING, MUSHROOM_RED, MUSHROOM_BROWN,
        DEAD_BUSH, FERN, CACTUS
    };
    static constexpr std::string_view kFlowerTypeNames[22] { "empty", "rose", "blue_orchid", "allium", "houstonia", "red_tulip",
        "orange_tulip", "white_tulip", "pink_tulip", "oxeye_daisy", "dandelion", "oak_sapling", "spruce_sapling", "birch_sapling",
        "jungle_sapling", "acacia_sapling", "dark_oak_sapling", "mushroom_red", "mushroom_brown", "dead_bush", "fern", "cactus" };

    static const PropertyInteger LEGACY_DATA;
    static const PropertyEnum<EnumFlowerType> CONTENTS;

    BlockFlowerPot()
        : BlockContainer(Material::circuits)
    {
    }

    void setBlockBoundsForItemRender() override
    {
        const float f = 0.375f;
        const float f1 = f / 2.0f;
        setBlockBounds(0.5f - f1, 0.0f, 0.5f - f1, 0.5f + f1, f, 0.5f + f1);
    }
    bool isOpaqueCube() const override { return false; }
    int getRenderType() const override { return 3; }
    bool isFullCube() const override { return false; }
    bool isFlowerPot() const override { return true; }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(LEGACY_DATA); }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::flower_pot; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(CONTENTS, EnumFlowerType::EMPTY)->withProperty(LEGACY_DATA, 0));
        setBlockBoundsForItemRender();
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &CONTENTS, &LEGACY_DATA }); }
};

MC_ENUM_TRAITS(BlockFlowerPot::EnumFlowerType, 22, BlockFlowerPot::kFlowerTypeNames[static_cast<int>(value)])

inline const PropertyInteger BlockFlowerPot::LEGACY_DATA("legacy_data", 0, 15);
inline const PropertyEnum<BlockFlowerPot::EnumFlowerType> BlockFlowerPot::CONTENTS("contents");

} // namespace mc
