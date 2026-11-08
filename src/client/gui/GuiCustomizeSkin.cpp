#include "client/gui/GuiCustomizeSkin.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"

namespace mc {

class GuiCustomizeSkin::ButtonPart : public GuiButton {
public:
    ButtonPart(GuiCustomizeSkin& screen, int buttonId, int x, int y, int widthIn, int heightIn, EnumPlayerModelParts part)
        : GuiButton(buttonId, x, y, widthIn, heightIn, screen.partLabel(part))
        , playerModelParts(part)
    {
    }

    const EnumPlayerModelParts playerModelParts;
};

GuiCustomizeSkin::GuiCustomizeSkin(std::shared_ptr<GuiScreen> parentScreenIn)
    : parentScreen(std::move(parentScreenIn))
{
}

void GuiCustomizeSkin::initGui()
{
    int i = 0;
    title = I18n::format("options.skinCustomisation.title");

    for (const EnumPlayerModelParts part : kPlayerModelParts) {
        addButton<ButtonPart>(*this, getPartId(part), width / 2 - 155 + i % 2 * 160, height / 6 + 24 * (i >> 1), 150, 20, part);
        ++i;
    }

    if (i % 2 == 1)
        ++i;

    addButton<GuiButton>(200, width / 2 - 100, height / 6 + 24 * (i >> 1), I18n::format("gui.done"));
}

void GuiCustomizeSkin::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;

    if (button.id == 200) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(parentScreen);
    } else if (auto* part = dynamic_cast<ButtonPart*>(&button)) {
        mc->gameSettings->switchModelPartEnabled(part->playerModelParts);
        button.displayString = partLabel(part->playerModelParts);
    }
}

void GuiCustomizeSkin::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    drawCenteredString(*fontRendererObj, title, width / 2, 20, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

std::string GuiCustomizeSkin::partLabel(EnumPlayerModelParts playerModelParts) const
{
    const std::string s = mc->gameSettings->getModelParts().contains(playerModelParts) ? I18n::format("options.on") : I18n::format("options.off");
    // Java: ChatComponentTranslation("options.modelPart." + name).getFormattedText() ends with a reset code.
    return I18n::format("options.modelPart." + std::string(getPartName(playerModelParts))) + "§r: " + s;
}

} // namespace mc
