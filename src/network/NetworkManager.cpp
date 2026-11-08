#include "network/NetworkManager.h"

#include "core/Log.h"
#include "util/ChatComponent.h"
#include "util/ITickable.h"

namespace mc {

std::pair<std::shared_ptr<NetworkManager>, std::shared_ptr<NetworkManager>> NetworkManager::createLocalPair()
{
    auto client = std::make_shared<NetworkManager>(EnumPacketDirection::CLIENTBOUND);
    auto server = std::make_shared<NetworkManager>(EnumPacketDirection::SERVERBOUND);
    client->peer = server;
    server->peer = client;
    return { client, server };
}

void NetworkManager::setNetHandler(std::shared_ptr<INetHandler> handler)
{
    packetListener = std::move(handler);
}

void NetworkManager::sendPacket(std::shared_ptr<Packet> packetIn)
{
    if (!isChannelOpen())
        return;
    if (std::shared_ptr<NetworkManager> other = peer.lock())
        other->receive(std::move(packetIn));
}

void NetworkManager::receive(std::shared_ptr<Packet> packet)
{
    std::lock_guard lock(inboundMutex);
    inboundPackets.push_back(std::move(packet));
}

void NetworkManager::processInboundPackets()
{
    for (;;) {
        std::shared_ptr<Packet> packet;
        {
            std::lock_guard lock(inboundMutex);
            if (inboundPackets.empty())
                return;
            packet = std::move(inboundPackets.front());
            inboundPackets.pop_front();
        }
        if (!isChannelOpen())
            continue;
        // Keep the handler alive: a login handler replaces itself while handling its last packet.
        const std::shared_ptr<INetHandler> handler = packetListener;
        if (handler)
            packet->processPacket(*handler);
    }
}

void NetworkManager::processReceivedPackets()
{
    if (auto* tickable = dynamic_cast<ITickable*>(packetListener.get())) {
        const std::shared_ptr<INetHandler> keepAlive = packetListener;
        tickable->update();
    }
}

void NetworkManager::closeChannel(std::shared_ptr<IChatComponent> message)
{
    if (channelOpen.exchange(false)) {
        {
            std::lock_guard lock(inboundMutex);
            terminationReason = std::move(message);
        }
        if (std::shared_ptr<NetworkManager> other = peer.lock())
            other->onPeerClosed();
    }
}

void NetworkManager::onPeerClosed()
{
    // Java: channelInactive -> closeChannel(disconnect.endOfStream); the local channel closes both ends.
    if (channelOpen.exchange(false)) {
        std::lock_guard lock(inboundMutex);
        terminationReason = std::make_shared<ChatComponentTranslation>("disconnect.endOfStream");
    }
}

void NetworkManager::checkDisconnected()
{
    if (isChannelOpen())
        return;
    if (disconnected) {
        Log::warn("handleDisconnection() called twice");
        return;
    }
    disconnected = true;
    const std::shared_ptr<INetHandler> handler = packetListener;
    if (!handler)
        return;
    std::shared_ptr<IChatComponent> reason;
    {
        std::lock_guard lock(inboundMutex);
        reason = terminationReason;
    }
    if (reason)
        handler->onDisconnect(reason);
    else
        handler->onDisconnect(std::make_shared<ChatComponentText>("Disconnected"));
}

} // namespace mc
