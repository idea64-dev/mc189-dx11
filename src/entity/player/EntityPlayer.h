#pragma once

// Original: net/minecraft/entity/player/EntityPlayer.java
// Not ported yet (each call site is marked): statistics and achievements (addStat / triggerAchievement do
// nothing), the scoreboard and teams, EntityItem drops (dropItem returns null), recipes, the item use hooks
// (onItemUseFinish, onPlayerStoppedUsing, interactWithEntity, hitEntity), and the riding stats for pigs,
// minecarts, boats and horses.

#include "auth/GameProfile.h"
#include "entity/EntityLivingBase.h"
#include "entity/player/InventoryPlayer.h"
#include "entity/player/PlayerCapabilities.h"
#include "entity/player/PlayerEnums.h"
#include "util/FoodStats.h"
#include "world/WorldSettings.h"

#include <memory>
#include <optional>
#include <string>

namespace mc {

class Container;
class EntityItem;
class IInventory;
class InventoryEnderChest;
class StatBase;
class TileEntitySign;

class EntityPlayer : public EntityLivingBase {
public:
    enum class EnumStatus { OK, NOT_POSSIBLE_HERE, NOT_POSSIBLE_NOW, TOO_FAR_AWAY, OTHER_PROBLEM, NOT_SAFE };

    EntityPlayer(World* worldIn, GameProfile gameProfileIn);
    ~EntityPlayer() override;

    const std::shared_ptr<ItemStack>& getItemInUse() const { return itemInUse; }
    int getItemInUseCount() const { return itemInUseCount; }
    bool isUsingItem() const { return itemInUse != nullptr; }
    int getItemInUseDuration() const;
    void stopUsingItem();
    void clearItemInUse();
    bool isBlocking() const;
    void onUpdate() override;
    int getMaxInPortalTime() override { return capabilities.disableDamage ? 0 : 80; }
    int getPortalCooldown() override { return 10; }
    void playSound(const std::string& name, float volume, float pitch) override;
    void handleStatusUpdate(int8_t id) override;
    void updateRidden() override;
    void preparePlayerToSpawn() override;
    void onLivingUpdate() override;
    int getScore() const { return dataWatcher.getWatchableObjectInt(18); }
    void setScore(int p_85040_1_) { dataWatcher.updateObject(18, static_cast<int32_t>(p_85040_1_)); }
    void addScore(int p_85039_1_);
    void onDeath(const DamageSource& cause) override;
    void addToPlayerScore(Entity* entityIn, int amount) override;
    virtual std::shared_ptr<EntityItem> dropOneItem(bool dropAll);
    std::shared_ptr<EntityItem> dropPlayerItemWithRandomChoice(const std::shared_ptr<ItemStack>& itemStackIn, bool unused);
    std::shared_ptr<EntityItem> dropItem(const std::shared_ptr<ItemStack>& droppedItem, bool dropAround, bool traceItem);
    float getToolDigEfficiency(const Block* p_180471_1_);
    bool canHarvestBlock(const Block* blockToHarvest) const { return inventory.canHeldItemHarvest(blockToHarvest); }
    void readEntityFromNBT(const NBTTagCompound& tagCompund) override;
    void writeEntityToNBT(NBTTagCompound& tagCompound) override;
    bool attackEntityFrom(const DamageSource& source, float amount) override;
    virtual bool canAttackPlayer(EntityPlayer* other);
    int getTotalArmorValue() override { return inventory.getTotalArmorValue(); }
    float getArmorVisibility() const;
    virtual void openEditSign(TileEntitySign*) { }
    virtual void displayGUIChest(IInventory*) { }
    virtual void displayGUIBook(const std::shared_ptr<ItemStack>&) { }
    bool interactWith(Entity* targetEntity);
    std::shared_ptr<ItemStack> getCurrentEquippedItem() const { return inventory.getCurrentItem(); }
    void destroyCurrentEquippedItem();
    double getYOffset() override { return -0.35; }
    virtual void attackTargetEntityWithCurrentItem(Entity* targetEntity);
    virtual void onCriticalHit(Entity*) { }
    virtual void onEnchantmentCritical(Entity*) { }
    virtual void respawnPlayer() { }
    void setDead() override;
    bool isEntityInsideOpaqueBlock() override { return !sleeping && EntityLivingBase::isEntityInsideOpaqueBlock(); }
    virtual bool isUser() const { return false; }
    const GameProfile& getGameProfile() const { return gameProfile; }
    virtual EnumStatus trySleep(const BlockPos& bedLocation);
    virtual void wakeUpPlayer(bool immediately, bool updateWorldFlag, bool setSpawn);
    static std::optional<BlockPos> getBedSpawnLocation(World* worldIn, const BlockPos& bedLocation, bool forceSpawn);
    float getBedOrientationInDegrees();
    bool isPlayerSleeping() override { return sleeping; }
    bool isPlayerFullyAsleep() const { return sleeping && sleepTimer >= 100; }
    int getSleepTimer() const { return sleepTimer; }
    virtual void addChatComponentMessage(std::shared_ptr<IChatComponent>) { }
    const std::optional<BlockPos>& getBedLocation() const { return spawnChunk; }
    bool isSpawnForced() const { return spawnForced; }
    void setSpawnPoint(const std::optional<BlockPos>& pos, bool forced);
    // Statistics come with their stage: both do nothing for now (as in Java's base class).
    void triggerAchievement(const StatBase* achievementIn) { addStat(achievementIn, 1); }
    virtual void addStat(const StatBase*, int) { }
    virtual void func_175145_a(const StatBase*) { }
    void jump() override;
    void moveEntityWithHeading(float strafe, float forward) override;
    float getAIMoveSpeed() override;
    void addMovementStat(double p_71000_1_, double p_71000_3_, double p_71000_5_);
    void fall(float distance, float damageMultiplier) override;
    void onKillEntity(EntityLivingBase* entityLivingIn) override;
    // Java overrides Entity.setInWeb, which isn't virtual here (Entity.h).
    void setInWeb();
    std::shared_ptr<ItemStack> getCurrentArmor(int slotIn) override { return inventory.armorItemInSlot(slotIn); }
    virtual void addExperience(int amount);
    int getXPSeed() const { return xpSeed; }
    virtual void removeExperienceLevel(int levels);
    virtual void addExperienceLevel(int levels);
    int xpBarCap() const;
    void addExhaustion(float p_71020_1_);
    FoodStats& getFoodStats() { return foodStats; }
    bool canEat(bool ignoreHunger) const { return (ignoreHunger || foodStats.needFood()) && !capabilities.disableDamage; }
    bool shouldHeal() const { return getHealth() > 0.0f && getHealth() < getMaxHealth(); }
    virtual void setItemInUse(const std::shared_ptr<ItemStack>& stack, int duration);
    bool isAllowEdit() const { return capabilities.allowEdit; }
    bool canPlayerEdit(const BlockPos& p_175151_1_, EnumFacing p_175151_2_, ItemStack* p_175151_3_);
    bool getAlwaysRenderNameTagForRender() override { return true; }
    virtual void clonePlayer(EntityPlayer* oldPlayer, bool respawnFromEnd);
    virtual void sendPlayerAbilities() { }
    virtual void setGameType(GameType) { }
    std::string getName() override { return gameProfile.getName(); }
    InventoryEnderChest& getInventoryEnderChest() { return *theInventoryEnderChest; }
    std::shared_ptr<ItemStack> getEquipmentInSlot(int slotIn) override;
    std::shared_ptr<ItemStack> getHeldItem() override { return inventory.getCurrentItem(); }
    void setCurrentItemOrArmor(int slotIn, std::shared_ptr<ItemStack> stack) override;
    bool isInvisibleToPlayer(EntityPlayer* player) override;
    virtual bool isSpectator() = 0;
    std::vector<std::shared_ptr<ItemStack>> getInventory() override;
    bool isPushedByWater() override { return !capabilities.isFlying; }
    std::shared_ptr<IChatComponent> getDisplayName() override;
    float getEyeHeight() const override;
    void setAbsorptionAmount(float amount) override;
    float getAbsorptionAmount() override { return dataWatcher.getWatchableObjectFloat(17); }
    static UUID getUUID(const GameProfile& profile);
    static UUID getOfflineUUID(const std::string& username);
    // Java: canOpen(LockCode) -- the empty code is open to all.
    bool canOpen(const std::string& lockCode);
    bool isWearing(EnumPlayerModelParts p_175148_1_) const;
    bool replaceItemInInventory(int inventorySlot, std::shared_ptr<ItemStack> itemStackIn) override;
    bool hasReducedDebug() const { return reducedDebug; }
    void setReducedDebug(bool reducedDebugIn) { reducedDebug = reducedDebugIn; }
    void setOpenContainer(std::shared_ptr<Container> container)
    {
        openContainer = container.get();
        openContainerHolder = std::move(container);
    }

    InventoryPlayer inventory { this };
    std::shared_ptr<Container> inventoryContainer;
    // Java: the container shown (inventoryContainer when no GUI is open). setOpenContainer keeps it alive.
    Container* openContainer = nullptr;
    float prevCameraYaw = 0.0f;
    float cameraYaw = 0.0f;
    int xpCooldown = 0;
    double prevChasingPosX = 0.0;
    double prevChasingPosY = 0.0;
    double prevChasingPosZ = 0.0;
    double chasingPosX = 0.0;
    double chasingPosY = 0.0;
    double chasingPosZ = 0.0;
    std::optional<BlockPos> playerLocation;
    float renderOffsetX = 0.0f;
    float renderOffsetY = 0.0f;
    float renderOffsetZ = 0.0f;
    PlayerCapabilities capabilities;
    int experienceLevel = 0;
    int experienceTotal = 0;
    float experience = 0.0f;
    // Java: EntityFishHook -- fishing comes with its entity.
    Entity* fishEntity = nullptr;

protected:
    void construct() override;
    void applyEntityAttributes() override;
    void entityInit() override;
    std::string getSwimSound() override { return "game.player.swim"; }
    std::string getSplashSound() override { return "game.player.swim.splash"; }
    virtual void updateItemUse(const ItemStack& itemStackIn, int p_71010_2_);
    virtual void onItemUseFinish();
    bool isMovementBlocked() override { return getHealth() <= 0.0f || isPlayerSleeping(); }
    virtual void closeScreen();
    void updateEntityActionState() override;
    std::optional<std::string> getHurtSound() override { return "game.player.hurt"; }
    std::optional<std::string> getDeathSound() override { return "game.player.die"; }
    virtual void joinEntityItemWithWorld(const std::shared_ptr<EntityItem>& itemIn);
    void damageArmor(float p_70675_1_) override { inventory.damageArmor(p_70675_1_); }
    void damageEntity(const DamageSource& damageSrc, float damageAmount) override;
    void resetHeight() override;
    std::string getFallSoundString(int damageValue) override { return damageValue > 4 ? "game.player.hurt.fall.big" : "game.player.hurt.fall.small"; }
    int getExperiencePoints(EntityPlayer* player) override;
    bool isPlayer() override { return true; }
    bool canTriggerWalking() override { return !capabilities.isFlying; }

    FoodStats foodStats;
    int flyToggleTimer = 0;
    bool sleeping = false;
    float speedOnGround = 0.1f;
    float speedInAir = 0.02f;

private:
    void collideWithPlayer(Entity* p_71044_1_) { p_71044_1_->onCollideWithPlayer(this); }
    void func_175139_a(EnumFacing p_175139_1_);
    bool isInBed();
    void addMountedMovementStat(double p_71015_1_, double p_71015_3_, double p_71015_5_);

    std::shared_ptr<Container> openContainerHolder;
    std::shared_ptr<InventoryEnderChest> theInventoryEnderChest;
    int sleepTimer = 0;
    std::optional<BlockPos> spawnChunk;
    bool spawnForced = false;
    std::optional<BlockPos> startMinecartRidingCoordinate;
    int xpSeed = 0;
    std::shared_ptr<ItemStack> itemInUse;
    int itemInUseCount = 0;
    int lastXPSound = 0;
    const GameProfile gameProfile;
    bool reducedDebug = false;
};

} // namespace mc
