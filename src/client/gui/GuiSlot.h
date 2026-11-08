#pragma once

// Original: net/minecraft/client/gui/GuiSlot.java and GuiListExtended.java

#include <cstdint>

namespace mc {

class GuiButton;
class Minecraft;
class Tessellator;

class GuiSlot {
public:
    GuiSlot(Minecraft& mcIn, int width, int height, int topIn, int bottomIn, int slotHeightIn);
    virtual ~GuiSlot() = default;

    void setDimensions(int widthIn, int heightIn, int topIn, int bottomIn);
    void setShowSelectionBox(bool showSelectionBoxIn) { showSelectionBox = showSelectionBoxIn; }
    int getSlotIndexFromScreenCoords(int x, int y) const;
    void registerScrollButtons(int scrollUpButtonIDIn, int scrollDownButtonIDIn);
    // Java: func_148135_f
    int getMaxScroll() const;
    int getAmountScrolled() const;
    bool isMouseYWithinSlotBounds(int y) const;
    void scrollBy(int amount);
    void actionPerformed(GuiButton& button);
    virtual void drawScreen(int mouseXIn, int mouseYIn, float partialTicks);
    void handleMouseInput();
    void setEnabled(bool enabledIn) { enabled = enabledIn; }
    bool getEnabled() const { return enabled; }
    virtual int getListWidth() const { return 220; }
    void setSlotXBoundsFromLeft(int leftIn);
    int getSlotHeight() const { return slotHeight; }

protected:
    void setHasListHeader(bool hasListHeaderIn, int headerPaddingIn);
    virtual int getSize() const = 0;
    virtual void elementClicked(int slotIndex, bool isDoubleClick, int mouseX, int mouseY) = 0;
    virtual bool isSelected(int slotIndex) const = 0;
    virtual int getContentHeight() const;
    virtual void drawBackground() = 0;
    // Java: func_178040_a, called for slots outside the visible area
    virtual void updateItemPos(int entryID, int insideLeft, int yPos);
    virtual void drawSlot(int entryID, int insideLeft, int yPos, int insideSlotHeight, int mouseXIn, int mouseYIn) = 0;
    virtual void drawListHeader(int insideLeft, int insideTop, Tessellator& tessellator);
    // Java: func_148132_a, a click on the header
    virtual void clickedHeader(int x, int y);
    // Java: func_148142_b, drawn after the list
    virtual void renderDecorations(int mouseXIn, int mouseYIn);
    void bindAmountScrolled();
    virtual void drawSelectionBox(int insideLeft, int insideTop, int mouseXIn, int mouseYIn);
    virtual int getScrollBarX() const { return width / 2 + 124; }
    void overlayBackground(int startY, int endY, int startAlpha, int endAlpha);

    Minecraft& mc;
    int width;
    int height;
    int top;
    int bottom;
    int right;
    int left;
    const int slotHeight;
    int mouseX = 0;
    int mouseY = 0;
    // Java: field_148163_i, set but never read
    bool centerListVertically = true;
    int initialClickY = -2;
    float scrollMultiplier = 0.0f;
    float amountScrolled = 0.0f;
    int selectedElement = -1;
    int64_t lastClicked = 0;
    // Java: field_178041_q
    bool visible = true;
    bool showSelectionBox = true;
    bool hasListHeader = false;
    int headerPadding = 0;

private:
    int scrollUpButtonID = 0;
    int scrollDownButtonID = 0;
    bool enabled = true;
};

class GuiListExtended : public GuiSlot {
public:
    class IGuiListEntry {
    public:
        virtual ~IGuiListEntry() = default;
        virtual void setSelected(int slotIndex, int insideLeft, int yPos) = 0;
        virtual void drawEntry(int slotIndex, int x, int y, int listWidth, int slotHeight, int mouseX, int mouseY, bool isSelected) = 0;
        virtual bool mousePressed(int slotIndex, int x, int y, int mouseEvent, int relativeX, int relativeY) = 0;
        virtual void mouseReleased(int slotIndex, int x, int y, int mouseEvent, int relativeX, int relativeY) = 0;
    };

    using GuiSlot::GuiSlot;

    bool mouseClicked(int mouseX, int mouseY, int mouseEvent);
    bool mouseReleased(int x, int y, int mouseEvent);
    virtual IGuiListEntry& getListEntry(int index) = 0;

protected:
    void elementClicked(int slotIndex, bool isDoubleClick, int mouseX, int mouseY) override;
    bool isSelected(int slotIndex) const override;
    void drawBackground() override;
    void drawSlot(int entryID, int insideLeft, int yPos, int insideSlotHeight, int mouseXIn, int mouseYIn) override;
    void updateItemPos(int entryID, int insideLeft, int yPos) override;
};

} // namespace mc
