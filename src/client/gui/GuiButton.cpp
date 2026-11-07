#include "client/gui/GuiButton.h"

#include "client/Minecraft.h"
#include "client/audio/SoundHandler.h"
#include "client/gui/FontRenderer.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/texture/TextureManager.h"

namespace mc {

GuiButton::GuiButton(int buttonId, int x, int y, std::string buttonText)
    : GuiButton(buttonId, x, y, 200, 20, std::move(buttonText))
{
}

GuiButton::GuiButton(int buttonId, int x, int y, int widthIn, int heightIn, std::string buttonText)
    : xPosition(x)
    , yPosition(y)
    , displayString(std::move(buttonText))
    , id(buttonId)
    , width(widthIn)
    , height(heightIn)
{
}

int GuiButton::getHoverState(bool mouseOver) const
{
    int i = 1;
    if (!enabled)
        i = 0;
    else if (mouseOver)
        i = 2;
    return i;
}

void GuiButton::drawButton(Minecraft& mc, int mouseX, int mouseY)
{
    if (!visible)
        return;

    FontRenderer& fontrenderer = *mc.fontRendererObj;
    mc.getTextureManager().bindTexture(buttonTextures);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    hovered = mouseX >= xPosition && mouseY >= yPosition && mouseX < xPosition + width && mouseY < yPosition + height;
    const int i = getHoverState(hovered);
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::blendFunc(770, 771);
    drawTexturedModalRect(xPosition, yPosition, 0, 46 + i * 20, width / 2, height);
    drawTexturedModalRect(xPosition + width / 2, yPosition, 200 - width / 2, 46 + i * 20, width / 2, height);
    mouseDragged(mc, mouseX, mouseY);
    int j = 14737632;
    if (!enabled)
        j = 10526880;
    else if (hovered)
        j = 16777120;
    drawCenteredString(fontrenderer, displayString, xPosition + width / 2, yPosition + (height - 8) / 2, j);
}

void GuiButton::mouseDragged(Minecraft& /*mc*/, int /*mouseX*/, int /*mouseY*/)
{
}

void GuiButton::mouseReleased(int /*mouseX*/, int /*mouseY*/)
{
}

bool GuiButton::mousePressed(Minecraft& /*mc*/, int mouseX, int mouseY)
{
    return enabled && visible && mouseX >= xPosition && mouseY >= yPosition && mouseX < xPosition + width && mouseY < yPosition + height;
}

void GuiButton::drawButtonForegroundLayer(int /*mouseX*/, int /*mouseY*/)
{
}

void GuiButton::playPressSound(SoundHandler& soundHandlerIn)
{
    soundHandlerIn.playSound(PositionedSoundRecord::create(ResourceLocation("gui.button.press"), 1.0f));
}

GuiButtonLanguage::GuiButtonLanguage(int buttonId, int xPos, int yPos)
    : GuiButton(buttonId, xPos, yPos, 20, 20, "")
{
}

void GuiButtonLanguage::drawButton(Minecraft& mc, int mouseX, int mouseY)
{
    if (!visible)
        return;

    mc.getTextureManager().bindTexture(GuiButton::buttonTextures);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    const bool flag = mouseX >= xPosition && mouseY >= yPosition && mouseX < xPosition + width && mouseY < yPosition + height;
    int i = 106;
    if (flag)
        i += height;
    drawTexturedModalRect(xPosition, yPosition, 0, i, width, height);
}

} // namespace mc
