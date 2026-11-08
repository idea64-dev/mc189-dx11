#pragma once

// Original: net/minecraft/client/gui/GuiYesNo.java

#include "client/gui/GuiScreen.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class GuiYesNo : public GuiScreen {
public:
    GuiYesNo(std::shared_ptr<GuiYesNoCallback> parentScreen, std::string messageLine1, std::string messageLine2, int parentButtonClickedId);
    GuiYesNo(std::shared_ptr<GuiYesNoCallback> parentScreen, std::string messageLine1, std::string messageLine2, std::string confirmButtonText, std::string cancelButtonText, int parentButtonClickedId);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void setButtonDelay(int ticks);
    void updateScreen() override;

protected:
    void actionPerformed(GuiButton& button) override;

    std::shared_ptr<GuiYesNoCallback> parentScreen;
    std::string messageLine1;
    std::string confirmButtonText;
    std::string cancelButtonText;
    int parentButtonClickedId;

private:
    std::string messageLine2;
    std::vector<std::string> listLines;
    int ticksUntilEnable = 0;
};

} // namespace mc
