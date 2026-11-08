#include "tileentity/TileEntitySign.h"

#include "nbt/NBTTagCompound.h"
#include "network/play/ServerPlayPackets.h"
#include "util/ChatComponent.h"

#include <nlohmann/json.hpp>

namespace mc {

TileEntitySign::TileEntitySign()
{
    for (size_t i = 0; i < signText.size(); ++i) {
        signText[i] = std::make_shared<ChatComponentText>("");
        // Java: componentToJson(new ChatComponentText("")) -- a plain text component serializes as a string.
        signJson[i] = "\"\"";
    }
}

void TileEntitySign::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    for (size_t i = 0; i < signJson.size(); ++i)
        compound.setString("Text" + std::to_string(i + 1), signJson[i]);
    // Java: stats.writeStatsToNBT -- only written when a stat is set.
    if (stats)
        compound.setTag("CommandStats", stats->copy());
}

void TileEntitySign::readFromNBT(const NBTTagCompound& compound)
{
    isEditable = false;
    TileEntity::readFromNBT(compound);
    for (size_t i = 0; i < signText.size(); ++i) {
        const std::string s = compound.getString("Text" + std::to_string(i + 1));
        try {
            // Java: ChatComponentProcessor.processComponent resolves selectors and scores -- commands come with
            // stage 12; plain components come out unchanged.
            signText[i] = IChatComponent::jsonToComponent(s);
            signJson[i] = s;
        } catch (const JsonParseException&) {
            signText[i] = std::make_shared<ChatComponentText>(s);
            signJson[i] = nlohmann::json(s).dump();
        }
    }
    stats = compound.hasKey("CommandStats", 10) ? compound.getCompoundTag("CommandStats") : nullptr;
}

void TileEntitySign::setEditable(bool isEditableIn)
{
    isEditable = isEditableIn;
    if (!isEditableIn)
        player = nullptr;
}

bool TileEntitySign::executeCommand(EntityPlayer*)
{
    return true;
}

void TileEntitySign::setLine(int index, std::shared_ptr<IChatComponent> component, std::string json)
{
    signText.at(static_cast<size_t>(index)) = std::move(component);
    signJson.at(static_cast<size_t>(index)) = std::move(json);
}

std::shared_ptr<Packet> TileEntitySign::getDescriptionPacket()
{
    return std::make_shared<S33PacketUpdateSign>(worldObj, pos, std::vector<std::shared_ptr<IChatComponent>>(signText.begin(), signText.end()));
}

} // namespace mc
