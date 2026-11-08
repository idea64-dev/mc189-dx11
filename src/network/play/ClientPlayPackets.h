#pragma once

// Original: net/minecraft/network/play/INetHandlerPlayServer.java and network/play/client/C*.java -- the play
// packets the client sends. Each packet also has its own header under network/play/client/.

#include "entity/player/PlayerEnums.h"
#include "network/Packet.h"
#include "util/BlockPos.h"
#include "util/EnumFacing.h"
#include "util/UUID.h"
#include "util/Vec3.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class C00PacketKeepAlive;
class C01PacketChatMessage;
class C02PacketUseEntity;
class C03PacketPlayer;
class C07PacketPlayerDigging;
class C08PacketPlayerBlockPlacement;
class C09PacketHeldItemChange;
class C0APacketAnimation;
class C0BPacketEntityAction;
class C0CPacketInput;
class C0DPacketCloseWindow;
class C0EPacketClickWindow;
class C0FPacketConfirmTransaction;
class C10PacketCreativeInventoryAction;
class C11PacketEnchantItem;
class C12PacketUpdateSign;
class C13PacketPlayerAbilities;
class C14PacketTabComplete;
class C15PacketClientSettings;
class C16PacketClientStatus;
class C17PacketCustomPayload;
class C18PacketSpectate;
class C19PacketResourcePackStatus;
class Entity;
class IChatComponent;
class ItemStack;
class PlayerCapabilities;
class World;
class WorldServer;

class INetHandlerPlayServer : public INetHandler {
public:
    virtual void handleAnimation(C0APacketAnimation& packetIn) = 0;
    virtual void processChatMessage(C01PacketChatMessage& packetIn) = 0;
    virtual void processTabComplete(C14PacketTabComplete& packetIn) = 0;
    virtual void processClientStatus(C16PacketClientStatus& packetIn) = 0;
    virtual void processClientSettings(C15PacketClientSettings& packetIn) = 0;
    virtual void processConfirmTransaction(C0FPacketConfirmTransaction& packetIn) = 0;
    virtual void processEnchantItem(C11PacketEnchantItem& packetIn) = 0;
    virtual void processClickWindow(C0EPacketClickWindow& packetIn) = 0;
    virtual void processCloseWindow(C0DPacketCloseWindow& packetIn) = 0;
    virtual void processVanilla250Packet(C17PacketCustomPayload& packetIn) = 0;
    virtual void processUseEntity(C02PacketUseEntity& packetIn) = 0;
    virtual void processKeepAlive(C00PacketKeepAlive& packetIn) = 0;
    virtual void processPlayer(C03PacketPlayer& packetIn) = 0;
    virtual void processPlayerAbilities(C13PacketPlayerAbilities& packetIn) = 0;
    virtual void processPlayerDigging(C07PacketPlayerDigging& packetIn) = 0;
    virtual void processEntityAction(C0BPacketEntityAction& packetIn) = 0;
    virtual void processInput(C0CPacketInput& packetIn) = 0;
    virtual void processHeldItemChange(C09PacketHeldItemChange& packetIn) = 0;
    virtual void processCreativeInventoryAction(C10PacketCreativeInventoryAction& packetIn) = 0;
    virtual void processUpdateSign(C12PacketUpdateSign& packetIn) = 0;
    virtual void processPlayerBlockPlacement(C08PacketPlayerBlockPlacement& packetIn) = 0;
    virtual void handleSpectate(C18PacketSpectate& packetIn) = 0;
    virtual void handleResourcePackStatus(C19PacketResourcePackStatus& packetIn) = 0;
};

#define MC_PLAY_SERVER_PACKET(NAME, METHOD)                                                                                                                  \
    std::string_view getPacketName() const override { return #NAME; }                                                                                    \
    void processPacket(INetHandler& handler) override { static_cast<INetHandlerPlayServer&>(handler).METHOD(*this); }

class C00PacketKeepAlive : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C00PacketKeepAlive, processKeepAlive)
    explicit C00PacketKeepAlive(int keyIn)
        : key(keyIn)
    {
    }
    int getKey() const { return key; }

private:
    int key;
};

class C01PacketChatMessage : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C01PacketChatMessage, processChatMessage)
    // Java: cut to 100 characters (UTF-16 units).
    explicit C01PacketChatMessage(std::string messageIn);
    const std::string& getMessage() const { return message; }

private:
    std::string message;
};

class C02PacketUseEntity : public Packet {
public:
    enum class Action { INTERACT, ATTACK, INTERACT_AT };

    MC_PLAY_SERVER_PACKET(C02PacketUseEntity, processUseEntity)
    C02PacketUseEntity(Entity* entity, Action actionIn);
    C02PacketUseEntity(Entity* entity, const Vec3& hitVecIn);
    Entity* getEntityFromWorld(World* worldIn) const;
    Action getAction() const { return action; }
    const std::optional<Vec3>& getHitVec() const { return hitVec; }

private:
    int entityId;
    Action action;
    std::optional<Vec3> hitVec;
};

class C03PacketPlayer : public Packet {
public:
    class C04PacketPlayerPosition;
    class C05PacketPlayerLook;
    class C06PacketPlayerPosLook;

    MC_PLAY_SERVER_PACKET(C03PacketPlayer, processPlayer)
    explicit C03PacketPlayer(bool isOnGround)
        : onGround(isOnGround)
    {
    }
    double getPositionX() const { return x; }
    double getPositionY() const { return y; }
    double getPositionZ() const { return z; }
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }
    bool isOnGround() const { return onGround; }
    bool isMoving() const { return moving; }
    bool getRotating() const { return rotating; }
    void setMoving(bool isMoving) { moving = isMoving; }

protected:
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool onGround;
    bool moving = false;
    bool rotating = false;
};

class C03PacketPlayer::C04PacketPlayerPosition : public C03PacketPlayer {
public:
    std::string_view getPacketName() const override { return "C04PacketPlayerPosition"; }
    C04PacketPlayerPosition(double posX, double posY, double posZ, bool isOnGround)
        : C03PacketPlayer(isOnGround)
    {
        x = posX;
        y = posY;
        z = posZ;
        moving = true;
    }
};

class C03PacketPlayer::C05PacketPlayerLook : public C03PacketPlayer {
public:
    std::string_view getPacketName() const override { return "C05PacketPlayerLook"; }
    C05PacketPlayerLook(float playerYaw, float playerPitch, bool isOnGround)
        : C03PacketPlayer(isOnGround)
    {
        yaw = playerYaw;
        pitch = playerPitch;
        rotating = true;
    }
};

class C03PacketPlayer::C06PacketPlayerPosLook : public C03PacketPlayer {
public:
    std::string_view getPacketName() const override { return "C06PacketPlayerPosLook"; }
    C06PacketPlayerPosLook(double playerX, double playerY, double playerZ, float playerYaw, float playerPitch, bool playerIsOnGround)
        : C03PacketPlayer(playerIsOnGround)
    {
        x = playerX;
        y = playerY;
        z = playerZ;
        yaw = playerYaw;
        pitch = playerPitch;
        rotating = true;
        moving = true;
    }
};

class C07PacketPlayerDigging : public Packet {
public:
    enum class Action { START_DESTROY_BLOCK, ABORT_DESTROY_BLOCK, STOP_DESTROY_BLOCK, DROP_ALL_ITEMS, DROP_ITEM, RELEASE_USE_ITEM };

    MC_PLAY_SERVER_PACKET(C07PacketPlayerDigging, processPlayerDigging)
    C07PacketPlayerDigging(Action statusIn, const BlockPos& posIn, EnumFacing facingIn)
        : position(posIn)
        , facing(facingIn)
        , status(statusIn)
    {
    }
    const BlockPos& getPosition() const { return position; }
    EnumFacing getFacing() const { return facing; }
    Action getStatus() const { return status; }

private:
    BlockPos position;
    EnumFacing facing;
    Action status;
};

class C08PacketPlayerBlockPlacement : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C08PacketPlayerBlockPlacement, processPlayerBlockPlacement)
    // Using the held item without a block: position (-1, -1, -1), direction 255.
    explicit C08PacketPlayerBlockPlacement(const ItemStack* stackIn);
    C08PacketPlayerBlockPlacement(const BlockPos& positionIn, int placedBlockDirectionIn, const ItemStack* stackIn, float facingXIn, float facingYIn, float facingZIn);
    const BlockPos& getPosition() const { return position; }
    int getPlacedBlockDirection() const { return placedBlockDirection; }
    const std::shared_ptr<ItemStack>& getStack() const { return stack; }
    float getPlacedBlockOffsetX() const { return facingX; }
    float getPlacedBlockOffsetY() const { return facingY; }
    float getPlacedBlockOffsetZ() const { return facingZ; }

private:
    BlockPos position;
    int placedBlockDirection;
    std::shared_ptr<ItemStack> stack;
    float facingX;
    float facingY;
    float facingZ;
};

class C09PacketHeldItemChange : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C09PacketHeldItemChange, processHeldItemChange)
    explicit C09PacketHeldItemChange(int slotIdIn)
        : slotId(slotIdIn)
    {
    }
    int getSlotId() const { return slotId; }

private:
    int slotId;
};

class C0APacketAnimation : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C0APacketAnimation, handleAnimation)
};

class C0BPacketEntityAction : public Packet {
public:
    enum class Action { START_SNEAKING, STOP_SNEAKING, STOP_SLEEPING, START_SPRINTING, STOP_SPRINTING, RIDING_JUMP, OPEN_INVENTORY };

    MC_PLAY_SERVER_PACKET(C0BPacketEntityAction, processEntityAction)
    C0BPacketEntityAction(Entity* entity, Action actionIn, int auxDataIn = 0);
    Action getAction() const { return action; }
    int getAuxData() const { return auxData; }

private:
    int entityID;
    Action action;
    int auxData;
};

class C0CPacketInput : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C0CPacketInput, processInput)
    C0CPacketInput(float strafeSpeedIn, float forwardSpeedIn, bool jumpingIn, bool sneakingIn)
        : strafeSpeed(strafeSpeedIn)
        , forwardSpeed(forwardSpeedIn)
        , jumping(jumpingIn)
        , sneaking(sneakingIn)
    {
    }
    float getStrafeSpeed() const { return strafeSpeed; }
    float getForwardSpeed() const { return forwardSpeed; }
    bool isJumping() const { return jumping; }
    bool isSneaking() const { return sneaking; }

private:
    float strafeSpeed;
    float forwardSpeed;
    bool jumping;
    bool sneaking;
};

class C0DPacketCloseWindow : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C0DPacketCloseWindow, processCloseWindow)
    explicit C0DPacketCloseWindow(int windowIdIn)
        : windowId(windowIdIn)
    {
    }

    int windowId;
};

class C0EPacketClickWindow : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C0EPacketClickWindow, processClickWindow)
    C0EPacketClickWindow(int windowIdIn, int slotIdIn, int usedButtonIn, int modeIn, const ItemStack* clickedItemIn, int16_t actionNumberIn);
    int getWindowId() const { return windowId; }
    int getSlotId() const { return slotId; }
    int getUsedButton() const { return usedButton; }
    int16_t getActionNumber() const { return actionNumber; }
    const std::shared_ptr<ItemStack>& getClickedItem() const { return clickedItem; }
    int getMode() const { return mode; }

private:
    int windowId;
    int slotId;
    int usedButton;
    int16_t actionNumber;
    std::shared_ptr<ItemStack> clickedItem;
    int mode;
};

class C0FPacketConfirmTransaction : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C0FPacketConfirmTransaction, processConfirmTransaction)
    C0FPacketConfirmTransaction(int windowIdIn, int16_t uidIn, bool acceptedIn)
        : windowId(windowIdIn)
        , uid(uidIn)
        , accepted(acceptedIn)
    {
    }
    int getWindowId() const { return windowId; }
    int16_t getUid() const { return uid; }

private:
    int windowId;
    int16_t uid;
    bool accepted;
};

class C10PacketCreativeInventoryAction : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C10PacketCreativeInventoryAction, processCreativeInventoryAction)
    C10PacketCreativeInventoryAction(int slotIdIn, const ItemStack* stackIn);
    int getSlotId() const { return slotId; }
    const std::shared_ptr<ItemStack>& getStack() const { return stack; }

private:
    int slotId;
    std::shared_ptr<ItemStack> stack;
};

class C11PacketEnchantItem : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C11PacketEnchantItem, processEnchantItem)
    C11PacketEnchantItem(int windowIdIn, int buttonIn)
        : windowId(windowIdIn)
        , button(buttonIn)
    {
    }
    int getWindowId() const { return windowId; }
    int getButton() const { return button; }

private:
    int windowId;
    int button;
};

class C12PacketUpdateSign : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C12PacketUpdateSign, processUpdateSign)
    C12PacketUpdateSign(const BlockPos& posIn, std::vector<std::shared_ptr<IChatComponent>> linesIn)
        : pos(posIn)
        , lines(std::move(linesIn))
    {
        lines.resize(4);
    }
    const BlockPos& getPosition() const { return pos; }
    const std::vector<std::shared_ptr<IChatComponent>>& getLines() const { return lines; }

private:
    BlockPos pos;
    std::vector<std::shared_ptr<IChatComponent>> lines;
};

class C13PacketPlayerAbilities : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C13PacketPlayerAbilities, processPlayerAbilities)
    explicit C13PacketPlayerAbilities(const PlayerCapabilities& capabilities);
    bool isInvulnerable() const { return invulnerable; }
    void setInvulnerable(bool v) { invulnerable = v; }
    bool isFlying() const { return flying; }
    void setFlying(bool v) { flying = v; }
    bool isAllowFlying() const { return allowFlying; }
    void setAllowFlying(bool v) { allowFlying = v; }
    bool isCreativeMode() const { return creativeMode; }
    void setCreativeMode(bool v) { creativeMode = v; }
    void setFlySpeed(float v) { flySpeed = v; }
    void setWalkSpeed(float v) { walkSpeed = v; }

private:
    bool invulnerable = false;
    bool flying = false;
    bool allowFlying = false;
    bool creativeMode = false;
    float flySpeed = 0.0f;
    float walkSpeed = 0.0f;
};

class C14PacketTabComplete : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C14PacketTabComplete, processTabComplete)
    explicit C14PacketTabComplete(std::string msg, std::optional<BlockPos> target = std::nullopt)
        : message(std::move(msg))
        , targetBlock(target)
    {
    }
    const std::string& getMessage() const { return message; }
    const std::optional<BlockPos>& getTargetBlock() const { return targetBlock; }

private:
    std::string message;
    std::optional<BlockPos> targetBlock;
};

class C15PacketClientSettings : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C15PacketClientSettings, processClientSettings)
    C15PacketClientSettings(std::string langIn, int viewIn, EnumChatVisibility chatVisibilityIn, bool enableColorsIn, int modelPartFlagsIn)
        : lang(std::move(langIn))
        , view(viewIn)
        , chatVisibility(chatVisibilityIn)
        , enableColors(enableColorsIn)
        , modelPartFlags(modelPartFlagsIn)
    {
    }
    const std::string& getLang() const { return lang; }
    EnumChatVisibility getChatVisibility() const { return chatVisibility; }
    bool isColorsEnabled() const { return enableColors; }
    int getModelPartFlags() const { return modelPartFlags; }

private:
    std::string lang;
    int view;
    EnumChatVisibility chatVisibility;
    bool enableColors;
    int modelPartFlags;
};

class C16PacketClientStatus : public Packet {
public:
    enum class EnumState { PERFORM_RESPAWN, REQUEST_STATS, OPEN_INVENTORY_ACHIEVEMENT };

    MC_PLAY_SERVER_PACKET(C16PacketClientStatus, processClientStatus)
    explicit C16PacketClientStatus(EnumState statusIn)
        : status(statusIn)
    {
    }
    EnumState getStatus() const { return status; }

private:
    EnumState status;
};

class C17PacketCustomPayload : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C17PacketCustomPayload, processVanilla250Packet)
    // Throws std::invalid_argument above 32767 bytes, as Java.
    C17PacketCustomPayload(std::string channelIn, std::vector<uint8_t> dataIn);
    const std::string& getChannelName() const { return channel; }
    const std::vector<uint8_t>& getBufferData() const { return data; }

private:
    std::string channel;
    std::vector<uint8_t> data;
};

class C18PacketSpectate : public Packet {
public:
    MC_PLAY_SERVER_PACKET(C18PacketSpectate, handleSpectate)
    explicit C18PacketSpectate(const UUID& idIn)
        : id(idIn)
    {
    }
    const UUID& getId() const { return id; }

private:
    UUID id;
};

class C19PacketResourcePackStatus : public Packet {
public:
    enum class Action { SUCCESSFULLY_LOADED, DECLINED, FAILED_DOWNLOAD, ACCEPTED };

    MC_PLAY_SERVER_PACKET(C19PacketResourcePackStatus, handleResourcePackStatus)
    C19PacketResourcePackStatus(std::string hashIn, Action statusIn)
        : hash(hashIn.size() > 40 ? hashIn.substr(0, 40) : std::move(hashIn))
        , status(statusIn)
    {
    }

    std::string hash;
    Action status;
};

} // namespace mc
