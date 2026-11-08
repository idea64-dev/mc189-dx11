#pragma once

// Original: net/minecraft/block/BlockStairs.java

#include "block/BlockBasics.h"

namespace mc {

class BlockStairs : public Block {
public:
    MC_BLOCK_CLASS(BlockStairs)

    enum class EnumHalf { TOP, BOTTOM };
    enum class EnumShape { STRAIGHT, INNER_LEFT, INNER_RIGHT, OUTER_LEFT, OUTER_RIGHT };

    static const PropertyDirection FACING;
    static const PropertyEnum<EnumHalf> HALF;
    static const PropertyEnum<EnumShape> SHAPE;

    explicit BlockStairs(const IBlockState* modelStateIn)
        : Block(modelStateIn->getBlock()->getMaterial())
        , modelBlock(modelStateIn->getBlock())
        , modelState(modelStateIn)
    {
    }

    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    void setBaseCollisionBounds(IBlockAccess* worldIn, const BlockPos& pos);
    static bool isBlockStairs(const Block* blockIn);
    static bool isSameStair(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state);
    int func_176307_f(IBlockAccess* blockAccess, const BlockPos& pos);
    int func_176305_g(IBlockAccess* blockAccess, const BlockPos& pos);
    bool func_176306_h(IBlockAccess* blockAccess, const BlockPos& pos);
    bool func_176304_i(IBlockAccess* blockAccess, const BlockPos& pos);
    void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity) override;
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void onBlockClicked(World* worldIn, const BlockPos& pos, EntityPlayer* playerIn) override;
    void onBlockDestroyedByPlayer(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    int getMixedBrightnessForBlock(IBlockAccess* worldIn, const BlockPos& pos) override;
    int tickRate(World* worldIn) override;
    AxisAlignedBB getSelectedBoundingBox(World* worldIn, const BlockPos& pos) override;
    Vec3 modifyAcceleration(World* worldIn, const BlockPos& pos, Entity* entityIn, const Vec3& motion) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, Entity* entityIn) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    void onBlockDestroyedByExplosion(World* worldIn, const BlockPos& pos, Explosion* explosionIn) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    std::optional<MovingObjectPosition> collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end) override;
    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    float getExplosionResistance() const override { return modelBlock->getExplosionResistance(); }
    EnumWorldBlockLayer getBlockLayer() const override { return modelBlock->getBlockLayer(); }
    bool isCollidable() const override { return modelBlock->isCollidable(); }
    bool canCollideCheck(const IBlockState* state, bool hitIfLiquid) const override { return modelBlock->canCollideCheck(state, hitIfLiquid); }
    const MapColor& getMapColor(const IBlockState*) const override { return modelBlock->getMapColor(modelState); }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const IBlockState* iblockstate = getDefaultState()->withProperty(HALF, (meta & 4) > 0 ? EnumHalf::TOP : EnumHalf::BOTTOM);
        return iblockstate->withProperty(FACING, getFront(5 - (meta & 3)));
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        if (state->getValue(HALF) == EnumHalf::TOP)
            i |= 4;
        i = i | (5 - getIndex(state->getValue(FACING)));
        return i;
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(HALF, EnumHalf::BOTTOM)->withProperty(SHAPE, EnumShape::STRAIGHT));
        setHardness(modelBlock->getBlockHardnessValue());
        setResistance(modelBlock->getBlockResistanceValue() / 3.0f);
        setStepSound(*modelBlock->stepSound);
        setLightOpacity(255);
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &HALF, &SHAPE }); }

private:
    Block* modelBlock;
    const IBlockState* modelState;
    bool hasRaytraced = false;
    int rayTracePass = 0;
};

MC_ENUM_TRAITS(BlockStairs::EnumHalf, 2, value == BlockStairs::EnumHalf::TOP ? "top" : "bottom")
MC_ENUM_TRAITS(BlockStairs::EnumShape, 5,
    value == BlockStairs::EnumShape::STRAIGHT ? "straight"
        : value == BlockStairs::EnumShape::INNER_LEFT ? "inner_left"
        : value == BlockStairs::EnumShape::INNER_RIGHT ? "inner_right"
        : value == BlockStairs::EnumShape::OUTER_LEFT ? "outer_left"
                                                      : "outer_right")

inline const PropertyDirection BlockStairs::FACING("facing", isHorizontalFacing);
inline const PropertyEnum<BlockStairs::EnumHalf> BlockStairs::HALF("half");
inline const PropertyEnum<BlockStairs::EnumShape> BlockStairs::SHAPE("shape");

} // namespace mc
