#include "client/gui/GuiDownloadTerrain.h"

#include "client/gui/FontRenderer.h"
#include "client/network/NetHandlerPlayClient.h"
#include "client/resources/Locale.h"
#include "network/play/ClientPlayPackets.h"

namespace mc {

void GuiDownloadTerrain::updateScreen()
{
    ++progress;

    // Java: new C00PacketKeepAlive() -- the default key 0.
    if (progress % 20 == 0)
        netHandlerPlayClient->addToSendQueue(std::make_shared<C00PacketKeepAlive>(0));
}

void GuiDownloadTerrain::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawBackground(0);
    drawCenteredString(*fontRendererObj, I18n::format("multiplayer.downloadingTerrain"), width / 2, height / 2 - 50, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
