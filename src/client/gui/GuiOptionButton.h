#pragma once

// Original: net/minecraft/client/gui/GuiOptionButton.java and GuiOptionSlider.java

#include "client/gui/GuiButton.h"
#include "client/settings/GameSettings.h"

namespace mc {

class GuiOptionButton : public GuiButton {
public:
    GuiOptionButton(int buttonId, int x, int y, std::string buttonText);
    GuiOptionButton(int buttonId, int x, int y, int widthIn, int heightIn, std::string buttonText);
    GuiOptionButton(int buttonId, int x, int y, const GameSettings::Options* option, std::string buttonText);

    // Null for plain buttons (Java's null enum).
    const GameSettings::Options* returnEnumOptions() const { return enumOptions; }

private:
    const GameSettings::Options* enumOptions;
};

class GuiOptionSlider : public GuiButton {
public:
    GuiOptionSlider(int buttonId, int x, int y, const GameSettings::Options& option);
    GuiOptionSlider(int buttonId, int x, int y, const GameSettings::Options& option, float minValue, float maxValue);

    bool mousePressed(Minecraft& mc, int mouseX, int mouseY) override;
    void mouseReleased(int mouseX, int mouseY) override;

    bool dragging = false;

protected:
    int getHoverState(bool mouseOver) const override;
    void mouseDragged(Minecraft& mc, int mouseX, int mouseY) override;

private:
    float sliderValue = 1.0f;
    const GameSettings::Options* options;
    float minValue; // Java: field_146132_r, unused
    float maxValue; // Java: field_146131_s, unused
};

} // namespace mc
