#pragma once

// Original: net/minecraft/tileentity/TileEntityPiston.java -- the block being moved by a piston, kept in the
// BlockPistonMoving (piston_extension) block until the move finishes.

#include "tileentity/TileEntity.h"
#include "util/EnumFacing.h"

namespace mc {

class IBlockState;

class TileEntityPiston : public TileEntity {
public:
    TileEntityPiston() = default;
    TileEntityPiston(const IBlockState* pistonStateIn, EnumFacing pistonFacingIn, bool extendingIn, bool shouldHeadBeRenderedIn)
        : pistonState(pistonStateIn)
        , pistonFacing(pistonFacingIn)
        , extending(extendingIn)
        , shouldHeadBeRendered(shouldHeadBeRenderedIn)
    {
    }

    const IBlockState* getPistonState() const { return pistonState; }
    // Java: getBlockMetadata() returns 0 here; TileEntity::getBlockMetadata isn't virtual in this port.
    bool isExtending() const { return extending; }
    EnumFacing getFacing() const { return pistonFacing; }
    bool shouldPistonHeadBeRendered() const { return shouldHeadBeRendered; }
    float getProgress(float ticks) const;
    float getOffsetX(float ticks) const;
    float getOffsetY(float ticks) const;
    float getOffsetZ(float ticks) const;
    void clearPistonTileEntity();
    bool isTickable() const override { return true; }
    void update() override;
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    std::string_view getMappingName() const override { return "Piston"; }

private:
    void launchWithSlimeBlock(float p_145863_1_, float p_145863_2_);

    const IBlockState* pistonState = nullptr;
    EnumFacing pistonFacing = EnumFacing::DOWN;
    bool extending = false;
    bool shouldHeadBeRendered = false;
    float progress = 0.0f;
    float lastProgress = 0.0f;
};

} // namespace mc
