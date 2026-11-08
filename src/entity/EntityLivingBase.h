#pragma once

// Original: net/minecraft/entity/EntityLivingBase.java
// The attacker references (revenge target, last attacker, attacking player) are weak: Java keeps the object
// alive, but every use checks isEntityAlive() first and a removed entity is dead, so an expired pointer reads
// as Java's dead one being cleared. Teams come with the scoreboard (stage 12): getTeam() is null.

#include "core/JavaHashMap.h"
#include "entity/Entity.h"
#include "entity/EnumCreatureAttribute.h"
#include "entity/ai/attributes/Attributes.h"
#include "potion/Potion.h"
#include "util/CombatTracker.h"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class Team;

class EntityLivingBase : public Entity {
public:
    explicit EntityLivingBase(World* worldIn);
    ~EntityLivingBase() override;

    void onKillCommand() override;
    virtual bool canBreatheUnderwater() { return false; }
    void onEntityUpdate() override;
    virtual bool isChild() const { return false; }
    Random& getRNG() { return rand; }
    EntityLivingBase* getAITarget() const;
    int getRevengeTimer() const { return revengeTimer; }
    void setRevengeTarget(EntityLivingBase* livingBase);
    EntityLivingBase* getLastAttacker() const;
    int getLastAttackerTime() const { return lastAttackerTime; }
    void setLastAttacker(Entity* entityIn);
    int getAge() const { return entityAge; }
    void writeEntityToNBT(NBTTagCompound& tagCompound) override;
    void readEntityFromNBT(const NBTTagCompound& tagCompund) override;
    void clearActivePotions();
    // Java: getActivePotionEffects -- the values in the map's iteration order.
    std::vector<PotionEffect> getActivePotionEffects() const;
    bool isPotionActive(int potionId) const { return activePotionsMap.contains(potionId); }
    bool isPotionActive(const Potion* potionIn) const { return activePotionsMap.contains(potionIn->id); }
    // Null when the potion isn't active.
    const PotionEffect* getActivePotionEffect(const Potion* potionIn) const { return activePotionsMap.find(potionIn->id); }
    void addPotionEffect(const PotionEffect& potioneffectIn);
    virtual bool isPotionApplicable(const PotionEffect& potioneffectIn);
    bool isEntityUndead() { return getCreatureAttribute() == EnumCreatureAttribute::UNDEAD; }
    void removePotionEffectClient(int potionId) { activePotionsMap.erase(potionId); }
    void removePotionEffect(int potionId);
    virtual void heal(float healAmount);
    float getHealth() const { return dataWatcher.getWatchableObjectFloat(6); }
    void setHealth(float health);
    bool attackEntityFrom(const DamageSource& source, float amount) override;
    void renderBrokenItemStack(const ItemStack& stack);
    virtual void onDeath(const DamageSource& cause);
    virtual void knockBack(Entity* entityIn, float p_70653_2_, double p_70653_3_, double p_70653_5_);
    virtual bool isOnLadder();
    bool isEntityAlive() override { return !isDead && getHealth() > 0.0f; }
    void fall(float distance, float damageMultiplier) override;
    void performHurtAnimation() override;
    virtual int getTotalArmorValue();
    CombatTracker& getCombatTracker() { return _combatTracker; }
    EntityLivingBase* getAttackingEntity();
    float getMaxHealth() const;
    int getArrowCountInEntity() const { return dataWatcher.getWatchableObjectByte(9); }
    void setArrowCountInEntity(int count) { dataWatcher.updateObject(9, static_cast<int8_t>(count)); }
    virtual void swingItem();
    void handleStatusUpdate(int8_t id) override;
    // Null when the attribute isn't registered.
    IAttributeInstance* getEntityAttribute(const IAttribute& attribute) { return getAttributeMap().getAttributeInstance(attribute); }
    BaseAttributeMap& getAttributeMap();
    virtual EnumCreatureAttribute getCreatureAttribute() { return EnumCreatureAttribute::UNDEFINED; }
    virtual std::shared_ptr<ItemStack> getHeldItem() = 0;
    virtual std::shared_ptr<ItemStack> getEquipmentInSlot(int slotIn) = 0;
    virtual std::shared_ptr<ItemStack> getCurrentArmor(int slotIn) = 0;
    void setCurrentItemOrArmor(int slotIn, std::shared_ptr<ItemStack> stack) override = 0;
    void setSprinting(bool sprinting) override;
    std::vector<std::shared_ptr<ItemStack>> getInventory() override = 0;
    void dismountEntity(Entity* entityIn);
    bool getAlwaysRenderNameTagForRender() override { return false; }
    virtual void moveEntityWithHeading(float strafe, float forward);
    virtual float getAIMoveSpeed() { return landMovementFactor; }
    virtual void setAIMoveSpeed(float speedIn) { landMovementFactor = speedIn; }
    virtual bool attackEntityAsMob(Entity* entityIn);
    virtual bool isPlayerSleeping() { return false; }
    void onUpdate() override;
    virtual void onLivingUpdate();
    void mountEntity(Entity* entityIn) override;
    void updateRidden() override;
    void setPositionAndRotation2(double x, double y, double z, float yaw, float pitch, int posRotationIncrements, bool p_180426_10_) override;
    void setJumping(bool jumping) { isJumping = jumping; }
    virtual void onItemPickup(Entity* p_71001_1_, int p_71001_2_);
    bool canEntityBeSeen(Entity* entityIn);
    std::optional<Vec3> getLookVec() override { return getLook(1.0f); }
    Vec3 getLook(float partialTicks) override;
    float getSwingProgress(float partialTickTime) const;
    virtual bool isServerWorld();
    bool canBeCollidedWith() override { return !isDead; }
    bool canBePushed() override { return !isDead; }
    float getRotationYawHead() override { return rotationYawHead; }
    void setRotationYawHead(float rotation) override { rotationYawHead = rotation; }
    void setRenderYawOffset(float offset) override { renderYawOffset = offset; }
    virtual float getAbsorptionAmount() { return absorptionAmount; }
    virtual void setAbsorptionAmount(float amount);
    virtual Team* getTeam() { return nullptr; }
    bool isOnSameTeam(EntityLivingBase* otherEntity) { return isOnTeam(otherEntity->getTeam()); }
    bool isOnTeam(Team* teamIn);
    virtual void sendEnterCombat() { }
    virtual void sendEndCombat() { }

    bool isSwingInProgress = false;
    int swingProgressInt = 0;
    int arrowHitTimer = 0;
    int hurtTime = 0;
    int maxHurtTime = 0;
    float attackedAtYaw = 0.0f;
    int deathTime = 0;
    float prevSwingProgress = 0.0f;
    float swingProgress = 0.0f;
    float prevLimbSwingAmount = 0.0f;
    float limbSwingAmount = 0.0f;
    float limbSwing = 0.0f;
    int maxHurtResistantTime = 20;
    float prevCameraPitch = 0.0f;
    float cameraPitch = 0.0f;
    float randomUnused2 = 0.0f;
    float randomUnused1 = 0.0f;
    float renderYawOffset = 0.0f;
    float prevRenderYawOffset = 0.0f;
    float rotationYawHead = 0.0f;
    float prevRotationYawHead = 0.0f;
    float jumpMovementFactor = 0.02f;
    float moveStrafing = 0.0f;
    float moveForward = 0.0f;

protected:
    void construct() override;
    void entityInit() override;
    virtual void applyEntityAttributes();
    void updateFallState(double y, bool onGroundIn, Block* blockIn, const BlockPos& pos) override;
    virtual void onDeathUpdate();
    virtual bool canDropLoot() { return !isChild(); }
    virtual int decreaseAirSupply(int p_70682_1_);
    virtual int getExperiencePoints(EntityPlayer*) { return 0; }
    virtual bool isPlayer() { return false; }
    virtual void updatePotionEffects();
    virtual void updatePotionMetadata();
    void resetPotionEffectMetadata();
    virtual void onNewPotionEffect(const PotionEffect& id);
    virtual void onChangedPotionEffect(const PotionEffect& id, bool p_70695_2_);
    virtual void onFinishedPotionEffect(const PotionEffect& effect);
    virtual void dropEquipment(bool, int) { }
    // Java: null for no sound.
    virtual std::optional<std::string> getHurtSound() { return "game.neutral.hurt"; }
    virtual std::optional<std::string> getDeathSound() { return "game.neutral.die"; }
    virtual void addRandomDrop() { }
    virtual void dropFewItems(bool, int) { }
    virtual std::string getFallSoundString(int damageValue) { return damageValue > 4 ? "game.neutral.hurt.fall.big" : "game.neutral.hurt.fall.small"; }
    virtual void damageArmor(float) { }
    virtual float applyArmorCalculations(const DamageSource& source, float damage);
    virtual float applyPotionDamageCalculations(const DamageSource& source, float damage);
    virtual void damageEntity(const DamageSource& damageSrc, float damageAmount);
    void kill() override;
    void updateArmSwingProgress();
    virtual float getSoundVolume() { return 1.0f; }
    virtual float getSoundPitch();
    virtual bool isMovementBlocked() { return getHealth() <= 0.0f; }
    virtual float getJumpUpwardsMotion() { return 0.42f; }
    virtual void jump();
    virtual void updateAITick() { motionY += 0.03999999910593033; }
    virtual void handleJumpLava() { motionY += 0.03999999910593033; }
    virtual float updateDistance(float p_110146_1_, float p_110146_2_);
    virtual void updateEntityActionState() { }
    virtual void collideWithNearbyEntities();
    virtual void collideWithEntity(Entity* entityIn) { entityIn->applyEntityCollision(this); }
    void setBeenAttacked() override;
    void markPotionsDirty() { potionsNeedUpdate = true; }

    EntityPlayer* getAttackingPlayer() const;
    void setAttackingPlayer(EntityPlayer* player);

    int recentlyHit = 0;
    bool dead = false;
    int entityAge = 0;
    float prevOnGroundSpeedFactor = 0.0f;
    float onGroundSpeedFactor = 0.0f;
    float movedDistance = 0.0f;
    float prevMovedDistance = 0.0f;
    float unused180 = 0.0f;
    int scoreValue = 0;
    float lastDamage = 0.0f;
    bool isJumping = false;
    float randomYawVelocity = 0.0f;
    int newPosRotationIncrements = 0;
    double newPosX = 0.0;
    double newPosY = 0.0;
    double newPosZ = 0.0;
    double newRotationYaw = 0.0;
    double newRotationPitch = 0.0;

private:
    int getArmSwingAnimationEnd() const;

    std::unique_ptr<BaseAttributeMap> attributeMap;
    CombatTracker _combatTracker { this };
    JavaHashMap<int32_t, PotionEffect> activePotionsMap;
    std::array<std::shared_ptr<ItemStack>, 5> previousEquipment {};
    std::weak_ptr<Entity> attackingPlayer;
    bool potionsNeedUpdate = true;
    std::weak_ptr<Entity> entityLivingToAttack;
    int revengeTimer = 0;
    std::weak_ptr<Entity> lastAttacker;
    int lastAttackerTime = 0;
    float landMovementFactor = 0.0f;
    int jumpTicks = 0;
    float absorptionAmount = 0.0f;
};

} // namespace mc
