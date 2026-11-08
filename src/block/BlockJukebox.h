#pragma once

// Original: net/minecraft/block/BlockJukebox.java (TileEntityJukebox comes with tile entities, stage 9)

#include "block/BlockContainer.h"

namespace mc {

class BlockJukebox : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockJukebox)

    static const PropertyBool HAS_RECORD;

    BlockJukebox()
        : BlockContainer(Material::wood, MapColor::dirtColor)
    {
    }

    bool hasComparatorInputOverride() const override { return true; }
    int getRenderType() const override { return 3; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(HAS_RECORD, meta > 0); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(HAS_RECORD) ? 1 : 0; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(HAS_RECORD, false));
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &HAS_RECORD }); }
};

inline const PropertyBool BlockJukebox::HAS_RECORD("has_record");

} // namespace mc
