#pragma once

// Original: net/minecraft/block/BlockBed.java

#include "block/BlockDirectional.h"

namespace mc {

class BlockBed : public BlockDirectional {
public:
    MC_BLOCK_CLASS(BlockBed)

    enum class EnumPartType { HEAD, FOOT };
    static const PropertyEnum<EnumPartType> PART;
    static const PropertyBool OCCUPIED;

    BlockBed()
        : BlockDirectional(Material::cloth)
    {
    }

    // Java: onBlockActivated lets the player sleep (or blows the bed up where respawning is impossible, e.g. the
    // Nether) -- players come with stage 8, the explosion with stage 10.
    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    void setBlockBoundsBasedOnState(IBlockAccess*, const BlockPos&) override { setBedBounds(); }
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    // Java: returns null when no position is safe.
    static std::optional<BlockPos> getSafeExitLocation(World* worldIn, const BlockPos& pos, int tries);
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    Item* getItem(World*, const BlockPos&) override { return Items::bed; }
    // Java: onBlockHarvested removes the foot for creative players -- the player comes with stage 8.
    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    int getMobilityFlag() const override { return 1; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const EnumFacing enumfacing = getHorizontal(meta);
        return (meta & 8) > 0
            ? getDefaultState()->withProperty(PART, EnumPartType::HEAD)->withProperty(FACING, enumfacing)->withProperty(OCCUPIED, (meta & 4) > 0)
            : getDefaultState()->withProperty(PART, EnumPartType::FOOT)->withProperty(FACING, enumfacing);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        if (state->getValue(PART) == EnumPartType::HEAD) {
            i |= 8;
            if (state->getValue(OCCUPIED))
                i |= 4;
        }
        return i;
    }
    Item* getItemDropped(const IBlockState* state, Random&, int) const override { return state->getValue(PART) == EnumPartType::HEAD ? nullptr : Items::bed; }

protected:
    void construct() override
    {
        BlockDirectional::construct();
        setDefaultState(blockState->getBaseState()->withProperty(PART, EnumPartType::FOOT)->withProperty(OCCUPIED, false));
        setBedBounds();
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &PART, &OCCUPIED }); }

    static bool hasRoomForPlayer(World* worldIn, const BlockPos& pos);

private:
    void setBedBounds() { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5625f, 1.0f); }
};

MC_ENUM_TRAITS(BlockBed::EnumPartType, 2, value == BlockBed::EnumPartType::HEAD ? "head" : "foot")

inline const PropertyEnum<BlockBed::EnumPartType> BlockBed::PART("part");
inline const PropertyBool BlockBed::OCCUPIED("occupied");

} // namespace mc
