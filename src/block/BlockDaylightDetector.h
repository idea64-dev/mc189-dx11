#pragma once

// Original: net/minecraft/block/BlockDaylightDetector.java

#include "block/BlockContainer.h"

namespace mc {

class BlockDaylightDetector : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockDaylightDetector)

    static const PropertyInteger POWER;

    explicit BlockDaylightDetector(bool invertedIn)
        : BlockContainer(Material::wood)
        , inverted(invertedIn)
    {
    }

    void setBlockBoundsBasedOnState(IBlockAccess*, const BlockPos&) override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.375f, 1.0f); }
    int getWeakPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing) override { return state->getValue(POWER); }
    void updatePower(World* worldIn, const BlockPos& pos);
    // Java: onBlockActivated swaps to the other detector when playerIn.isAllowEdit() -- the player comes with stage 8.
    Item* getItem(World*, const BlockPos&) override { return Item::getItemFromBlock(Blocks::daylight_detector); }
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    int getRenderType() const override { return 3; }
    bool canProvidePower() const override { return true; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(POWER, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(POWER); }
    void getSubBlocks(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& list) const override
    {
        if (!inverted)
            Block::getSubBlocks(item, tab, list);
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(Blocks::daylight_detector); }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(POWER, 0));
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.375f, 1.0f);
        setCreativeTab(&CreativeTabs::tabRedstone);
        setHardness(0.2f);
        setStepSound(soundTypeWood);
        setUnlocalizedName("daylightDetector");
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &POWER }); }

private:
    const bool inverted;
};

inline const PropertyInteger BlockDaylightDetector::POWER("power", 0, 15);

} // namespace mc
