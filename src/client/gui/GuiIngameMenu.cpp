#include "client/gui/GuiIngameMenu.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiMainMenu.h"
#include "client/gui/GuiOptions.h"
#include "client/multiplayer/WorldClient.h"
#include "client/resources/Locale.h"

namespace mc {

void GuiIngameMenu::initGui()
{
    field_146445_a = 0;
    buttonList.clear();
    const int i = -16;
    addButton<GuiButton>(1, width / 2 - 100, height / 4 + 120 + i, I18n::format("menu.returnToMenu"));

    if (!mc->isIntegratedServerRunning())
        buttonList[0]->displayString = I18n::format("menu.disconnect");

    addButton<GuiButton>(4, width / 2 - 100, height / 4 + 24 + i, I18n::format("menu.returnToGame"));
    addButton<GuiButton>(0, width / 2 - 100, height / 4 + 96 + i, 98, 20, I18n::format("menu.options"));
    GuiButton& guibutton = addButton<GuiButton>(7, width / 2 + 2, height / 4 + 96 + i, 98, 20, I18n::format("menu.shareToLan"));
    addButton<GuiButton>(5, width / 2 - 100, height / 4 + 48 + i, 98, 20, I18n::format("gui.achievements"));
    addButton<GuiButton>(6, width / 2 + 2, height / 4 + 48 + i, 98, 20, I18n::format("gui.stats"));
    // Java: mc.isSingleplayer() && !mc.getIntegratedServer().getPublic() -- the server is never public yet.
    guibutton.enabled = mc->isIntegratedServerRunning();
}

void GuiIngameMenu::actionPerformed(GuiButton& button)
{
    switch (button.id) {
    case 0:
        mc->displayGuiScreen(std::make_shared<GuiOptions>(shared_from_this(), *mc->gameSettings));
        break;

    case 1: {
        const bool flag = mc->isIntegratedServerRunning();
        // Keep this screen alive: loadWorld and displayGuiScreen drop the references to it.
        const std::shared_ptr<GuiScreen> self = shared_from_this();
        button.enabled = false;
        mc->theWorld->sendQuittingDisconnectingPacket();
        mc->loadWorld(nullptr);

        // Java: GuiMultiplayer for a remote server -- the multiplayer screen comes with stage 14.
        (void)flag;
        mc->displayGuiScreen(std::make_shared<GuiMainMenu>());
        break;
    }

    case 4:
        mc->displayGuiScreen(nullptr);
        mc->setIngameFocus();
        break;

    // 5, 6: achievements and statistics (stage 12); 7: open to LAN (stage 14).
    default:
        break;
    }
}

void GuiIngameMenu::updateScreen()
{
    GuiScreen::updateScreen();
    ++field_146444_f;
}

void GuiIngameMenu::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    drawCenteredString(*fontRendererObj, I18n::format("menu.game"), width / 2, 40, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
