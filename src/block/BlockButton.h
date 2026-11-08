#pragma once

// Original: net/minecraft/block/BlockButton.java, BlockButtonStone.java and BlockButtonWood.java

#include "block/BlockBasics.h"

namespace mc {

class BlockButton : public Block {
public:
    static const PropertyDirection FACING;
    static const PropertyBool POWERED;

    explicit BlockButton(bool woodenIn)
        : Block(Material::circuits)
        , wooden(woodenIn)
    {
    }

    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    int tickRate(World*) override { return wooden ? 30 : 20; }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    // Java: protected static; the lever shares it.
    static bool func_181088_a(World* worldIn, const BlockPos& pos, EnumFacing side);
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    int getWeakPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing) override { return state->getValue(POWERED) ? 15 : 0; }
    int getStrongPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing side) override
    {
        return !state->getValue(POWERED) ? 0 : (state->getValue(FACING) == side ? 15 : 0);
    }
    bool canProvidePower() const override { return true; }
    void randomTick(World*, const BlockPos&, const IBlockState*, Random&) override { }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn) override;
    void setBlockBoundsForItemRender() override
    {
        const float f = 0.1875f;
        const float f1 = 0.125f;
        const float f2 = 0.125f;
        setBlockBounds(0.5f - f, 0.5f - f1, 0.5f - f2, 0.5f + f, 0.5f + f1, 0.5f + f2);
    }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        EnumFacing enumfacing;
        switch (meta & 7) {
        case 0: enumfacing = EnumFacing::DOWN; break;
        case 1: enumfacing = EnumFacing::EAST; break;
        case 2: enumfacing = EnumFacing::WEST; break;
        case 3: enumfacing = EnumFacing::SOUTH; break;
        case 4: enumfacing = EnumFacing::NORTH; break;
        default: enumfacing = EnumFacing::UP;
        }
        return getDefaultState()->withProperty(FACING, enumfacing)->withProperty(POWERED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i;
        switch (state->getValue(FACING)) {
        case EnumFacing::EAST: i = 1; break;
        case EnumFacing::WEST: i = 2; break;
        case EnumFacing::SOUTH: i = 3; break;
        case EnumFacing::NORTH: i = 4; break;
        case EnumFacing::DOWN: i = 0; break;
        default: i = 5; break;
        }
        if (state->getValue(POWERED))
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(POWERED, false));
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &POWERED }); }

    const bool wooden;

private:
    bool checkForDrop(World* worldIn, const BlockPos& pos, const IBlockState* state);
    void updateBlockBounds(const IBlockState* state);
    void checkForArrows(World* worldIn, const BlockPos& pos, const IBlockState* state);
    void notifyNeighbors(World* worldIn, const BlockPos& pos, EnumFacing facing);
};

class BlockButtonStone : public BlockButton {
public:
    MC_BLOCK_CLASS(BlockButtonStone)

    BlockButtonStone()
        : BlockButton(false)
    {
    }
};

class BlockButtonWood : public BlockButton {
public:
    MC_BLOCK_CLASS(BlockButtonWood)

    BlockButtonWood()
        : BlockButton(true)
    {
    }
};

inline const PropertyDirection BlockButton::FACING("facing");
inline const PropertyBool BlockButton::POWERED("powered");

} // namespace mc
