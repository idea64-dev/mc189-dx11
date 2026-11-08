#pragma once

// Original: net/minecraft/entity/EntityList.java -- so far only the name/id table and the spawn egg colors.
// Creating entities by name, id or NBT comes with the entities (stage 9); the egg kill statistics with StatList.

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mc {

class EntityList {
public:
    struct EntityEggInfo {
        int spawnedID;
        int primaryColor;
        int secondaryColor;
    };

    // The registered name for an id, nullopt when none.
    static std::optional<std::string> getStringFromID(int id);
    // -1 when the name is unknown (Java: getIDFromString through stringToIDMapping, null there).
    static int getIDFromString(std::string_view name);
    // Java: a LinkedHashMap by id, iterated in registration order.
    static const std::vector<EntityEggInfo>& entityEggs();
    static const EntityEggInfo* getEggInfo(int id);
};

} // namespace mc
