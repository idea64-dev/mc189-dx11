#include "tileentity/TileEntityPiston.h"

#include "block/BlockPiston.h"
#include "entity/Entity.h"
#include "init/Blocks.h"
#include "nbt/NBTTagCompound.h"
#include "world/World.h"

namespace mc {

float TileEntityPiston::getProgress(float ticks) const
{
    if (ticks > 1.0f)
        ticks = 1.0f;
    return lastProgress + (progress - lastProgress) * ticks;
}

float TileEntityPiston::getOffsetX(float ticks) const
{
    return extending ? (getProgress(ticks) - 1.0f) * static_cast<float>(getFrontOffsetX(pistonFacing))
                     : (1.0f - getProgress(ticks)) * static_cast<float>(getFrontOffsetX(pistonFacing));
}

float TileEntityPiston::getOffsetY(float ticks) const
{
    return extending ? (getProgress(ticks) - 1.0f) * static_cast<float>(getFrontOffsetY(pistonFacing))
                     : (1.0f - getProgress(ticks)) * static_cast<float>(getFrontOffsetY(pistonFacing));
}

float TileEntityPiston::getOffsetZ(float ticks) const
{
    return extending ? (getProgress(ticks) - 1.0f) * static_cast<float>(getFrontOffsetZ(pistonFacing))
                     : (1.0f - getProgress(ticks)) * static_cast<float>(getFrontOffsetZ(pistonFacing));
}

void TileEntityPiston::launchWithSlimeBlock(float p_145863_1_, float p_145863_2_)
{
    if (extending)
        p_145863_1_ = 1.0f - p_145863_1_;
    else
        --p_145863_1_;
    const std::optional<AxisAlignedBB> axisalignedbb = Blocks::piston_extension->getBoundingBox(worldObj, pos, pistonState, p_145863_1_, pistonFacing);
    if (axisalignedbb) {
        // Java copies the list into field_174933_k, walks it and clears it again.
        const std::vector<Entity*> list = worldObj->getEntitiesWithinAABBExcludingEntity(nullptr, *axisalignedbb);
        for (Entity* entity : list) {
            if (pistonState->getBlock() == Blocks::slime_block && extending) {
                switch (getAxis(pistonFacing)) {
                case Axis::X: entity->motionX = getFrontOffsetX(pistonFacing); break;
                case Axis::Y: entity->motionY = getFrontOffsetY(pistonFacing); break;
                case Axis::Z: entity->motionZ = getFrontOffsetZ(pistonFacing); break;
                }
            } else {
                entity->moveEntity(p_145863_2_ * static_cast<float>(getFrontOffsetX(pistonFacing)), p_145863_2_ * static_cast<float>(getFrontOffsetY(pistonFacing)),
                    p_145863_2_ * static_cast<float>(getFrontOffsetZ(pistonFacing)));
            }
        }
    }
}

void TileEntityPiston::clearPistonTileEntity()
{
    if (lastProgress < 1.0f && worldObj != nullptr) {
        lastProgress = progress = 1.0f;
        // Keep this alive while the world drops its references.
        const auto self = worldObj->getTileEntityShared(pos);
        worldObj->removeTileEntity(pos);
        invalidate();
        if (worldObj->getBlockState(pos)->getBlock() == Blocks::piston_extension) {
            worldObj->setBlockState(pos, pistonState, 3);
            worldObj->notifyBlockOfStateChange(pos, pistonState->getBlock());
        }
    }
}

void TileEntityPiston::update()
{
    lastProgress = progress;
    if (lastProgress >= 1.0f) {
        launchWithSlimeBlock(1.0f, 0.25f);
        const auto self = worldObj->getTileEntityShared(pos);
        worldObj->removeTileEntity(pos);
        invalidate();
        if (worldObj->getBlockState(pos)->getBlock() == Blocks::piston_extension) {
            worldObj->setBlockState(pos, pistonState, 3);
            worldObj->notifyBlockOfStateChange(pos, pistonState->getBlock());
        }
    } else {
        progress += 0.5f;
        if (progress >= 1.0f)
            progress = 1.0f;
        if (extending)
            launchWithSlimeBlock(progress, progress - lastProgress + 0.0625f);
    }
}

void TileEntityPiston::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    pistonState = Block::getBlockById(compound.getInteger("blockId"))->getStateFromMeta(compound.getInteger("blockData"));
    pistonFacing = getFront(compound.getInteger("facing"));
    lastProgress = progress = compound.getFloat("progress");
    extending = compound.getBoolean("extending");
}

void TileEntityPiston::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    compound.setInteger("blockId", Block::getIdFromBlock(pistonState->getBlock()));
    compound.setInteger("blockData", pistonState->getBlock()->getMetaFromState(pistonState));
    compound.setInteger("facing", getIndex(pistonFacing));
    compound.setFloat("progress", lastProgress);
    compound.setBoolean("extending", extending);
}

} // namespace mc
