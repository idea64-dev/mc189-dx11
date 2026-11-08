#pragma once

// Original: net/minecraft/entity/Entity.java
// Entities are owned by std::shared_ptr (the world's lists, chunks, trackers); other references are raw pointers
// or shared pointers where Java keeps an entity alive (riding). Commands (ICommandSender, CommandResultStats),
// the hover event and crash report details come with the systems that need them.

#include "entity/DataWatcher.h"
#include "util/BlockPos.h"
#include "util/EnumFacing.h"
#include "util/Random.h"
#include "util/UUID.h"
#include "util/Vec3.h"

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mc {

class Block;
class DamageSource;
class EntityItem;
class EntityLivingBase;
class EntityPlayer;
class EntityPlayerMP;
class Explosion;
class IBlockState;
class IChatComponent;
class Item;
class ItemStack;
class Material;
class NBTTagCompound;
class NBTTagList;
class World;

class Entity : public std::enable_shared_from_this<Entity> {
public:
    explicit Entity(World* worldIn);
    virtual ~Entity();

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    // The Java constructor's call to the overridable entityInit, run after construction (see create()).
    template <class T, class... Args>
    static std::shared_ptr<T> create(Args&&... args)
    {
        auto entity = std::make_shared<T>(std::forward<Args>(args)...);
        static_cast<Entity*>(entity.get())->construct();
        return entity;
    }

    int getEntityId() const { return entityId; }
    void setEntityId(int id) { entityId = id; }
    virtual void onKillCommand() { setDead(); }
    DataWatcher& getDataWatcher() { return dataWatcher; }
    const DataWatcher& getDataWatcher() const { return dataWatcher; }
    int32_t hashCode() const { return entityId; }
    virtual void setDead() { isDead = true; }
    virtual void setPosition(double x, double y, double z);
    void setAngles(float yaw, float pitch);
    virtual void onUpdate() { onEntityUpdate(); }
    virtual void onEntityUpdate();
    virtual int getMaxInPortalTime() { return 0; }
    void setFire(int seconds);
    void extinguish() { fire = 0; }
    bool isOffsetPositionInLiquid(double x, double y, double z);
    virtual void moveEntity(double x, double y, double z);
    bool isSilent() const { return dataWatcher.getWatchableObjectByte(4) == 1; }
    void setSilent(bool value) { dataWatcher.updateObject(4, int8_t(value ? 1 : 0)); }
    virtual void playSound(const std::string& name, float volume, float pitch);
    // Java returns null for entities without a solid box.
    virtual std::optional<AxisAlignedBB> getCollisionBoundingBox() { return std::nullopt; }
    bool isImmuneToFire() const { return isImmuneToFireFlag; }
    virtual void fall(float distance, float damageMultiplier);
    bool isWet();
    virtual bool isInWater() { return inWater; }
    virtual bool handleWaterMovement();
    void spawnRunningParticles();
    bool isInsideOfMaterial(const Material& materialIn);
    bool isInLava();
    void moveFlying(float strafe, float forward, float friction);
    virtual int getBrightnessForRender(float partialTicks);
    virtual float getBrightness(float partialTicks);
    void setWorld(World* worldIn) { worldObj = worldIn; }
    void setPositionAndRotation(double x, double y, double z, float yaw, float pitch);
    void moveToBlockPosAndAngles(const BlockPos& pos, float yaw, float pitch);
    void setLocationAndAngles(double x, double y, double z, float yaw, float pitch);
    float getDistanceToEntity(const Entity* entityIn) const;
    double getDistanceSq(double x, double y, double z) const;
    double getDistanceSq(const BlockPos& pos) const { return pos.distanceSq(posX, posY, posZ); }
    double getDistanceSqToCenter(const BlockPos& pos) const { return pos.distanceSqToCenter(posX, posY, posZ); }
    double getDistance(double x, double y, double z) const;
    double getDistanceSqToEntity(const Entity* entityIn) const;
    virtual void onCollideWithPlayer(EntityPlayer*) { }
    virtual void applyEntityCollision(Entity* entityIn);
    virtual void addVelocity(double x, double y, double z);
    virtual bool attackEntityFrom(const DamageSource& source, float amount);
    virtual Vec3 getLook(float partialTicks);
    Vec3 getPositionEyes(float partialTicks);
    std::optional<MovingObjectPosition> rayTrace(double blockReachDistance, float partialTicks);
    virtual bool canBeCollidedWith() { return false; }
    virtual bool canBePushed() { return false; }
    virtual void addToPlayerScore(Entity*, int) { }
    virtual bool isInRangeToRender3d(double x, double y, double z);
    virtual bool isInRangeToRenderDist(double distance);
    bool writeMountToNBT(NBTTagCompound& tagCompund);
    bool writeToNBTOptional(NBTTagCompound& tagCompund);
    void writeToNBT(NBTTagCompound& tagCompund);
    void readFromNBT(const NBTTagCompound& tagCompund);
    virtual void onChunkLoad() { }
    // Java: dropItem / dropItemWithOffset / entityDropItem -- EntityItem comes with stage 10.
    virtual bool isEntityAlive() { return !isDead; }
    virtual bool isEntityInsideOpaqueBlock();
    virtual bool interactFirst(EntityPlayer*) { return false; }
    virtual std::optional<AxisAlignedBB> getCollisionBox(Entity*) { return std::nullopt; }
    virtual void updateRidden();
    virtual void updateRiderPosition();
    virtual double getYOffset() { return 0.0; }
    virtual double getMountedYOffset() { return static_cast<double>(height) * 0.75; }
    virtual void mountEntity(Entity* entityIn);
    virtual void setPositionAndRotation2(double x, double y, double z, float yaw, float pitch, int posRotationIncrements, bool teleport);
    virtual float getCollisionBorderSize() { return 0.1f; }
    // Java returns null for non-living entities.
    virtual std::optional<Vec3> getLookVec() { return std::nullopt; }
    void setPortal(const BlockPos& pos);
    virtual int getPortalCooldown() { return 300; }
    virtual void setVelocity(double x, double y, double z);
    virtual void handleStatusUpdate(int8_t) { }
    virtual void performHurtAnimation() { }
    // Java: getInventory -- the equipment of living entities (null otherwise: empty here).
    virtual std::vector<std::shared_ptr<ItemStack>> getInventory() { return {}; }
    virtual void setCurrentItemOrArmor(int, std::shared_ptr<ItemStack>) { }
    virtual bool isBurning() const;
    bool isRiding() const { return ridingEntity != nullptr; }
    virtual bool isSneaking() const { return getFlag(1); }
    virtual void setSneaking(bool sneaking) { setFlag(1, sneaking); }
    virtual bool isSprinting() const { return getFlag(3); }
    virtual void setSprinting(bool sprinting) { setFlag(3, sprinting); }
    virtual bool isInvisible() const { return getFlag(5); }
    virtual bool isInvisibleToPlayer(EntityPlayer* player);
    virtual void setInvisible(bool invisible) { setFlag(5, invisible); }
    bool isEating() const { return getFlag(4); }
    void setEating(bool eating) { setFlag(4, eating); }
    virtual int getAir() const { return dataWatcher.getWatchableObjectShort(1); }
    virtual void setAir(int air) { dataWatcher.updateObject(1, static_cast<int16_t>(air)); }
    virtual void onKillEntity(EntityLivingBase*) { }
    virtual void setInWeb()
    {
        isInWeb = true;
        fallDistance = 0.0f;
    }
    virtual std::string getName();
    virtual std::vector<Entity*> getParts() { return {}; }
    virtual bool isEntityEqual(const Entity* entityIn) const { return this == entityIn; }
    virtual float getRotationYawHead() { return 0.0f; }
    virtual void setRotationYawHead(float) { }
    virtual void setRenderYawOffset(float) { }
    virtual bool canAttackWithItem() { return true; }
    virtual bool hitByEntity(Entity*) { return false; }
    std::string toString();
    virtual bool isEntityInvulnerable(const DamageSource& source) const;
    void copyLocationAndAnglesFrom(const Entity* entityIn);
    virtual void copyDataFromOld(Entity* entityIn);
    // Java: travelToDimension -- the dimension change goes through the server (stage 9).
    virtual void travelToDimension(int dimensionId);
    virtual int getMaxFallHeight() { return 3; }
    std::optional<Vec3> func_181014_aG() const { return lastPortalVec; }
    std::optional<EnumFacing> getTeleportDirection() const { return teleportDirection; }
    virtual bool doesEntityNotTriggerPressurePlate() { return false; }
    virtual bool canRenderOnFire() { return isBurning(); }
    const UUID& getUniqueID() const { return entityUniqueID; }
    virtual bool isPushedByWater() { return true; }
    virtual std::shared_ptr<IChatComponent> getDisplayName();
    virtual void setCustomNameTag(const std::string& name) { dataWatcher.updateObject(2, name); }
    virtual std::string getCustomNameTag() const { return dataWatcher.getWatchableObjectString(2); }
    virtual bool hasCustomName() const { return !dataWatcher.getWatchableObjectString(2).empty(); }
    void setAlwaysRenderNameTag(bool value) { dataWatcher.updateObject(3, int8_t(value ? 1 : 0)); }
    bool getAlwaysRenderNameTag() const { return dataWatcher.getWatchableObjectByte(3) == 1; }
    virtual void setPositionAndUpdate(double x, double y, double z);
    virtual bool getAlwaysRenderNameTagForRender() { return getAlwaysRenderNameTag(); }
    virtual void onDataWatcherUpdate(int) { }
    EnumFacing getHorizontalFacing() const;
    virtual bool isSpectatedByPlayer(EntityPlayerMP*) { return true; }
    const AxisAlignedBB& getEntityBoundingBox() const { return boundingBox; }
    void setEntityBoundingBox(const AxisAlignedBB& bb) { boundingBox = bb; }
    virtual float getEyeHeight() const { return height * 0.85f; }
    bool isOutsideBorder() const { return outsideBorder; }
    void setOutsideBorder(bool value) { outsideBorder = value; }
    virtual bool replaceItemInInventory(int, std::shared_ptr<ItemStack>) { return false; }
    virtual void addChatMessage(std::shared_ptr<IChatComponent>) { }
    virtual bool canCommandSenderUseCommand(int, const std::string&) { return true; }
    BlockPos getPosition() const { return BlockPos(posX, posY + 0.5, posZ); }
    Vec3 getPositionVector() const { return Vec3(posX, posY, posZ); }
    World* getEntityWorld() const { return worldObj; }
    // Java: getNBTTagCompound / clientUpdateEntityNBT -- armor stands.
    virtual bool interactAt(EntityPlayer*, const Vec3&) { return false; }
    virtual bool isImmuneToExplosions() { return false; }
    // Java: getClass().getSimpleName(), for toString and the debug screen.
    virtual std::string_view getClassName() const = 0;

    static std::atomic<int> nextEntityID;

    double renderDistanceWeight = 1.0;
    bool preventEntitySpawning = false;
    std::shared_ptr<Entity> riddenByEntity;
    std::shared_ptr<Entity> ridingEntity;
    bool forceSpawn = false;
    World* worldObj;
    double prevPosX = 0.0;
    double prevPosY = 0.0;
    double prevPosZ = 0.0;
    double posX = 0.0;
    double posY = 0.0;
    double posZ = 0.0;
    double motionX = 0.0;
    double motionY = 0.0;
    double motionZ = 0.0;
    float rotationYaw = 0.0f;
    float rotationPitch = 0.0f;
    float prevRotationYaw = 0.0f;
    float prevRotationPitch = 0.0f;
    bool onGround = false;
    bool isCollidedHorizontally = false;
    bool isCollidedVertically = false;
    bool isCollided = false;
    bool velocityChanged = false;
    bool isDead = false;
    float width = 0.6f;
    float height = 1.8f;
    float prevDistanceWalkedModified = 0.0f;
    float distanceWalkedModified = 0.0f;
    float distanceWalkedOnStepModified = 0.0f;
    float fallDistance = 0.0f;
    double lastTickPosX = 0.0;
    double lastTickPosY = 0.0;
    double lastTickPosZ = 0.0;
    float stepHeight = 0.0f;
    bool noClip = false;
    float entityCollisionReduction = 0.0f;
    int ticksExisted = 0;
    int fireResistance = 1;
    int hurtResistantTime = 0;
    bool addedToChunk = false;
    int chunkCoordX = 0;
    int chunkCoordY = 0;
    int chunkCoordZ = 0;
    int serverPosX = 0;
    int serverPosY = 0;
    int serverPosZ = 0;
    bool ignoreFrustumCheck = false;
    bool isAirBorne = false;
    int timeUntilPortal = 0;
    int dimension = 0;

protected:
    // The Java constructor body after the fields (DataWatcher entries, entityInit).
    virtual void construct();
    virtual void entityInit() = 0;
    virtual void preparePlayerToSpawn();
    virtual void setSize(float widthIn, float heightIn);
    void setRotation(float yaw, float pitch);
    virtual void setOnFireFromLava();
    virtual void kill() { setDead(); }
    virtual std::string getSwimSound() { return "game.neutral.swim"; }
    virtual void doBlockCollisions();
    virtual void playStepSound(const BlockPos& pos, Block* blockIn);
    virtual bool canTriggerWalking() { return true; }
    virtual void updateFallState(double y, bool onGroundIn, Block* blockIn, const BlockPos& pos);
    virtual void dealFireDamage(int amount);
    virtual void resetHeight();
    virtual void createRunningParticles();
    virtual std::string getSplashSound() { return "game.neutral.swim.splash"; }
    virtual void setBeenAttacked() { velocityChanged = true; }
    Vec3 getVectorForRotation(float pitch, float yaw) const;
    virtual bool shouldSetPosAfterLoading() { return true; }
    std::optional<std::string> getEntityString() const;
    virtual void readEntityFromNBT(const NBTTagCompound& tagCompund) = 0;
    virtual void writeEntityToNBT(NBTTagCompound& tagCompound) = 0;
    static std::shared_ptr<NBTTagList> newDoubleNBTList(std::initializer_list<double> numbers);
    static std::shared_ptr<NBTTagList> newFloatNBTList(std::initializer_list<float> numbers);
    bool getFlag(int flag) const { return (dataWatcher.getWatchableObjectByte(0) & 1 << flag) != 0; }
    void setFlag(int flag, bool set);
    virtual bool pushOutOfBlocks(double x, double y, double z);
    std::shared_ptr<Entity> sharedThis() { return shared_from_this(); }

    bool isInWeb = false;
    Random rand;
    bool inWater = false;
    bool firstUpdate = true;
    bool isImmuneToFireFlag = false;
    DataWatcher dataWatcher;
    bool inPortal = false;
    int portalCounter = 0;
    std::optional<BlockPos> lastPortalPos;
    std::optional<Vec3> lastPortalVec;
    std::optional<EnumFacing> teleportDirection;
    UUID entityUniqueID;
    int fire = 0;

private:
    bool isLiquidPresentInAABB(const AxisAlignedBB& bb);
    void resetPositionToBB();

    int entityId;
    AxisAlignedBB boundingBox { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    bool outsideBorder = false;
    int nextStepDistance = 1;
    double entityRiderPitchDelta = 0.0;
    double entityRiderYawDelta = 0.0;
    bool invulnerable = false;
};

} // namespace mc
