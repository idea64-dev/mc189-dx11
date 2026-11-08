#pragma once

// Original: net/minecraft/server/network/NetHandlerLoginServer.java and NetHandlerHandshakeMemory.java -- the
// server side of the handshake and the login of a local connection. Encryption and session authentication
// (online mode) only apply to remote connections and come with multiplayer.

#include "auth/GameProfile.h"
#include "network/LoginPackets.h"
#include "util/ITickable.h"

#include <memory>
#include <optional>
#include <string>

namespace mc {

class EntityPlayerMP;
class MinecraftServer;
class NetworkManager;

class NetHandlerLoginServer : public INetHandlerLoginServer, public ITickable {
public:
    NetHandlerLoginServer(MinecraftServer* serverIn, std::shared_ptr<NetworkManager> networkManagerIn)
        : server(serverIn)
        , networkManager(std::move(networkManagerIn))
    {
    }

    void update() override;
    void closeConnection(const std::string& reason);
    void tryAcceptPlayer();
    void onDisconnect(const std::shared_ptr<IChatComponent>& reason) override;
    std::string getConnectionInfo() const;
    void processLoginStart(C00PacketLoginStart& packetIn) override;
    static GameProfile getOfflineProfile(const GameProfile& original);

    const std::shared_ptr<NetworkManager> networkManager;

private:
    enum class LoginState { HELLO, KEY, AUTHENTICATING, READY_TO_ACCEPT, DELAY_ACCEPT, ACCEPTED };

    MinecraftServer* const server;
    LoginState currentLoginState = LoginState::HELLO;
    int connectionTimer = 0;
    std::optional<GameProfile> loginGameProfile;
    std::shared_ptr<EntityPlayerMP> player;
};

class NetHandlerHandshakeMemory : public INetHandlerHandshakeServer {
public:
    NetHandlerHandshakeMemory(MinecraftServer* serverIn, std::shared_ptr<NetworkManager> netManagerIn)
        : mcServer(serverIn)
        , networkManager(std::move(netManagerIn))
    {
    }

    void processHandshake(C00Handshake& packetIn) override;
    void onDisconnect(const std::shared_ptr<IChatComponent>&) override { }

private:
    MinecraftServer* const mcServer;
    const std::shared_ptr<NetworkManager> networkManager;
};

} // namespace mc
