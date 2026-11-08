#pragma once

// Original: net/minecraft/network/play/server/S*.java -- the play packets the server sends. Constructors take
// what Java's do (entities by pointer, stacks are copied as in Java); getters keep the Java names.
// Each packet also has its own header under network/play/server/, as the Java files.

#include "entity/DataWatcher.h"
#include "entity/ai/attributes/Attributes.h"
#include "network/play/INetHandlerPlayClient.h"
#include "util/BlockPos.h"
#include "util/EnumFacing.h"
#include "util/EnumParticleTypes.h"
#include "util/UUID.h"
#include "util/Vec3.h"
#include "world/ChunkCoordIntPair.h"
#include "world/EnumDifficulty.h"
#include "world/WorldSettings.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#define MC_PLAY_CLIENT_PACKET(NAME, METHOD)                                                                                                                  \
    std::string_view getPacketName() const override { return #NAME; }                                                                                    \
    void processPacket(INetHandler& handler) override { static_cast<INetHandlerPlayClient&>(handler).METHOD(*this); }

namespace mc {

class Block;
class Chunk;
class CombatTracker;
class Entity;
class EntityLivingBase;
class EntityPlayer;
class EntityPlayerMP;
class IAttributeInstance;
class IBlockState;
class IChatComponent;
class ItemStack;
class NBTTagCompound;
class PlayerCapabilities;
class PotionEffect;
class World;
class WorldBorder;
class WorldType;
struct GameProfile;

// Java: (byte)((int)(angle * 256.0F / 360.0F))
int8_t packAngle(float angle);

class S00PacketKeepAlive : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S00PacketKeepAlive, handleKeepAlive)
    explicit S00PacketKeepAlive(int idIn)
        : id(idIn)
    {
    }
    int func_149134_c() const { return id; }

private:
    int id;
};

class S01PacketJoinGame : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S01PacketJoinGame, handleJoinGame)
    S01PacketJoinGame(int entityIdIn, GameType gameTypeIn, bool hardcoreModeIn, int dimensionIn, EnumDifficulty difficultyIn, int maxPlayersIn,
        const WorldType* worldTypeIn, bool reducedDebugInfoIn)
        : entityId(entityIdIn)
        , hardcoreMode(hardcoreModeIn)
        , gameType(gameTypeIn)
        , dimension(dimensionIn)
        , difficulty(difficultyIn)
        , maxPlayers(maxPlayersIn)
        , worldType(worldTypeIn)
        , reducedDebugInfo(reducedDebugInfoIn)
    {
    }
    int getEntityId() const { return entityId; }
    bool isHardcoreMode() const { return hardcoreMode; }
    GameType getGameType() const { return gameType; }
    int getDimension() const { return dimension; }
    EnumDifficulty getDifficulty() const { return difficulty; }
    int getMaxPlayers() const { return maxPlayers; }
    const WorldType* getWorldType() const { return worldType; }
    bool isReducedDebugInfo() const { return reducedDebugInfo; }

private:
    int entityId;
    bool hardcoreMode;
    GameType gameType;
    int dimension;
    EnumDifficulty difficulty;
    int maxPlayers;
    const WorldType* worldType;
    bool reducedDebugInfo;
};

class S02PacketChat : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S02PacketChat, handleChat)
    explicit S02PacketChat(std::shared_ptr<IChatComponent> component, int8_t typeIn = 1)
        : chatComponent(std::move(component))
        , type(typeIn)
    {
    }
    const std::shared_ptr<IChatComponent>& getChatComponent() const { return chatComponent; }
    bool isChat() const { return type == 1 || type == 2; }
    int8_t getType() const { return type; }

private:
    std::shared_ptr<IChatComponent> chatComponent;
    int8_t type;
};

class S03PacketTimeUpdate : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S03PacketTimeUpdate, handleTimeUpdate)
    S03PacketTimeUpdate(int64_t totalWorldTimeIn, int64_t totalTimeIn, bool doDayLightCycle)
        : totalWorldTime(totalWorldTimeIn)
        , worldTime(totalTimeIn)
    {
        if (!doDayLightCycle) {
            worldTime = -worldTime;
            if (worldTime == 0)
                worldTime = -1;
        }
    }
    int64_t getTotalWorldTime() const { return totalWorldTime; }
    int64_t getWorldTime() const { return worldTime; }

private:
    int64_t totalWorldTime;
    int64_t worldTime;
};

class S04PacketEntityEquipment : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S04PacketEntityEquipment, handleEntityEquipment)
    S04PacketEntityEquipment(int entityIDIn, int slot, const ItemStack* itemStackIn);
    const std::shared_ptr<ItemStack>& getItemStack() const { return itemStack; }
    int getEntityID() const { return entityID; }
    int getEquipmentSlot() const { return equipmentSlot; }

private:
    int entityID;
    int equipmentSlot;
    std::shared_ptr<ItemStack> itemStack;
};

class S05PacketSpawnPosition : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S05PacketSpawnPosition, handleSpawnPosition)
    explicit S05PacketSpawnPosition(const BlockPos& pos)
        : spawnBlockPos(pos)
    {
    }
    const BlockPos& getSpawnPos() const { return spawnBlockPos; }

private:
    BlockPos spawnBlockPos;
};

class S06PacketUpdateHealth : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S06PacketUpdateHealth, handleUpdateHealth)
    S06PacketUpdateHealth(float healthIn, int foodLevelIn, float saturationIn)
        : health(healthIn)
        , foodLevel(foodLevelIn)
        , saturationLevel(saturationIn)
    {
    }
    float getHealth() const { return health; }
    int getFoodLevel() const { return foodLevel; }
    float getSaturationLevel() const { return saturationLevel; }

private:
    float health;
    int foodLevel;
    float saturationLevel;
};

class S07PacketRespawn : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S07PacketRespawn, handleRespawn)
    S07PacketRespawn(int dimensionIDIn, EnumDifficulty difficultyIn, const WorldType* worldTypeIn, GameType gameTypeIn)
        : dimensionID(dimensionIDIn)
        , difficulty(difficultyIn)
        , gameType(gameTypeIn)
        , worldType(worldTypeIn)
    {
    }
    int getDimensionID() const { return dimensionID; }
    EnumDifficulty getDifficulty() const { return difficulty; }
    GameType getGameType() const { return gameType; }
    const WorldType* getWorldType() const { return worldType; }

private:
    int dimensionID;
    EnumDifficulty difficulty;
    GameType gameType;
    const WorldType* worldType;
};

class S08PacketPlayerPosLook : public Packet {
public:
    enum class EnumFlags { X, Y, Z, Y_ROT, X_ROT };

    MC_PLAY_CLIENT_PACKET(S08PacketPlayerPosLook, handlePlayerPosLook)
    S08PacketPlayerPosLook(double xIn, double yIn, double zIn, float yawIn, float pitchIn, std::set<EnumFlags> flags)
        : x(xIn)
        , y(yIn)
        , z(zIn)
        , yaw(yawIn)
        , pitch(pitchIn)
        , field_179835_f(std::move(flags))
    {
    }
    double getX() const { return x; }
    double getY() const { return y; }
    double getZ() const { return z; }
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }
    const std::set<EnumFlags>& func_179834_f() const { return field_179835_f; }

private:
    double x;
    double y;
    double z;
    float yaw;
    float pitch;
    std::set<EnumFlags> field_179835_f;
};

class S09PacketHeldItemChange : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S09PacketHeldItemChange, handleHeldItemChange)
    explicit S09PacketHeldItemChange(int hotbarIndexIn)
        : heldItemHotbarIndex(hotbarIndexIn)
    {
    }
    int getHeldItemHotbarIndex() const { return heldItemHotbarIndex; }

private:
    int heldItemHotbarIndex;
};

class S0APacketUseBed : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S0APacketUseBed, handleUseBed)
    S0APacketUseBed(EntityPlayer* player, const BlockPos& bedPosIn);
    EntityPlayer* getPlayer(World* worldIn) const;
    const BlockPos& getBedPosition() const { return bedPos; }

private:
    int playerID;
    BlockPos bedPos;
};

class S0BPacketAnimation : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S0BPacketAnimation, handleAnimation)
    S0BPacketAnimation(Entity* ent, int animationType);
    int getEntityID() const { return entityId; }
    int getAnimationType() const { return type; }

private:
    int entityId;
    int type;
};

class S0CPacketSpawnPlayer : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S0CPacketSpawnPlayer, handleSpawnPlayer)
    // The DataWatcher is read at construction (Java reads it lazily on the client thread).
    explicit S0CPacketSpawnPlayer(EntityPlayer* player);
    const std::vector<DataWatcher::WatchableObject>& func_148944_c() const { return field_148958_j; }
    int getEntityID() const { return entityId; }
    const UUID& getPlayer() const { return playerId; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getZ() const { return z; }
    int8_t getYaw() const { return yaw; }
    int8_t getPitch() const { return pitch; }
    int getCurrentItemID() const { return currentItem; }

private:
    int entityId;
    UUID playerId;
    int x;
    int y;
    int z;
    int8_t yaw;
    int8_t pitch;
    int currentItem;
    std::vector<DataWatcher::WatchableObject> field_148958_j;
};

class S0DPacketCollectItem : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S0DPacketCollectItem, handleCollectItem)
    S0DPacketCollectItem(int collectedItemEntityIdIn, int entityIdIn)
        : collectedItemEntityId(collectedItemEntityIdIn)
        , entityId(entityIdIn)
    {
    }
    int getCollectedItemEntityID() const { return collectedItemEntityId; }
    int getEntityID() const { return entityId; }

private:
    int collectedItemEntityId;
    int entityId;
};

class S0EPacketSpawnObject : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S0EPacketSpawnObject, handleSpawnObject)
    S0EPacketSpawnObject(Entity* entityIn, int typeIn, int data = 0);
    int getEntityID() const { return entityId; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getZ() const { return z; }
    int getSpeedX() const { return speedX; }
    int getSpeedY() const { return speedY; }
    int getSpeedZ() const { return speedZ; }
    int getPitch() const { return pitch; }
    int getYaw() const { return yaw; }
    int getType() const { return type; }
    int func_149009_m() const { return field_149020_k; }
    void setX(int v) { x = v; }
    void setY(int v) { y = v; }
    void setZ(int v) { z = v; }
    void setSpeedX(int v) { speedX = v; }
    void setSpeedY(int v) { speedY = v; }
    void setSpeedZ(int v) { speedZ = v; }
    void func_149002_g(int v) { field_149020_k = v; }

private:
    int entityId;
    int x;
    int y;
    int z;
    int speedX = 0;
    int speedY = 0;
    int speedZ = 0;
    int pitch;
    int yaw;
    int type;
    int field_149020_k;
};

class S0FPacketSpawnMob : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S0FPacketSpawnMob, handleSpawnMob)
    explicit S0FPacketSpawnMob(EntityLivingBase* entityIn);
    const std::vector<DataWatcher::WatchableObject>& func_149027_c() const { return watcher; }
    int getEntityID() const { return entityId; }
    int getEntityType() const { return type; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getZ() const { return z; }
    int getVelocityX() const { return velocityX; }
    int getVelocityY() const { return velocityY; }
    int getVelocityZ() const { return velocityZ; }
    int8_t getYaw() const { return yaw; }
    int8_t getPitch() const { return pitch; }
    int8_t getHeadPitch() const { return headPitch; }

private:
    int entityId;
    int type;
    int x;
    int y;
    int z;
    int velocityX;
    int velocityY;
    int velocityZ;
    int8_t yaw;
    int8_t pitch;
    int8_t headPitch;
    std::vector<DataWatcher::WatchableObject> watcher;
};

class S10PacketSpawnPainting : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S10PacketSpawnPainting, handleSpawnPainting)
    S10PacketSpawnPainting(int entityIDIn, const BlockPos& positionIn, EnumFacing facingIn, std::string titleIn)
        : entityID(entityIDIn)
        , position(positionIn)
        , facing(facingIn)
        , title(std::move(titleIn))
    {
    }
    int getEntityID() const { return entityID; }
    const BlockPos& getPosition() const { return position; }
    EnumFacing getFacing() const { return facing; }
    const std::string& getTitle() const { return title; }

private:
    int entityID;
    BlockPos position;
    EnumFacing facing;
    std::string title;
};

class S11PacketSpawnExperienceOrb : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S11PacketSpawnExperienceOrb, handleSpawnExperienceOrb)
    S11PacketSpawnExperienceOrb(Entity* xpOrb, int xpValueIn);
    int getEntityID() const { return entityID; }
    int getX() const { return posX; }
    int getY() const { return posY; }
    int getZ() const { return posZ; }
    int getXPValue() const { return xpValue; }

private:
    int entityID;
    int posX;
    int posY;
    int posZ;
    int xpValue;
};

class S12PacketEntityVelocity : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S12PacketEntityVelocity, handleEntityVelocity)
    explicit S12PacketEntityVelocity(Entity* entityIn);
    S12PacketEntityVelocity(int entityIDIn, double motionXIn, double motionYIn, double motionZIn);
    int getEntityID() const { return entityID; }
    int getMotionX() const { return motionX; }
    int getMotionY() const { return motionY; }
    int getMotionZ() const { return motionZ; }

private:
    int entityID;
    int motionX;
    int motionY;
    int motionZ;
};

class S13PacketDestroyEntities : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S13PacketDestroyEntities, handleDestroyEntities)
    explicit S13PacketDestroyEntities(std::vector<int> entityIDsIn)
        : entityIDs(std::move(entityIDsIn))
    {
    }
    const std::vector<int>& getEntityIDs() const { return entityIDs; }

private:
    std::vector<int> entityIDs;
};

class S14PacketEntity : public Packet {
public:
    class S15PacketEntityRelMove;
    class S16PacketEntityLook;
    class S17PacketEntityLookMove;

    MC_PLAY_CLIENT_PACKET(S14PacketEntity, handleEntityMovement)
    explicit S14PacketEntity(int entityIdIn)
        : entityId(entityIdIn)
    {
    }
    Entity* getEntity(World* worldIn) const;
    int8_t func_149062_c() const { return posX; }
    int8_t func_149061_d() const { return posY; }
    int8_t func_149064_e() const { return posZ; }
    int8_t func_149066_f() const { return yaw; }
    int8_t func_149063_g() const { return pitch; }
    bool func_149060_h() const { return field_149069_g; }
    bool getOnGround() const { return onGround; }

protected:
    int entityId;
    int8_t posX = 0;
    int8_t posY = 0;
    int8_t posZ = 0;
    int8_t yaw = 0;
    int8_t pitch = 0;
    bool onGround = false;
    bool field_149069_g = false;
};

class S14PacketEntity::S15PacketEntityRelMove : public S14PacketEntity {
public:
    std::string_view getPacketName() const override { return "S15PacketEntityRelMove"; }
    S15PacketEntityRelMove(int entityIdIn, int8_t x, int8_t y, int8_t z, bool onGroundIn)
        : S14PacketEntity(entityIdIn)
    {
        posX = x;
        posY = y;
        posZ = z;
        onGround = onGroundIn;
    }
};

class S14PacketEntity::S16PacketEntityLook : public S14PacketEntity {
public:
    std::string_view getPacketName() const override { return "S16PacketEntityLook"; }
    S16PacketEntityLook(int entityIdIn, int8_t yawIn, int8_t pitchIn, bool onGroundIn)
        : S14PacketEntity(entityIdIn)
    {
        yaw = yawIn;
        pitch = pitchIn;
        field_149069_g = true;
        onGround = onGroundIn;
    }
};

class S14PacketEntity::S17PacketEntityLookMove : public S14PacketEntity {
public:
    std::string_view getPacketName() const override { return "S17PacketEntityLookMove"; }
    S17PacketEntityLookMove(int entityIdIn, int8_t x, int8_t y, int8_t z, int8_t yawIn, int8_t pitchIn, bool onGroundIn)
        : S14PacketEntity(entityIdIn)
    {
        posX = x;
        posY = y;
        posZ = z;
        yaw = yawIn;
        pitch = pitchIn;
        onGround = onGroundIn;
        field_149069_g = true;
    }
};

class S18PacketEntityTeleport : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S18PacketEntityTeleport, handleEntityTeleport)
    explicit S18PacketEntityTeleport(Entity* entityIn);
    S18PacketEntityTeleport(int entityIdIn, int posXIn, int posYIn, int posZIn, int8_t yawIn, int8_t pitchIn, bool onGroundIn)
        : entityId(entityIdIn)
        , posX(posXIn)
        , posY(posYIn)
        , posZ(posZIn)
        , yaw(yawIn)
        , pitch(pitchIn)
        , onGround(onGroundIn)
    {
    }
    int getEntityId() const { return entityId; }
    int getX() const { return posX; }
    int getY() const { return posY; }
    int getZ() const { return posZ; }
    int8_t getYaw() const { return yaw; }
    int8_t getPitch() const { return pitch; }
    bool getOnGround() const { return onGround; }

private:
    int entityId;
    int posX;
    int posY;
    int posZ;
    int8_t yaw;
    int8_t pitch;
    bool onGround;
};

class S19PacketEntityHeadLook : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S19PacketEntityHeadLook, handleEntityHeadLook)
    S19PacketEntityHeadLook(Entity* entityIn, int8_t yawIn);
    Entity* getEntity(World* worldIn) const;
    int8_t getYaw() const { return yaw; }

private:
    int entityId;
    int8_t yaw;
};

class S19PacketEntityStatus : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S19PacketEntityStatus, handleEntityStatus)
    S19PacketEntityStatus(Entity* entityIn, int8_t opCodeIn);
    Entity* getEntity(World* worldIn) const;
    int8_t getOpCode() const { return logicOpcode; }

private:
    int entityId;
    int8_t logicOpcode;
};

class S1BPacketEntityAttach : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S1BPacketEntityAttach, handleEntityAttach)
    S1BPacketEntityAttach(int leashIn, Entity* entityIn, Entity* vehicle);
    int getLeash() const { return leash; }
    int getEntityId() const { return entityId; }
    int getVehicleEntityId() const { return vehicleEntityId; }

private:
    int leash;
    int entityId;
    int vehicleEntityId;
};

class S1CPacketEntityMetadata : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S1CPacketEntityMetadata, handleEntityMetadata)
    S1CPacketEntityMetadata(int entityIdIn, DataWatcher& watcher, bool all)
        : entityId(entityIdIn)
        , field_149378_b(all ? watcher.getAllWatched() : watcher.getChanged())
    {
    }
    // Java: null when nothing changed -- empty here.
    const std::vector<DataWatcher::WatchableObject>& func_149376_c() const { return field_149378_b; }
    int getEntityId() const { return entityId; }

private:
    int entityId;
    std::vector<DataWatcher::WatchableObject> field_149378_b;
};

class S1DPacketEntityEffect : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S1DPacketEntityEffect, handleEntityEffect)
    S1DPacketEntityEffect(int entityIdIn, const PotionEffect& effect);
    bool func_149429_c() const { return duration == 32767; }
    int getEntityId() const { return entityId; }
    int8_t getEffectId() const { return effectId; }
    int8_t getAmplifier() const { return amplifier; }
    int getDuration() const { return duration; }
    bool func_179707_f() const { return hideParticles != 0; }

private:
    int entityId;
    int8_t effectId;
    int8_t amplifier;
    int duration;
    int8_t hideParticles;
};

class S1EPacketRemoveEntityEffect : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S1EPacketRemoveEntityEffect, handleRemoveEntityEffect)
    S1EPacketRemoveEntityEffect(int entityIdIn, const PotionEffect& effect);
    int getEntityId() const { return entityId; }
    int getEffectId() const { return effectId; }

private:
    int entityId;
    int effectId;
};

class S1FPacketSetExperience : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S1FPacketSetExperience, handleSetExperience)
    S1FPacketSetExperience(float experienceBar, int totalExperienceIn, int levelIn)
        : field_149401_a(experienceBar)
        , totalExperience(totalExperienceIn)
        , level(levelIn)
    {
    }
    float func_149397_c() const { return field_149401_a; }
    int getTotalExperience() const { return totalExperience; }
    int getLevel() const { return level; }

private:
    float field_149401_a;
    int totalExperience;
    int level;
};

class S20PacketEntityProperties : public Packet {
public:
    struct Snapshot {
        std::string field_151412_b;
        double field_151413_c;
        std::vector<AttributeModifier> field_151411_d;
        const std::string& func_151409_a() const { return field_151412_b; }
        double func_151410_b() const { return field_151413_c; }
        const std::vector<AttributeModifier>& func_151408_c() const { return field_151411_d; }
    };

    MC_PLAY_CLIENT_PACKET(S20PacketEntityProperties, handleEntityProperties)
    S20PacketEntityProperties(int entityIdIn, const std::vector<IAttributeInstance*>& attributes);
    int getEntityId() const { return entityId; }
    const std::vector<Snapshot>& func_149441_d() const { return field_149444_b; }

private:
    int entityId;
    std::vector<Snapshot> field_149444_b;
};

class S21PacketChunkData : public Packet {
public:
    struct Extracted {
        std::vector<uint8_t> data;
        int dataSize = 0;
    };

    MC_PLAY_CLIENT_PACKET(S21PacketChunkData, handleChunkData)
    S21PacketChunkData(Chunk* chunkIn, bool fullChunk, int sectionMask);
    const std::vector<uint8_t>& getExtractedDataBytes() const { return extractedData.data; }
    static Extracted getExtractedData(Chunk* chunk, bool fullChunk, bool hasSky, int sectionMask);
    int getChunkX() const { return chunkX; }
    int getChunkZ() const { return chunkZ; }
    int getExtractedSize() const { return extractedData.dataSize; }
    bool func_149274_i() const { return field_149279_g; }

private:
    static int func_180737_a(int sections, bool hasSky, bool fullChunk);

    int chunkX;
    int chunkZ;
    Extracted extractedData;
    bool field_149279_g;
};

class S22PacketMultiBlockChange : public Packet {
public:
    struct BlockUpdateData {
        int16_t chunkPosCrammed;
        const IBlockState* blockState;
        BlockPos pos;
        const BlockPos& getPos() const { return pos; }
        int16_t func_180089_b() const { return chunkPosCrammed; }
        const IBlockState* getBlockState() const { return blockState; }
    };

    MC_PLAY_CLIENT_PACKET(S22PacketMultiBlockChange, handleMultiBlockChange)
    S22PacketMultiBlockChange(int count, const int16_t* crammedPositionsIn, Chunk* chunkIn);
    const std::vector<BlockUpdateData>& getChangedBlocks() const { return changedBlocks; }

private:
    ChunkCoordIntPair chunkPosCoord;
    std::vector<BlockUpdateData> changedBlocks;
};

class S23PacketBlockChange : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S23PacketBlockChange, handleBlockChange)
    S23PacketBlockChange(World* worldIn, const BlockPos& blockPositionIn);
    const IBlockState* getBlockState() const { return blockState; }
    const BlockPos& getBlockPosition() const { return blockPosition; }

    // Java: the field is set directly by the server in a few places.
    const IBlockState* blockState;

private:
    BlockPos blockPosition;
};

class S24PacketBlockAction : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S24PacketBlockAction, handleBlockAction)
    S24PacketBlockAction(const BlockPos& blockPositionIn, Block* blockIn, int instrumentIn, int pitchIn)
        : blockPosition(blockPositionIn)
        , instrument(instrumentIn)
        , pitch(pitchIn)
        , block(blockIn)
    {
    }
    const BlockPos& getBlockPosition() const { return blockPosition; }
    int getData1() const { return instrument; }
    int getData2() const { return pitch; }
    Block* getBlockType() const { return block; }

private:
    BlockPos blockPosition;
    int instrument;
    int pitch;
    Block* block;
};

class S25PacketBlockBreakAnim : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S25PacketBlockBreakAnim, handleBlockBreakAnim)
    S25PacketBlockBreakAnim(int breakerIdIn, const BlockPos& pos, int progressIn)
        : breakerId(breakerIdIn)
        , position(pos)
        , progress(progressIn)
    {
    }
    int getBreakerId() const { return breakerId; }
    const BlockPos& getPosition() const { return position; }
    int getProgress() const { return progress; }

private:
    int breakerId;
    BlockPos position;
    int progress;
};

class S26PacketMapChunkBulk : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S26PacketMapChunkBulk, handleMapChunkBulk)
    explicit S26PacketMapChunkBulk(const std::vector<Chunk*>& chunks);
    int getChunkX(int i) const { return xPositions[static_cast<size_t>(i)]; }
    int getChunkZ(int i) const { return zPositions[static_cast<size_t>(i)]; }
    int getChunkCount() const { return static_cast<int>(xPositions.size()); }
    const std::vector<uint8_t>& getChunkBytes(int i) const { return chunksData[static_cast<size_t>(i)].data; }
    int getChunkSize(int i) const { return chunksData[static_cast<size_t>(i)].dataSize; }

private:
    std::vector<int> xPositions;
    std::vector<int> zPositions;
    std::vector<S21PacketChunkData::Extracted> chunksData;
    bool isOverworld;
};

class S27PacketExplosion : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S27PacketExplosion, handleExplosion)
    S27PacketExplosion(double x, double y, double z, float strengthIn, std::vector<BlockPos> affectedBlocksIn, const std::optional<Vec3>& playerMotion)
        : posX(x)
        , posY(y)
        , posZ(z)
        , strength(strengthIn)
        , affectedBlockPositions(std::move(affectedBlocksIn))
    {
        if (playerMotion) {
            field_149152_f = static_cast<float>(playerMotion->xCoord);
            field_149153_g = static_cast<float>(playerMotion->yCoord);
            field_149159_h = static_cast<float>(playerMotion->zCoord);
        }
    }
    float func_149149_c() const { return field_149152_f; }
    float func_149144_d() const { return field_149153_g; }
    float func_149147_e() const { return field_149159_h; }
    double getX() const { return posX; }
    double getY() const { return posY; }
    double getZ() const { return posZ; }
    float getStrength() const { return strength; }
    const std::vector<BlockPos>& getAffectedBlockPositions() const { return affectedBlockPositions; }

private:
    double posX;
    double posY;
    double posZ;
    float strength;
    std::vector<BlockPos> affectedBlockPositions;
    float field_149152_f = 0.0f;
    float field_149153_g = 0.0f;
    float field_149159_h = 0.0f;
};

class S28PacketEffect : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S28PacketEffect, handleEffect)
    S28PacketEffect(int soundTypeIn, const BlockPos& soundPosIn, int soundDataIn, bool serverWideIn)
        : soundType(soundTypeIn)
        , soundPos(soundPosIn)
        , soundData(soundDataIn)
        , serverWide(serverWideIn)
    {
    }
    bool isSoundServerwide() const { return serverWide; }
    int getSoundType() const { return soundType; }
    int getSoundData() const { return soundData; }
    const BlockPos& getSoundPos() const { return soundPos; }

private:
    int soundType;
    BlockPos soundPos;
    int soundData;
    bool serverWide;
};

class S29PacketSoundEffect : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S29PacketSoundEffect, handleSoundEffect)
    S29PacketSoundEffect(std::string soundNameIn, double soundX, double soundY, double soundZ, float volume, float pitch);
    const std::string& getSoundName() const { return soundName; }
    double getX() const { return static_cast<double>(static_cast<float>(posX) / 8.0f); }
    double getY() const { return static_cast<double>(static_cast<float>(posY) / 8.0f); }
    double getZ() const { return static_cast<double>(static_cast<float>(posZ) / 8.0f); }
    float getVolume() const { return soundVolume; }
    float getPitch() const { return static_cast<float>(soundPitch) / 63.0f; }

private:
    std::string soundName;
    int posX;
    int posY;
    int posZ;
    float soundVolume;
    int soundPitch;
};

class S2APacketParticles : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S2APacketParticles, handleParticles)
    S2APacketParticles(EnumParticleTypes particleTypeIn, bool longDistanceIn, float x, float y, float z, float xOffsetIn, float yOffsetIn, float zOffsetIn,
        float particleSpeedIn, int particleCountIn, std::vector<int> particleArgumentsIn)
        : particleType(particleTypeIn)
        , xCoord(x)
        , yCoord(y)
        , zCoord(z)
        , xOffset(xOffsetIn)
        , yOffset(yOffsetIn)
        , zOffset(zOffsetIn)
        , particleSpeed(particleSpeedIn)
        , particleCount(particleCountIn)
        , longDistance(longDistanceIn)
        , particleArguments(std::move(particleArgumentsIn))
    {
    }
    EnumParticleTypes getParticleType() const { return particleType; }
    bool isLongDistance() const { return longDistance; }
    double getXCoordinate() const { return static_cast<double>(xCoord); }
    double getYCoordinate() const { return static_cast<double>(yCoord); }
    double getZCoordinate() const { return static_cast<double>(zCoord); }
    float getXOffset() const { return xOffset; }
    float getYOffset() const { return yOffset; }
    float getZOffset() const { return zOffset; }
    float getParticleSpeed() const { return particleSpeed; }
    int getParticleCount() const { return particleCount; }
    const std::vector<int>& getParticleArgs() const { return particleArguments; }

private:
    EnumParticleTypes particleType;
    float xCoord;
    float yCoord;
    float zCoord;
    float xOffset;
    float yOffset;
    float zOffset;
    float particleSpeed;
    int particleCount;
    bool longDistance;
    std::vector<int> particleArguments;
};

class S2BPacketChangeGameState : public Packet {
public:
    static constexpr const char* MESSAGE_NAMES[1] { "tile.bed.notValid" };

    MC_PLAY_CLIENT_PACKET(S2BPacketChangeGameState, handleChangeGameState)
    S2BPacketChangeGameState(int stateIn, float value)
        : state(stateIn)
        , field_149141_c(value)
    {
    }
    int getGameState() const { return state; }
    float func_149137_d() const { return field_149141_c; }

private:
    int state;
    float field_149141_c;
};

class S2CPacketSpawnGlobalEntity : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S2CPacketSpawnGlobalEntity, handleSpawnGlobalEntity)
    // type 1 is the lightning bolt, the only global entity.
    S2CPacketSpawnGlobalEntity(Entity* entityIn, int typeIn);
    int func_149052_c() const { return entityId; }
    int func_149051_d() const { return x; }
    int func_149050_e() const { return y; }
    int func_149049_f() const { return z; }
    int func_149053_g() const { return type; }

private:
    int entityId;
    int x;
    int y;
    int z;
    int type;
};

class S2DPacketOpenWindow : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S2DPacketOpenWindow, handleOpenWindow)
    S2DPacketOpenWindow(int windowIdIn, std::string guiId, std::shared_ptr<IChatComponent> windowTitleIn, int slotCountIn = 0, int incomingEntityId = 0)
        : windowId(windowIdIn)
        , inventoryType(std::move(guiId))
        , windowTitle(std::move(windowTitleIn))
        , slotCount(slotCountIn)
        , entityId(incomingEntityId)
    {
    }
    int getWindowId() const { return windowId; }
    const std::string& getGuiId() const { return inventoryType; }
    const std::shared_ptr<IChatComponent>& getWindowTitle() const { return windowTitle; }
    int getSlotCount() const { return slotCount; }
    int getEntityId() const { return entityId; }
    bool hasSlots() const { return slotCount > 0; }

private:
    int windowId;
    std::string inventoryType;
    std::shared_ptr<IChatComponent> windowTitle;
    int slotCount;
    int entityId;
};

class S2EPacketCloseWindow : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S2EPacketCloseWindow, handleCloseWindow)
    explicit S2EPacketCloseWindow(int windowIdIn)
        : windowId(windowIdIn)
    {
    }
    int getWindowId() const { return windowId; }

private:
    int windowId;
};

class S2FPacketSetSlot : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S2FPacketSetSlot, handleSetSlot)
    S2FPacketSetSlot(int windowIdIn, int slotIn, const ItemStack* itemIn);
    int func_149175_c() const { return windowId; }
    int func_149173_d() const { return slot; }
    const std::shared_ptr<ItemStack>& func_149174_e() const { return item; }

private:
    int windowId;
    int slot;
    std::shared_ptr<ItemStack> item;
};

class S30PacketWindowItems : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S30PacketWindowItems, handleWindowItems)
    S30PacketWindowItems(int windowIdIn, const std::vector<std::shared_ptr<ItemStack>>& stacks);
    int func_148911_c() const { return windowId; }
    const std::vector<std::shared_ptr<ItemStack>>& getItemStacks() const { return itemStacks; }

private:
    int windowId;
    std::vector<std::shared_ptr<ItemStack>> itemStacks;
};

class S31PacketWindowProperty : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S31PacketWindowProperty, handleWindowProperty)
    S31PacketWindowProperty(int windowIdIn, int varIndexIn, int varValueIn)
        : windowId(windowIdIn)
        , varIndex(varIndexIn)
        , varValue(varValueIn)
    {
    }
    int getWindowId() const { return windowId; }
    int getVarIndex() const { return varIndex; }
    int getVarValue() const { return varValue; }

private:
    int windowId;
    int varIndex;
    int varValue;
};

class S32PacketConfirmTransaction : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S32PacketConfirmTransaction, handleConfirmTransaction)
    S32PacketConfirmTransaction(int windowIdIn, int16_t actionNumberIn, bool accepted)
        : windowId(windowIdIn)
        , actionNumber(actionNumberIn)
        , field_148893_c(accepted)
    {
    }
    int getWindowId() const { return windowId; }
    int16_t getActionNumber() const { return actionNumber; }
    bool func_148888_e() const { return field_148893_c; }

private:
    int windowId;
    int16_t actionNumber;
    bool field_148893_c;
};

class S33PacketUpdateSign : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S33PacketUpdateSign, handleUpdateSign)
    S33PacketUpdateSign(World* worldIn, const BlockPos& blockPosIn, std::vector<std::shared_ptr<IChatComponent>> linesIn)
        : world(worldIn)
        , blockPos(blockPosIn)
        , lines(std::move(linesIn))
    {
        lines.resize(4);
    }
    const BlockPos& getPos() const { return blockPos; }
    const std::vector<std::shared_ptr<IChatComponent>>& getLines() const { return lines; }

private:
    World* world;
    BlockPos blockPos;
    std::vector<std::shared_ptr<IChatComponent>> lines;
};

class S35PacketUpdateTileEntity : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S35PacketUpdateTileEntity, handleUpdateTileEntity)
    S35PacketUpdateTileEntity(const BlockPos& blockPosIn, int metadataIn, std::shared_ptr<NBTTagCompound> nbtIn)
        : blockPos(blockPosIn)
        , metadata(metadataIn)
        , nbt(std::move(nbtIn))
    {
    }
    const BlockPos& getPos() const { return blockPos; }
    int getTileEntityType() const { return metadata; }
    const std::shared_ptr<NBTTagCompound>& getNbtCompound() const { return nbt; }

private:
    BlockPos blockPos;
    int metadata;
    std::shared_ptr<NBTTagCompound> nbt;
};

class S36PacketSignEditorOpen : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S36PacketSignEditorOpen, handleSignEditorOpen)
    explicit S36PacketSignEditorOpen(const BlockPos& pos)
        : signPosition(pos)
    {
    }
    const BlockPos& getSignPosition() const { return signPosition; }

private:
    BlockPos signPosition;
};

class S38PacketPlayerListItem : public Packet {
public:
    enum class Action { ADD_PLAYER, UPDATE_GAME_MODE, UPDATE_LATENCY, UPDATE_DISPLAY_NAME, REMOVE_PLAYER };

    struct AddPlayerData {
        std::shared_ptr<GameProfile> profile;
        int ping;
        GameType gamemode;
        std::shared_ptr<IChatComponent> displayName;
        const GameProfile& getProfile() const { return *profile; }
        int getPing() const { return ping; }
        GameType getGameMode() const { return gamemode; }
        const std::shared_ptr<IChatComponent>& getDisplayName() const { return displayName; }
    };

    MC_PLAY_CLIENT_PACKET(S38PacketPlayerListItem, handlePlayerListItem)
    S38PacketPlayerListItem(Action actionIn, const std::vector<EntityPlayerMP*>& players);
    const std::vector<AddPlayerData>& getEntries() const { return players; }
    Action getAction() const { return action; }

private:
    Action action;
    std::vector<AddPlayerData> players;
};

class S39PacketPlayerAbilities : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S39PacketPlayerAbilities, handlePlayerAbilities)
    explicit S39PacketPlayerAbilities(const PlayerCapabilities& capabilities);
    bool isInvulnerable() const { return invulnerable; }
    void setInvulnerable(bool v) { invulnerable = v; }
    bool isFlying() const { return flying; }
    void setFlying(bool v) { flying = v; }
    bool isAllowFlying() const { return allowFlying; }
    void setAllowFlying(bool v) { allowFlying = v; }
    bool isCreativeMode() const { return creativeMode; }
    void setCreativeMode(bool v) { creativeMode = v; }
    float getFlySpeed() const { return flySpeed; }
    void setFlySpeed(float v) { flySpeed = v; }
    float getWalkSpeed() const { return walkSpeed; }
    void setWalkSpeed(float v) { walkSpeed = v; }

private:
    bool invulnerable = false;
    bool flying = false;
    bool allowFlying = false;
    bool creativeMode = false;
    float flySpeed = 0.0f;
    float walkSpeed = 0.0f;
};

class S3APacketTabComplete : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S3APacketTabComplete, handleTabComplete)
    explicit S3APacketTabComplete(std::vector<std::string> matchesIn)
        : matches(std::move(matchesIn))
    {
    }
    const std::vector<std::string>& func_149630_c() const { return matches; }

private:
    std::vector<std::string> matches;
};

class S3FPacketCustomPayload : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S3FPacketCustomPayload, handleCustomPayload)
    // Java: a PacketBuffer; the bytes of it here. Throws std::invalid_argument above 1048576 bytes, as Java.
    S3FPacketCustomPayload(std::string channelName, std::vector<uint8_t> dataIn);
    const std::string& getChannelName() const { return channel; }
    const std::vector<uint8_t>& getBufferData() const { return data; }

private:
    std::string channel;
    std::vector<uint8_t> data;
};

class S40PacketDisconnect : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S40PacketDisconnect, handleDisconnect)
    explicit S40PacketDisconnect(std::shared_ptr<IChatComponent> reasonIn)
        : reason(std::move(reasonIn))
    {
    }
    const std::shared_ptr<IChatComponent>& getReason() const { return reason; }

private:
    std::shared_ptr<IChatComponent> reason;
};

class S41PacketServerDifficulty : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S41PacketServerDifficulty, handleServerDifficulty)
    S41PacketServerDifficulty(EnumDifficulty difficultyIn, bool lockedIn)
        : difficulty(difficultyIn)
        , difficultyLocked(lockedIn)
    {
    }
    bool isDifficultyLocked() const { return difficultyLocked; }
    EnumDifficulty getDifficulty() const { return difficulty; }

private:
    EnumDifficulty difficulty;
    bool difficultyLocked;
};

class S42PacketCombatEvent : public Packet {
public:
    enum class Event { ENTER_COMBAT, END_COMBAT, ENTITY_DIED };

    MC_PLAY_CLIENT_PACKET(S42PacketCombatEvent, handleCombatEvent)
    S42PacketCombatEvent(CombatTracker& combatTrackerIn, Event combatEventType);

    Event eventType;
    int field_179774_b = 0;
    int field_179775_c = 0;
    int field_179772_d = 0;
    std::string deathMessage;
};

class S43PacketCamera : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S43PacketCamera, handleCamera)
    explicit S43PacketCamera(Entity* entityIn);
    Entity* getEntity(World* worldIn) const;

    int entityId;
};

class S44PacketWorldBorder : public Packet {
public:
    enum class Action { SET_SIZE, LERP_SIZE, SET_CENTER, INITIALIZE, SET_WARNING_TIME, SET_WARNING_BLOCKS };

    MC_PLAY_CLIENT_PACKET(S44PacketWorldBorder, handleWorldBorder)
    S44PacketWorldBorder(const WorldBorder& border, Action actionIn);
    void func_179788_a(WorldBorder& border) const;

private:
    Action action;
    int size;
    double centerX;
    double centerZ;
    double targetSize;
    double diameter;
    int64_t timeUntilTarget;
    int warningTime;
    int warningDistance;
};

class S45PacketTitle : public Packet {
public:
    enum class Type { TITLE, SUBTITLE, TIMES, CLEAR, RESET };

    MC_PLAY_CLIENT_PACKET(S45PacketTitle, handleTitle)
    S45PacketTitle(Type typeIn, std::shared_ptr<IChatComponent> messageIn, int fadeIn = -1, int display = -1, int fadeOut = -1)
        : type(typeIn)
        , message(std::move(messageIn))
        , fadeInTime(fadeIn)
        , displayTime(display)
        , fadeOutTime(fadeOut)
    {
    }
    Type getType() const { return type; }
    const std::shared_ptr<IChatComponent>& getMessage() const { return message; }
    int getFadeInTime() const { return fadeInTime; }
    int getDisplayTime() const { return displayTime; }
    int getFadeOutTime() const { return fadeOutTime; }

private:
    Type type;
    std::shared_ptr<IChatComponent> message;
    int fadeInTime;
    int displayTime;
    int fadeOutTime;
};

class S46PacketSetCompressionLevel : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S46PacketSetCompressionLevel, handleSetCompressionLevel)
    explicit S46PacketSetCompressionLevel(int thresholdIn)
        : threshold(thresholdIn)
    {
    }
    int getThreshold() const { return threshold; }

private:
    int threshold;
};

class S47PacketPlayerListHeaderFooter : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S47PacketPlayerListHeaderFooter, handlePlayerListHeaderFooter)
    explicit S47PacketPlayerListHeaderFooter(std::shared_ptr<IChatComponent> headerIn)
        : header(std::move(headerIn))
    {
    }
    const std::shared_ptr<IChatComponent>& getHeader() const { return header; }
    const std::shared_ptr<IChatComponent>& getFooter() const { return footer; }

    std::shared_ptr<IChatComponent> footer;

private:
    std::shared_ptr<IChatComponent> header;
};

class S48PacketResourcePackSend : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S48PacketResourcePackSend, handleResourcePack)
    S48PacketResourcePackSend(std::string urlIn, std::string hashIn);
    const std::string& getURL() const { return url; }
    const std::string& getHash() const { return hash; }

private:
    std::string url;
    std::string hash;
};

class S49PacketUpdateEntityNBT : public Packet {
public:
    MC_PLAY_CLIENT_PACKET(S49PacketUpdateEntityNBT, handleEntityNBT)
    S49PacketUpdateEntityNBT(int entityIdIn, std::shared_ptr<NBTTagCompound> tagCompoundIn)
        : entityId(entityIdIn)
        , tagCompound(std::move(tagCompoundIn))
    {
    }
    const std::shared_ptr<NBTTagCompound>& getTagCompound() const { return tagCompound; }
    Entity* getEntity(World* worldIn) const;

private:
    int entityId;
    std::shared_ptr<NBTTagCompound> tagCompound;
};

} // namespace mc
