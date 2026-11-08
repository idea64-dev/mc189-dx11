#include "entity/EntityTracker.h"

#include "core/Log.h"
#include "entity/DataWatcher.h"
#include "entity/Entity.h"
#include "entity/EntityLivingBase.h"
#include "entity/ai/attributes/Attributes.h"
#include "entity/player/EntityPlayer.h"
#include "entity/player/EntityPlayerMP.h"
#include "item/ItemStack.h"
#include "network/NetHandlerPlayServer.h"
#include "network/play/ServerPlayPackets.h"
#include "potion/Potion.h"
#include "server/MinecraftServer.h"
#include "server/management/PlayerManager.h"
#include "server/management/ServerConfigurationManager.h"
#include "util/MathHelper.h"
#include "world/WorldServer.h"
#include "world/chunk/Chunk.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace mc {

namespace {

int encodeAngle(float angle)
{
    return MathHelper::floor_float(angle * 256.0f / 360.0f);
}

} // namespace

EntityTrackerEntry::EntityTrackerEntry(Entity* trackedEntityIn, int trackingDistanceThresholdIn, int updateFrequencyIn, bool sendVelocityUpdatesIn)
    : trackedEntity(trackedEntityIn)
    , trackingDistanceThreshold(trackingDistanceThresholdIn)
    , updateFrequency(updateFrequencyIn)
    , encodedPosX(MathHelper::floor_double(trackedEntityIn->posX * 32.0))
    , encodedPosY(MathHelper::floor_double(trackedEntityIn->posY * 32.0))
    , encodedPosZ(MathHelper::floor_double(trackedEntityIn->posZ * 32.0))
    , encodedRotationYaw(encodeAngle(trackedEntityIn->rotationYaw))
    , encodedRotationPitch(encodeAngle(trackedEntityIn->rotationPitch))
    , lastHeadMotion(encodeAngle(trackedEntityIn->getRotationYawHead()))
    , sendVelocityUpdates(sendVelocityUpdatesIn)
    , onGround(trackedEntityIn->onGround)
{
}

bool EntityTrackerEntry::isTracking(EntityPlayerMP* playerMP) const
{
    return std::find(trackingPlayers.begin(), trackingPlayers.end(), playerMP) != trackingPlayers.end();
}

void EntityTrackerEntry::stopTracking(EntityPlayerMP* playerMP)
{
    if (const auto it = std::find(trackingPlayers.begin(), trackingPlayers.end(), playerMP); it != trackingPlayers.end())
        trackingPlayers.erase(it);
}

void EntityTrackerEntry::updatePlayerList(const std::vector<std::shared_ptr<EntityPlayer>>& players)
{
    playerEntitiesUpdated = false;

    if (!firstUpdateDone || trackedEntity->getDistanceSq(lastTrackedEntityPosX, lastTrackedEntityPosY, lastTrackedEntityPosZ) > 16.0) {
        lastTrackedEntityPosX = trackedEntity->posX;
        lastTrackedEntityPosY = trackedEntity->posY;
        lastTrackedEntityPosZ = trackedEntity->posZ;
        firstUpdateDone = true;
        playerEntitiesUpdated = true;
        updatePlayerEntities(players);
    }

    Entity* riding = trackedEntity->ridingEntity.get();
    if (field_85178_v != riding || (riding != nullptr && updateCounter % 60 == 0)) {
        field_85178_v = riding;
        sendPacketToTrackedPlayers(std::make_shared<S1BPacketEntityAttach>(0, trackedEntity, riding));
    }

    // Java: item frames holding a map send its data every 10 ticks -- with the item frames (stage 10).

    if (updateCounter % updateFrequency == 0 || trackedEntity->isAirBorne || trackedEntity->getDataWatcher().hasObjectChanged()) {
        if (trackedEntity->ridingEntity == nullptr) {
            ++ticksSinceLastForcedTeleport;
            const int k = MathHelper::floor_double(trackedEntity->posX * 32.0);
            const int j1 = MathHelper::floor_double(trackedEntity->posY * 32.0);
            const int k1 = MathHelper::floor_double(trackedEntity->posZ * 32.0);
            const int l1 = encodeAngle(trackedEntity->rotationYaw);
            const int i2 = encodeAngle(trackedEntity->rotationPitch);
            const int j2 = k - encodedPosX;
            const int k2 = j1 - encodedPosY;
            const int i = k1 - encodedPosZ;
            std::shared_ptr<Packet> packet1;
            const bool flag = std::abs(j2) >= 4 || std::abs(k2) >= 4 || std::abs(i) >= 4 || updateCounter % 60 == 0;
            const bool flag1 = std::abs(l1 - encodedRotationYaw) >= 4 || std::abs(i2 - encodedRotationPitch) >= 4;
            // Java: || trackedEntity instanceof EntityArrow -- arrows come with stage 10.
            const bool isArrow = false;

            if (updateCounter > 0 || isArrow) {
                if (j2 >= -128 && j2 < 128 && k2 >= -128 && k2 < 128 && i >= -128 && i < 128 && ticksSinceLastForcedTeleport <= 400 && !ridingEntity
                    && onGround == trackedEntity->onGround) {
                    if ((!flag || !flag1) && !isArrow) {
                        if (flag) {
                            packet1 = std::make_shared<S14PacketEntity::S15PacketEntityRelMove>(trackedEntity->getEntityId(), static_cast<int8_t>(j2),
                                static_cast<int8_t>(k2), static_cast<int8_t>(i), trackedEntity->onGround);
                        } else if (flag1) {
                            packet1 = std::make_shared<S14PacketEntity::S16PacketEntityLook>(
                                trackedEntity->getEntityId(), static_cast<int8_t>(l1), static_cast<int8_t>(i2), trackedEntity->onGround);
                        }
                    } else {
                        packet1 = std::make_shared<S14PacketEntity::S17PacketEntityLookMove>(trackedEntity->getEntityId(), static_cast<int8_t>(j2),
                            static_cast<int8_t>(k2), static_cast<int8_t>(i), static_cast<int8_t>(l1), static_cast<int8_t>(i2), trackedEntity->onGround);
                    }
                } else {
                    onGround = trackedEntity->onGround;
                    ticksSinceLastForcedTeleport = 0;
                    packet1 = std::make_shared<S18PacketEntityTeleport>(
                        trackedEntity->getEntityId(), k, j1, k1, static_cast<int8_t>(l1), static_cast<int8_t>(i2), trackedEntity->onGround);
                }
            }

            if (sendVelocityUpdates) {
                const double d0 = trackedEntity->motionX - lastTrackedEntityMotionX;
                const double d1 = trackedEntity->motionY - lastTrackedEntityMotionY;
                const double d2 = trackedEntity->motionZ - motionZ;
                const double d3 = 0.02;
                const double d4 = d0 * d0 + d1 * d1 + d2 * d2;

                if (d4 > d3 * d3 || (d4 > 0.0 && trackedEntity->motionX == 0.0 && trackedEntity->motionY == 0.0 && trackedEntity->motionZ == 0.0)) {
                    lastTrackedEntityMotionX = trackedEntity->motionX;
                    lastTrackedEntityMotionY = trackedEntity->motionY;
                    motionZ = trackedEntity->motionZ;
                    sendPacketToTrackedPlayers(
                        std::make_shared<S12PacketEntityVelocity>(trackedEntity->getEntityId(), lastTrackedEntityMotionX, lastTrackedEntityMotionY, motionZ));
                }
            }

            if (packet1)
                sendPacketToTrackedPlayers(packet1);

            sendMetadataToAllAssociatedPlayers();

            if (flag) {
                encodedPosX = k;
                encodedPosY = j1;
                encodedPosZ = k1;
            }

            if (flag1) {
                encodedRotationYaw = l1;
                encodedRotationPitch = i2;
            }

            ridingEntity = false;
        } else {
            const int j = encodeAngle(trackedEntity->rotationYaw);
            const int i1 = encodeAngle(trackedEntity->rotationPitch);
            const bool flag2 = std::abs(j - encodedRotationYaw) >= 4 || std::abs(i1 - encodedRotationPitch) >= 4;

            if (flag2) {
                sendPacketToTrackedPlayers(std::make_shared<S14PacketEntity::S16PacketEntityLook>(
                    trackedEntity->getEntityId(), static_cast<int8_t>(j), static_cast<int8_t>(i1), trackedEntity->onGround));
                encodedRotationYaw = j;
                encodedRotationPitch = i1;
            }

            encodedPosX = MathHelper::floor_double(trackedEntity->posX * 32.0);
            encodedPosY = MathHelper::floor_double(trackedEntity->posY * 32.0);
            encodedPosZ = MathHelper::floor_double(trackedEntity->posZ * 32.0);
            sendMetadataToAllAssociatedPlayers();
            ridingEntity = true;
        }

        const int l = encodeAngle(trackedEntity->getRotationYawHead());
        if (std::abs(l - lastHeadMotion) >= 4) {
            sendPacketToTrackedPlayers(std::make_shared<S19PacketEntityHeadLook>(trackedEntity, static_cast<int8_t>(l)));
            lastHeadMotion = l;
        }

        trackedEntity->isAirBorne = false;
    }

    ++updateCounter;

    if (trackedEntity->velocityChanged) {
        func_151261_b(std::make_shared<S12PacketEntityVelocity>(trackedEntity));
        trackedEntity->velocityChanged = false;
    }
}

void EntityTrackerEntry::sendMetadataToAllAssociatedPlayers()
{
    DataWatcher& datawatcher = trackedEntity->getDataWatcher();
    if (datawatcher.hasObjectChanged())
        func_151261_b(std::make_shared<S1CPacketEntityMetadata>(trackedEntity->getEntityId(), datawatcher, false));

    if (auto* living = dynamic_cast<EntityLivingBase*>(trackedEntity)) {
        auto& serversideattributemap = static_cast<ServersideAttributeMap&>(living->getAttributeMap());
        std::vector<IAttributeInstance*>& set = serversideattributemap.getAttributeInstanceSet();
        if (!set.empty())
            func_151261_b(std::make_shared<S20PacketEntityProperties>(trackedEntity->getEntityId(), set));
        set.clear();
    }
}

void EntityTrackerEntry::sendPacketToTrackedPlayers(const std::shared_ptr<Packet>& packetIn)
{
    for (EntityPlayerMP* entityplayermp : trackingPlayers)
        entityplayermp->playerNetServerHandler->sendPacket(packetIn);
}

void EntityTrackerEntry::func_151261_b(const std::shared_ptr<Packet>& packetIn)
{
    sendPacketToTrackedPlayers(packetIn);
    if (auto* player = dynamic_cast<EntityPlayerMP*>(trackedEntity))
        player->playerNetServerHandler->sendPacket(packetIn);
}

void EntityTrackerEntry::sendDestroyEntityPacketToTrackedPlayers()
{
    for (EntityPlayerMP* entityplayermp : trackingPlayers)
        entityplayermp->removeEntity(trackedEntity);
}

void EntityTrackerEntry::removeFromTrackedPlayers(EntityPlayerMP* playerMP)
{
    if (isTracking(playerMP)) {
        playerMP->removeEntity(trackedEntity);
        stopTracking(playerMP);
    }
}

void EntityTrackerEntry::updatePlayerEntity(EntityPlayerMP* playerMP)
{
    if (playerMP == trackedEntity)
        return;

    if (func_180233_c(playerMP)) {
        if (!isTracking(playerMP) && (isPlayerWatchingThisChunk(playerMP) || trackedEntity->forceSpawn)) {
            trackingPlayers.push_back(playerMP);
            const std::shared_ptr<Packet> packet = createSpawnPacket();
            NetHandlerPlayServer* handler = playerMP->playerNetServerHandler.get();
            handler->sendPacket(packet);

            if (!trackedEntity->getDataWatcher().getIsBlank())
                handler->sendPacket(std::make_shared<S1CPacketEntityMetadata>(trackedEntity->getEntityId(), trackedEntity->getDataWatcher(), true));

            // Java: getNBTTagCompound -- null for every entity but the armor stand (stage 10).

            auto* living = dynamic_cast<EntityLivingBase*>(trackedEntity);
            if (living) {
                auto& serversideattributemap = static_cast<ServersideAttributeMap&>(living->getAttributeMap());
                const std::vector<IAttributeInstance*> collection = serversideattributemap.getWatchedAttributes();
                if (!collection.empty())
                    handler->sendPacket(std::make_shared<S20PacketEntityProperties>(trackedEntity->getEntityId(), collection));
            }

            lastTrackedEntityMotionX = trackedEntity->motionX;
            lastTrackedEntityMotionY = trackedEntity->motionY;
            motionZ = trackedEntity->motionZ;

            if (sendVelocityUpdates && !dynamic_cast<S0FPacketSpawnMob*>(packet.get())) {
                handler->sendPacket(
                    std::make_shared<S12PacketEntityVelocity>(trackedEntity->getEntityId(), trackedEntity->motionX, trackedEntity->motionY, trackedEntity->motionZ));
            }

            if (trackedEntity->ridingEntity != nullptr)
                handler->sendPacket(std::make_shared<S1BPacketEntityAttach>(0, trackedEntity, trackedEntity->ridingEntity.get()));

            // Java: the leash of an EntityLiving -- stage 10.

            if (living) {
                for (int i = 0; i < 5; ++i) {
                    if (const std::shared_ptr<ItemStack> itemstack = living->getEquipmentInSlot(i))
                        handler->sendPacket(std::make_shared<S04PacketEntityEquipment>(trackedEntity->getEntityId(), i, itemstack.get()));
                }
            }

            if (auto* entityplayer = dynamic_cast<EntityPlayer*>(trackedEntity)) {
                if (entityplayer->isPlayerSleeping())
                    handler->sendPacket(std::make_shared<S0APacketUseBed>(entityplayer, BlockPos(trackedEntity->posX, trackedEntity->posY, trackedEntity->posZ)));
            }

            if (living) {
                for (const PotionEffect& potioneffect : living->getActivePotionEffects())
                    handler->sendPacket(std::make_shared<S1DPacketEntityEffect>(trackedEntity->getEntityId(), potioneffect));
            }
        }
    } else if (isTracking(playerMP)) {
        stopTracking(playerMP);
        playerMP->removeEntity(trackedEntity);
    }
}

bool EntityTrackerEntry::func_180233_c(EntityPlayerMP* playerMP)
{
    // Java: integer division of the encoded position.
    const double d0 = playerMP->posX - static_cast<double>(encodedPosX / 32);
    const double d1 = playerMP->posZ - static_cast<double>(encodedPosZ / 32);
    return d0 >= -trackingDistanceThreshold && d0 <= trackingDistanceThreshold && d1 >= -trackingDistanceThreshold && d1 <= trackingDistanceThreshold
        && trackedEntity->isSpectatedByPlayer(playerMP);
}

bool EntityTrackerEntry::isPlayerWatchingThisChunk(EntityPlayerMP* playerMP)
{
    return playerMP->getServerForPlayer()->getPlayerManager()->isPlayerWatchingChunk(playerMP, trackedEntity->chunkCoordX, trackedEntity->chunkCoordZ);
}

void EntityTrackerEntry::updatePlayerEntities(const std::vector<std::shared_ptr<EntityPlayer>>& players)
{
    for (size_t i = 0; i < players.size(); ++i)
        updatePlayerEntity(static_cast<EntityPlayerMP*>(players[i].get()));
}

std::shared_ptr<Packet> EntityTrackerEntry::createSpawnPacket()
{
    if (trackedEntity->isDead)
        Log::warn("Fetching addPacket for removed entity");

    if (auto* player = dynamic_cast<EntityPlayerMP*>(trackedEntity))
        return std::make_shared<S0CPacketSpawnPlayer>(player);

    // Java: the spawn packets of the other entity kinds -- with their classes (stage 10).
    throw std::invalid_argument("Don't know how to add " + std::string(trackedEntity->getClassName()) + "!");
}

void EntityTrackerEntry::removeTrackedPlayerSymmetric(EntityPlayerMP* playerMP)
{
    if (isTracking(playerMP)) {
        stopTracking(playerMP);
        playerMP->removeEntity(trackedEntity);
    }
}

EntityTracker::EntityTracker(WorldServer* theWorldIn)
    : theWorld(theWorldIn)
    , maxTrackingDistanceThreshold(theWorldIn->getMinecraftServer()->getConfigurationManager()->getEntityViewDistance())
{
}

EntityTracker::~EntityTracker() = default;

void EntityTracker::trackEntity(Entity* entityIn)
{
    if (auto* entityplayermp = dynamic_cast<EntityPlayerMP*>(entityIn)) {
        trackEntity(entityIn, 512, 2);
        for (const auto& entry : trackedEntities) {
            if (entry.second->trackedEntity != entityplayermp)
                entry.second->updatePlayerEntity(entityplayermp);
        }
    }
    // Java: the ranges and frequencies of the other entity kinds -- with their classes (stage 10).
}

void EntityTracker::addEntityToTracker(Entity* entityIn, int trackingRange, int updateFrequency, bool sendVelocityUpdates)
{
    if (trackingRange > maxTrackingDistanceThreshold)
        trackingRange = maxTrackingDistanceThreshold;

    if (trackedEntityHashTable.contains(entityIn->getEntityId())) {
        // Java throws and "silently" catches it.
        Log::error("\"Silently\" catching entity tracking error. Entity is already tracked!");
        return;
    }

    auto created = std::make_unique<EntityTrackerEntry>(entityIn, trackingRange, updateFrequency, sendVelocityUpdates);
    EntityTrackerEntry* entitytrackerentry = created.get();
    trackedEntities.put(entityIn->getEntityId(), entitytrackerentry);
    trackedEntityHashTable.emplace(entityIn->getEntityId(), std::move(created));
    try {
        entitytrackerentry->updatePlayerEntities(theWorld->getPlayerEntities());
    } catch (const std::exception& e) {
        Log::error("\"Silently\" catching entity tracking error. {}", e.what());
    }
}

void EntityTracker::untrackEntity(Entity* entityIn)
{
    if (auto* entityplayermp = dynamic_cast<EntityPlayerMP*>(entityIn)) {
        for (const auto& entry : trackedEntities)
            entry.second->removeFromTrackedPlayers(entityplayermp);
    }

    const auto it = trackedEntityHashTable.find(entityIn->getEntityId());
    if (it != trackedEntityHashTable.end()) {
        std::unique_ptr<EntityTrackerEntry> entitytrackerentry1 = std::move(it->second);
        trackedEntityHashTable.erase(it);
        trackedEntities.erase(entityIn->getEntityId());
        entitytrackerentry1->sendDestroyEntityPacketToTrackedPlayers();
    }
}

void EntityTracker::updateTrackedEntities()
{
    std::vector<EntityPlayerMP*> list;

    for (const auto& entry : trackedEntities) {
        EntityTrackerEntry* entitytrackerentry = entry.second;
        entitytrackerentry->updatePlayerList(theWorld->getPlayerEntities());
        if (entitytrackerentry->playerEntitiesUpdated) {
            if (auto* player = dynamic_cast<EntityPlayerMP*>(entitytrackerentry->trackedEntity))
                list.push_back(player);
        }
    }

    for (EntityPlayerMP* entityplayermp : list) {
        for (const auto& entry : trackedEntities) {
            if (entry.second->trackedEntity != entityplayermp)
                entry.second->updatePlayerEntity(entityplayermp);
        }
    }
}

void EntityTracker::func_180245_a(EntityPlayerMP* player)
{
    for (const auto& entry : trackedEntities) {
        if (entry.second->trackedEntity == player)
            entry.second->updatePlayerEntities(theWorld->getPlayerEntities());
        else
            entry.second->updatePlayerEntity(player);
    }
}

void EntityTracker::sendToAllTrackingEntity(Entity* entityIn, const std::shared_ptr<Packet>& packet)
{
    if (const auto it = trackedEntityHashTable.find(entityIn->getEntityId()); it != trackedEntityHashTable.end())
        it->second->sendPacketToTrackedPlayers(packet);
}

void EntityTracker::func_151248_b(Entity* entityIn, const std::shared_ptr<Packet>& packet)
{
    if (const auto it = trackedEntityHashTable.find(entityIn->getEntityId()); it != trackedEntityHashTable.end())
        it->second->func_151261_b(packet);
}

void EntityTracker::removePlayerFromTrackers(EntityPlayerMP* player)
{
    for (const auto& entry : trackedEntities)
        entry.second->removeTrackedPlayerSymmetric(player);
}

void EntityTracker::func_85172_a(EntityPlayerMP* player, Chunk* chunk)
{
    for (const auto& entry : trackedEntities) {
        Entity* tracked = entry.second->trackedEntity;
        if (tracked != player && tracked->chunkCoordX == chunk->xPosition && tracked->chunkCoordZ == chunk->zPosition)
            entry.second->updatePlayerEntity(player);
    }
}

} // namespace mc
