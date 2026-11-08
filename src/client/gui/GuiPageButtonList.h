#pragma once

// Original: net/minecraft/client/gui/GuiPageButtonList.java -- pages of sliders, toggles, text fields and labels,
// two per row (the customized world screen).

#include "client/gui/GuiSlider.h"
#include "client/gui/GuiSlot.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

class GuiLabel;
class GuiResponder;
class GuiTextField;

class GuiPageButtonList : public GuiListExtended {
public:
    // The description of one control (Java: GuiListEntry and its subclasses).
    struct GuiListEntry {
        enum class Kind { SLIDER, BUTTON, EDIT_BOX, LABEL };

        static GuiListEntry slider(int id, std::string text, bool visible, GuiSlider::FormatHelper format, float min, float max, float value)
        {
            GuiListEntry e { Kind::SLIDER, id, std::move(text), visible };
            e.format = std::move(format);
            e.min = min;
            e.max = max;
            e.value = value;
            return e;
        }
        static GuiListEntry button(int id, std::string text, bool visible, bool value)
        {
            GuiListEntry e { Kind::BUTTON, id, std::move(text), visible };
            e.buttonValue = value;
            return e;
        }
        // A null validator accepts everything.
        static GuiListEntry editBox(int id, std::string text, bool visible, std::function<bool(const std::string&)> validator)
        {
            GuiListEntry e { Kind::EDIT_BOX, id, std::move(text), visible };
            e.validator = std::move(validator);
            return e;
        }
        static GuiListEntry label(int id, std::string text, bool visible) { return GuiListEntry { Kind::LABEL, id, std::move(text), visible }; }

        Kind kind;
        int id; // Java: func_178935_b
        std::string text; // Java: func_178936_c
        bool visible; // Java: func_178934_d
        GuiSlider::FormatHelper format;
        float min = 0.0f;
        float max = 0.0f;
        float value = 0.0f;
        bool buttonValue = false;
        std::function<bool(const std::string&)> validator;
    };
    // Java: an array of entries per page; empty optionals are Java's null slots.
    using Page = std::vector<std::optional<GuiListEntry>>;

    // A row of the current page: up to two controls.
    class GuiEntry : public IGuiListEntry {
    public:
        GuiEntry(Minecraft& mcIn, Gui* left, Gui* right)
            : mc(mcIn)
            , field_178029_b(left)
            , field_178030_c(right)
        {
        }
        Gui* func_178022_a() const { return field_178029_b; }
        Gui* func_178021_b() const { return field_178030_c; }
        void drawEntry(int slotIndex, int x, int y, int listWidth, int slotHeight, int mx, int my, bool isSelected) override;
        void setSelected(int slotIndex, int insideLeft, int yPos) override;
        bool mousePressed(int slotIndex, int x, int y, int mouseEvent, int relativeX, int relativeY) override;
        void mouseReleased(int slotIndex, int x, int y, int mouseEvent, int relativeX, int relativeY) override;

        Gui* field_178028_d = nullptr; // the control last pressed

    private:
        void draw(Gui* gui, int y, int mx, int my, bool positionOnly);
        bool press(Gui* gui, int x, int y, int mouseEvent);

        Minecraft& mc;
        Gui* field_178029_b;
        Gui* field_178030_c;
    };

    GuiPageButtonList(Minecraft& mcIn, int widthIn, int heightIn, int topIn, int bottomIn, int slotHeightIn, GuiResponder& responder, std::vector<Page> pages);
    ~GuiPageButtonList() override;

    // Java: func_181156_c -- shows page n.
    void setPage(int page);
    // Java: func_178059_e
    int getPage() const { return field_178077_y; }
    // Java: func_178057_f
    int getPageCount() const { return static_cast<int>(field_178078_x.size()); }
    // Java: func_178056_g -- the control last clicked.
    Gui* getFocusedControl() const { return field_178075_A; }
    // Java: func_178071_h / func_178064_i
    void previousPage();
    void nextPage();
    // Java: func_178061_c -- a control by id; null if none.
    Gui* getComponent(int id) const;
    // Java: func_181155_a -- enables or disables the buttons of the current page.
    void setButtonsEnabled(bool enable);
    bool mouseClicked(int mx, int my, int mouseEvent);
    // Java: func_178062_a -- keys for the focused text field (tab cycles, ctrl+V pastes "a;b;c" across fields).
    void onKeyPressed(char16_t typedChar, int keyCode);
    GuiEntry& getListEntry(int index) override { return *field_178074_u[static_cast<size_t>(index)]; }
    int getSize() const override { return static_cast<int>(field_178074_u.size()); }
    int getListWidth() const override { return 400; }

protected:
    int getScrollBarX() const override { return GuiListExtended::getScrollBarX() + 32; }

private:
    void func_178069_s();
    void func_178055_t();
    void func_178060_e(int oldPage, int newPage);
    static void setVisible(Gui* gui, bool visible);
    Gui* createControl(const std::optional<GuiListEntry>& entry, int xOffset, bool alone);

    std::vector<std::unique_ptr<GuiEntry>> field_178074_u;
    std::vector<std::unique_ptr<Gui>> controls;
    std::unordered_map<int, Gui*> field_178073_v;
    std::vector<GuiTextField*> field_178072_w;
    std::vector<Page> field_178078_x;
    int field_178077_y = 0;
    GuiResponder& field_178076_z;
    Gui* field_178075_A = nullptr;
};

} // namespace mc
