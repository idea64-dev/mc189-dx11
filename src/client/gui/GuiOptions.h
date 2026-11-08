#pragma once

// Original: net/minecraft/client/gui/GuiOptions.java
// The difficulty button and its lock (GuiLockIconButton) only exist with a world (stage 9).

#include "client/gui/GuiScreen.h"

#include <memory>
#include <string>

namespace mc {

class GameSettings;

class GuiOptions : public GuiScreen {
public:
    GuiOptions(std::shared_ptr<GuiScreen> parentScreen, GameSettings& gameSettings);

    void initGui() override;
    void confirmClicked(bool result, int id) override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;

    std::string screenTitle = "Options";

private:
    std::shared_ptr<GuiScreen> parentScreen;
    GameSettings& game_settings_1;
};

} // namespace mc
