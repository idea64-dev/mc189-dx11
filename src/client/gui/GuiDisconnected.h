#pragma once

// Original: net/minecraft/client/gui/GuiDisconnected.java

#include "client/gui/GuiScreen.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class IChatComponent;

class GuiDisconnected : public GuiScreen {
public:
    // A null screen returns to the main menu (displayGuiScreen(null) without a world).
    GuiDisconnected(std::shared_ptr<GuiScreen> screen, const std::string& reasonLocalizationKey, std::shared_ptr<IChatComponent> chatComp);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void keyTyped(char16_t, int) override { }
    void actionPerformed(GuiButton& button) override;

private:
    std::string reason;
    std::shared_ptr<IChatComponent> message;
    std::vector<std::string> multilineMessage;
    std::shared_ptr<GuiScreen> parentScreen;
    int field_175353_i = 0;
};

} // namespace mc
