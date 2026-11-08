#include "client/gui/ScreenChatOptions.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiOptionButton.h"
#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"

#include <array>

namespace mc {

namespace {

using Options = GameSettings::Options;

const std::array<const Options*, 10> chatOptions { &Options::CHAT_VISIBILITY, &Options::CHAT_COLOR, &Options::CHAT_LINKS,
    &Options::CHAT_OPACITY, &Options::CHAT_LINKS_PROMPT, &Options::CHAT_SCALE, &Options::CHAT_HEIGHT_FOCUSED,
    &Options::CHAT_HEIGHT_UNFOCUSED, &Options::CHAT_WIDTH, &Options::REDUCED_DEBUG_INFO };

} // namespace

ScreenChatOptions::ScreenChatOptions(std::shared_ptr<GuiScreen> parentScreenIn, GameSettings& gameSettingsIn)
    : parentScreen(std::move(parentScreenIn))
    , game_settings(gameSettingsIn)
{
}

void ScreenChatOptions::initGui()
{
    int i = 0;
    screenTitle = I18n::format("options.chat.title");

    for (const Options* option : chatOptions) {
        if (option->getEnumFloat())
            addButton<GuiOptionSlider>(option->returnEnumOrdinal(), width / 2 - 155 + i % 2 * 160, height / 6 + 24 * (i >> 1), *option);
        else
            addButton<GuiOptionButton>(option->returnEnumOrdinal(), width / 2 - 155 + i % 2 * 160, height / 6 + 24 * (i >> 1), option, game_settings.getKeyBinding(*option));
        ++i;
    }

    addButton<GuiButton>(200, width / 2 - 100, height / 6 + 120, I18n::format("gui.done"));
}

void ScreenChatOptions::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;

    if (button.id < 100) {
        if (auto* optionButton = dynamic_cast<GuiOptionButton*>(&button)) {
            game_settings.setOptionValue(*optionButton->returnEnumOptions(), 1);
            button.displayString = game_settings.getKeyBinding(*Options::getEnumOptions(button.id));
        }
    }

    if (button.id == 200) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(parentScreen);
    }
}

void ScreenChatOptions::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    drawCenteredString(*fontRendererObj, screenTitle, width / 2, 20, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
