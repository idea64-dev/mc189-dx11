// Original: net/minecraft/block/BlockFalling.java, BlockDragonEgg.java and BlockAnvil.java -- the world side.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"

namespace mc {

// --- BlockFalling ---

void BlockFalling::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
}

void BlockFalling::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState*, Block*)
{
    worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
}

void BlockFalling::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    if (!worldIn->isRemote)
        checkFallable(worldIn, pos);
}

void BlockFalling::checkFallable(World* worldIn, const BlockPos& pos)
{
    if (canFallInto(worldIn, pos.down()) && pos.getY() >= 0) {
        const int i = 32;
        if (!fallInstantly && worldIn->isAreaLoaded(pos.add(-i, -i, -i), pos.add(i, i, i))) {
            // Java: spawns an EntityFallingBlock for the state at pos (onStartFalling lets the anvil hurt entities)
            // -- entities come with stage 10; until then the block stays where it is.
        } else {
            worldIn->setBlockToAir(pos);
            BlockPos blockpos;
            for (blockpos = pos.down(); canFallInto(worldIn, blockpos) && blockpos.getY() > 0; blockpos = blockpos.down()) {
            }
            if (blockpos.getY() > 0)
                worldIn->setBlockState(blockpos.up(), getDefaultState());
        }
    }
}

bool BlockFalling::canFallInto(World* worldIn, const BlockPos& pos)
{
    const Block* block = worldIn->getBlockState(pos)->getBlock();
    const Material& material = block->getMaterial();
    return block == Blocks::fire || &material == &Material::air || &material == &Material::water || &material == &Material::lava;
}

// --- BlockDragonEgg ---

void BlockDragonEgg::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
}

void BlockDragonEgg::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState*, Block*)
{
    worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
}

void BlockDragonEgg::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    checkFall(worldIn, pos);
}

void BlockDragonEgg::checkFall(World* worldIn, const BlockPos& pos)
{
    if (BlockFalling::canFallInto(worldIn, pos.down()) && pos.getY() >= 0) {
        const int i = 32;
        if (!BlockFalling::fallInstantly && worldIn->isAreaLoaded(pos.add(-i, -i, -i), pos.add(i, i, i))) {
            // Java: spawns an EntityFallingBlock -- entities come with stage 10.
        } else {
            worldIn->setBlockToAir(pos);
            BlockPos blockpos;
            for (blockpos = pos; BlockFalling::canFallInto(worldIn, blockpos) && blockpos.getY() > 0; blockpos = blockpos.down()) {
            }
            if (blockpos.getY() > 0)
                worldIn->setBlockState(blockpos, getDefaultState(), 2);
        }
    }
}

bool BlockDragonEgg::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState*, EntityPlayer*, EnumFacing, float, float, float)
{
    teleport(worldIn, pos);
    return true;
}

void BlockDragonEgg::onBlockClicked(World* worldIn, const BlockPos& pos, EntityPlayer*)
{
    teleport(worldIn, pos);
}

void BlockDragonEgg::teleport(World* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() != this)
        return;
    Random& rand = worldIn->rand;
    for (int i = 0; i < 1000; ++i) {
        const int a = rand.nextInt(16);
        const int dx = a - rand.nextInt(16);
        const int b = rand.nextInt(8);
        const int dy = b - rand.nextInt(8);
        const int c = rand.nextInt(16);
        const int dz = c - rand.nextInt(16);
        const BlockPos blockpos = pos.add(dx, dy, dz);
        if (&worldIn->getBlockState(blockpos)->getBlock()->getMaterial() == &Material::air) {
            if (worldIn->isRemote) {
                for (int j = 0; j < 128; ++j) {
                    const double d0 = rand.nextDouble();
                    const float f = (rand.nextFloat() - 0.5f) * 0.2f;
                    const float f1 = (rand.nextFloat() - 0.5f) * 0.2f;
                    const float f2 = (rand.nextFloat() - 0.5f) * 0.2f;
                    const double d1 = static_cast<double>(blockpos.getX()) + static_cast<double>(pos.getX() - blockpos.getX()) * d0 + (rand.nextDouble() - 0.5) * 1.0 + 0.5;
                    const double d2 = static_cast<double>(blockpos.getY()) + static_cast<double>(pos.getY() - blockpos.getY()) * d0 + rand.nextDouble() * 1.0 - 0.5;
                    const double d3 = static_cast<double>(blockpos.getZ()) + static_cast<double>(pos.getZ() - blockpos.getZ()) * d0 + (rand.nextDouble() - 0.5) * 1.0 + 0.5;
                    spawnParticle(worldIn, EnumParticleTypes::PORTAL, d1, d2, d3, static_cast<double>(f), static_cast<double>(f1), static_cast<double>(f2));
                }
            } else {
                worldIn->setBlockState(blockpos, iblockstate, 2);
                worldIn->setBlockToAir(pos);
            }
            return;
        }
    }
}

// --- BlockAnvil ---

const IBlockState* BlockAnvil::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
    EntityLivingBase* placer)
{
    const EnumFacing enumfacing = rotateY(placerHorizontalFacing(placer));
    return BlockFalling::onBlockPlaced(worldIn, pos, facing, hitX, hitY, hitZ, meta, placer)->withProperty(FACING, enumfacing)->withProperty(DAMAGE, meta >> 2);
}

void BlockAnvil::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const EnumFacing enumfacing = worldIn->getBlockState(pos)->getValue(FACING);
    if (getAxis(enumfacing) == Axis::X)
        setBlockBounds(0.0f, 0.0f, 0.125f, 1.0f, 1.0f, 0.875f);
    else
        setBlockBounds(0.125f, 0.0f, 0.0f, 0.875f, 1.0f, 1.0f);
}

void BlockAnvil::onEndFalling(World* worldIn, const BlockPos& pos)
{
    worldIn->playAuxSFX(1022, pos, 0);
}

} // namespace mc
