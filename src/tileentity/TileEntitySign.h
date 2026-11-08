#pragma once

// Original: net/minecraft/tileentity/TileEntitySign.java -- the four text lines of a sign.
// Java re-serializes each line with IChatComponent.Serializer.componentToJson; that serializer isn't ported, so
// the JSON read from NBT is kept and written back as is (what vanilla wrote reads back unchanged). Lines that
// aren't JSON become a text component whose JSON is the quoted string.

#include "tileentity/TileEntity.h"

#include <array>
#include <memory>
#include <string>

namespace mc {

class IChatComponent;
class NBTTagCompound;

class TileEntitySign : public TileEntity {
public:
    TileEntitySign();

    void writeToNBT(NBTTagCompound& compound) const override;
    void readFromNBT(const NBTTagCompound& compound) override;
    bool func_183000_F() const override { return true; }
    std::shared_ptr<Packet> getDescriptionPacket() override;
    bool getIsEditable() const { return isEditable; }
    void setEditable(bool isEditableIn);
    void setPlayer(EntityPlayer* playerIn) { player = playerIn; }
    EntityPlayer* getPlayer() const { return player; }
    // Java: runs the RUN_COMMAND click events of the lines as the player -- commands come with stage 12.
    bool executeCommand(EntityPlayer* playerIn);
    // Sets a line and the JSON written for it (the GUI path; the JSON is what componentToJson gives).
    void setLine(int index, std::shared_ptr<IChatComponent> component, std::string json);
    std::string_view getMappingName() const override { return "Sign"; }

    std::array<std::shared_ptr<IChatComponent>, 4> signText;
    int lineBeingEdited = -1;

private:
    std::array<std::string, 4> signJson;
    bool isEditable = true;
    EntityPlayer* player = nullptr;
    // Java: CommandResultStats, kept as its "CommandStats" compound until commands are ported.
    std::shared_ptr<NBTTagCompound> stats;
};

} // namespace mc
