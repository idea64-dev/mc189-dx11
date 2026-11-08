#pragma once

// Original: net/minecraft/client/gui/GuiPageButtonList.GuiResponder -- what list buttons, sliders and text fields
// report to their screen.

#include <string>

namespace mc {

class GuiResponder {
public:
    virtual ~GuiResponder() = default;
    // Java: func_175321_a -- a GuiListButton toggled.
    virtual void onToggle(int id, bool value) = 0;
    virtual void onTick(int id, float value) = 0;
    // Java: func_175319_a -- a text field's text changed.
    virtual void onText(int id, const std::string& text) = 0;
};

} // namespace mc
