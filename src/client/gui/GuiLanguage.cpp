#include "client/gui/GuiLanguage.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiOptionButton.h"
#include "client/gui/ScaledResolution.h"
#include "client/resources/LanguageManager.h"
#include "client/settings/GameSettings.h"

namespace mc {

GuiLanguage::GuiLanguage(std::shared_ptr<GuiScreen> parentScreenIn, GameSettings& gameSettings, LanguageManager& manager)
    : parentScreen(std::move(parentScreenIn))
    , game_settings_3(gameSettings)
    , languageManager(manager)
{
}

void GuiLanguage::initGui()
{
    const GameSettings::Options& unicode = GameSettings::Options::FORCE_UNICODE_FONT;
    forceUnicodeFontBtn = &addButton<GuiOptionButton>(100, width / 2 - 155, height - 38, &unicode, game_settings_3.getKeyBinding(unicode));
    confirmSettingsBtn = &addButton<GuiOptionButton>(6, width / 2 - 155 + 160, height - 38, I18n::format("gui.done"));
    list = std::make_shared<List>(*this, *mc);
    list->registerScrollButtons(7, 8);
}

void GuiLanguage::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    const std::shared_ptr<List> slot = list;
    slot->handleMouseInput();
}

void GuiLanguage::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;

    switch (button.id) {
    case 5:
        break;
    case 6:
        mc->displayGuiScreen(parentScreen);
        break;
    case 100:
        if (auto* optionButton = dynamic_cast<GuiOptionButton*>(&button)) {
            game_settings_3.setOptionValue(*optionButton->returnEnumOptions(), 1);
            button.displayString = game_settings_3.getKeyBinding(GameSettings::Options::FORCE_UNICODE_FONT);
            const ScaledResolution scaledresolution(*mc);
            setWorldAndResolution(*mc, scaledresolution.getScaledWidth(), scaledresolution.getScaledHeight());
        }
        break;
    default:
        list->actionPerformed(button);
    }
}

void GuiLanguage::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    list->drawScreen(mouseX, mouseY, partialTicks);
    drawCenteredString(*fontRendererObj, I18n::format("options.language"), width / 2, 16, 16777215);
    drawCenteredString(*fontRendererObj, "(" + I18n::format("options.languageWarning") + ")", width / 2, height - 56, 8421504);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

GuiLanguage::List::List(GuiLanguage& screenIn, Minecraft& mcIn)
    : GuiSlot(mcIn, screenIn.width, screenIn.height, 32, screenIn.height - 65 + 4, 18)
    , screen(screenIn)
{
    for (const Language& language : screen.languageManager.getLanguages()) {
        languageMap.insert_or_assign(language.getLanguageCode(), language);
        langCodeList.push_back(language.getLanguageCode());
    }
}

void GuiLanguage::List::elementClicked(int slotIndex, bool /*isDoubleClick*/, int /*mouseX*/, int /*mouseY*/)
{
    const Language& language = languageMap.at(langCodeList[static_cast<size_t>(slotIndex)]);
    screen.languageManager.setCurrentLanguage(language);
    screen.game_settings_3.language = language.getLanguageCode();
    mc.refreshResources();
    screen.fontRendererObj->setUnicodeFlag(screen.languageManager.isCurrentLocaleUnicode() || screen.game_settings_3.forceUnicodeFont);
    screen.fontRendererObj->setBidiFlag(screen.languageManager.isCurrentLanguageBidirectional());
    screen.confirmSettingsBtn->displayString = I18n::format("gui.done");
    screen.forceUnicodeFontBtn->displayString = screen.game_settings_3.getKeyBinding(GameSettings::Options::FORCE_UNICODE_FONT);
    screen.game_settings_3.saveOptions();
}

bool GuiLanguage::List::isSelected(int slotIndex) const
{
    const Language* current = screen.languageManager.getCurrentLanguage();
    return current != nullptr && langCodeList[static_cast<size_t>(slotIndex)] == current->getLanguageCode();
}

void GuiLanguage::List::drawBackground()
{
    screen.drawDefaultBackground();
}

void GuiLanguage::List::drawSlot(int entryID, int /*insideLeft*/, int yPos, int /*insideSlotHeight*/, int /*mouseXIn*/, int /*mouseYIn*/)
{
    FontRenderer& font = *screen.fontRendererObj;
    font.setBidiFlag(true);
    screen.drawCenteredString(font, languageMap.at(langCodeList[static_cast<size_t>(entryID)]).toString(), width / 2, yPos + 1, 16777215);
    const Language* current = screen.languageManager.getCurrentLanguage();
    font.setBidiFlag(current != nullptr && current->isBidirectional());
}

} // namespace mc
