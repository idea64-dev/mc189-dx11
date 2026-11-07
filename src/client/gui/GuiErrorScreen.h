#pragma once

// Original: net/minecraft/client/gui/GuiErrorScreen.java

#include "client/gui/GuiScreen.h"

#include <string>

namespace mc {

class GuiErrorScreen : public GuiScreen {
public:
    GuiErrorScreen(std::string title, std::string message);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

    // Stand-in for screens that are not ported yet: names the ROADMAP stage that brings them.
    static std::shared_ptr<GuiErrorScreen> notPortedYet(const std::string& what, const std::string& stage);

protected:
    void keyTyped(char16_t typedChar, int keyCode) override;
    void actionPerformed(GuiButton& button) override;

private:
    std::string title;
    std::string message;
};

} // namespace mc
