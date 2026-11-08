#pragma once

// Original: net/minecraft/client/gui/GuiOptionsRowList.java

#include "client/gui/GuiSlot.h"
#include "client/settings/GameSettings.h"

#include <memory>
#include <span>
#include <vector>

namespace mc {

class GuiOptionsRowList : public GuiListExtended {
public:
    class Row : public IGuiListEntry {
    public:
        Row(std::unique_ptr<GuiButton> left, std::unique_ptr<GuiButton> right);

        void drawEntry(int slotIndex, int x, int y, int listWidth, int slotHeight, int mouseX, int mouseY, bool isSelected) override;
        bool mousePressed(int slotIndex, int x, int y, int mouseEvent, int relativeX, int relativeY) override;
        void mouseReleased(int slotIndex, int x, int y, int mouseEvent, int relativeX, int relativeY) override;
        void setSelected(int /*slotIndex*/, int /*insideLeft*/, int /*yPos*/) override { }

    private:
        Minecraft& mc;
        std::unique_ptr<GuiButton> buttonA;
        std::unique_ptr<GuiButton> buttonB;
    };

    GuiOptionsRowList(Minecraft& mcIn, int widthIn, int heightIn, int topIn, int bottomIn, int slotHeightIn, std::span<const GameSettings::Options* const> options);

    Row& getListEntry(int index) override { return rows[static_cast<size_t>(index)]; }
    int getListWidth() const override { return 400; }

protected:
    int getSize() const override { return static_cast<int>(rows.size()); }
    int getScrollBarX() const override { return GuiListExtended::getScrollBarX() + 32; }

private:
    static std::unique_ptr<GuiButton> createButton(Minecraft& mcIn, int x, int y, const GameSettings::Options* option);

    std::vector<Row> rows;
};

} // namespace mc
