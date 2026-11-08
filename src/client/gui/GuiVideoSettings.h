#pragma once

// Original: net/minecraft/client/gui/GuiVideoSettings.java

#include "client/gui/GuiOptionsRowList.h"
#include "client/gui/GuiScreen.h"

#include <memory>
#include <string>

namespace mc {

class GameSettings;

class GuiVideoSettings : public GuiScreen {
public:
    GuiVideoSettings(std::shared_ptr<GuiScreen> parentScreen, GameSettings& gameSettings);

    void initGui() override;
    void handleMouseInput() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;
    void mouseClicked(int mouseX, int mouseY, int mouseButton) override;
    void mouseReleased(int mouseX, int mouseY, int state) override;

    std::string screenTitle = "Video Settings";

private:
    std::shared_ptr<GuiScreen> parentGuiScreen;
    GameSettings& guiGameSettings;
    // Shared so a call into the list survives the screen re-initializing itself (fullscreen, GUI scale).
    std::shared_ptr<GuiOptionsRowList> optionsRowList;
};

} // namespace mc
