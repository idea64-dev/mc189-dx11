#include "client/renderer/EntityRenderer.h"

#include "client/Minecraft.h"
#include "client/gui/GuiScreen.h"
#include "client/gui/ScaledResolution.h"
#include "client/renderer/DxStateManager.h"
#include "client/settings/GameSettings.h"
#include "platform/Display.h"
#include "platform/Mouse.h"
#include "platform/Sys.h"

namespace mc {

EntityRenderer::EntityRenderer(Minecraft& minecraft)
    : mc(minecraft)
    , prevFrameTime(Minecraft::getSystemTime())
{
}

void EntityRenderer::updateCameraAndRender(float partialTicks, int64_t /*nanoTime*/)
{
    const bool active = Display::isActive();
    if (!active && mc.gameSettings->pauseOnLostFocus && (!mc.gameSettings->touchscreen || !Mouse::isButtonDown(1))) {
        if (Minecraft::getSystemTime() - prevFrameTime > 500)
            mc.displayInGameMenu();
    } else {
        prevFrameTime = Minecraft::getSystemTime();
    }

    // Java: mouse look and smooth camera when in game (stage 8).

    if (mc.skipRenderWorld)
        return;

    const ScaledResolution scaledresolution(mc);
    const int i1 = scaledresolution.getScaledWidth();
    const int j1 = scaledresolution.getScaledHeight();
    const int k1 = Mouse::getX() * i1 / mc.displayWidth;
    const int l1 = j1 - Mouse::getY() * j1 / mc.displayHeight - 1;

    if (mc.theWorld != nullptr) {
        // Java: renderWorld + ingameGUI.renderGameOverlay (stage 7/8)
    } else {
        DxStateManager::viewport(0, 0, mc.displayWidth, mc.displayHeight);
        DxStateManager::matrixMode(5889);
        DxStateManager::loadIdentity();
        DxStateManager::matrixMode(5888);
        DxStateManager::loadIdentity();
        setupOverlayRendering();
        renderEndNanoTime = Sys::nanoTime();
    }

    if (const std::shared_ptr<GuiScreen> screen = mc.currentScreen) {
        DxStateManager::clear(256);
        screen->drawScreen(k1, l1, partialTicks);
    }
}

void EntityRenderer::setupOverlayRendering()
{
    const ScaledResolution scaledresolution(mc);
    DxStateManager::clear(256);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    DxStateManager::ortho(0.0, scaledresolution.getScaledWidth_double(), scaledresolution.getScaledHeight_double(), 0.0, 1000.0, 3000.0);
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();
    DxStateManager::translate(0.0f, 0.0f, -2000.0f);
}

} // namespace mc
