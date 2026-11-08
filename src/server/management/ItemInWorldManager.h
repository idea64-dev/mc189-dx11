#pragma once

// Original: net/minecraft/server/management/ItemInWorldManager.java -- a player's game mode on the server, block
// breaking (with the damage progress) and item use.

#include "util/BlockPos.h"
#include "util/EnumFacing.h"
#include "world/WorldSettings.h"

#include <memory>

namespace mc {

class EntityPlayer;
class EntityPlayerMP;
class ItemStack;
class World;
class WorldServer;

class ItemInWorldManager {
public:
    explicit ItemInWorldManager(World* worldIn)
        : theWorld(worldIn)
    {
    }

    void setGameType(GameType type);
    GameType getGameType() const { return gameType; }
    bool survivalOrAdventure() const { return isSurvivalOrAdventure(gameType); }
    bool isCreative() const { return mc::isCreative(gameType); }
    void initializeGameType(GameType type);
    void updateBlockRemoving();
    void onBlockClicked(const BlockPos& pos, EnumFacing side);
    void blockRemoving(const BlockPos& pos);
    void cancelDestroyingBlock();
    bool tryHarvestBlock(const BlockPos& pos);
    bool tryUseItem(EntityPlayer* player, World* worldIn, const std::shared_ptr<ItemStack>& stack);
    bool activateBlockOrUseItem(EntityPlayer* player, World* worldIn, const std::shared_ptr<ItemStack>& stack, const BlockPos& pos, EnumFacing side,
        float offsetX, float offsetY, float offsetZ);
    void setWorld(WorldServer* serverWorld);

    World* theWorld;
    EntityPlayerMP* thisPlayerMP = nullptr;

private:
    bool removeBlock(const BlockPos& pos);

    GameType gameType = GameType::NOT_SET;
    bool isDestroyingBlock = false;
    int initialDamage = 0;
    BlockPos field_180240_f = BlockPos::ORIGIN;
    int curblockDamage = 0;
    bool receivedFinishDiggingPacket = false;
    BlockPos field_180241_i = BlockPos::ORIGIN;
    int initialBlockDamage = 0;
    int durabilityRemainingOnBlock = -1;
};

} // namespace mc
