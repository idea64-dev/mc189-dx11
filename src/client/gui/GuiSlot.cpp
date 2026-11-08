#include "client/gui/GuiSlot.h"

#include "client/Minecraft.h"
#include "client/gui/Gui.h"
#include "client/gui/GuiButton.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "core/Java.h"
#include "platform/Mouse.h"
#include "util/MathHelper.h"

#include <algorithm>

namespace mc {

GuiSlot::GuiSlot(Minecraft& mcIn, int widthIn, int heightIn, int topIn, int bottomIn, int slotHeightIn)
    : mc(mcIn)
    , width(widthIn)
    , height(heightIn)
    , top(topIn)
    , bottom(bottomIn)
    , right(widthIn)
    , left(0)
    , slotHeight(slotHeightIn)
{
}

void GuiSlot::setDimensions(int widthIn, int heightIn, int topIn, int bottomIn)
{
    width = widthIn;
    height = heightIn;
    top = topIn;
    bottom = bottomIn;
    left = 0;
    right = widthIn;
}

void GuiSlot::setHasListHeader(bool hasListHeaderIn, int headerPaddingIn)
{
    hasListHeader = hasListHeaderIn;
    headerPadding = headerPaddingIn;
    if (!hasListHeaderIn)
        headerPadding = 0;
}

int GuiSlot::getContentHeight() const
{
    return getSize() * slotHeight + headerPadding;
}

void GuiSlot::updateItemPos(int /*entryID*/, int /*insideLeft*/, int /*yPos*/)
{
}

void GuiSlot::drawListHeader(int /*insideLeft*/, int /*insideTop*/, Tessellator& /*tessellator*/)
{
}

void GuiSlot::clickedHeader(int /*x*/, int /*y*/)
{
}

void GuiSlot::renderDecorations(int /*mouseXIn*/, int /*mouseYIn*/)
{
}

int GuiSlot::getSlotIndexFromScreenCoords(int x, int y) const
{
    const int i = left + width / 2 - getListWidth() / 2;
    const int j = left + width / 2 + getListWidth() / 2;
    const int k = y - top - headerPadding + java::f2i(amountScrolled) - 4;
    const int l = k / slotHeight;
    return x < getScrollBarX() && x >= i && x <= j && l >= 0 && k >= 0 && l < getSize() ? l : -1;
}

void GuiSlot::registerScrollButtons(int scrollUpButtonIDIn, int scrollDownButtonIDIn)
{
    scrollUpButtonID = scrollUpButtonIDIn;
    scrollDownButtonID = scrollDownButtonIDIn;
}

void GuiSlot::bindAmountScrolled()
{
    amountScrolled = MathHelper::clamp_float(amountScrolled, 0.0f, static_cast<float>(getMaxScroll()));
}

int GuiSlot::getMaxScroll() const
{
    return std::max(0, getContentHeight() - (bottom - top - 4));
}

int GuiSlot::getAmountScrolled() const
{
    return java::f2i(amountScrolled);
}

bool GuiSlot::isMouseYWithinSlotBounds(int y) const
{
    return y >= top && y <= bottom && mouseX >= left && mouseX <= right;
}

void GuiSlot::scrollBy(int amount)
{
    amountScrolled += static_cast<float>(amount);
    bindAmountScrolled();
    initialClickY = -2;
}

void GuiSlot::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;

    if (button.id == scrollUpButtonID) {
        amountScrolled -= static_cast<float>(slotHeight * 2 / 3);
        initialClickY = -2;
        bindAmountScrolled();
    } else if (button.id == scrollDownButtonID) {
        amountScrolled += static_cast<float>(slotHeight * 2 / 3);
        initialClickY = -2;
        bindAmountScrolled();
    }
}

void GuiSlot::drawScreen(int mouseXIn, int mouseYIn, float /*partialTicks*/)
{
    if (!visible)
        return;

    mouseX = mouseXIn;
    mouseY = mouseYIn;
    drawBackground();
    const int i = getScrollBarX();
    const int j = i + 6;
    bindAmountScrolled();
    DxStateManager::disableLighting();
    DxStateManager::disableFog();
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    mc.getTextureManager().bindTexture(Gui::optionsBackground);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    const float f = 32.0f;
    const int scrolled = java::f2i(amountScrolled);
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(left, bottom, 0.0).tex(static_cast<float>(left) / f, static_cast<float>(bottom + scrolled) / f).color(32, 32, 32, 255).endVertex();
    worldrenderer.pos(right, bottom, 0.0).tex(static_cast<float>(right) / f, static_cast<float>(bottom + scrolled) / f).color(32, 32, 32, 255).endVertex();
    worldrenderer.pos(right, top, 0.0).tex(static_cast<float>(right) / f, static_cast<float>(top + scrolled) / f).color(32, 32, 32, 255).endVertex();
    worldrenderer.pos(left, top, 0.0).tex(static_cast<float>(left) / f, static_cast<float>(top + scrolled) / f).color(32, 32, 32, 255).endVertex();
    tessellator.draw();

    const int k = left + width / 2 - getListWidth() / 2 + 2;
    const int l = top + 4 - java::f2i(amountScrolled);
    if (hasListHeader)
        drawListHeader(k, l, tessellator);
    drawSelectionBox(k, l, mouseXIn, mouseYIn);

    DxStateManager::disableDepth();
    const int i1 = 4;
    overlayBackground(0, top, 255, 255);
    overlayBackground(bottom, height, 255, 255);
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 0, 1);
    DxStateManager::disableAlpha();
    DxStateManager::shadeModel(7425);
    DxStateManager::disableTexture2D();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(left, top + i1, 0.0).tex(0.0, 1.0).color(0, 0, 0, 0).endVertex();
    worldrenderer.pos(right, top + i1, 0.0).tex(1.0, 1.0).color(0, 0, 0, 0).endVertex();
    worldrenderer.pos(right, top, 0.0).tex(1.0, 0.0).color(0, 0, 0, 255).endVertex();
    worldrenderer.pos(left, top, 0.0).tex(0.0, 0.0).color(0, 0, 0, 255).endVertex();
    tessellator.draw();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(left, bottom, 0.0).tex(0.0, 1.0).color(0, 0, 0, 255).endVertex();
    worldrenderer.pos(right, bottom, 0.0).tex(1.0, 1.0).color(0, 0, 0, 255).endVertex();
    worldrenderer.pos(right, bottom - i1, 0.0).tex(1.0, 0.0).color(0, 0, 0, 0).endVertex();
    worldrenderer.pos(left, bottom - i1, 0.0).tex(0.0, 0.0).color(0, 0, 0, 0).endVertex();
    tessellator.draw();

    const int j1 = getMaxScroll();
    if (j1 > 0) {
        int k1 = (bottom - top) * (bottom - top) / getContentHeight();
        k1 = MathHelper::clamp_int(k1, 32, bottom - top - 8);
        int l1 = java::f2i(amountScrolled) * (bottom - top - k1) / j1 + top;
        if (l1 < top)
            l1 = top;

        worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
        worldrenderer.pos(i, bottom, 0.0).tex(0.0, 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(j, bottom, 0.0).tex(1.0, 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(j, top, 0.0).tex(1.0, 0.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(i, top, 0.0).tex(0.0, 0.0).color(0, 0, 0, 255).endVertex();
        tessellator.draw();
        worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
        worldrenderer.pos(i, l1 + k1, 0.0).tex(0.0, 1.0).color(128, 128, 128, 255).endVertex();
        worldrenderer.pos(j, l1 + k1, 0.0).tex(1.0, 1.0).color(128, 128, 128, 255).endVertex();
        worldrenderer.pos(j, l1, 0.0).tex(1.0, 0.0).color(128, 128, 128, 255).endVertex();
        worldrenderer.pos(i, l1, 0.0).tex(0.0, 0.0).color(128, 128, 128, 255).endVertex();
        tessellator.draw();
        worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
        worldrenderer.pos(i, l1 + k1 - 1, 0.0).tex(0.0, 1.0).color(192, 192, 192, 255).endVertex();
        worldrenderer.pos(j - 1, l1 + k1 - 1, 0.0).tex(1.0, 1.0).color(192, 192, 192, 255).endVertex();
        worldrenderer.pos(j - 1, l1, 0.0).tex(1.0, 0.0).color(192, 192, 192, 255).endVertex();
        worldrenderer.pos(i, l1, 0.0).tex(0.0, 0.0).color(192, 192, 192, 255).endVertex();
        tessellator.draw();
    }

    renderDecorations(mouseXIn, mouseYIn);
    DxStateManager::enableTexture2D();
    DxStateManager::shadeModel(7424);
    DxStateManager::enableAlpha();
    DxStateManager::disableBlend();
}

void GuiSlot::handleMouseInput()
{
    if (!isMouseYWithinSlotBounds(mouseY))
        return;

    if (Mouse::getEventButton() == 0 && Mouse::getEventButtonState() && mouseY >= top && mouseY <= bottom) {
        const int i = (width - getListWidth()) / 2;
        const int j = (width + getListWidth()) / 2;
        const int k = mouseY - top - headerPadding + java::f2i(amountScrolled) - 4;
        const int l = k / slotHeight;

        if (l < getSize() && mouseX >= i && mouseX <= j && l >= 0 && k >= 0) {
            elementClicked(l, false, mouseX, mouseY);
            selectedElement = l;
        } else if (mouseX >= i && mouseX <= j && k < 0) {
            clickedHeader(mouseX - i, mouseY - top + java::f2i(amountScrolled) - 4);
        }
    }

    if (Mouse::isButtonDown(0) && getEnabled()) {
        if (initialClickY == -1) {
            bool flag1 = true;

            if (mouseY >= top && mouseY <= bottom) {
                const int j2 = (width - getListWidth()) / 2;
                const int k2 = (width + getListWidth()) / 2;
                const int l2 = mouseY - top - headerPadding + java::f2i(amountScrolled) - 4;
                const int i1 = l2 / slotHeight;

                if (i1 < getSize() && mouseX >= j2 && mouseX <= k2 && i1 >= 0 && l2 >= 0) {
                    const bool flag = i1 == selectedElement && Minecraft::getSystemTime() - lastClicked < 250LL;
                    elementClicked(i1, flag, mouseX, mouseY);
                    selectedElement = i1;
                    lastClicked = Minecraft::getSystemTime();
                } else if (mouseX >= j2 && mouseX <= k2 && l2 < 0) {
                    clickedHeader(mouseX - j2, mouseY - top + java::f2i(amountScrolled) - 4);
                    flag1 = false;
                }

                const int i3 = getScrollBarX();
                const int j1 = i3 + 6;
                if (mouseX >= i3 && mouseX <= j1) {
                    scrollMultiplier = -1.0f;
                    int k1 = getMaxScroll();
                    if (k1 < 1)
                        k1 = 1;
                    int l1 = java::f2i(static_cast<float>((bottom - top) * (bottom - top)) / static_cast<float>(getContentHeight()));
                    l1 = MathHelper::clamp_int(l1, 32, bottom - top - 8);
                    scrollMultiplier /= static_cast<float>(bottom - top - l1) / static_cast<float>(k1);
                } else {
                    scrollMultiplier = 1.0f;
                }

                initialClickY = flag1 ? mouseY : -2;
            } else {
                initialClickY = -2;
            }
        } else if (initialClickY >= 0) {
            amountScrolled -= static_cast<float>(mouseY - initialClickY) * scrollMultiplier;
            initialClickY = mouseY;
        }
    } else {
        initialClickY = -1;
    }

    int i2 = Mouse::getEventDWheel();
    if (i2 != 0) {
        if (i2 > 0)
            i2 = -1;
        else if (i2 < 0)
            i2 = 1;
        amountScrolled += static_cast<float>(i2 * slotHeight / 2);
    }
}

void GuiSlot::setSlotXBoundsFromLeft(int leftIn)
{
    left = leftIn;
    right = leftIn + width;
}

void GuiSlot::drawSelectionBox(int insideLeft, int insideTop, int mouseXIn, int mouseYIn)
{
    const int i = getSize();
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();

    for (int j = 0; j < i; ++j) {
        const int k = insideTop + j * slotHeight + headerPadding;
        const int l = slotHeight - 4;

        if (k > bottom || k + l < top)
            updateItemPos(j, insideLeft, k);

        if (showSelectionBox && isSelected(j)) {
            const int i1 = left + (width / 2 - getListWidth() / 2);
            const int j1 = left + width / 2 + getListWidth() / 2;
            DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
            DxStateManager::disableTexture2D();
            worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
            worldrenderer.pos(i1, k + l + 2, 0.0).tex(0.0, 1.0).color(128, 128, 128, 255).endVertex();
            worldrenderer.pos(j1, k + l + 2, 0.0).tex(1.0, 1.0).color(128, 128, 128, 255).endVertex();
            worldrenderer.pos(j1, k - 2, 0.0).tex(1.0, 0.0).color(128, 128, 128, 255).endVertex();
            worldrenderer.pos(i1, k - 2, 0.0).tex(0.0, 0.0).color(128, 128, 128, 255).endVertex();
            worldrenderer.pos(i1 + 1, k + l + 1, 0.0).tex(0.0, 1.0).color(0, 0, 0, 255).endVertex();
            worldrenderer.pos(j1 - 1, k + l + 1, 0.0).tex(1.0, 1.0).color(0, 0, 0, 255).endVertex();
            worldrenderer.pos(j1 - 1, k - 1, 0.0).tex(1.0, 0.0).color(0, 0, 0, 255).endVertex();
            worldrenderer.pos(i1 + 1, k - 1, 0.0).tex(0.0, 0.0).color(0, 0, 0, 255).endVertex();
            tessellator.draw();
            DxStateManager::enableTexture2D();
        }

        drawSlot(j, insideLeft, k, l, mouseXIn, mouseYIn);
    }
}

void GuiSlot::overlayBackground(int startY, int endY, int startAlpha, int endAlpha)
{
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    mc.getTextureManager().bindTexture(Gui::optionsBackground);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(left, endY, 0.0).tex(0.0, static_cast<float>(endY) / 32.0f).color(64, 64, 64, endAlpha).endVertex();
    worldrenderer.pos(left + width, endY, 0.0).tex(static_cast<float>(width) / 32.0f, static_cast<float>(endY) / 32.0f).color(64, 64, 64, endAlpha).endVertex();
    worldrenderer.pos(left + width, startY, 0.0).tex(static_cast<float>(width) / 32.0f, static_cast<float>(startY) / 32.0f).color(64, 64, 64, startAlpha).endVertex();
    worldrenderer.pos(left, startY, 0.0).tex(0.0, static_cast<float>(startY) / 32.0f).color(64, 64, 64, startAlpha).endVertex();
    tessellator.draw();
}

void GuiListExtended::elementClicked(int /*slotIndex*/, bool /*isDoubleClick*/, int /*mouseX*/, int /*mouseY*/)
{
}

bool GuiListExtended::isSelected(int /*slotIndex*/) const
{
    return false;
}

void GuiListExtended::drawBackground()
{
}

void GuiListExtended::drawSlot(int entryID, int insideLeft, int yPos, int insideSlotHeight, int mouseXIn, int mouseYIn)
{
    getListEntry(entryID).drawEntry(entryID, insideLeft, yPos, getListWidth(), insideSlotHeight, mouseXIn, mouseYIn, getSlotIndexFromScreenCoords(mouseXIn, mouseYIn) == entryID);
}

void GuiListExtended::updateItemPos(int entryID, int insideLeft, int yPos)
{
    getListEntry(entryID).setSelected(entryID, insideLeft, yPos);
}

bool GuiListExtended::mouseClicked(int x, int y, int mouseEvent)
{
    if (isMouseYWithinSlotBounds(y)) {
        const int i = getSlotIndexFromScreenCoords(x, y);
        if (i >= 0) {
            const int j = left + width / 2 - getListWidth() / 2 + 2;
            const int k = top + 4 - getAmountScrolled() + i * slotHeight + headerPadding;
            const int l = x - j;
            const int i1 = y - k;
            if (getListEntry(i).mousePressed(i, x, y, mouseEvent, l, i1)) {
                setEnabled(false);
                return true;
            }
        }
    }
    return false;
}

bool GuiListExtended::mouseReleased(int x, int y, int mouseEvent)
{
    for (int i = 0; i < getSize(); ++i) {
        const int j = left + width / 2 - getListWidth() / 2 + 2;
        const int k = top + 4 - getAmountScrolled() + i * slotHeight + headerPadding;
        const int l = x - j;
        const int i1 = y - k;
        getListEntry(i).mouseReleased(i, x, y, mouseEvent, l, i1);
    }
    setEnabled(true);
    return false;
}

} // namespace mc
