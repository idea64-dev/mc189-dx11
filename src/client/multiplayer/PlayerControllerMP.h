#pragma once

// Original: net/minecraft/client/multiplayer/PlayerControllerMP.java
// The player's actions on the client: block hitting and breaking, using items, attacking, window clicks -- each
// done locally and sent to the server.

#include "util/BlockPos.h"
#include "util/EnumFacing.h"
#include "util/Vec3.h"
#include "world/WorldSettings.h"

#include <memory>

namespace mc {

class Entity;
class EntityPlayer;
class EntityPlayerSP;
class ItemStack;
class Minecraft;
class NetHandlerPlayClient;
class World;
class WorldClient;
struct MovingObjectPosition;

class PlayerControllerMP {
public:
    PlayerControllerMP(Minecraft* mcIn, NetHandlerPlayClient* netHandler);

    static void clickBlockCreative(Minecraft* mcIn, PlayerControllerMP* playerController, const BlockPos& pos, EnumFacing facing);
    void setPlayerCapabilities(EntityPlayer* player);
    bool isSpectator() const { return currentGameType == GameType::SPECTATOR; }
    void setGameType(GameType type);
    void flipPlayer(EntityPlayer* playerIn);
    bool shouldDrawHUD() const { return isSurvivalOrAdventure(currentGameType); }
    bool onPlayerDestroyBlock(const BlockPos& pos, EnumFacing side);
    bool clickBlock(const BlockPos& loc, EnumFacing face);
    void resetBlockRemoving();
    bool onPlayerDamageBlock(const BlockPos& posBlock, EnumFacing directionFacing);
    float getBlockReachDistance() const { return isCreative(currentGameType) ? 5.0f : 4.5f; }
    void updateController();
    bool onPlayerRightClick(EntityPlayerSP* player, WorldClient* worldIn, const std::shared_ptr<ItemStack>& heldStack, const BlockPos& hitPos, EnumFacing side,
        const Vec3& hitVec);
    bool sendUseItem(EntityPlayer* playerIn, World* worldIn, const std::shared_ptr<ItemStack>& itemStackIn);
    // Java: func_178892_a -- the player of a new world (the stat file comes with the statistics, stage 12).
    std::shared_ptr<EntityPlayerSP> func_178892_a(World* worldIn);
    void attackEntity(EntityPlayer* playerIn, Entity* targetEntity);
    bool interactWithEntitySendPacket(EntityPlayer* playerIn, Entity* targetEntity);
    bool isPlayerRightClickingOnEntity(EntityPlayer* player, Entity* entityIn, const MovingObjectPosition& movingObject);
    std::shared_ptr<ItemStack> windowClick(int windowId, int slotId, int mouseButtonClicked, int mode, EntityPlayer* playerIn);
    void sendEnchantPacket(int windowID, int button);
    void sendSlotPacket(const ItemStack* itemStackIn, int slotId);
    void sendPacketDropItem(const ItemStack* itemStackIn);
    void onStoppedUsingItem(EntityPlayer* playerIn);
    bool gameIsSurvivalOrAdventure() const { return isSurvivalOrAdventure(currentGameType); }
    bool isNotCreative() const { return !isCreative(currentGameType); }
    bool isInCreativeMode() const { return isCreative(currentGameType); }
    bool extendedReach() const { return isCreative(currentGameType); }
    bool isRidingHorse() const;
    bool isSpectatorMode() const { return currentGameType == GameType::SPECTATOR; }
    GameType getCurrentGameType() const { return currentGameType; }
    bool getIsHittingBlock() const { return isHittingBlock; }

    // Java: read by the renderer for the block damage overlay (curBlockDamageMP is private in Java, read through
    // RenderGlobal's damagedBlocks); kept accessible for the debug screen.
    float getCurBlockDamageMP() const { return curBlockDamageMP; }

private:
    bool isHittingPosition(const BlockPos& pos) const;
    void syncCurrentPlayItem();

    Minecraft* mc;
    NetHandlerPlayClient* netClientHandler;
    BlockPos currentBlock { -1, -1, -1 };
    std::shared_ptr<ItemStack> currentItemHittingBlock;
    float curBlockDamageMP = 0.0f;
    float stepSoundTickCounter = 0.0f;
    int blockHitDelay = 0;
    bool isHittingBlock = false;
    GameType currentGameType = GameType::SURVIVAL;
    int currentPlayerItem = 0;
};

} // namespace mc
