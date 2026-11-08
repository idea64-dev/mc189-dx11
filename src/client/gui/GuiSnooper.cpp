#include "client/gui/GuiSnooper.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"
#include "profiler/PlayerUsageSnooper.h"

namespace mc {

GuiSnooper::GuiSnooper(std::shared_ptr<GuiScreen> parentScreenIn, GameSettings& gameSettings)
    : parentScreen(std::move(parentScreenIn))
    , game_settings_2(gameSettings)
{
}

void GuiSnooper::initGui()
{
    title = I18n::format("options.snooper.title");
    const std::string s = I18n::format("options.snooper.desc");
    description = fontRendererObj->listFormattedStringToWidth(s, width - 30);
    statNames.clear();
    statValues.clear();
    toggleButton = &addButton<GuiButton>(1, width / 2 - 152, height - 30, 150, 20, game_settings_2.getKeyBinding(GameSettings::Options::SNOOPER_ENABLED));
    addButton<GuiButton>(2, width / 2 + 2, height - 30, 150, 20, I18n::format("gui.done"));

    PlayerUsageSnooper* serverSnooper = mc->getIntegratedServerSnooper();
    const bool flag = serverSnooper != nullptr;

    for (const auto& [key, value] : mc->getPlayerUsageSnooper().getCurrentStats()) {
        statNames.push_back((flag ? "C " : "") + key);
        statValues.push_back(fontRendererObj->trimStringToWidth(value, width - 220));
    }

    if (flag) {
        for (const auto& [key, value] : serverSnooper->getCurrentStats()) {
            statNames.push_back("S " + key);
            statValues.push_back(fontRendererObj->trimStringToWidth(value, width - 220));
        }
    }

    list = std::make_shared<List>(*this);
}

void GuiSnooper::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    const std::shared_ptr<List> slot = list;
    slot->handleMouseInput();
}

void GuiSnooper::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;

    if (button.id == 2) {
        game_settings_2.saveOptions();
        game_settings_2.saveOptions();
        mc->displayGuiScreen(parentScreen);
    }

    if (button.id == 1) {
        game_settings_2.setOptionValue(GameSettings::Options::SNOOPER_ENABLED, 1);
        toggleButton->displayString = game_settings_2.getKeyBinding(GameSettings::Options::SNOOPER_ENABLED);
    }
}

void GuiSnooper::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    list->drawScreen(mouseX, mouseY, partialTicks);
    drawCenteredString(*fontRendererObj, title, width / 2, 8, 16777215);
    int i = 22;
    for (const std::string& s : description) {
        drawCenteredString(*fontRendererObj, s, width / 2, i, 8421504);
        i += fontRendererObj->FONT_HEIGHT;
    }
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

GuiSnooper::List::List(GuiSnooper& screenIn)
    : GuiSlot(*screenIn.mc, screenIn.width, screenIn.height, 80, screenIn.height - 40, screenIn.fontRendererObj->FONT_HEIGHT + 1)
    , screen(screenIn)
{
}

void GuiSnooper::List::drawSlot(int entryID, int /*insideLeft*/, int yPos, int /*insideSlotHeight*/, int /*mouseXIn*/, int /*mouseYIn*/)
{
    FontRenderer& font = *screen.fontRendererObj;
    font.drawString(screen.statNames[static_cast<size_t>(entryID)], 10, yPos, 16777215);
    font.drawString(screen.statValues[static_cast<size_t>(entryID)], 230, yPos, 16777215);
}

} // namespace mc
