#pragma once

// Original: net/minecraft/tileentity/TileEntitySkull.java -- the skull type, its rotation and a player head's owner.

#include "auth/GameProfile.h"
#include "tileentity/TileEntity.h"

#include <memory>
#include <optional>

namespace mc {

class NBTTagCompound;

class TileEntitySkull : public TileEntity {
public:
    void writeToNBT(NBTTagCompound& compound) const override;
    void readFromNBT(const NBTTagCompound& compound) override;
    const std::optional<GameProfile>& getPlayerProfile() const { return playerProfile; }
    std::shared_ptr<Packet> getDescriptionPacket() override;
    void setType(int type);
    void setPlayerProfile(std::optional<GameProfile> profile);
    // Java: fills in the profile's textures through the server's profile cache -- the session service comes with
    // stage 14; the profile is returned as is.
    static std::optional<GameProfile> updateGameprofile(std::optional<GameProfile> input) { return input; }
    int getSkullType() const { return skullType; }
    int getSkullRotation() const { return skullRotation; }
    void setSkullRotation(int rotation) { skullRotation = rotation; }
    std::string_view getMappingName() const override { return "Skull"; }

    // Java: NBTUtil.readGameProfileFromNBT / writeGameProfile. The "Properties" (skin textures) aren't part of
    // GameProfile yet; the skull keeps them as read.
    static std::optional<GameProfile> readGameProfileFromNBT(const NBTTagCompound& compound);
    static void writeGameProfile(NBTTagCompound& tagCompound, const GameProfile& profile);

private:
    void updatePlayerProfile();

    int skullType = 0;
    int skullRotation = 0;
    std::optional<GameProfile> playerProfile;
    std::shared_ptr<NBTTagCompound> profileProperties;
};

} // namespace mc
