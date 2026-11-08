#pragma once

// Original: net/minecraft/client/gui/GuiSlider.java and GuiListButton.java -- the slider and the yes/no toggle
// of GuiPageButtonList pages (the customized world screen).

#include "client/gui/GuiButton.h"

#include <functional>
#include <string>

namespace mc {

class GuiResponder;

class GuiSlider : public GuiButton {
public:
    // Java: GuiSlider.FormatHelper -- (id, translated name, value) to the button text.
    using FormatHelper = std::function<std::string(int id, const std::string& name, float value)>;

    GuiSlider(GuiResponder& guiResponder, int idIn, int x, int y, std::string nameIn, float minIn, float maxIn, float defaultValue, FormatHelper formatter);

    // Java: func_175220_c -- the value.
    float getSliderValue() const { return min + (max - min) * sliderPosition; }
    // Java: func_175218_a
    void setSliderValue(float value, bool notifyResponder);
    // Java: func_175217_d -- the position, 0..1.
    float getSliderPosition() const { return sliderPosition; }
    // Java: func_175219_a
    void setSliderPosition(float position);
    bool mousePressed(Minecraft& mc, int mouseX, int mouseY) override;
    void mouseReleased(int mouseX, int mouseY) override;

    bool isMouseDown = false;

protected:
    int getHoverState(bool) const override { return 0; }
    void mouseDragged(Minecraft& mc, int mouseX, int mouseY) override;

private:
    std::string getDisplayString() const;

    float sliderPosition = 1.0f;
    std::string name;
    const float min;
    const float max;
    GuiResponder& responder;
    FormatHelper formatHelper;
};

class GuiListButton : public GuiButton {
public:
    GuiListButton(GuiResponder& responder, int buttonId, int x, int y, std::string localizationStrIn, bool value);

    // Java: func_175212_b
    void setValue(bool value);
    bool mousePressed(Minecraft& mc, int mouseX, int mouseY) override;

private:
    std::string buildDisplayString() const;

    bool field_175216_o;
    std::string localizationStr;
    GuiResponder& guiResponder;
};

} // namespace mc
