#pragma once

// com.mojang.authlib.GameProfile -- a player's id and name. The textures and other properties come with skins
// (stage 14).

#include "util/UUID.h"

#include <optional>
#include <string>

namespace mc {

struct GameProfile {
    GameProfile() = default;
    GameProfile(std::optional<UUID> idIn, std::string nameIn)
        : id(idIn)
        , name(std::move(nameIn))
    {
    }

    const std::optional<UUID>& getId() const { return id; }
    const std::string& getName() const { return name; }
    // Java: isComplete -- an id and a non-blank name.
    bool isComplete() const { return id.has_value() && !name.empty(); }
    bool operator==(const GameProfile& other) const { return id == other.id && name == other.name; }

    std::optional<UUID> id;
    std::string name;
};

} // namespace mc
