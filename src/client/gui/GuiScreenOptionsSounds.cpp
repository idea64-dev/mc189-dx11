#include "client/gui/GuiScreenOptionsSounds.h"

#include "client/Minecraft.h"
#include "client/audio/SoundHandler.h"
#include "client/gui/FontRenderer.h"
#include "client/renderer/DxStateManager.h"
#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"
#include "core/Java.h"
#include "core/JavaString.h"
#include "util/MathHelper.h"

namespace mc {

// A volume slider for one sound category; changes are saved immediately.
class GuiScreenOptionsSounds::Button : public GuiButton {
public:
    Button(GuiScreenOptionsSounds& screenIn, int buttonId, int x, int y, SoundCategory category, bool wide)
        : GuiButton(buttonId, x, y, wide ? 310 : 150, 20, "")
        , screen(screenIn)
        , soundCategory(category)
        , categoryName(I18n::format("soundCategory." + std::string(getCategoryName(category))))
    {
        displayString = categoryName + ": " + screen.getSoundVolume(category);
        volume = screen.game_settings_4.getSoundLevel(category);
    }

    bool mousePressed(Minecraft& minecraft, int mouseX, int mouseY) override
    {
        if (!GuiButton::mousePressed(minecraft, mouseX, mouseY))
            return false;
        volume = static_cast<float>(mouseX - (xPosition + 4)) / static_cast<float>(width - 8);
        volume = MathHelper::clamp_float(volume, 0.0f, 1.0f);
        minecraft.gameSettings->setSoundLevel(soundCategory, volume);
        minecraft.gameSettings->saveOptions();
        displayString = categoryName + ": " + screen.getSoundVolume(soundCategory);
        pressed = true;
        return true;
    }

    void playPressSound(SoundHandler& /*soundHandlerIn*/) override
    {
    }

    void mouseReleased(int /*mouseX*/, int /*mouseY*/) override
    {
        // Java computes the category level here and discards it; the click sound plays on release instead.
        if (pressed)
            screen.mc->getSoundHandler().playSound(PositionedSoundRecord::create(ResourceLocation("gui.button.press"), 1.0f));
        pressed = false;
    }

protected:
    int getHoverState(bool /*mouseOver*/) const override
    {
        return 0;
    }

    void mouseDragged(Minecraft& minecraft, int mouseX, int /*mouseY*/) override
    {
        if (!visible)
            return;

        if (pressed) {
            volume = static_cast<float>(mouseX - (xPosition + 4)) / static_cast<float>(width - 8);
            volume = MathHelper::clamp_float(volume, 0.0f, 1.0f);
            minecraft.gameSettings->setSoundLevel(soundCategory, volume);
            minecraft.gameSettings->saveOptions();
            displayString = categoryName + ": " + screen.getSoundVolume(soundCategory);
        }

        // Java doesn't rebind the texture: widgets.png is still bound from drawing the button.
        DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
        drawTexturedModalRect(xPosition + static_cast<int>(volume * static_cast<float>(width - 8)), yPosition, 0, 66, 4, 20);
        drawTexturedModalRect(xPosition + static_cast<int>(volume * static_cast<float>(width - 8)) + 4, yPosition, 196, 66, 4, 20);
    }

private:
    GuiScreenOptionsSounds& screen;
    SoundCategory soundCategory;
    std::string categoryName;
    float volume = 1.0f;
    bool pressed = false;
};

GuiScreenOptionsSounds::GuiScreenOptionsSounds(std::shared_ptr<GuiScreen> parentScreenIn, GameSettings& settings)
    : parentScreen(std::move(parentScreenIn))
    , game_settings_4(settings)
{
}

void GuiScreenOptionsSounds::initGui()
{
    int i = 0;
    screenTitle = I18n::format("options.sounds.title");
    offDisplayString = I18n::format("options.off");
    addButton<Button>(*this, getCategoryId(SoundCategory::MASTER), width / 2 - 155 + i % 2 * 160, height / 6 - 12 + 24 * (i >> 1), SoundCategory::MASTER, true);
    i += 2;

    for (const SoundCategory category : kSoundCategories) {
        if (category != SoundCategory::MASTER) {
            addButton<Button>(*this, getCategoryId(category), width / 2 - 155 + i % 2 * 160, height / 6 - 12 + 24 * (i >> 1), category, false);
            ++i;
        }
    }

    addButton<GuiButton>(200, width / 2 - 100, height / 6 + 168, I18n::format("gui.done"));
}

void GuiScreenOptionsSounds::actionPerformed(GuiButton& button)
{
    if (button.enabled && button.id == 200) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(parentScreen);
    }
}

void GuiScreenOptionsSounds::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    drawCenteredString(*fontRendererObj, screenTitle, width / 2, 15, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

std::string GuiScreenOptionsSounds::getSoundVolume(SoundCategory category) const
{
    const float f = game_settings_4.getSoundLevel(category);
    return f == 0.0f ? offDisplayString : java::toString(java::f2i(f * 100.0f)) + "%";
}

} // namespace mc
