#include "client/gui/GuiTextField.h"

#include "client/gui/FontRenderer.h"
#include "client/gui/GuiResponder.h"
#include "client/gui/GuiScreen.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "core/StringUtil.h"
#include "util/ChatAllowedCharacters.h"
#include "util/MathHelper.h"

#include <cstdlib>

namespace mc {

GuiTextField::GuiTextField(int componentId, FontRenderer& fontrendererObj, int x, int y, int widthIn, int heightIn)
    : xPosition(x)
    , yPosition(y)
    , id(componentId)
    , fontRendererInstance(fontrendererObj)
    , width(widthIn)
    , height(heightIn)
{
}

std::u16string GuiTextField::toU16(const std::string& utf8)
{
    return toUtf16(utf8);
}

bool GuiTextField::validate(const std::u16string& candidate) const
{
    return !validator || validator(fromUtf16(candidate));
}

std::string GuiTextField::getText() const
{
    return fromUtf16(text);
}

void GuiTextField::setText16(const std::u16string& newText)
{
    if (!validate(newText))
        return;
    if (static_cast<int>(newText.size()) > maxStringLength)
        text = newText.substr(0, static_cast<size_t>(maxStringLength));
    else
        text = newText;
    setCursorPositionEnd();
}

std::string GuiTextField::getSelectedText() const
{
    const int i = cursorPosition < selectionEnd ? cursorPosition : selectionEnd;
    const int j = cursorPosition < selectionEnd ? selectionEnd : cursorPosition;
    return fromUtf16(text.substr(static_cast<size_t>(i), static_cast<size_t>(j - i)));
}

void GuiTextField::writeText16(std::u16string_view textToWrite)
{
    std::u16string s;
    const std::u16string s1 = ChatAllowedCharacters::filterAllowedCharacters(textToWrite);
    const int i = cursorPosition < selectionEnd ? cursorPosition : selectionEnd;
    const int j = cursorPosition < selectionEnd ? selectionEnd : cursorPosition;
    const int k = maxStringLength - static_cast<int>(text.size()) - (i - j);
    int l;
    if (!text.empty())
        s += text.substr(0, static_cast<size_t>(i));
    if (k < static_cast<int>(s1.size())) {
        s += s1.substr(0, static_cast<size_t>(k));
        l = k;
    } else {
        s += s1;
        l = static_cast<int>(s1.size());
    }
    if (!text.empty() && j < static_cast<int>(text.size()))
        s += text.substr(static_cast<size_t>(j));
    if (validate(s)) {
        text = s;
        moveCursorBy(i - selectionEnd + l);
        if (guiResponder)
            guiResponder->onText(id, getText());
    }
}

void GuiTextField::deleteWords(int num)
{
    if (text.empty())
        return;
    if (selectionEnd != cursorPosition)
        writeText16(u"");
    else
        deleteFromCursor(getNthWordFromCursor(num) - cursorPosition);
}

void GuiTextField::deleteFromCursor(int num)
{
    if (text.empty())
        return;
    if (selectionEnd != cursorPosition) {
        writeText16(u"");
        return;
    }
    const bool flag = num < 0;
    const int i = flag ? cursorPosition + num : cursorPosition;
    const int j = flag ? cursorPosition : cursorPosition + num;
    std::u16string s;
    if (i >= 0)
        s = text.substr(0, static_cast<size_t>(i));
    if (j < static_cast<int>(text.size()))
        s += text.substr(static_cast<size_t>(j));
    if (validate(s)) {
        text = s;
        if (flag)
            moveCursorBy(num);
        if (guiResponder)
            guiResponder->onText(id, getText());
    }
}

int GuiTextField::func_146197_a(int n, int pos, bool skipWs) const
{
    int i = pos;
    const bool flag = n < 0;
    const int j = std::abs(n);
    for (int k = 0; k < j; ++k) {
        if (!flag) {
            const int l = static_cast<int>(text.size());
            const size_t found = text.find(u' ', static_cast<size_t>(i));
            if (found == std::u16string::npos) {
                i = l;
            } else {
                i = static_cast<int>(found);
                while (skipWs && i < l && text[static_cast<size_t>(i)] == u' ')
                    ++i;
            }
        } else {
            while (skipWs && i > 0 && text[static_cast<size_t>(i - 1)] == u' ')
                --i;
            while (i > 0 && text[static_cast<size_t>(i - 1)] != u' ')
                --i;
        }
    }
    return i;
}

void GuiTextField::setCursorPosition(int pos)
{
    cursorPosition = MathHelper::clamp_int(pos, 0, static_cast<int>(text.size()));
    setSelectionPos(cursorPosition);
}

bool GuiTextField::textboxKeyTyped(char16_t typedChar, int keyCode)
{
    if (!focused)
        return false;
    if (GuiScreen::isKeyComboCtrlA(keyCode)) {
        setCursorPositionEnd();
        setSelectionPos(0);
        return true;
    }
    if (GuiScreen::isKeyComboCtrlC(keyCode)) {
        GuiScreen::setClipboardString(getSelectedText());
        return true;
    }
    if (GuiScreen::isKeyComboCtrlV(keyCode)) {
        if (isEnabled)
            writeText(GuiScreen::getClipboardString());
        return true;
    }
    if (GuiScreen::isKeyComboCtrlX(keyCode)) {
        GuiScreen::setClipboardString(getSelectedText());
        if (isEnabled)
            writeText16(u"");
        return true;
    }
    switch (keyCode) {
    case 14: // backspace
        if (GuiScreen::isCtrlKeyDown()) {
            if (isEnabled)
                deleteWords(-1);
        } else if (isEnabled) {
            deleteFromCursor(-1);
        }
        return true;
    case 199: // home
        if (GuiScreen::isShiftKeyDown())
            setSelectionPos(0);
        else
            setCursorPositionZero();
        return true;
    case 203: // left
        if (GuiScreen::isShiftKeyDown()) {
            if (GuiScreen::isCtrlKeyDown())
                setSelectionPos(getNthWordFromPos(-1, getSelectionEnd()));
            else
                setSelectionPos(getSelectionEnd() - 1);
        } else if (GuiScreen::isCtrlKeyDown()) {
            setCursorPosition(getNthWordFromCursor(-1));
        } else {
            moveCursorBy(-1);
        }
        return true;
    case 205: // right
        if (GuiScreen::isShiftKeyDown()) {
            if (GuiScreen::isCtrlKeyDown())
                setSelectionPos(getNthWordFromPos(1, getSelectionEnd()));
            else
                setSelectionPos(getSelectionEnd() + 1);
        } else if (GuiScreen::isCtrlKeyDown()) {
            setCursorPosition(getNthWordFromCursor(1));
        } else {
            moveCursorBy(1);
        }
        return true;
    case 207: // end
        if (GuiScreen::isShiftKeyDown())
            setSelectionPos(static_cast<int>(text.size()));
        else
            setCursorPositionEnd();
        return true;
    case 211: // delete
        if (GuiScreen::isCtrlKeyDown()) {
            if (isEnabled)
                deleteWords(1);
        } else if (isEnabled) {
            deleteFromCursor(1);
        }
        return true;
    default:
        if (ChatAllowedCharacters::isAllowedCharacter(typedChar)) {
            if (isEnabled)
                writeText16(std::u16string(1, typedChar));
            return true;
        }
        return false;
    }
}

void GuiTextField::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    const bool flag = mouseX >= xPosition && mouseX < xPosition + width && mouseY >= yPosition && mouseY < yPosition + height;
    if (canLoseFocus)
        setFocused(flag);
    if (focused && flag && mouseButton == 0) {
        int i = mouseX - xPosition;
        if (enableBackgroundDrawing)
            i -= 4;
        const std::u16string s = fontRendererInstance.trimStringToWidth16(std::u16string_view(text).substr(static_cast<size_t>(lineScrollOffset)), getWidth());
        setCursorPosition(static_cast<int>(fontRendererInstance.trimStringToWidth16(s, i).size()) + lineScrollOffset);
    }
}

void GuiTextField::drawTextBox()
{
    if (!getVisible())
        return;
    if (getEnableBackgroundDrawing()) {
        drawRect(xPosition - 1, yPosition - 1, xPosition + width + 1, yPosition + height + 1, -6250336);
        drawRect(xPosition, yPosition, xPosition + width, yPosition + height, -16777216);
    }
    const int i = isEnabled ? enabledColor : disabledColor;
    const int j = cursorPosition - lineScrollOffset;
    int k = selectionEnd - lineScrollOffset;
    const std::u16string s = fontRendererInstance.trimStringToWidth16(std::u16string_view(text).substr(static_cast<size_t>(lineScrollOffset)), getWidth());
    const int length = static_cast<int>(s.size());
    const bool flag = j >= 0 && j <= length;
    const bool flag1 = focused && cursorCounter / 6 % 2 == 0 && flag;
    const int l = enableBackgroundDrawing ? xPosition + 4 : xPosition;
    const int i1 = enableBackgroundDrawing ? yPosition + (height - 8) / 2 : yPosition;
    int j1 = l;
    if (k > length)
        k = length;
    if (length > 0) {
        const std::u16string s1 = flag ? s.substr(0, static_cast<size_t>(j)) : s;
        j1 = fontRendererInstance.drawStringWithShadow16(s1, static_cast<float>(l), static_cast<float>(i1), i);
    }
    const bool flag2 = cursorPosition < static_cast<int>(text.size()) || static_cast<int>(text.size()) >= getMaxStringLength();
    int k1 = j1;
    if (!flag) {
        k1 = j > 0 ? l + width : l;
    } else if (flag2) {
        k1 = j1 - 1;
        --j1;
    }
    if (length > 0 && flag && j < length)
        j1 = fontRendererInstance.drawStringWithShadow16(s.substr(static_cast<size_t>(j)), static_cast<float>(j1), static_cast<float>(i1), i);
    if (flag1) {
        if (flag2)
            Gui::drawRect(k1, i1 - 1, k1 + 1, i1 + 1 + fontRendererInstance.FONT_HEIGHT, -3092272);
        else
            fontRendererInstance.drawStringWithShadow16(u"_", static_cast<float>(k1), static_cast<float>(i1), i);
    }
    if (k != j) {
        const int l1 = l + fontRendererInstance.getStringWidth16(s.substr(0, static_cast<size_t>(k)));
        drawCursorVertical(k1, i1 - 1, l1 - 1, i1 + 1 + fontRendererInstance.FONT_HEIGHT);
    }
}

void GuiTextField::drawCursorVertical(int startX, int startY, int endX, int endY)
{
    if (startX < endX)
        std::swap(startX, endX);
    if (startY < endY)
        std::swap(startY, endY);
    if (endX > xPosition + width)
        endX = xPosition + width;
    if (startX > xPosition + width)
        startX = xPosition + width;
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    DxStateManager::color(0.0f, 0.0f, 255.0f, 255.0f);
    DxStateManager::disableTexture2D();
    DxStateManager::enableColorLogic();
    DxStateManager::colorLogicOp(5387);
    worldrenderer.begin(7, DefaultVertexFormats::POSITION);
    worldrenderer.pos(static_cast<double>(startX), static_cast<double>(endY), 0.0).endVertex();
    worldrenderer.pos(static_cast<double>(endX), static_cast<double>(endY), 0.0).endVertex();
    worldrenderer.pos(static_cast<double>(endX), static_cast<double>(startY), 0.0).endVertex();
    worldrenderer.pos(static_cast<double>(startX), static_cast<double>(startY), 0.0).endVertex();
    tessellator.draw();
    DxStateManager::disableColorLogic();
    DxStateManager::enableTexture2D();
}

void GuiTextField::setMaxStringLength(int length)
{
    maxStringLength = length;
    if (static_cast<int>(text.size()) > length)
        text = text.substr(0, static_cast<size_t>(length));
}

void GuiTextField::setFocused(bool value)
{
    if (value && !focused)
        cursorCounter = 0;
    focused = value;
}

void GuiTextField::setSelectionPos(int pos)
{
    const int i = static_cast<int>(text.size());
    if (pos > i)
        pos = i;
    if (pos < 0)
        pos = 0;
    selectionEnd = pos;
    if (lineScrollOffset > i)
        lineScrollOffset = i;
    const int j = getWidth();
    const std::u16string s = fontRendererInstance.trimStringToWidth16(std::u16string_view(text).substr(static_cast<size_t>(lineScrollOffset)), j);
    const int k = static_cast<int>(s.size()) + lineScrollOffset;
    if (pos == lineScrollOffset)
        lineScrollOffset -= static_cast<int>(fontRendererInstance.trimStringToWidth16(text, j, true).size());
    if (pos > k)
        lineScrollOffset += pos - k;
    else if (pos <= lineScrollOffset)
        lineScrollOffset -= lineScrollOffset - pos;
    lineScrollOffset = MathHelper::clamp_int(lineScrollOffset, 0, i);
}

} // namespace mc
