#pragma once

// Original: net/minecraft/tileentity/TileEntityBeacon.java -- the pyramid levels, the beam segments and the
// chosen effects.

#include "tileentity/TileEntity.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class TileEntityBeacon : public TileEntityLockable {
public:
    class BeamSegment {
    public:
        explicit BeamSegment(std::array<float, 3> colorsIn)
            : colors(colorsIn)
        {
        }
        void incrementHeight() { ++height; }
        const std::array<float, 3>& getColors() const { return colors; }
        int getHeight() const { return height; }

    private:
        std::array<float, 3> colors;
        int height = 1;
    };

    bool isTickable() const override { return true; }
    void update() override;
    void updateBeacon();
    const std::vector<BeamSegment>& getBeamSegments() const { return beamSegments; }
    float shouldBeamRender();
    std::shared_ptr<Packet> getDescriptionPacket() override;
    double getMaxRenderDistanceSquared() const override { return 65536.0; }
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    int getSizeInventory() const override { return 1; }
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override { return index == 0 ? payment : nullptr; }
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override;
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    std::string getName() const override { return hasCustomName() ? customName : "container.beacon"; }
    bool hasCustomName() const override { return !customName.empty(); }
    void setName(std::string name) { customName = std::move(name); }
    int getInventoryStackLimit() const override { return 1; }
    bool isUseableByPlayer(EntityPlayer* player) override;
    void openInventory(EntityPlayer*) override { }
    void closeInventory(EntityPlayer*) override { }
    bool isItemValidForSlot(int index, const ItemStack& stack) const override;
    int getField(int id) const override;
    void setField(int id, int value) override;
    int getFieldCount() const override { return 3; }
    void clear() override { payment = nullptr; }
    bool receiveClientEvent(int id, int type) override;
    std::string_view getMappingName() const override { return "Beacon"; }

private:
    void addEffectsToPlayers();
    void updateSegmentColors();
    static int func_183001_h(int p_183001_1_);

    std::vector<BeamSegment> beamSegments;
    int64_t beamRenderCounter = 0;
    float field_146014_j = 0.0f;
    bool isComplete = false;
    int levels = -1;
    int primaryEffect = 0;
    int secondaryEffect = 0;
    std::shared_ptr<ItemStack> payment;
    std::string customName;
};

} // namespace mc
