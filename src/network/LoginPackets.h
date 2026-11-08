#pragma once

// Original: net/minecraft/network/handshake/** and network/login/** -- the handshake and login packets with
// their handler interfaces. The encryption packets (C01/S01) are not used by the local connection and come with
// multiplayer.

#include "auth/GameProfile.h"
#include "network/Packet.h"

#include <memory>
#include <string>

namespace mc {

enum class EnumConnectionState { HANDSHAKING = -1, PLAY = 0, STATUS = 1, LOGIN = 2 };

class C00Handshake;
class C00PacketLoginStart;
class S00PacketDisconnect;
class S02PacketLoginSuccess;
class S03PacketEnableCompression;

class INetHandlerHandshakeServer : public INetHandler {
public:
    virtual void processHandshake(C00Handshake& packetIn) = 0;
};

class INetHandlerLoginServer : public INetHandler {
public:
    virtual void processLoginStart(C00PacketLoginStart& packetIn) = 0;
};

class INetHandlerLoginClient : public INetHandler {
public:
    virtual void handleLoginSuccess(S02PacketLoginSuccess& packetIn) = 0;
    virtual void handleDisconnect(S00PacketDisconnect& packetIn) = 0;
    virtual void handleEnableCompression(S03PacketEnableCompression& packetIn) = 0;
};

#define MC_PACKET_FOR(HANDLER, NAME, METHOD)                                                                                                                 \
    std::string_view getPacketName() const override { return #NAME; }                                                                                    \
    void processPacket(INetHandler& handler) override { static_cast<HANDLER&>(handler).METHOD(*this); }

class C00Handshake : public Packet {
public:
    MC_PACKET_FOR(INetHandlerHandshakeServer, C00Handshake, processHandshake)
    C00Handshake(int version, std::string ip, int port, EnumConnectionState requestedState)
        : protocolVersion(version)
        , ip(std::move(ip))
        , port(port)
        , requestedState(requestedState)
    {
    }
    EnumConnectionState getRequestedState() const { return requestedState; }
    int getProtocolVersion() const { return protocolVersion; }

private:
    int protocolVersion;
    std::string ip;
    int port;
    EnumConnectionState requestedState;
};

class C00PacketLoginStart : public Packet {
public:
    MC_PACKET_FOR(INetHandlerLoginServer, C00PacketLoginStart, processLoginStart)
    explicit C00PacketLoginStart(GameProfile profileIn)
        : profile(std::move(profileIn))
    {
    }
    const GameProfile& getProfile() const { return profile; }

private:
    GameProfile profile;
};

class S00PacketDisconnect : public Packet {
public:
    MC_PACKET_FOR(INetHandlerLoginClient, S00PacketDisconnect, handleDisconnect)
    explicit S00PacketDisconnect(std::shared_ptr<IChatComponent> reasonIn)
        : reason(std::move(reasonIn))
    {
    }
    const std::shared_ptr<IChatComponent>& func_149603_c() const { return reason; }

private:
    std::shared_ptr<IChatComponent> reason;
};

class S02PacketLoginSuccess : public Packet {
public:
    MC_PACKET_FOR(INetHandlerLoginClient, S02PacketLoginSuccess, handleLoginSuccess)
    explicit S02PacketLoginSuccess(GameProfile profileIn)
        : profile(std::move(profileIn))
    {
    }
    const GameProfile& getProfile() const { return profile; }

private:
    GameProfile profile;
};

class S03PacketEnableCompression : public Packet {
public:
    MC_PACKET_FOR(INetHandlerLoginClient, S03PacketEnableCompression, handleEnableCompression)
    explicit S03PacketEnableCompression(int thresholdIn)
        : compressionTreshold(thresholdIn)
    {
    }
    int getCompressionTreshold() const { return compressionTreshold; }

private:
    int compressionTreshold;
};

#undef MC_PACKET_FOR

} // namespace mc
