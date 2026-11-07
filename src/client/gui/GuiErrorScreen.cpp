#include "client/gui/GuiErrorScreen.h"

#include "client/Minecraft.h"
#include "client/resources/Locale.h"

namespace mc {

GuiErrorScreen::GuiErrorScreen(std::string titleIn, std::string messageIn)
    : title(std::move(titleIn))
    , message(std::move(messageIn))
{
}

std::shared_ptr<GuiErrorScreen> GuiErrorScreen::notPortedYet(const std::string& what, const std::string& stage)
{
    return std::make_shared<GuiErrorScreen>(what, "Not ported to C++ yet (ROADMAP " + stage + ")");
}

void GuiErrorScreen::initGui()
{
    GuiScreen::initGui();
    addButton<GuiButton>(0, width / 2 - 100, 140, I18n::format("gui.cancel"));
}

void GuiErrorScreen::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawGradientRect(0, 0, width, height, -12574688, -11530224);
    drawCenteredString(*fontRendererObj, title, width / 2, 90, 16777215);
    drawCenteredString(*fontRendererObj, message, width / 2, 110, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

void GuiErrorScreen::keyTyped(char16_t /*typedChar*/, int /*keyCode*/)
{
}

void GuiErrorScreen::actionPerformed(GuiButton& /*button*/)
{
    mc->displayGuiScreen(nullptr);
}

} // namespace mc
