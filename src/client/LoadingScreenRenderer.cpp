#include "client/LoadingScreenRenderer.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/Gui.h"
#include "client/gui/ScaledResolution.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/shader/Framebuffer.h"

#include <thread>

namespace mc {

LoadingScreenRenderer::LoadingScreenRenderer(Minecraft& mcIn)
    : mc(mcIn)
    , systemTime(Minecraft::getSystemTime())
    , framebuffer(std::make_unique<Framebuffer>(mcIn.displayWidth, mcIn.displayHeight, false))
{
    framebuffer->setFramebufferFilter(9728);
}

LoadingScreenRenderer::~LoadingScreenRenderer() = default;

void LoadingScreenRenderer::resetProgressAndMessage(const std::string& messageIn)
{
    loadingSuccess = false;
    displayString(messageIn);
}

void LoadingScreenRenderer::displaySavingString(const std::string& messageIn)
{
    loadingSuccess = true;
    displayString(messageIn);
}

void LoadingScreenRenderer::displayString(const std::string& messageIn)
{
    currentlyDisplayedText = messageIn;
    // Java throws MinecraftError when the game is closing and the screen isn't a saving one; nothing to stop here.
    if (!mc.isRunning())
        return;

    const ScaledResolution scaledResolution(mc);
    DxStateManager::clear(256);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    const int i = scaledResolution.getScaleFactor();
    DxStateManager::ortho(0.0, static_cast<double>(scaledResolution.getScaledWidth() * i), static_cast<double>(scaledResolution.getScaledHeight() * i), 0.0,
        100.0, 300.0);
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();
    DxStateManager::translate(0.0f, 0.0f, -200.0f);
}

void LoadingScreenRenderer::displayLoadingString(const std::string& messageIn)
{
    if (!mc.isRunning())
        return;
    systemTime = 0;
    message = messageIn;
    setLoadingProgress(-1);
    systemTime = 0;
}

void LoadingScreenRenderer::setLoadingProgress(int progress)
{
    if (!mc.isRunning())
        return;

    const int64_t i = Minecraft::getSystemTime();
    if (i - systemTime < 100)
        return;

    systemTime = i;
    const ScaledResolution scaledresolution(mc);
    const int j = scaledresolution.getScaleFactor();
    const int k = scaledresolution.getScaledWidth();
    const int l = scaledresolution.getScaledHeight();
    framebuffer->framebufferClear();
    framebuffer->bindFramebuffer(false);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    DxStateManager::ortho(0.0, scaledresolution.getScaledWidth_double(), scaledresolution.getScaledHeight_double(), 0.0, 100.0, 300.0);
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();
    DxStateManager::translate(0.0f, 0.0f, -200.0f);

    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    mc.getTextureManager().bindTexture(Gui::optionsBackground);
    const float f = 32.0f;
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(0.0, l, 0.0).tex(0.0, static_cast<double>(static_cast<float>(l) / f)).color(64, 64, 64, 255).endVertex();
    worldrenderer.pos(k, l, 0.0).tex(static_cast<double>(static_cast<float>(k) / f), static_cast<double>(static_cast<float>(l) / f)).color(64, 64, 64, 255).endVertex();
    worldrenderer.pos(k, 0.0, 0.0).tex(static_cast<double>(static_cast<float>(k) / f), 0.0).color(64, 64, 64, 255).endVertex();
    worldrenderer.pos(0.0, 0.0, 0.0).tex(0.0, 0.0).color(64, 64, 64, 255).endVertex();
    tessellator.draw();

    if (progress >= 0) {
        const int i1 = 100;
        const int j1 = 2;
        const int k1 = k / 2 - i1 / 2;
        const int l1 = l / 2 + 16;
        DxStateManager::disableTexture2D();
        worldrenderer.begin(7, DefaultVertexFormats::POSITION_COLOR);
        worldrenderer.pos(k1, l1, 0.0).color(128, 128, 128, 255).endVertex();
        worldrenderer.pos(k1, l1 + j1, 0.0).color(128, 128, 128, 255).endVertex();
        worldrenderer.pos(k1 + i1, l1 + j1, 0.0).color(128, 128, 128, 255).endVertex();
        worldrenderer.pos(k1 + i1, l1, 0.0).color(128, 128, 128, 255).endVertex();
        worldrenderer.pos(k1, l1, 0.0).color(128, 255, 128, 255).endVertex();
        worldrenderer.pos(k1, l1 + j1, 0.0).color(128, 255, 128, 255).endVertex();
        worldrenderer.pos(k1 + progress, l1 + j1, 0.0).color(128, 255, 128, 255).endVertex();
        worldrenderer.pos(k1 + progress, l1, 0.0).color(128, 255, 128, 255).endVertex();
        tessellator.draw();
        DxStateManager::enableTexture2D();
    }

    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    FontRenderer& font = *mc.fontRendererObj;
    font.drawStringWithShadow(currentlyDisplayedText, static_cast<float>((k - font.getStringWidth(currentlyDisplayedText)) / 2), static_cast<float>(l / 2 - 4 - 16),
        16777215);
    font.drawStringWithShadow(message, static_cast<float>((k - font.getStringWidth(message)) / 2), static_cast<float>(l / 2 - 4 + 8), 16777215);
    framebuffer->unbindFramebuffer();
    framebuffer->framebufferRender(k * j, l * j);
    mc.updateDisplay();
    std::this_thread::yield();
}

} // namespace mc
