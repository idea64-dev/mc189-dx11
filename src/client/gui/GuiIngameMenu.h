#pragma once

// Original: net/minecraft/client/gui/GuiIngameMenu.java
// Not ported yet: the achievements and statistics screens (stage 12) and opening to LAN (stage 14); their
// buttons are there and do nothing.

#include "client/gui/GuiScreen.h"

namespace mc {

class GuiIngameMenu : public GuiScreen {
public:
    void initGui() override;
    void updateScreen() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;

private:
    int field_146445_a = 0;
    int field_146444_f = 0;
};

} // namespace mc
