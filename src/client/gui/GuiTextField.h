#pragma once

// Original: net/minecraft/client/gui/GuiTextField.java
// The text is kept in UTF-16 so cursor and selection indices mean what they mean in Java (one per char); the
// UTF-8 accessors convert.

#include "client/gui/Gui.h"

#include <functional>
#include <string>
#include <string_view>

namespace mc {

class FontRenderer;
class GuiResponder;

class GuiTextField : public Gui {
public:
    GuiTextField(int componentId, FontRenderer& fontrendererObj, int x, int y, int widthIn, int heightIn);

    // Java: func_175207_a
    void setGuiResponder(GuiResponder* responder) { guiResponder = responder; }
    void updateCursorCounter() { ++cursorCounter; }
    void setText(const std::string& newText) { setText16(toU16(newText)); }
    void setText16(const std::u16string& newText);
    std::string getText() const;
    const std::u16string& getText16() const { return text; }
    std::string getSelectedText() const;
    // Java: Predicate<String>; called with the whole new text.
    void setValidator(std::function<bool(const std::string&)> theValidator) { validator = std::move(theValidator); }
    void writeText(const std::string& textToWrite) { writeText16(toU16(textToWrite)); }
    void writeText16(std::u16string_view textToWrite);
    void deleteWords(int num);
    void deleteFromCursor(int num);
    int getId() const { return id; }
    int getNthWordFromCursor(int numWords) const { return getNthWordFromPos(numWords, getCursorPosition()); }
    int getNthWordFromPos(int n, int pos) const { return func_146197_a(n, pos, true); }
    int func_146197_a(int n, int pos, bool skipWs) const;
    void moveCursorBy(int num) { setCursorPosition(selectionEnd + num); }
    void setCursorPosition(int pos);
    void setCursorPositionZero() { setCursorPosition(0); }
    void setCursorPositionEnd() { setCursorPosition(static_cast<int>(text.size())); }
    // typedChar is the UTF-16 unit, keyCode the LWJGL key.
    bool textboxKeyTyped(char16_t typedChar, int keyCode);
    void mouseClicked(int mouseX, int mouseY, int mouseButton);
    void drawTextBox();
    void setMaxStringLength(int length);
    int getMaxStringLength() const { return maxStringLength; }
    int getCursorPosition() const { return cursorPosition; }
    bool getEnableBackgroundDrawing() const { return enableBackgroundDrawing; }
    void setEnableBackgroundDrawing(bool value) { enableBackgroundDrawing = value; }
    void setTextColor(int color) { enabledColor = color; }
    void setDisabledTextColour(int color) { disabledColor = color; }
    void setFocused(bool focused);
    bool isFocused() const { return focused; }
    void setEnabled(bool value) { isEnabled = value; }
    int getSelectionEnd() const { return selectionEnd; }
    int getWidth() const { return getEnableBackgroundDrawing() ? width - 8 : width; }
    void setSelectionPos(int pos);
    void setCanLoseFocus(bool value) { canLoseFocus = value; }
    bool getVisible() const { return visible; }
    void setVisible(bool value) { visible = value; }

    int xPosition;
    int yPosition;

private:
    static std::u16string toU16(const std::string& utf8);
    bool validate(const std::u16string& candidate) const;
    void drawCursorVertical(int startX, int startY, int endX, int endY);

    const int id;
    FontRenderer& fontRendererInstance;
    const int width;
    const int height;
    std::u16string text;
    int maxStringLength = 32;
    int cursorCounter = 0;
    bool enableBackgroundDrawing = true;
    bool canLoseFocus = true;
    bool focused = false;
    bool isEnabled = true;
    int lineScrollOffset = 0;
    int cursorPosition = 0;
    int selectionEnd = 0;
    int enabledColor = 14737632;
    int disabledColor = 7368816;
    bool visible = true;
    GuiResponder* guiResponder = nullptr;
    std::function<bool(const std::string&)> validator;
};

} // namespace mc
