#pragma once

// Original: net/minecraft/entity/player/EntityPlayerMP.java -- the server's player.
// Not ported yet (marked where they are called): statistics (StatisticsFile, achievements, explored biomes),
// the scoreboard and teams (death message visibility, score criteria), commands and op levels, map item
// packets, villagers, horses, and the GUIs of IInteractionObject / ILockableContainer containers.

#include "entity/player/EntityPlayer.h"
#include "inventory/Container.h"
#include "world/ChunkCoordIntPair.h"

#include <cstdint>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class C15PacketClientSettings;
class ItemInWorldManager;
class MinecraftServer;
class NetHandlerPlayServer;
class TileEntity;
class WorldServer;

class EntityPlayerMP : public EntityPlayer, public ICrafting {
public:
    EntityPlayerMP(MinecraftServer* server, WorldServer* worldIn, const GameProfile& profile, std::unique_ptr<ItemInWorldManager> interactionManager);
    ~EntityPlayerMP() override;

    std::string_view getClassName() const override { return "EntityPlayerMP"; }

    void readEntityFromNBT(const NBTTagCompound& tagCompund) override;
    void writeEntityToNBT(NBTTagCompound& tagCompound) override;
    void addExperienceLevel(int levels) override;
    void removeExperienceLevel(int levels) override;
    void addSelfToInternalCraftingInventory();
    void sendEnterCombat() override;
    void sendEndCombat() override;
    void onUpdate() override;
    void onUpdateEntity();
    void onDeath(const DamageSource& cause) override;
    bool attackEntityFrom(const DamageSource& source, float amount) override;
    bool canAttackPlayer(EntityPlayer* other) override;
    void travelToDimension(int dimensionId) override;
    bool isSpectatedByPlayer(EntityPlayerMP* player) override;
    void onItemPickup(Entity* p_71001_1_, int p_71001_2_) override;
    EnumStatus trySleep(const BlockPos& bedLocation) override;
    void wakeUpPlayer(bool immediately, bool updateWorldFlag, bool setSpawn) override;
    void mountEntity(Entity* entityIn) override;
    void handleFalling(double p_71122_1_, bool p_71122_3_);
    void openEditSign(TileEntitySign* signTile) override;
    void displayGUIChest(IInventory* chestInventory) override;
    void displayGUIBook(const std::shared_ptr<ItemStack>& bookStack) override;
    void sendContainerToPlayer(Container* p_71120_1_);
    void closeScreen() override;
    void updateHeldItem();
    void closeContainer();
    void setEntityActionState(float p_110430_1_, float p_110430_2_, bool p_110430_3_, bool sneaking);
    void mountEntityAndWakeUp();
    void setPlayerHealthUpdated() { lastHealth = -1.0E8f; }
    void addChatComponentMessage(std::shared_ptr<IChatComponent> chatComponent) override;
    void setItemInUse(const std::shared_ptr<ItemStack>& stack, int duration) override;
    void clonePlayer(EntityPlayer* oldPlayer, bool respawnFromEnd) override;
    void setPositionAndUpdate(double x, double y, double z) override;
    void onCriticalHit(Entity* entityHit) override;
    void onEnchantmentCritical(Entity* entityHit) override;
    void sendPlayerAbilities() override;
    WorldServer* getServerForPlayer() const;
    void setGameType(GameType gameType) override;
    bool isSpectator() override;
    void addChatMessage(std::shared_ptr<IChatComponent> component) override;
    bool canCommandSenderUseCommand(int permLevel, const std::string& commandName) override;
    void handleClientSettings(const C15PacketClientSettings& packetIn);
    EnumChatVisibility getChatVisibility() const { return chatVisibility; }
    void loadResourcePack(const std::string& url, const std::string& hash);
    void markPlayerActive();
    void removeEntity(Entity* p_152339_1_);
    Entity* getSpectatingEntity() { return spectatingEntity ? spectatingEntity : this; }
    void setSpectatingEntity(Entity* entityToSpectate);
    void attackTargetEntityWithCurrentItem(Entity* targetEntity) override;
    int64_t getLastActiveTime() const { return playerLastActiveTime; }
    // Java: null -- the tab list shows the name.
    std::shared_ptr<IChatComponent> getTabListDisplayName() const { return nullptr; }
    const std::string& getTranslator() const { return translator; }

    // ICrafting
    void updateCraftingInventory(Container* containerToSend, const std::vector<std::shared_ptr<ItemStack>>& itemsList) override;
    void sendSlotContents(Container* containerToSend, int slotInd, const ItemStack* stack) override;
    void sendProgressBarUpdate(Container* containerIn, int varToUpdate, int newValue) override;
    void sendAllWindowProperties(Container* p_175173_1_, IInventory* p_175173_2_) override;

    std::shared_ptr<NetHandlerPlayServer> playerNetServerHandler;
    MinecraftServer* const mcServer;
    const std::unique_ptr<ItemInWorldManager> theItemInWorldManager;
    double managedPosX = 0.0;
    double managedPosZ = 0.0;
    // Java: a LinkedList, iterated and edited in place.
    std::list<ChunkCoordIntPair> loadedChunks;
    bool isChangingQuantityOnly = false;
    int ping = 0;
    bool playerConqueredTheEnd = false;

protected:
    void construct() override;
    void updateFallState(double, bool, Block*, const BlockPos&) override { }
    void onItemUseFinish() override;
    void onNewPotionEffect(const PotionEffect& id) override;
    void onChangedPotionEffect(const PotionEffect& id, bool p_70695_2_) override;
    void onFinishedPotionEffect(const PotionEffect& effect) override;
    void updatePotionMetadata() override;

private:
    void sendTileEntityUpdate(TileEntity* p_147097_1_);
    void getNextWindowId() { currentWindowId = currentWindowId % 100 + 1; }
    bool canPlayersAttack() const;

    std::string translator = "en_US";
    std::list<int> destroyedItemsNetCache;
    float combinedHealth = std::numeric_limits<float>::denorm_min(); // Java: Float.MIN_VALUE
    float lastHealth = -1.0E8f;
    int lastFoodLevel = -99999999;
    bool wasHungry = true;
    int lastExperience = -99999999;
    int respawnInvulnerabilityTicks = 60;
    EnumChatVisibility chatVisibility = EnumChatVisibility::FULL;
    bool chatColours = true;
    int64_t playerLastActiveTime;
    Entity* spectatingEntity = nullptr;
    int currentWindowId = 0;
};

} // namespace mc
