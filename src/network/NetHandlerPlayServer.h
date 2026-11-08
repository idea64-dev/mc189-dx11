#pragma once

// Original: net/minecraft/network/NetHandlerPlayServer.java -- the server side of a player's connection.
// Packets arrive on the server thread (see NetworkManager.h), so the handlers run their bodies directly where
// Java first re-schedules them with PacketThreadUtil.checkThreadAndEnqueue.
// Not ported yet: the commands and tab completion (stage 12), books, trading, command blocks, beacons and the
// anvil's item name (MC|BEdit, MC|BSign, MC|TrSel, MC|AdvCdm, MC|AutoCmd, MC|Beacon, MC|ItemName -- stages 11
// and 12), signs (with TileEntitySign) and the horse actions (stage 10).

#include "network/play/ClientPlayPackets.h"
#include "network/play/server/S08PacketPlayerPosLook.h"
#include "util/ITickable.h"

#include <cstdint>
#include <memory>
#include <set>
#include <unordered_map>

namespace mc {

class EntityPlayerMP;
class MinecraftServer;
class NetworkManager;

class NetHandlerPlayServer : public INetHandlerPlayServer, public ITickable, public std::enable_shared_from_this<NetHandlerPlayServer> {
public:
    // Java's constructor installs the handler on the connection and the player; that needs the shared pointer.
    static std::shared_ptr<NetHandlerPlayServer> create(MinecraftServer* server, std::shared_ptr<NetworkManager> networkManagerIn, std::shared_ptr<EntityPlayerMP> playerIn);
    NetHandlerPlayServer(MinecraftServer* server, std::shared_ptr<NetworkManager> networkManagerIn, std::shared_ptr<EntityPlayerMP> playerIn);

    void update() override;
    NetworkManager* getNetworkManager() const { return netManager.get(); }
    void kickPlayerFromServer(const std::string& reason);
    void setPlayerLocation(double x, double y, double z, float yaw, float pitch) { setPlayerLocation(x, y, z, yaw, pitch, {}); }
    void setPlayerLocation(double x, double y, double z, float yaw, float pitch, const std::set<S08PacketPlayerPosLook::EnumFlags>& relativeSet);
    void sendPacket(const std::shared_ptr<Packet>& packetIn);
    void onDisconnect(const std::shared_ptr<IChatComponent>& reason) override;

    void processInput(C0CPacketInput& packetIn) override;
    void processPlayer(C03PacketPlayer& packetIn) override;
    void processPlayerDigging(C07PacketPlayerDigging& packetIn) override;
    void processPlayerBlockPlacement(C08PacketPlayerBlockPlacement& packetIn) override;
    void handleSpectate(C18PacketSpectate& packetIn) override;
    void handleResourcePackStatus(C19PacketResourcePackStatus&) override { }
    void processHeldItemChange(C09PacketHeldItemChange& packetIn) override;
    void processChatMessage(C01PacketChatMessage& packetIn) override;
    void handleAnimation(C0APacketAnimation& packetIn) override;
    void processEntityAction(C0BPacketEntityAction& packetIn) override;
    void processUseEntity(C02PacketUseEntity& packetIn) override;
    void processClientStatus(C16PacketClientStatus& packetIn) override;
    void processCloseWindow(C0DPacketCloseWindow& packetIn) override;
    void processClickWindow(C0EPacketClickWindow& packetIn) override;
    void processEnchantItem(C11PacketEnchantItem& packetIn) override;
    void processCreativeInventoryAction(C10PacketCreativeInventoryAction& packetIn) override;
    void processConfirmTransaction(C0FPacketConfirmTransaction& packetIn) override;
    void processUpdateSign(C12PacketUpdateSign& packetIn) override;
    void processKeepAlive(C00PacketKeepAlive& packetIn) override;
    void processPlayerAbilities(C13PacketPlayerAbilities& packetIn) override;
    void processTabComplete(C14PacketTabComplete& packetIn) override;
    void processClientSettings(C15PacketClientSettings& packetIn) override;
    void processVanilla250Packet(C17PacketCustomPayload& packetIn) override;

    const std::shared_ptr<NetworkManager> netManager;
    std::shared_ptr<EntityPlayerMP> playerEntity;

private:
    static bool func_183006_b(const C03PacketPlayer& packet);
    static int64_t currentTimeMillis();

    MinecraftServer* const serverController;
    int networkTickCount = 0;
    int field_175090_f = 0;
    int floatingTickCount = 0;
    bool field_147366_g = false;
    int field_147378_h = 0;
    int64_t lastPingTime = 0;
    int64_t lastSentPingPacket = 0;
    int chatSpamThresholdCount = 0;
    int itemDropThreshold = 0;
    std::unordered_map<int, int16_t> field_147372_n;
    double lastPosX = 0.0;
    double lastPosY = 0.0;
    double lastPosZ = 0.0;
    bool hasMoved = true;
};

} // namespace mc
