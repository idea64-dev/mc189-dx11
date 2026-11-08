#include "client/gui/GuiOptions.h"

#include "client/Minecraft.h"
#include "client/audio/SoundHandler.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiCustomizeSkin.h"
#include "client/gui/GuiControls.h"
#include "client/gui/GuiLanguage.h"
#include "client/gui/GuiOptionButton.h"
#include "client/gui/GuiScreenOptionsSounds.h"
#include "client/gui/GuiScreenResourcePacks.h"
#include "client/gui/GuiSnooper.h"
#include "client/gui/GuiVideoSettings.h"
#include "client/gui/ScreenChatOptions.h"
#include "client/gui/stream/GuiStreamUnavailable.h"
#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"

#include <array>

namespace mc {

namespace {

const std::array<const GameSettings::Options*, 1> field_146440_f { &GameSettings::Options::FOV };

// The "Super Secret Settings..." button plays a random sound instead of the click.
class SuperSecretButton : public GuiButton {
public:
    using GuiButton::GuiButton;

    void playPressSound(SoundHandler& soundHandlerIn) override
    {
        if (SoundEventAccessorComposite* composite = soundHandlerIn.getRandomSoundFromCategories(
                { SoundCategory::ANIMALS, SoundCategory::BLOCKS, SoundCategory::MOBS, SoundCategory::PLAYERS, SoundCategory::WEATHER }))
            soundHandlerIn.playSound(PositionedSoundRecord::create(composite->getSoundEventLocation(), 0.5f));
    }
};

} // namespace

GuiOptions::GuiOptions(std::shared_ptr<GuiScreen> parentScreenIn, GameSettings& gameSettings)
    : parentScreen(std::move(parentScreenIn))
    , game_settings_1(gameSettings)
{
}

void GuiOptions::initGui()
{
    int i = 0;
    screenTitle = I18n::format("options.title");

    for (const GameSettings::Options* option : field_146440_f) {
        if (option->getEnumFloat())
            addButton<GuiOptionSlider>(option->returnEnumOrdinal(), width / 2 - 155 + i % 2 * 160, height / 6 - 12 + 24 * (i >> 1), *option);
        else
            addButton<GuiOptionButton>(option->returnEnumOrdinal(), width / 2 - 155 + i % 2 * 160, height / 6 - 12 + 24 * (i >> 1), option, game_settings_1.getKeyBinding(*option));
        ++i;
    }

    // Java: with a world this slot holds the difficulty button and its lock (stage 9).
    const GameSettings::Options& realms = GameSettings::Options::REALMS_NOTIFICATIONS;
    addButton<GuiOptionButton>(realms.returnEnumOrdinal(), width / 2 - 155 + i % 2 * 160, height / 6 - 12 + 24 * (i >> 1), &realms, game_settings_1.getKeyBinding(realms));

    addButton<GuiButton>(110, width / 2 - 155, height / 6 + 48 - 6, 150, 20, I18n::format("options.skinCustomisation"));
    addButton<SuperSecretButton>(8675309, width / 2 + 5, height / 6 + 48 - 6, 150, 20, "Super Secret Settings...");
    addButton<GuiButton>(106, width / 2 - 155, height / 6 + 72 - 6, 150, 20, I18n::format("options.sounds"));
    addButton<GuiButton>(107, width / 2 + 5, height / 6 + 72 - 6, 150, 20, I18n::format("options.stream"));
    addButton<GuiButton>(101, width / 2 - 155, height / 6 + 96 - 6, 150, 20, I18n::format("options.video"));
    addButton<GuiButton>(100, width / 2 + 5, height / 6 + 96 - 6, 150, 20, I18n::format("options.controls"));
    addButton<GuiButton>(102, width / 2 - 155, height / 6 + 120 - 6, 150, 20, I18n::format("options.language"));
    addButton<GuiButton>(103, width / 2 + 5, height / 6 + 120 - 6, 150, 20, I18n::format("options.chat.title"));
    addButton<GuiButton>(105, width / 2 - 155, height / 6 + 144 - 6, 150, 20, I18n::format("options.resourcepack"));
    addButton<GuiButton>(104, width / 2 + 5, height / 6 + 144 - 6, 150, 20, I18n::format("options.snooper.view"));
    addButton<GuiButton>(200, width / 2 - 100, height / 6 + 168, I18n::format("gui.done"));
}

void GuiOptions::confirmClicked(bool /*result*/, int /*id*/)
{
    mc->displayGuiScreen(shared_from_this());
    // Java: id 109 locks the world difficulty (stage 9).
}

void GuiOptions::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;

    if (button.id < 100) {
        if (auto* optionButton = dynamic_cast<GuiOptionButton*>(&button)) {
            game_settings_1.setOptionValue(*optionButton->returnEnumOptions(), 1);
            button.displayString = game_settings_1.getKeyBinding(*GameSettings::Options::getEnumOptions(button.id));
        }
    }

    // Java: 108 (difficulty) and 109 (difficulty lock) need a world (stage 9).

    if (button.id == 110) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(std::make_shared<GuiCustomizeSkin>(shared_from_this()));
    }

    if (button.id == 8675309) {
        // Java: mc.entityRenderer.activateNextShader(), which only acts with a player in the world (stage 15).
    }

    if (button.id == 101) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(std::make_shared<GuiVideoSettings>(shared_from_this(), game_settings_1));
    }

    if (button.id == 100) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(std::make_shared<GuiControls>(shared_from_this(), game_settings_1));
    }

    if (button.id == 102) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(std::make_shared<GuiLanguage>(shared_from_this(), game_settings_1, mc->getLanguageManager()));
    }

    if (button.id == 103) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(std::make_shared<ScreenChatOptions>(shared_from_this(), game_settings_1));
    }

    if (button.id == 104) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(std::make_shared<GuiSnooper>(shared_from_this(), game_settings_1));
    }

    if (button.id == 200) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(parentScreen);
    }

    if (button.id == 105) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(std::make_shared<GuiScreenResourcePacks>(shared_from_this()));
    }

    if (button.id == 106) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(std::make_shared<GuiScreenOptionsSounds>(shared_from_this(), game_settings_1));
    }

    if (button.id == 107) {
        mc->gameSettings->saveOptions();
        // Twitch broadcasting is gone; Java takes this branch whenever the stream isn't ready.
        GuiStreamUnavailable::func_152321_a(shared_from_this());
    }
}

void GuiOptions::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    drawCenteredString(*fontRendererObj, screenTitle, width / 2, 15, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
