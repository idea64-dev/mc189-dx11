#pragma once

// Original: net/minecraft/block/BlockAnvil.java (the Anvil interaction object comes with containers, stage 11)

#include "block/BlockFalling.h"

namespace mc {

class BlockAnvil : public BlockFalling {
public:
    MC_BLOCK_CLASS(BlockAnvil)

    static const PropertyDirection FACING;
    static const PropertyInteger DAMAGE;

    BlockAnvil()
        : BlockFalling(Material::anvil)
    {
    }

    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    int damageDropped(const IBlockState* state) const override { return state->getValue(DAMAGE); }
    const IBlockState* getStateForEntityRender(const IBlockState*) const override { return getDefaultState()->withProperty(FACING, EnumFacing::SOUTH); }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getHorizontal(meta & 3))->withProperty(DAMAGE, (meta & 15) >> 2);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        i = i | state->getValue(DAMAGE) << 2;
        return i;
    }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, { 0, 1, 2 }); }
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    // Java: onBlockActivated opens the repair GUI -- stage 11.
    bool onBlockActivated(World*, const BlockPos&, const IBlockState*, EntityPlayer*, EnumFacing, float, float, float) override { return true; }
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void onEndFalling(World* worldIn, const BlockPos& pos) override;
    bool shouldSideBeRendered(IBlockAccess*, const BlockPos&, EnumFacing) override { return true; }

protected:
    void construct() override
    {
        BlockFalling::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(DAMAGE, 0));
        setLightOpacity(0);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &DAMAGE }); }
};

inline const PropertyDirection BlockAnvil::FACING("facing", isHorizontalFacing);
inline const PropertyInteger BlockAnvil::DAMAGE("damage", 0, 2);

} // namespace mc
