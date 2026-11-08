#pragma once

// Original: net/minecraft/client/gui/ScreenChatOptions.java

#include "client/gui/GuiScreen.h"

#include <memory>
#include <string>

namespace mc {

class GameSettings;

class ScreenChatOptions : public GuiScreen {
public:
    ScreenChatOptions(std::shared_ptr<GuiScreen> parentScreen, GameSettings& gameSettings);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;

private:
    std::shared_ptr<GuiScreen> parentScreen;
    GameSettings& game_settings;
    std::string screenTitle;
};

} // namespace mc
