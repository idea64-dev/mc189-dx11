#pragma once

// Original: net/minecraft/client/network/NetworkPlayerInfo.java and client/resources/DefaultPlayerSkin.java
// An entry of the tab list. Downloaded skins and capes come with the skin manager (stage 14); until then the
// default skin of the player's id is used, as Java does while the download runs. The team comes with the
// scoreboard (stage 12).

#include "auth/GameProfile.h"
#include "network/play/ServerPlayPackets.h"
#include "util/ResourceLocation.h"
#include "world/WorldSettings.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace mc {

class IChatComponent;

class DefaultPlayerSkin {
public:
    static ResourceLocation getDefaultSkinLegacy() { return ResourceLocation("textures/entity/steve.png"); }
    static ResourceLocation getDefaultSkin(const UUID& playerUUID)
    {
        return isSlimSkin(playerUUID) ? ResourceLocation("textures/entity/alex.png") : ResourceLocation("textures/entity/steve.png");
    }
    static std::string getSkinType(const UUID& playerUUID) { return isSlimSkin(playerUUID) ? "slim" : "default"; }

private:
    static bool isSlimSkin(const UUID& playerUUID) { return (playerUUID.hashCode() & 1) == 1; }
};

class NetworkPlayerInfo {
public:
    explicit NetworkPlayerInfo(GameProfile profile)
        : gameProfile(std::move(profile))
    {
    }
    explicit NetworkPlayerInfo(const S38PacketPlayerListItem::AddPlayerData& data)
        : gameProfile(data.getProfile())
        , gameType(data.getGameMode())
        , responseTime(data.getPing())
        , displayName(data.getDisplayName())
    {
    }

    const GameProfile& getGameProfile() const { return gameProfile; }
    GameType getGameType() const { return gameType; }
    int getResponseTime() const { return responseTime; }
    void setGameType(GameType type) { gameType = type; }
    void setResponseTime(int time) { responseTime = time; }
    bool hasLocationSkin() const { return locationSkin.has_value(); }
    std::string getSkinType() const { return skinType ? *skinType : DefaultPlayerSkin::getSkinType(gameProfile.getId().value_or(UUID())); }
    ResourceLocation getLocationSkin() const { return locationSkin ? *locationSkin : DefaultPlayerSkin::getDefaultSkin(gameProfile.getId().value_or(UUID())); }
    const std::optional<ResourceLocation>& getLocationCape() const { return locationCape; }
    void setDisplayName(std::shared_ptr<IChatComponent> name) { displayName = std::move(name); }
    const std::shared_ptr<IChatComponent>& getDisplayName() const { return displayName; }
    int func_178835_l() const { return field_178873_i; }
    void func_178836_b(int value) { field_178873_i = value; }
    int func_178860_m() const { return field_178870_j; }
    void func_178857_c(int value) { field_178870_j = value; }
    int64_t func_178847_n() const { return field_178871_k; }
    void func_178846_a(int64_t value) { field_178871_k = value; }
    int64_t func_178858_o() const { return field_178868_l; }
    void func_178844_b(int64_t value) { field_178868_l = value; }
    int64_t func_178855_p() const { return field_178869_m; }
    void func_178843_c(int64_t value) { field_178869_m = value; }

private:
    GameProfile gameProfile;
    // Java: null until the server sends it.
    GameType gameType = GameType::NOT_SET;
    int responseTime = 0;
    std::optional<ResourceLocation> locationSkin;
    std::optional<ResourceLocation> locationCape;
    std::optional<std::string> skinType;
    std::shared_ptr<IChatComponent> displayName;
    int field_178873_i = 0;
    int field_178870_j = 0;
    int64_t field_178871_k = 0;
    int64_t field_178868_l = 0;
    int64_t field_178869_m = 0;
};

} // namespace mc
