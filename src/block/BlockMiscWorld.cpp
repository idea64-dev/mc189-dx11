// The remaining world callbacks that need the placer or a colliding entity: BlockCactus.java, BlockPortal.java,
// BlockSlime.java, BlockEndPortal.java, BlockEndPortalFrame.java, BlockPumpkin.java, BlockCocoa.java,
// BlockRedstoneDiode.java (onBlockPlaced), BlockWeb.java, BlockHay.java and BlockQuartz.java.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"
#include "entity/Entity.h"
#include "util/DamageSource.h"

#include <cmath>

namespace mc {

// --- BlockCactus ---

void BlockCactus::onEntityCollidedWithBlock(World*, const BlockPos&, const IBlockState*, Entity* entityIn)
{
    entityIn->attackEntityFrom(DamageSource::cactus, 1.0f);
}

// --- BlockPortal ---

void BlockPortal::onEntityCollidedWithBlock(World*, const BlockPos& pos, const IBlockState*, Entity* entityIn)
{
    if (entityIn->ridingEntity == nullptr && entityIn->riddenByEntity == nullptr)
        entityIn->setPortal(pos);
}

// --- BlockSlime ---

void BlockSlime::onFallenUpon(World* worldIn, const BlockPos& pos, Entity* entityIn, float fallDistance)
{
    if (entityIn->isSneaking())
        BlockBreakable::onFallenUpon(worldIn, pos, entityIn, fallDistance);
    else
        entityIn->fall(fallDistance, 0.0f);
}

void BlockSlime::onLanded(World* worldIn, Entity* entityIn)
{
    if (entityIn->isSneaking())
        BlockBreakable::onLanded(worldIn, entityIn);
    else if (entityIn->motionY < 0.0)
        entityIn->motionY = -entityIn->motionY;
}

void BlockSlime::onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, Entity* entityIn)
{
    if (std::abs(entityIn->motionY) < 0.1 && !entityIn->isSneaking()) {
        const double d0 = 0.4 + std::abs(entityIn->motionY) * 0.2;
        entityIn->motionX *= d0;
        entityIn->motionZ *= d0;
    }
    BlockBreakable::onEntityCollidedWithBlock(worldIn, pos, entityIn);
}

// --- BlockEndPortal, BlockEndPortalFrame ---

void BlockEndPortal::onEntityCollidedWithBlock(World* worldIn, const BlockPos&, const IBlockState*, Entity* entityIn)
{
    if (entityIn->ridingEntity == nullptr && entityIn->riddenByEntity == nullptr && !worldIn->isRemote)
        entityIn->travelToDimension(1);
}

const IBlockState* BlockEndPortalFrame::onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, getOpposite(placerHorizontalFacing(placer)))->withProperty(EYE, false);
}

// --- BlockPumpkin ---

const IBlockState* BlockPumpkin::onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, getOpposite(placerHorizontalFacing(placer)));
}

// --- BlockCocoa ---

void BlockCocoa::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack&)
{
    // Java: EnumFacing.fromAngle(placer.rotationYaw); without the placer (stage 8) the placed facing is kept.
    if (const Entity* entity = placerEntity(placer))
        worldIn->setBlockState(pos, state->withProperty(FACING, fromAngle(entity->rotationYaw)), 2);
}

// --- BlockRedstoneDiode ---

const IBlockState* BlockRedstoneDiode::onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, getOpposite(placerHorizontalFacing(placer)));
}

// --- BlockWeb ---

void BlockWeb::onEntityCollidedWithBlock(World*, const BlockPos&, const IBlockState*, Entity* entityIn)
{
    entityIn->setInWeb();
}

// --- BlockHay ---

const IBlockState* BlockHay::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
    EntityLivingBase* placer)
{
    return BlockRotatedPillar::onBlockPlaced(worldIn, pos, facing, hitX, hitY, hitZ, meta, placer)->withProperty(AXIS, getAxis(facing));
}

// --- BlockQuartz ---

const IBlockState* BlockQuartz::onBlockPlaced(World*, const BlockPos&, EnumFacing facing, float, float, float, int meta, EntityLivingBase*)
{
    if (meta == info(EnumType::LINES_Y).meta) {
        switch (getAxis(facing)) {
        case Axis::Z: return getDefaultState()->withProperty(VARIANT, EnumType::LINES_Z);
        case Axis::X: return getDefaultState()->withProperty(VARIANT, EnumType::LINES_X);
        default: return getDefaultState()->withProperty(VARIANT, EnumType::LINES_Y);
        }
    }
    return meta == info(EnumType::CHISELED).meta ? getDefaultState()->withProperty(VARIANT, EnumType::CHISELED)
                                                 : getDefaultState()->withProperty(VARIANT, EnumType::DEFAULT);
}

} // namespace mc
