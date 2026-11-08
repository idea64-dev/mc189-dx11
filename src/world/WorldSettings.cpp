#include "world/WorldSettings.h"

#include "entity/player/PlayerCapabilities.h"
#include "world/storage/WorldInfo.h"

namespace mc {

void configurePlayerCapabilities(GameType type, PlayerCapabilities& capabilities)
{
    if (type == GameType::CREATIVE) {
        capabilities.allowFlying = true;
        capabilities.isCreativeMode = true;
        capabilities.disableDamage = true;
    } else if (type == GameType::SPECTATOR) {
        capabilities.allowFlying = true;
        capabilities.isCreativeMode = false;
        capabilities.disableDamage = true;
        capabilities.isFlying = true;
    } else {
        capabilities.allowFlying = false;
        capabilities.isCreativeMode = false;
        capabilities.disableDamage = false;
        capabilities.isFlying = false;
    }

    capabilities.allowEdit = !isAdventure(type);
}

WorldSettings::WorldSettings(const WorldInfo& info)
    : WorldSettings(info.getSeed(), info.getGameType(), info.isMapFeaturesEnabled(), info.isHardcoreModeEnabled(), info.getTerrainType())
{
}

} // namespace mc
