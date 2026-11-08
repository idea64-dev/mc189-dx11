#pragma once

// Original: net/minecraft/client/gui/GuiScreenOptionsSounds.java

#include "client/audio/SoundCategory.h"
#include "client/gui/GuiScreen.h"

#include <memory>
#include <string>

namespace mc {

class GameSettings;

class GuiScreenOptionsSounds : public GuiScreen {
public:
    GuiScreenOptionsSounds(std::shared_ptr<GuiScreen> parentScreen, GameSettings& settings);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;
    std::string getSoundVolume(SoundCategory category) const;

    std::string screenTitle = "Options";

private:
    class Button;

    std::shared_ptr<GuiScreen> parentScreen;
    GameSettings& game_settings_4;
    std::string offDisplayString;
};

} // namespace mc
