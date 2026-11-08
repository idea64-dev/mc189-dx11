#include "client/gui/GuiScreen.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/settings/GameSettings.h"
#include "core/StringUtil.h"
#include "platform/Keyboard.h"
#include "platform/Mouse.h"

#include <Windows.h>

namespace mc {

void GuiScreen::drawScreen(int mouseX, int mouseY, float /*partialTicks*/)
{
    // Index loop like Java: a button's draw may not change the list, but keep the original's shape.
    for (size_t i = 0; i < buttonList.size(); ++i)
        buttonList[i]->drawButton(*mc, mouseX, mouseY);
    for (size_t j = 0; j < labelList.size(); ++j)
        labelList[j]->drawLabel(*mc, mouseX, mouseY);
}

void GuiScreen::keyTyped(char16_t /*typedChar*/, int keyCode)
{
    if (keyCode == 1) {
        mc->displayGuiScreen(nullptr);
        if (mc->currentScreen == nullptr)
            mc->setIngameFocus();
    }
}

std::string GuiScreen::getClipboardString()
{
    std::string result;
    if (!OpenClipboard(nullptr))
        return result;
    if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
        if (const auto* text = static_cast<const wchar_t*>(GlobalLock(data))) {
            result = toUtf8(text);
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return result;
}

void GuiScreen::setClipboardString(const std::string& copyText)
{
    if (copyText.empty() || !OpenClipboard(nullptr))
        return;
    const std::wstring wide = toWide(copyText);
    if (HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, (wide.size() + 1) * sizeof(wchar_t))) {
        if (auto* target = static_cast<wchar_t*>(GlobalLock(memory))) {
            std::copy(wide.c_str(), wide.c_str() + wide.size() + 1, target);
            GlobalUnlock(memory);
            EmptyClipboard();
            if (!SetClipboardData(CF_UNICODETEXT, memory))
                GlobalFree(memory);
        } else {
            GlobalFree(memory);
        }
    }
    CloseClipboard();
}

void GuiScreen::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    if (mouseButton == 0) {
        for (size_t i = 0; i < buttonList.size(); ++i) {
            GuiButton& guibutton = *buttonList[i];
            if (guibutton.mousePressed(*mc, mouseX, mouseY)) {
                selectedButton = &guibutton;
                guibutton.playPressSound(mc->getSoundHandler());
                actionPerformed(guibutton);
                // actionPerformed may replace this screen and reinitialize the list (Java keeps iterating the
                // same, now possibly cleared, list); stop if the button vanished.
                if (i >= buttonList.size() || buttonList[i].get() != &guibutton)
                    break;
            }
        }
    }
}

void GuiScreen::mouseReleased(int mouseX, int mouseY, int state)
{
    if (selectedButton != nullptr && state == 0) {
        selectedButton->mouseReleased(mouseX, mouseY);
        selectedButton = nullptr;
    }
}

void GuiScreen::mouseClickMove(int /*mouseX*/, int /*mouseY*/, int /*clickedMouseButton*/, int64_t /*timeSinceLastClick*/)
{
}

void GuiScreen::actionPerformed(GuiButton& /*button*/)
{
}

void GuiScreen::setWorldAndResolution(Minecraft& mcIn, int widthIn, int heightIn)
{
    mc = &mcIn;
    // Java: itemRender = mc.getRenderItem() (stage 10)
    fontRendererObj = mcIn.fontRendererObj.get();
    width = widthIn;
    height = heightIn;
    buttonList.clear();
    selectedButton = nullptr;
    initGui();
}

void GuiScreen::setGuiSize(int w, int h)
{
    width = w;
    height = h;
}

void GuiScreen::initGui()
{
}

void GuiScreen::handleInput()
{
    // Hold a reference: a handler may replace this screen.
    const std::shared_ptr<GuiScreen> self = shared_from_this();
    while (Mouse::next())
        handleMouseInput();
    while (Keyboard::next())
        handleKeyboardInput();
}

void GuiScreen::handleMouseInput()
{
    const int i = Mouse::getEventX() * width / mc->displayWidth;
    const int j = height - Mouse::getEventY() * height / mc->displayHeight - 1;
    const int k = Mouse::getEventButton();

    if (Mouse::getEventButtonState()) {
        if (mc->gameSettings->touchscreen && touchValue++ > 0)
            return;
        eventButton = k;
        lastMouseEvent = Minecraft::getSystemTime();
        mouseClicked(i, j, eventButton);
    } else if (k != -1) {
        if (mc->gameSettings->touchscreen && --touchValue > 0)
            return;
        eventButton = -1;
        mouseReleased(i, j, k);
    } else if (eventButton != -1 && lastMouseEvent > 0) {
        const int64_t l = Minecraft::getSystemTime() - lastMouseEvent;
        mouseClickMove(i, j, eventButton, l);
    }
}

void GuiScreen::handleKeyboardInput()
{
    if (Keyboard::getEventKeyState())
        keyTyped(Keyboard::getEventCharacter(), Keyboard::getEventKey());
    mc->dispatchKeypresses();
}

void GuiScreen::updateScreen()
{
}

void GuiScreen::onGuiClosed()
{
}

void GuiScreen::drawDefaultBackground()
{
    drawWorldBackground(0);
}

void GuiScreen::drawWorldBackground(int tint)
{
    if (mc->theWorld != nullptr)
        drawGradientRect(0, 0, width, height, -1072689136, -804253680);
    else
        drawBackground(tint);
}

void GuiScreen::drawBackground(int tint)
{
    DxStateManager::disableLighting();
    DxStateManager::disableFog();
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    mc->getTextureManager().bindTexture(optionsBackground);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(0.0, height, 0.0).tex(0.0, static_cast<float>(height) / 32.0f + static_cast<float>(tint)).color(64, 64, 64, 255).endVertex();
    worldrenderer.pos(width, height, 0.0).tex(static_cast<float>(width) / 32.0f, static_cast<float>(height) / 32.0f + static_cast<float>(tint)).color(64, 64, 64, 255).endVertex();
    worldrenderer.pos(width, 0.0, 0.0).tex(static_cast<float>(width) / 32.0f, static_cast<float>(tint)).color(64, 64, 64, 255).endVertex();
    worldrenderer.pos(0.0, 0.0, 0.0).tex(0.0, static_cast<float>(tint)).color(64, 64, 64, 255).endVertex();
    tessellator.draw();
}

void GuiScreen::confirmClicked(bool /*result*/, int /*id*/)
{
    // Java: id 31102009 opens a chat link confirmed in GuiConfirmOpenLink (stage 8).
}

void GuiScreen::onResize(Minecraft& mcIn, int w, int h)
{
    setWorldAndResolution(mcIn, w, h);
}

bool GuiScreen::isCtrlKeyDown()
{
    return Keyboard::isKeyDown(29) || Keyboard::isKeyDown(157);
}

bool GuiScreen::isShiftKeyDown()
{
    return Keyboard::isKeyDown(42) || Keyboard::isKeyDown(54);
}

bool GuiScreen::isAltKeyDown()
{
    return Keyboard::isKeyDown(56) || Keyboard::isKeyDown(184);
}

bool GuiScreen::isKeyComboCtrlX(int keyID)
{
    return keyID == 45 && isCtrlKeyDown() && !isShiftKeyDown() && !isAltKeyDown();
}

bool GuiScreen::isKeyComboCtrlV(int keyID)
{
    return keyID == 47 && isCtrlKeyDown() && !isShiftKeyDown() && !isAltKeyDown();
}

bool GuiScreen::isKeyComboCtrlC(int keyID)
{
    return keyID == 46 && isCtrlKeyDown() && !isShiftKeyDown() && !isAltKeyDown();
}

bool GuiScreen::isKeyComboCtrlA(int keyID)
{
    return keyID == 30 && isCtrlKeyDown() && !isShiftKeyDown() && !isAltKeyDown();
}

} // namespace mc
