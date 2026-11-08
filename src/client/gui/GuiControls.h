#pragma once

// Original: net/minecraft/client/gui/GuiControls.java

#include "client/gui/GuiKeyBindingList.h"
#include "client/gui/GuiScreen.h"

#include <cstdint>
#include <memory>
#include <string>

namespace mc {

class GameSettings;
class KeyBinding;

class GuiControls : public GuiScreen {
public:
    GuiControls(std::shared_ptr<GuiScreen> parentScreen, GameSettings& settings);

    void initGui() override;
    void handleMouseInput() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

    // The binding waiting for a key, and when the last one was assigned (Minecraft ignores keys for 20 ms after).
    KeyBinding* buttonId = nullptr;
    int64_t time = 0;

protected:
    void actionPerformed(GuiButton& button) override;
    void mouseClicked(int mouseX, int mouseY, int mouseButton) override;
    void mouseReleased(int mouseX, int mouseY, int state) override;
    void keyTyped(char16_t typedChar, int keyCode) override;

    std::string screenTitle = "Controls";

private:
    std::shared_ptr<GuiScreen> parentScreen;
    GameSettings& options;
    std::shared_ptr<GuiKeyBindingList> keyBindingList;
    GuiButton* buttonReset = nullptr;
};

} // namespace mc
