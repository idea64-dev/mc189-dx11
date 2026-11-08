#pragma once

// Original: net/minecraft/client/gui/GuiKeyBindingList.java

#include "client/gui/GuiButton.h"
#include "client/gui/GuiSlot.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class GuiControls;
class KeyBinding;

class GuiKeyBindingList : public GuiListExtended {
public:
    GuiKeyBindingList(GuiControls& controls, Minecraft& mcIn);

    IGuiListEntry& getListEntry(int index) override { return *listEntries[static_cast<size_t>(index)]; }
    int getListWidth() const override { return GuiListExtended::getListWidth() + 32; }

protected:
    int getSize() const override { return static_cast<int>(listEntries.size()); }
    int getScrollBarX() const override { return GuiListExtended::getScrollBarX() + 15; }

private:
    class CategoryEntry : public IGuiListEntry {
    public:
        CategoryEntry(GuiKeyBindingList& list, const std::string& category);
        void drawEntry(int slotIndex, int x, int y, int listWidth, int slotHeight, int mouseX, int mouseY, bool isSelected) override;
        bool mousePressed(int, int, int, int, int, int) override { return false; }
        void mouseReleased(int, int, int, int, int, int) override { }
        void setSelected(int, int, int) override { }

    private:
        GuiKeyBindingList& list;
        std::string labelText;
        int labelWidth;
    };

    class KeyEntry : public IGuiListEntry {
    public:
        KeyEntry(GuiKeyBindingList& list, KeyBinding& keybinding);
        void drawEntry(int slotIndex, int x, int y, int listWidth, int slotHeight, int mouseX, int mouseY, bool isSelected) override;
        bool mousePressed(int slotIndex, int x, int y, int mouseEvent, int relativeX, int relativeY) override;
        void mouseReleased(int slotIndex, int x, int y, int mouseEvent, int relativeX, int relativeY) override;
        void setSelected(int, int, int) override { }

    private:
        GuiKeyBindingList& list;
        KeyBinding& keybinding;
        std::string keyDesc;
        GuiButton btnChangeKeyBinding;
        GuiButton btnReset;
    };

    GuiControls& controls;
    std::vector<std::unique_ptr<IGuiListEntry>> listEntries;
    int maxListLabelWidth = 0;
};

} // namespace mc
