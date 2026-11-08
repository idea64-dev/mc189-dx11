#pragma once

// Original: net/minecraft/block/BlockFlower.java, BlockRedFlower.java and BlockYellowFlower.java

#include "block/BlockBush.h"

#include <memory>

namespace mc {

class BlockFlower : public BlockBush {
public:
    enum class EnumFlowerColor { YELLOW, RED };
    enum class EnumFlowerType { DANDELION, POPPY, BLUE_ORCHID, ALLIUM, HOUSTONIA, RED_TULIP, ORANGE_TULIP, WHITE_TULIP, PINK_TULIP, OXEYE_DAISY };
    struct EnumFlowerTypeInfo {
        EnumFlowerColor blockType;
        int meta;
        std::string_view name;
        std::string_view unlocalizedName;
    };
    static constexpr EnumFlowerTypeInfo kFlowerTypes[10] {
        { EnumFlowerColor::YELLOW, 0, "dandelion", "dandelion" },
        { EnumFlowerColor::RED, 0, "poppy", "poppy" },
        { EnumFlowerColor::RED, 1, "blue_orchid", "blueOrchid" },
        { EnumFlowerColor::RED, 2, "allium", "allium" },
        { EnumFlowerColor::RED, 3, "houstonia", "houstonia" },
        { EnumFlowerColor::RED, 4, "red_tulip", "tulipRed" },
        { EnumFlowerColor::RED, 5, "orange_tulip", "tulipOrange" },
        { EnumFlowerColor::RED, 6, "white_tulip", "tulipWhite" },
        { EnumFlowerColor::RED, 7, "pink_tulip", "tulipPink" },
        { EnumFlowerColor::RED, 8, "oxeye_daisy", "oxeyeDaisy" },
    };
    static const EnumFlowerTypeInfo& info(EnumFlowerType type) { return kFlowerTypes[static_cast<int>(type)]; }
    // TYPES_FOR_BLOCK[color][meta], meta out of range giving the first.
    static EnumFlowerType getType(EnumFlowerColor blockType, int meta);
    static std::vector<EnumFlowerType> getTypes(EnumFlowerColor flowerColor);

    virtual EnumFlowerColor getBlockType() const = 0;
    // Created on first use: Java builds it from the overridden getBlockType() during construction.
    const PropertyEnum<EnumFlowerType>& getTypeProperty() const;

    int damageDropped(const IBlockState* state) const override { return info(state->getValue(getTypeProperty())).meta; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(getTypeProperty(), getType(getBlockType(), meta));
    }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(getTypeProperty())).meta; }
    EnumOffsetType getOffsetType() const override { return EnumOffsetType::XZ; }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override
    {
        for (const EnumFlowerType flowerType : getTypes(getBlockType()))
            addSubBlocks(item, list, { info(flowerType).meta });
    }

protected:
    void construct() override
    {
        BlockBush::construct();
        setDefaultState(blockState->getBaseState()->withProperty(getTypeProperty(), getBlockType() == EnumFlowerColor::RED ? EnumFlowerType::POPPY : EnumFlowerType::DANDELION));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &getTypeProperty() }); }

    mutable std::unique_ptr<PropertyEnum<EnumFlowerType>> type;
};

class BlockRedFlower : public BlockFlower {
public:
    MC_BLOCK_CLASS(BlockRedFlower)
    EnumFlowerColor getBlockType() const override { return EnumFlowerColor::RED; }
};

class BlockYellowFlower : public BlockFlower {
public:
    MC_BLOCK_CLASS(BlockYellowFlower)
    EnumFlowerColor getBlockType() const override { return EnumFlowerColor::YELLOW; }
};

MC_ENUM_TRAITS(BlockFlower::EnumFlowerType, 10, BlockFlower::info(value).name)

inline std::vector<BlockFlower::EnumFlowerType> BlockFlower::getTypes(EnumFlowerColor flowerColor)
{
    std::vector<EnumFlowerType> types;
    for (const EnumFlowerType value : EnumTraits<EnumFlowerType>::values()) {
        if (info(value).blockType == flowerColor)
            types.push_back(value);
    }
    return types;
}

inline BlockFlower::EnumFlowerType BlockFlower::getType(EnumFlowerColor blockType, int meta)
{
    const auto types = getTypes(blockType);
    if (meta < 0 || meta >= static_cast<int>(types.size()))
        meta = 0;
    return types[static_cast<size_t>(meta)];
}

inline const PropertyEnum<BlockFlower::EnumFlowerType>& BlockFlower::getTypeProperty() const
{
    if (!type) {
        const EnumFlowerColor color = getBlockType();
        type = std::make_unique<PropertyEnum<EnumFlowerType>>("type", [color](EnumFlowerType t) { return info(t).blockType == color; });
    }
    return *type;
}

} // namespace mc
