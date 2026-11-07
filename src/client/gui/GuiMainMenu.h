#pragma once

// Original: net/minecraft/client/gui/GuiMainMenu.java
// Not ported: the demo mode buttons, the Realms notification overlay (Realms is out of scope) and the
// "OpenGL 2.0 missing" warning, which cannot apply to the Direct3D 11 renderer.

#include "client/gui/GuiScreen.h"
#include "util/ResourceLocation.h"

#include <memory>
#include <optional>
#include <string>

namespace mc {

class DynamicTexture;

class GuiMainMenu : public GuiScreen {
public:
    GuiMainMenu();

    void updateScreen() override;
    bool doesGuiPauseGame() const override { return false; }
    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void confirmClicked(bool result, int id) override;

protected:
    void keyTyped(char16_t typedChar, int keyCode) override;
    void actionPerformed(GuiButton& button) override;

private:
    void addSingleplayerMultiplayerButtons(int y, int spacing);
    void drawPanorama(int mouseX, int mouseY, float partialTicks);
    void rotateAndBlurSkybox(float partialTicks);
    void renderSkybox(int mouseX, int mouseY, float partialTicks);

    float updateCounter;
    std::string splashText;
    int panoramaTimer = 0;
    std::shared_ptr<DynamicTexture> viewportTexture;
    std::optional<ResourceLocation> backgroundTexture;
    GuiButton* realmsButton = nullptr;
};

} // namespace mc
