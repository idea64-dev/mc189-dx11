#pragma once

// Original: net/minecraft/util/MovementInput.java and MovementInputFromOptions.java

#include "client/settings/GameSettings.h"

namespace mc {

class MovementInput {
public:
    virtual ~MovementInput() = default;
    virtual void updatePlayerMoveState() { }

    float moveStrafe = 0.0f;
    float moveForward = 0.0f;
    bool jump = false;
    bool sneak = false;
};

class MovementInputFromOptions : public MovementInput {
public:
    explicit MovementInputFromOptions(GameSettings& gameSettingsIn) : gameSettings(gameSettingsIn) { }

    void updatePlayerMoveState() override
    {
        moveStrafe = 0.0f;
        moveForward = 0.0f;
        if (gameSettings.keyBindForward.isKeyDown())
            ++moveForward;
        if (gameSettings.keyBindBack.isKeyDown())
            --moveForward;
        if (gameSettings.keyBindLeft.isKeyDown())
            ++moveStrafe;
        if (gameSettings.keyBindRight.isKeyDown())
            --moveStrafe;
        jump = gameSettings.keyBindJump.isKeyDown();
        sneak = gameSettings.keyBindSneak.isKeyDown();
        if (sneak) {
            moveStrafe = static_cast<float>(static_cast<double>(moveStrafe) * 0.3);
            moveForward = static_cast<float>(static_cast<double>(moveForward) * 0.3);
        }
    }

private:
    GameSettings& gameSettings;
};

} // namespace mc
