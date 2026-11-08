#include "client/gui/GuiDisconnected.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/resources/Locale.h"
#include "util/ChatComponent.h"

namespace mc {

GuiDisconnected::GuiDisconnected(std::shared_ptr<GuiScreen> screen, const std::string& reasonLocalizationKey, std::shared_ptr<IChatComponent> chatComp)
    : reason(I18n::format(reasonLocalizationKey))
    , message(std::move(chatComp))
    , parentScreen(std::move(screen))
{
}

void GuiDisconnected::initGui()
{
    buttonList.clear();
    multilineMessage = fontRendererObj->listFormattedStringToWidth(message->getFormattedText(), width - 50);
    field_175353_i = static_cast<int>(multilineMessage.size()) * fontRendererObj->FONT_HEIGHT;
    addButton<GuiButton>(0, width / 2 - 100, height / 2 + field_175353_i / 2 + fontRendererObj->FONT_HEIGHT, I18n::format("gui.toMenu"));
}

void GuiDisconnected::actionPerformed(GuiButton& button)
{
    if (button.id == 0)
        mc->displayGuiScreen(parentScreen);
}

void GuiDisconnected::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    drawCenteredString(*fontRendererObj, reason, width / 2, height / 2 - field_175353_i / 2 - fontRendererObj->FONT_HEIGHT * 2, 11184810);
    int i = height / 2 - field_175353_i / 2;

    for (const std::string& s : multilineMessage) {
        drawCenteredString(*fontRendererObj, s, width / 2, i, 16777215);
        i += fontRendererObj->FONT_HEIGHT;
    }

    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
