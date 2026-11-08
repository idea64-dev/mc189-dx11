#pragma once

// Original: net/minecraft/util/Session.java -- who plays: the name and the player id the launcher passed.

#include "auth/GameProfile.h"

#include <string>

namespace mc {

class Session {
public:
    Session() = default;
    Session(std::string usernameIn, std::string playerIDIn, std::string tokenIn, std::string sessionTypeIn)
        : username(std::move(usernameIn))
        , playerID(std::move(playerIDIn))
        , token(std::move(tokenIn))
        , sessionType(std::move(sessionTypeIn))
    {
    }

    std::string getSessionID() const { return "token:" + token + ":" + playerID; }
    const std::string& getPlayerID() const { return playerID; }
    const std::string& getUsername() const { return username; }
    const std::string& getToken() const { return token; }
    // Java: UUIDTypeAdapter.fromString(playerID) -- 32 hex digits without dashes; no id when it isn't one.
    GameProfile getProfile() const
    {
        std::string dashed = playerID;
        if (dashed.size() == 32)
            dashed = dashed.substr(0, 8) + "-" + dashed.substr(8, 4) + "-" + dashed.substr(12, 4) + "-" + dashed.substr(16, 4) + "-" + dashed.substr(20);
        return GameProfile(UUID::fromString(dashed), username);
    }

private:
    std::string username;
    std::string playerID;
    std::string token;
    std::string sessionType;
};

} // namespace mc
