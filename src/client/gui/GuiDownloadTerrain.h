#pragma once

// Original: net/minecraft/client/gui/GuiDownloadTerrain.java

#include "client/gui/GuiScreen.h"

namespace mc {

class NetHandlerPlayClient;

class GuiDownloadTerrain : public GuiScreen {
public:
    explicit GuiDownloadTerrain(NetHandlerPlayClient* netHandler)
        : netHandlerPlayClient(netHandler)
    {
    }

    void initGui() override { buttonList.clear(); }
    void updateScreen() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    bool doesGuiPauseGame() const override { return false; }

protected:
    void keyTyped(char16_t, int) override { }

private:
    NetHandlerPlayClient* netHandlerPlayClient;
    int progress = 0;
};

} // namespace mc
