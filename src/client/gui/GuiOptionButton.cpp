#include "client/gui/GuiOptionButton.h"

#include "client/Minecraft.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/texture/TextureManager.h"
#include "util/MathHelper.h"

namespace mc {

GuiOptionButton::GuiOptionButton(int buttonId, int x, int y, std::string buttonText)
    : GuiOptionButton(buttonId, x, y, nullptr, std::move(buttonText))
{
}

GuiOptionButton::GuiOptionButton(int buttonId, int x, int y, int widthIn, int heightIn, std::string buttonText)
    : GuiButton(buttonId, x, y, widthIn, heightIn, std::move(buttonText))
    , enumOptions(nullptr)
{
}

GuiOptionButton::GuiOptionButton(int buttonId, int x, int y, const GameSettings::Options* option, std::string buttonText)
    : GuiButton(buttonId, x, y, 150, 20, std::move(buttonText))
    , enumOptions(option)
{
}

GuiOptionSlider::GuiOptionSlider(int buttonId, int x, int y, const GameSettings::Options& option)
    : GuiOptionSlider(buttonId, x, y, option, 0.0f, 1.0f)
{
}

GuiOptionSlider::GuiOptionSlider(int buttonId, int x, int y, const GameSettings::Options& option, float minValueIn, float maxValueIn)
    : GuiButton(buttonId, x, y, 150, 20, "")
    , options(&option)
    , minValue(minValueIn)
    , maxValue(maxValueIn)
{
    const Minecraft& minecraft = *Minecraft::getMinecraft();
    sliderValue = option.normalizeValue(minecraft.gameSettings->getOptionFloatValue(option));
    displayString = minecraft.gameSettings->getKeyBinding(option);
}

int GuiOptionSlider::getHoverState(bool /*mouseOver*/) const
{
    return 0;
}

void GuiOptionSlider::mouseDragged(Minecraft& mc, int mouseX, int /*mouseY*/)
{
    if (!visible)
        return;

    if (dragging) {
        sliderValue = static_cast<float>(mouseX - (xPosition + 4)) / static_cast<float>(width - 8);
        sliderValue = MathHelper::clamp_float(sliderValue, 0.0f, 1.0f);
        const float f = options->denormalizeValue(sliderValue);
        mc.gameSettings->setOptionFloatValue(*options, f);
        sliderValue = options->normalizeValue(f);
        displayString = mc.gameSettings->getKeyBinding(*options);
    }

    mc.getTextureManager().bindTexture(buttonTextures);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    drawTexturedModalRect(xPosition + static_cast<int>(sliderValue * static_cast<float>(width - 8)), yPosition, 0, 66, 4, 20);
    drawTexturedModalRect(xPosition + static_cast<int>(sliderValue * static_cast<float>(width - 8)) + 4, yPosition, 196, 66, 4, 20);
}

bool GuiOptionSlider::mousePressed(Minecraft& mc, int mouseX, int mouseY)
{
    if (!GuiButton::mousePressed(mc, mouseX, mouseY))
        return false;

    sliderValue = static_cast<float>(mouseX - (xPosition + 4)) / static_cast<float>(width - 8);
    sliderValue = MathHelper::clamp_float(sliderValue, 0.0f, 1.0f);
    mc.gameSettings->setOptionFloatValue(*options, options->denormalizeValue(sliderValue));
    displayString = mc.gameSettings->getKeyBinding(*options);
    dragging = true;
    return true;
}

void GuiOptionSlider::mouseReleased(int /*mouseX*/, int /*mouseY*/)
{
    dragging = false;
}

} // namespace mc
