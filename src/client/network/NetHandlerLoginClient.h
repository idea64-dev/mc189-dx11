#pragma once

// Original: net/minecraft/client/network/NetHandlerLoginClient.java
// Encryption (handleEncryptionRequest) is not used by the local connection; it comes with multiplayer (stage 14).

#include "network/LoginPackets.h"

#include <memory>

namespace mc {

class GuiScreen;
class Minecraft;
class NetworkManager;

class NetHandlerLoginClient : public INetHandlerLoginClient {
public:
    NetHandlerLoginClient(NetworkManager* networkManagerIn, Minecraft* mcIn, std::shared_ptr<GuiScreen> previousScreen);

    void handleLoginSuccess(S02PacketLoginSuccess& packetIn) override;
    void onDisconnect(const std::shared_ptr<IChatComponent>& reason) override;
    void handleDisconnect(S00PacketDisconnect& packetIn) override;
    void handleEnableCompression(S03PacketEnableCompression& packetIn) override;

private:
    Minecraft* mc;
    std::shared_ptr<GuiScreen> previousGuiScreen;
    // The manager owns this handler.
    NetworkManager* networkManager;
    GameProfile gameProfile;
};

} // namespace mc
