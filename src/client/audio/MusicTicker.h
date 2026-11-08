#pragma once

// Original: net/minecraft/client/audio/MusicTicker.java

#include "util/Random.h"
#include "util/ResourceLocation.h"

#include <memory>

namespace mc {

class ISound;
class Minecraft;

class MusicTicker {
public:
    enum class MusicType { MENU, GAME, CREATIVE, CREDITS, NETHER, END_BOSS, END };

    static const ResourceLocation& getMusicLocation(MusicType type);
    static int getMinDelay(MusicType type);
    static int getMaxDelay(MusicType type);

    explicit MusicTicker(Minecraft& mcIn);

    void update();
    // Java: func_181558_a, starts the given music now
    void playMusic(MusicType type);
    // Java: func_181557_a, stops the current music
    void stopMusic();

private:
    Random rand;
    Minecraft& mc;
    std::shared_ptr<ISound> currentMusic;
    int timeUntilNextMusic = 100;
};

} // namespace mc
