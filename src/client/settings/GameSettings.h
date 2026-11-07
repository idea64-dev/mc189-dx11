#pragma once

// Original: net/minecraft/client/settings/GameSettings.java
// Only the fields the game loop needs so far; options.txt loading and KeyBinding arrive with stage 2.2.

#include "platform/Keyboard.h"

namespace mc {

class GameSettings {
public:
    // GameSettings.Options.FRAMERATE_LIMIT: 10..260, where 260 means "Unlimited".
    static constexpr float FRAMERATE_LIMIT_MAX = 260.0f;

    int limitFramerate = 120;
    bool enableVsync = true;
    bool fancyGraphics = true;
    int clouds = 2;
    bool fullScreen = false;
    int overrideWidth = 0;
    int overrideHeight = 0;

    // Stand-in for keyBindFullscreen.getKeyCode() until KeyBinding is ported.
    int keyBindFullscreen = Keyboard::KEY_F11;
};

} // namespace mc
