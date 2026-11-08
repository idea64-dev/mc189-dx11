#include "client/network/NetHandlerLoginClient.h"

#include "client/Minecraft.h"
#include "client/gui/GuiDisconnected.h"
#include "client/network/NetHandlerPlayClient.h"
#include "network/NetworkManager.h"

namespace mc {

NetHandlerLoginClient::NetHandlerLoginClient(NetworkManager* networkManagerIn, Minecraft* mcIn, std::shared_ptr<GuiScreen> previousScreen)
    : mc(mcIn)
    , previousGuiScreen(std::move(previousScreen))
    , networkManager(networkManagerIn)
{
}

void NetHandlerLoginClient::handleLoginSuccess(S02PacketLoginSuccess& packetIn)
{
    gameProfile = packetIn.getProfile();
    networkManager->setConnectionState(EnumConnectionState::PLAY);
    // Replaces this handler; NetworkManager keeps it alive until this call returns.
    networkManager->setNetHandler(std::make_shared<NetHandlerPlayClient>(mc, previousGuiScreen, networkManager, gameProfile));
}

void NetHandlerLoginClient::onDisconnect(const std::shared_ptr<IChatComponent>& reason)
{
    mc->displayGuiScreen(std::make_shared<GuiDisconnected>(previousGuiScreen, "connect.failed", reason));
}

void NetHandlerLoginClient::handleDisconnect(S00PacketDisconnect& packetIn)
{
    networkManager->closeChannel(packetIn.func_149603_c());
}

void NetHandlerLoginClient::handleEnableCompression(S03PacketEnableCompression&)
{
    // Java: compression only for a remote connection; the local one never compresses.
}

} // namespace mc
