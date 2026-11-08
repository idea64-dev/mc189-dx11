#pragma once

// Original: net/minecraft/network/NetworkManager.java and NetworkSystem.java (the local endpoint)
// Only the local connection of the integrated server: two managers joined back to back, each handing sent packets
// to the other's inbound queue (Java: Netty's LocalChannel, which passes the packet objects without encoding).
// Java processes a received packet at once on the Netty thread; the play handlers then re-schedule it onto the
// main thread (PacketThreadUtil.checkThreadAndEnqueue). Here the receiving thread drains its queue itself, at the
// point where Java runs those scheduled tasks, so the handlers always run on their main thread.

#include "network/LoginPackets.h"
#include "network/Packet.h"

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

namespace mc {

class IChatComponent;

enum class EnumPacketDirection { SERVERBOUND, CLIENTBOUND };

class NetworkManager : public std::enable_shared_from_this<NetworkManager> {
public:
    // Java: NetworkManager.provideLocalClient + the server side of NetworkSystem.addLocalEndpoint. The client
    // manager comes first.
    static std::pair<std::shared_ptr<NetworkManager>, std::shared_ptr<NetworkManager>> createLocalPair();

    explicit NetworkManager(EnumPacketDirection packetDirection)
        : direction(packetDirection)
    {
    }

    void setConnectionState(EnumConnectionState newState) { connectionState = newState; }
    EnumConnectionState getConnectionState() const { return connectionState; }
    // The handler is shared so that a handler replacing itself while it handles a packet stays alive.
    void setNetHandler(std::shared_ptr<INetHandler> handler);
    INetHandler* getNetHandler() const { return packetListener.get(); }
    void sendPacket(std::shared_ptr<Packet> packetIn);
    // Runs the received packets on the calling thread.
    void processInboundPackets();
    // Java: processReceivedPackets -- ticks an ITickable handler.
    void processReceivedPackets();
    std::string getRemoteAddress() const { return "local:E:0"; }
    void closeChannel(std::shared_ptr<IChatComponent> message);
    bool isLocalChannel() const { return true; }
    bool isChannelOpen() const { return channelOpen.load(); }
    bool hasNoChannel() const { return false; }
    std::shared_ptr<IChatComponent> getExitMessage()
    {
        std::lock_guard lock(inboundMutex);
        return terminationReason;
    }
    void disableAutoRead() { }
    void checkDisconnected();
    EnumPacketDirection getDirection() const { return direction; }

private:
    void receive(std::shared_ptr<Packet> packet);
    // Java: channelInactive when the peer closes.
    void onPeerClosed();

    EnumPacketDirection direction;
    EnumConnectionState connectionState = EnumConnectionState::HANDSHAKING;
    std::weak_ptr<NetworkManager> peer;
    std::shared_ptr<INetHandler> packetListener;
    std::shared_ptr<IChatComponent> terminationReason;
    std::atomic<bool> channelOpen { true };
    bool disconnected = false;
    std::mutex inboundMutex;
    std::deque<std::shared_ptr<Packet>> inboundPackets;
};

} // namespace mc
