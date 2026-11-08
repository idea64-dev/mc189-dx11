#include "tileentity/TileEntitySkull.h"

#include "nbt/NBTTagCompound.h"
#include "network/play/ServerPlayPackets.h"

namespace mc {

void TileEntitySkull::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    compound.setByte("SkullType", static_cast<int8_t>(skullType & 255));
    compound.setByte("Rot", static_cast<int8_t>(skullRotation & 255));
    if (playerProfile) {
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        writeGameProfile(*nbttagcompound, *playerProfile);
        if (profileProperties)
            nbttagcompound->setTag("Properties", profileProperties->copy());
        compound.setTag("Owner", nbttagcompound);
    }
}

void TileEntitySkull::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    skullType = compound.getByte("SkullType");
    skullRotation = compound.getByte("Rot");
    if (skullType == 3) {
        if (compound.hasKey("Owner", 10)) {
            const auto owner = compound.getCompoundTag("Owner");
            playerProfile = readGameProfileFromNBT(*owner);
            profileProperties = playerProfile && owner->hasKey("Properties", 10) ? owner->getCompoundTag("Properties") : nullptr;
        } else if (compound.hasKey("ExtraType", 8)) {
            const std::string s = compound.getString("ExtraType");
            if (!s.empty()) {
                playerProfile = GameProfile(std::nullopt, s);
                profileProperties = nullptr;
                updatePlayerProfile();
            }
        }
    }
}

void TileEntitySkull::setType(int type)
{
    skullType = type;
    playerProfile.reset();
    profileProperties = nullptr;
}

void TileEntitySkull::setPlayerProfile(std::optional<GameProfile> profile)
{
    skullType = 3;
    playerProfile = std::move(profile);
    profileProperties = nullptr;
    updatePlayerProfile();
}

void TileEntitySkull::updatePlayerProfile()
{
    playerProfile = updateGameprofile(playerProfile);
    markDirty();
}

std::optional<GameProfile> TileEntitySkull::readGameProfileFromNBT(const NBTTagCompound& compound)
{
    std::string s;
    std::string s1;
    if (compound.hasKey("Name", 8))
        s = compound.getString("Name");
    if (compound.hasKey("Id", 8))
        s1 = compound.getString("Id");
    if (s.empty() && s1.empty())
        return std::nullopt;
    // Java: UUID.fromString, null when it throws.
    return GameProfile(UUID::fromString(s1), s);
}

void TileEntitySkull::writeGameProfile(NBTTagCompound& tagCompound, const GameProfile& profile)
{
    if (!profile.getName().empty())
        tagCompound.setString("Name", profile.getName());
    if (profile.getId())
        tagCompound.setString("Id", profile.getId()->toString());
}

std::shared_ptr<Packet> TileEntitySkull::getDescriptionPacket()
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    writeToNBT(*nbttagcompound);
    return std::make_shared<S35PacketUpdateTileEntity>(pos, 4, nbttagcompound);
}

} // namespace mc
