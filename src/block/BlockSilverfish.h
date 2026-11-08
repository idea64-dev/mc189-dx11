#pragma once

// Original: net/minecraft/block/BlockSilverfish.java (getModelBlock comes with the silverfish, stage 10)

#include "block/BlockBasics.h"

namespace mc {

class BlockSilverfish : public Block {
public:
    MC_BLOCK_CLASS(BlockSilverfish)

    enum class EnumType { STONE, COBBLESTONE, STONEBRICK, MOSSY_STONEBRICK, CRACKED_STONEBRICK, CHISELED_STONEBRICK };
    struct EnumTypeInfo {
        int meta;
        std::string_view name;
        std::string_view unlocalizedName;
    };
    static constexpr EnumTypeInfo kTypes[6] {
        { 0, "stone", "stone" }, { 1, "cobblestone", "cobble" }, { 2, "stone_brick", "brick" }, { 3, "mossy_brick", "mossybrick" },
        { 4, "cracked_brick", "crackedbrick" }, { 5, "chiseled_brick", "chiseledbrick" },
    };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 6 ? 0 : meta); }

    static const PropertyEnum<EnumType> VARIANT;

    BlockSilverfish()
        : Block(Material::clay)
    {
    }

    int quantityDropped(Random&) const override { return 0; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, 0, 6); }
    ItemStack createStackedBlock(const IBlockState* state) const override;

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, EnumType::STONE));
        setHardness(0.0f);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT }); }
};

MC_ENUM_TRAITS(BlockSilverfish::EnumType, 6, BlockSilverfish::info(value).name)

inline const PropertyEnum<BlockSilverfish::EnumType> BlockSilverfish::VARIANT("variant");

} // namespace mc
