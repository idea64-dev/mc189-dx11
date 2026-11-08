#include "client/audio/MusicTicker.h"

#include "client/Minecraft.h"
#include "client/audio/SoundHandler.h"
#include "util/MathHelper.h"

#include <algorithm>
#include <array>
#include <climits>

namespace mc {

namespace {

struct MusicInfo {
    ResourceLocation location;
    int minDelay;
    int maxDelay;
};

const std::array<MusicInfo, 7>& musicTypes()
{
    static const std::array<MusicInfo, 7> types { {
        { ResourceLocation("minecraft:music.menu"), 20, 600 },
        { ResourceLocation("minecraft:music.game"), 12000, 24000 },
        { ResourceLocation("minecraft:music.game.creative"), 1200, 3600 },
        { ResourceLocation("minecraft:music.game.end.credits"), INT_MAX, INT_MAX },
        { ResourceLocation("minecraft:music.game.nether"), 1200, 3600 },
        { ResourceLocation("minecraft:music.game.end.dragon"), 0, 0 },
        { ResourceLocation("minecraft:music.game.end"), 6000, 24000 },
    } };
    return types;
}

const MusicInfo& info(MusicTicker::MusicType type)
{
    return musicTypes()[static_cast<size_t>(type)];
}

} // namespace

const ResourceLocation& MusicTicker::getMusicLocation(MusicType type)
{
    return info(type).location;
}

int MusicTicker::getMinDelay(MusicType type)
{
    return info(type).minDelay;
}

int MusicTicker::getMaxDelay(MusicType type)
{
    return info(type).maxDelay;
}

MusicTicker::MusicTicker(Minecraft& mcIn)
    : mc(mcIn)
{
}

void MusicTicker::update()
{
    const MusicType type = mc.getAmbientMusicType();

    if (currentMusic != nullptr) {
        if (!(getMusicLocation(type) == currentMusic->getSoundLocation())) {
            mc.getSoundHandler().stopSound(currentMusic);
            timeUntilNextMusic = MathHelper::getRandomIntegerInRange(rand, 0, getMinDelay(type) / 2);
        }

        if (!mc.getSoundHandler().isSoundPlaying(currentMusic)) {
            currentMusic = nullptr;
            timeUntilNextMusic = std::min(MathHelper::getRandomIntegerInRange(rand, getMinDelay(type), getMaxDelay(type)), timeUntilNextMusic);
        }
    }

    if (currentMusic == nullptr && timeUntilNextMusic-- <= 0)
        playMusic(type);
}

void MusicTicker::playMusic(MusicType type)
{
    currentMusic = PositionedSoundRecord::create(getMusicLocation(type));
    mc.getSoundHandler().playSound(currentMusic);
    timeUntilNextMusic = INT_MAX;
}

void MusicTicker::stopMusic()
{
    if (currentMusic != nullptr) {
        mc.getSoundHandler().stopSound(currentMusic);
        currentMusic = nullptr;
        timeUntilNextMusic = 0;
    }
}

} // namespace mc
