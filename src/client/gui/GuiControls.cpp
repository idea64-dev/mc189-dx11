#include "client/gui/GuiControls.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiOptionButton.h"
#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"

#include <array>

namespace mc {

namespace {

const std::array<const GameSettings::Options*, 3> optionsArr { &GameSettings::Options::INVERT_MOUSE, &GameSettings::Options::SENSITIVITY,
    &GameSettings::Options::TOUCHSCREEN };

} // namespace

GuiControls::GuiControls(std::shared_ptr<GuiScreen> parentScreenIn, GameSettings& settings)
    : parentScreen(std::move(parentScreenIn))
    , options(settings)
{
}

void GuiControls::initGui()
{
    keyBindingList = std::make_shared<GuiKeyBindingList>(*this, *mc);
    addButton<GuiButton>(200, width / 2 - 155, height - 29, 150, 20, I18n::format("gui.done"));
    buttonReset = &addButton<GuiButton>(201, width / 2 - 155 + 160, height - 29, 150, 20, I18n::format("controls.resetAll"));
    screenTitle = I18n::format("controls.title");

    int i = 0;
    for (const GameSettings::Options* option : optionsArr) {
        if (option->getEnumFloat())
            addButton<GuiOptionSlider>(option->returnEnumOrdinal(), width / 2 - 155 + i % 2 * 160, 18 + 24 * (i >> 1), *option);
        else
            addButton<GuiOptionButton>(option->returnEnumOrdinal(), width / 2 - 155 + i % 2 * 160, 18 + 24 * (i >> 1), option, options.getKeyBinding(*option));
        ++i;
    }
}

void GuiControls::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    const std::shared_ptr<GuiKeyBindingList> list = keyBindingList;
    list->handleMouseInput();
}

void GuiControls::actionPerformed(GuiButton& button)
{
    if (button.id == 200) {
        mc->displayGuiScreen(parentScreen);
    } else if (button.id == 201) {
        for (KeyBinding* keybinding : mc->gameSettings->keyBindings)
            keybinding->setKeyCode(keybinding->getKeyCodeDefault());
        KeyBinding::resetKeyBindingArrayAndHash();
    } else if (button.id < 100) {
        if (auto* optionButton = dynamic_cast<GuiOptionButton*>(&button)) {
            options.setOptionValue(*optionButton->returnEnumOptions(), 1);
            button.displayString = options.getKeyBinding(*GameSettings::Options::getEnumOptions(button.id));
        }
    }
}

void GuiControls::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    if (buttonId != nullptr) {
        options.setOptionKeyBinding(*buttonId, -100 + mouseButton);
        buttonId = nullptr;
        KeyBinding::resetKeyBindingArrayAndHash();
    } else if (mouseButton != 0 || !keyBindingList->mouseClicked(mouseX, mouseY, mouseButton)) {
        GuiScreen::mouseClicked(mouseX, mouseY, mouseButton);
    }
}

void GuiControls::mouseReleased(int mouseX, int mouseY, int state)
{
    if (state != 0 || !keyBindingList->mouseReleased(mouseX, mouseY, state))
        GuiScreen::mouseReleased(mouseX, mouseY, state);
}

void GuiControls::keyTyped(char16_t typedChar, int keyCode)
{
    if (buttonId != nullptr) {
        if (keyCode == 1)
            options.setOptionKeyBinding(*buttonId, 0);
        else if (keyCode != 0)
            options.setOptionKeyBinding(*buttonId, keyCode);
        else if (typedChar > 0)
            options.setOptionKeyBinding(*buttonId, typedChar + 256);

        buttonId = nullptr;
        time = Minecraft::getSystemTime();
        KeyBinding::resetKeyBindingArrayAndHash();
    } else {
        GuiScreen::keyTyped(typedChar, keyCode);
    }
}

void GuiControls::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    keyBindingList->drawScreen(mouseX, mouseY, partialTicks);
    drawCenteredString(*fontRendererObj, screenTitle, width / 2, 8, 16777215);

    bool flag = true;
    for (const KeyBinding* keybinding : options.keyBindings) {
        if (keybinding->getKeyCode() != keybinding->getKeyCodeDefault()) {
            flag = false;
            break;
        }
    }
    buttonReset->enabled = !flag;
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
