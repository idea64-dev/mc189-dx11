#include "server/network/NetHandlerLoginServer.h"

#include "core/Log.h"
#include "entity/player/EntityPlayerMP.h"
#include "network/NetworkManager.h"
#include "server/MinecraftServer.h"
#include "server/management/ServerConfigurationManager.h"
#include "util/ChatComponent.h"

#include <stdexcept>

namespace mc {

void NetHandlerLoginServer::update()
{
    if (currentLoginState == LoginState::READY_TO_ACCEPT) {
        tryAcceptPlayer();
    } else if (currentLoginState == LoginState::DELAY_ACCEPT) {
        EntityPlayerMP* entityplayermp = server->getConfigurationManager()->getPlayerByUUID(*loginGameProfile->getId());
        if (entityplayermp == nullptr) {
            currentLoginState = LoginState::READY_TO_ACCEPT;
            server->getConfigurationManager()->initializeConnectionToPlayer(networkManager, player);
            player = nullptr;
        }
    }

    if (connectionTimer++ == 600)
        closeConnection("Took too long to log in");
}

void NetHandlerLoginServer::closeConnection(const std::string& reason)
{
    Log::info("Disconnecting {}: {}", getConnectionInfo(), reason);
    const auto chatcomponenttext = std::make_shared<ChatComponentText>(reason);
    networkManager->sendPacket(std::make_shared<S00PacketDisconnect>(chatcomponenttext));
    networkManager->closeChannel(chatcomponenttext);
}

void NetHandlerLoginServer::tryAcceptPlayer()
{
    if (!loginGameProfile->isComplete())
        loginGameProfile = getOfflineProfile(*loginGameProfile);

    if (const std::optional<std::string> s = server->getConfigurationManager()->allowUserToConnect(*loginGameProfile)) {
        closeConnection(*s);
        return;
    }

    currentLoginState = LoginState::ACCEPTED;
    // Java: compression is enabled for remote connections only.
    networkManager->sendPacket(std::make_shared<S02PacketLoginSuccess>(*loginGameProfile));
    ServerConfigurationManager* manager = server->getConfigurationManager();

    if (manager->getPlayerByUUID(*loginGameProfile->getId()) != nullptr) {
        currentLoginState = LoginState::DELAY_ACCEPT;
        player = manager->createPlayerForUser(*loginGameProfile);
    } else {
        manager->initializeConnectionToPlayer(networkManager, manager->createPlayerForUser(*loginGameProfile));
    }
}

void NetHandlerLoginServer::onDisconnect(const std::shared_ptr<IChatComponent>& reason)
{
    Log::info("{} lost connection: {}", getConnectionInfo(), reason->getUnformattedText());
}

std::string NetHandlerLoginServer::getConnectionInfo() const
{
    if (!loginGameProfile)
        return networkManager->getRemoteAddress();
    // Java: GameProfile.toString()
    const std::string id = loginGameProfile->getId() ? loginGameProfile->getId()->toString() : "<null>";
    return "com.mojang.authlib.GameProfile@[id=" + id + ",name=" + loginGameProfile->getName() + ",properties={},legacy=false] (" + networkManager->getRemoteAddress()
        + ")";
}

void NetHandlerLoginServer::processLoginStart(C00PacketLoginStart& packetIn)
{
    if (currentLoginState != LoginState::HELLO)
        throw std::logic_error("Unexpected hello packet");
    loginGameProfile = packetIn.getProfile();
    // Java: online mode asks a remote client for encryption; a local one is accepted at once.
    currentLoginState = LoginState::READY_TO_ACCEPT;
}

GameProfile NetHandlerLoginServer::getOfflineProfile(const GameProfile& original)
{
    const UUID uuid = UUID::nameUUIDFromBytes("OfflinePlayer:" + original.getName());
    return GameProfile(uuid, original.getName());
}

void NetHandlerHandshakeMemory::processHandshake(C00Handshake& packetIn)
{
    networkManager->setConnectionState(packetIn.getRequestedState());
    networkManager->setNetHandler(std::make_shared<NetHandlerLoginServer>(mcServer, networkManager));
}

} // namespace mc
