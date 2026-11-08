#include "world/WorldSettings.h"

#include "world/storage/WorldInfo.h"

namespace mc {

WorldSettings::WorldSettings(const WorldInfo& info)
    : WorldSettings(info.getSeed(), info.getGameType(), info.isMapFeaturesEnabled(), info.isHardcoreModeEnabled(), info.getTerrainType())
{
}

} // namespace mc
