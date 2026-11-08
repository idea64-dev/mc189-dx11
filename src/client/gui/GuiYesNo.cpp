#include "client/gui/GuiYesNo.h"

#include "client/gui/FontRenderer.h"
#include "client/gui/GuiOptionButton.h"
#include "client/resources/Locale.h"

namespace mc {

GuiYesNo::GuiYesNo(std::shared_ptr<GuiYesNoCallback> parentScreenIn, std::string messageLine1In, std::string messageLine2In, int parentButtonClickedIdIn)
    : GuiYesNo(std::move(parentScreenIn), std::move(messageLine1In), std::move(messageLine2In), I18n::format("gui.yes"), I18n::format("gui.no"), parentButtonClickedIdIn)
{
}

GuiYesNo::GuiYesNo(std::shared_ptr<GuiYesNoCallback> parentScreenIn, std::string messageLine1In, std::string messageLine2In, std::string confirmButtonTextIn, std::string cancelButtonTextIn, int parentButtonClickedIdIn)
    : parentScreen(std::move(parentScreenIn))
    , messageLine1(std::move(messageLine1In))
    , confirmButtonText(std::move(confirmButtonTextIn))
    , cancelButtonText(std::move(cancelButtonTextIn))
    , parentButtonClickedId(parentButtonClickedIdIn)
    , messageLine2(std::move(messageLine2In))
{
}

void GuiYesNo::initGui()
{
    addButton<GuiOptionButton>(0, width / 2 - 155, height / 6 + 96, confirmButtonText);
    addButton<GuiOptionButton>(1, width / 2 - 155 + 160, height / 6 + 96, cancelButtonText);
    listLines = fontRendererObj->listFormattedStringToWidth(messageLine2, width - 50);
}

void GuiYesNo::actionPerformed(GuiButton& button)
{
    const std::shared_ptr<GuiYesNoCallback> parent = parentScreen;
    parent->confirmClicked(button.id == 0, parentButtonClickedId);
}

void GuiYesNo::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    drawCenteredString(*fontRendererObj, messageLine1, width / 2, 70, 16777215);
    int i = 90;
    for (const std::string& s : listLines) {
        drawCenteredString(*fontRendererObj, s, width / 2, i, 16777215);
        i += fontRendererObj->FONT_HEIGHT;
    }
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

void GuiYesNo::setButtonDelay(int ticks)
{
    ticksUntilEnable = ticks;
    for (const auto& guibutton : buttonList)
        guibutton->enabled = false;
}

void GuiYesNo::updateScreen()
{
    GuiScreen::updateScreen();
    if (--ticksUntilEnable == 0) {
        for (const auto& guibutton : buttonList)
            guibutton->enabled = true;
    }
}

} // namespace mc
