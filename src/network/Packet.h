#pragma once

// Original: net/minecraft/network/Packet.java and INetHandler.java
// The integrated server and the client talk over an in-memory channel (Java: Netty's LocalChannel), which hands
// the packet objects over without encoding them, as Java's local connection does. Packets therefore keep the
// values of their Java fields; the wire encoding (readPacketData / writePacketData, PacketBuffer) comes with
// the multiplayer stage.

#include <memory>
#include <string_view>

namespace mc {

class IChatComponent;

class INetHandler {
public:
    virtual ~INetHandler() = default;
    virtual void onDisconnect(const std::shared_ptr<IChatComponent>& reason) = 0;
};

class Packet {
public:
    virtual ~Packet() = default;
    // Calls the handler method for this packet; the handler must be of the packet's protocol side.
    virtual void processPacket(INetHandler& handler) = 0;
    // Java: getClass().getSimpleName(), for logs.
    virtual std::string_view getPacketName() const = 0;
};

} // namespace mc
