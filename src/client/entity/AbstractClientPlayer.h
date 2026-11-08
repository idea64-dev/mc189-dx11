#pragma once

// Original: net/minecraft/client/entity/AbstractClientPlayer.java
// Skins: the tab list entry's (NetworkPlayerInfo) or the default skin of the player's id; downloading
// (getDownloadImageSkin) comes with the skin manager.

#include "entity/player/EntityPlayer.h"
#include "util/ResourceLocation.h"

#include <optional>
#include <string>

namespace mc {

class NetworkPlayerInfo;

class AbstractClientPlayer : public EntityPlayer {
public:
    AbstractClientPlayer(World* worldIn, GameProfile playerProfile) : EntityPlayer(worldIn, std::move(playerProfile)) { }

    bool isSpectator() override;
    bool hasPlayerInfo() { return getPlayerInfo() != nullptr; }
    bool hasSkin();
    ResourceLocation getLocationSkin();
    // Java: null without a cape.
    std::optional<ResourceLocation> getLocationCape();
    static ResourceLocation getLocationSkin(const std::string& username);
    std::string getSkinType();
    float getFovModifier();

protected:
    NetworkPlayerInfo* getPlayerInfo();

private:
    NetworkPlayerInfo* playerInfo = nullptr;
};

} // namespace mc
