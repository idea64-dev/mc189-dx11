#include "client/gui/GuiSlider.h"

#include "client/Minecraft.h"
#include "client/gui/GuiResponder.h"
#include "client/renderer/DxStateManager.h"
#include "client/resources/Locale.h"
#include "core/JavaString.h"

namespace mc {

// --- GuiSlider ---

GuiSlider::GuiSlider(GuiResponder& guiResponder, int idIn, int x, int y, std::string nameIn, float minIn, float maxIn, float defaultValue,
    FormatHelper formatter)
    : GuiButton(idIn, x, y, 150, 20, "")
    , name(std::move(nameIn))
    , min(minIn)
    , max(maxIn)
    , responder(guiResponder)
    , formatHelper(std::move(formatter))
{
    sliderPosition = (defaultValue - minIn) / (maxIn - minIn);
    displayString = getDisplayString();
}

void GuiSlider::setSliderValue(float value, bool notifyResponder)
{
    sliderPosition = (value - min) / (max - min);
    displayString = getDisplayString();
    if (notifyResponder)
        responder.onTick(id, getSliderValue());
}

void GuiSlider::setSliderPosition(float position)
{
    sliderPosition = position;
    displayString = getDisplayString();
    responder.onTick(id, getSliderValue());
}

std::string GuiSlider::getDisplayString() const
{
    if (!formatHelper)
        return I18n::format(name) + ": " + java::toString(getSliderValue());
    return formatHelper(id, I18n::format(name), getSliderValue());
}

void GuiSlider::mouseDragged(Minecraft& /*mc*/, int mouseX, int /*mouseY*/)
{
    if (!visible)
        return;
    if (isMouseDown) {
        sliderPosition = static_cast<float>(mouseX - (xPosition + 4)) / static_cast<float>(width - 8);
        if (sliderPosition < 0.0f)
            sliderPosition = 0.0f;
        if (sliderPosition > 1.0f)
            sliderPosition = 1.0f;
        displayString = getDisplayString();
        responder.onTick(id, getSliderValue());
    }
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    drawTexturedModalRect(xPosition + static_cast<int>(sliderPosition * static_cast<float>(width - 8)), yPosition, 0, 66, 4, 20);
    drawTexturedModalRect(xPosition + static_cast<int>(sliderPosition * static_cast<float>(width - 8)) + 4, yPosition, 196, 66, 4, 20);
}

bool GuiSlider::mousePressed(Minecraft& mc, int mouseX, int mouseY)
{
    if (!GuiButton::mousePressed(mc, mouseX, mouseY))
        return false;
    sliderPosition = static_cast<float>(mouseX - (xPosition + 4)) / static_cast<float>(width - 8);
    if (sliderPosition < 0.0f)
        sliderPosition = 0.0f;
    if (sliderPosition > 1.0f)
        sliderPosition = 1.0f;
    displayString = getDisplayString();
    responder.onTick(id, getSliderValue());
    isMouseDown = true;
    return true;
}

void GuiSlider::mouseReleased(int /*mouseX*/, int /*mouseY*/)
{
    isMouseDown = false;
}

// --- GuiListButton ---

GuiListButton::GuiListButton(GuiResponder& responder, int buttonId, int x, int y, std::string localizationStrIn, bool value)
    : GuiButton(buttonId, x, y, 150, 20, "")
    , field_175216_o(value)
    , localizationStr(std::move(localizationStrIn))
    , guiResponder(responder)
{
    displayString = buildDisplayString();
}

std::string GuiListButton::buildDisplayString() const
{
    return I18n::format(localizationStr) + ": " + (field_175216_o ? I18n::format("gui.yes") : I18n::format("gui.no"));
}

void GuiListButton::setValue(bool value)
{
    field_175216_o = value;
    displayString = buildDisplayString();
    guiResponder.onToggle(id, value);
}

bool GuiListButton::mousePressed(Minecraft& mc, int mouseX, int mouseY)
{
    if (!GuiButton::mousePressed(mc, mouseX, mouseY))
        return false;
    field_175216_o = !field_175216_o;
    displayString = buildDisplayString();
    guiResponder.onToggle(id, field_175216_o);
    return true;
}

} // namespace mc
