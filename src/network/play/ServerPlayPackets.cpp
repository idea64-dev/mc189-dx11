#include "network/play/ServerPlayPackets.h"

#include "auth/GameProfile.h"
#include "core/Java.h"
#include "util/CombatTracker.h"
#include "entity/Entity.h"
#include "entity/EntityList.h"
#include "entity/EntityLivingBase.h"
#include "entity/player/EntityPlayer.h"
#include "entity/player/EntityPlayerMP.h"
#include "entity/player/PlayerCapabilities.h"
#include "item/ItemStack.h"
#include "potion/Potion.h"
#include "server/management/ItemInWorldManager.h"
#include "util/ChatComponent.h"
#include "util/MathHelper.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/border/WorldBorder.h"
#include "world/chunk/Chunk.h"
#include "world/chunk/storage/ExtendedBlockStorage.h"

#include <bit>
#include <stdexcept>

namespace mc {

namespace {

std::shared_ptr<ItemStack> copyStack(const ItemStack* stack)
{
    return stack ? std::make_shared<ItemStack>(stack->copy()) : nullptr;
}

// Java: (int)(min(max(motion, -3.9), 3.9) * 8000.0D)
int packVelocity(double motion)
{
    constexpr double kLimit = 3.9;
    if (motion < -kLimit)
        motion = -kLimit;
    if (motion > kLimit)
        motion = kLimit;
    return java::d2i(motion * 8000.0);
}

int packPosition(double position)
{
    return MathHelper::floor_double(position * 32.0);
}

// Java: func_179757_a -- appends the bytes and returns the new offset.
template <size_t N>
int appendBytes(const std::array<uint8_t, N>& source, std::vector<uint8_t>& target, int offset)
{
    std::copy(source.begin(), source.end(), target.begin() + offset);
    return offset + static_cast<int>(N);
}

} // namespace

int8_t packAngle(float angle)
{
    return static_cast<int8_t>(java::f2i(angle * 256.0f / 360.0f));
}

S04PacketEntityEquipment::S04PacketEntityEquipment(int entityIDIn, int slot, const ItemStack* itemStackIn)
    : entityID(entityIDIn)
    , equipmentSlot(slot)
    , itemStack(copyStack(itemStackIn))
{
}

S0APacketUseBed::S0APacketUseBed(EntityPlayer* player, const BlockPos& bedPosIn)
    : playerID(player->getEntityId())
    , bedPos(bedPosIn)
{
}

EntityPlayer* S0APacketUseBed::getPlayer(World* worldIn) const
{
    return dynamic_cast<EntityPlayer*>(worldIn->getEntityByID(playerID));
}

S0BPacketAnimation::S0BPacketAnimation(Entity* ent, int animationType)
    : entityId(ent->getEntityId())
    , type(animationType)
{
}

S0CPacketSpawnPlayer::S0CPacketSpawnPlayer(EntityPlayer* player)
    : entityId(player->getEntityId())
    , playerId(player->getGameProfile().getId().value_or(UUID()))
    , x(packPosition(player->posX))
    , y(packPosition(player->posY))
    , z(packPosition(player->posZ))
    , yaw(packAngle(player->rotationYaw))
    , pitch(packAngle(player->rotationPitch))
    , field_148958_j(player->getDataWatcher().getAllWatched())
{
    const std::shared_ptr<ItemStack> itemstack = player->inventory.getCurrentItem();
    currentItem = itemstack == nullptr ? 0 : Item::getIdFromItem(itemstack->getItem());
}

S0EPacketSpawnObject::S0EPacketSpawnObject(Entity* entityIn, int typeIn, int data)
    : entityId(entityIn->getEntityId())
    , x(packPosition(entityIn->posX))
    , y(packPosition(entityIn->posY))
    , z(packPosition(entityIn->posZ))
    , pitch(MathHelper::floor_float(entityIn->rotationPitch * 256.0f / 360.0f))
    , yaw(MathHelper::floor_float(entityIn->rotationYaw * 256.0f / 360.0f))
    , type(typeIn)
    , field_149020_k(data)
{
    if (data > 0) {
        speedX = packVelocity(entityIn->motionX);
        speedY = packVelocity(entityIn->motionY);
        speedZ = packVelocity(entityIn->motionZ);
    }
}

S0FPacketSpawnMob::S0FPacketSpawnMob(EntityLivingBase* entityIn)
    : entityId(entityIn->getEntityId())
    , type(static_cast<int8_t>(EntityList::getEntityID(entityIn)))
    , x(packPosition(entityIn->posX))
    , y(packPosition(entityIn->posY))
    , z(packPosition(entityIn->posZ))
    , velocityX(packVelocity(entityIn->motionX))
    , velocityY(packVelocity(entityIn->motionY))
    , velocityZ(packVelocity(entityIn->motionZ))
    , yaw(packAngle(entityIn->rotationYaw))
    , pitch(packAngle(entityIn->rotationPitch))
    , headPitch(packAngle(entityIn->rotationYawHead))
    , watcher(entityIn->getDataWatcher().getAllWatched())
{
}

S11PacketSpawnExperienceOrb::S11PacketSpawnExperienceOrb(Entity* xpOrb, int xpValueIn)
    : entityID(xpOrb->getEntityId())
    , posX(packPosition(xpOrb->posX))
    , posY(packPosition(xpOrb->posY))
    , posZ(packPosition(xpOrb->posZ))
    , xpValue(xpValueIn)
{
}

S12PacketEntityVelocity::S12PacketEntityVelocity(Entity* entityIn)
    : S12PacketEntityVelocity(entityIn->getEntityId(), entityIn->motionX, entityIn->motionY, entityIn->motionZ)
{
}

S12PacketEntityVelocity::S12PacketEntityVelocity(int entityIDIn, double motionXIn, double motionYIn, double motionZIn)
    : entityID(entityIDIn)
    , motionX(packVelocity(motionXIn))
    , motionY(packVelocity(motionYIn))
    , motionZ(packVelocity(motionZIn))
{
}

Entity* S14PacketEntity::getEntity(World* worldIn) const
{
    return worldIn->getEntityByID(entityId);
}

S18PacketEntityTeleport::S18PacketEntityTeleport(Entity* entityIn)
    : entityId(entityIn->getEntityId())
    , posX(packPosition(entityIn->posX))
    , posY(packPosition(entityIn->posY))
    , posZ(packPosition(entityIn->posZ))
    , yaw(packAngle(entityIn->rotationYaw))
    , pitch(packAngle(entityIn->rotationPitch))
    , onGround(entityIn->onGround)
{
}

S19PacketEntityHeadLook::S19PacketEntityHeadLook(Entity* entityIn, int8_t yawIn)
    : entityId(entityIn->getEntityId())
    , yaw(yawIn)
{
}

Entity* S19PacketEntityHeadLook::getEntity(World* worldIn) const
{
    return worldIn->getEntityByID(entityId);
}

S19PacketEntityStatus::S19PacketEntityStatus(Entity* entityIn, int8_t opCodeIn)
    : entityId(entityIn->getEntityId())
    , logicOpcode(opCodeIn)
{
}

Entity* S19PacketEntityStatus::getEntity(World* worldIn) const
{
    return worldIn->getEntityByID(entityId);
}

S1BPacketEntityAttach::S1BPacketEntityAttach(int leashIn, Entity* entityIn, Entity* vehicle)
    : leash(leashIn)
    , entityId(entityIn->getEntityId())
    , vehicleEntityId(vehicle != nullptr ? vehicle->getEntityId() : -1)
{
}

S1DPacketEntityEffect::S1DPacketEntityEffect(int entityIdIn, const PotionEffect& effect)
    : entityId(entityIdIn)
    , effectId(static_cast<int8_t>(effect.getPotionID() & 255))
    , amplifier(static_cast<int8_t>(effect.getAmplifier() & 255))
    , duration(effect.getDuration() > 32767 ? 32767 : effect.getDuration())
    , hideParticles(static_cast<int8_t>(effect.getIsShowParticles() ? 1 : 0))
{
}

S1EPacketRemoveEntityEffect::S1EPacketRemoveEntityEffect(int entityIdIn, const PotionEffect& effect)
    : entityId(entityIdIn)
    , effectId(effect.getPotionID())
{
}

S20PacketEntityProperties::S20PacketEntityProperties(int entityIdIn, const std::vector<IAttributeInstance*>& attributes)
    : entityId(entityIdIn)
{
    for (IAttributeInstance* iattributeinstance : attributes)
        field_149444_b.push_back(Snapshot { iattributeinstance->getAttribute().getAttributeUnlocalizedName(), iattributeinstance->getBaseValue(), iattributeinstance->func_111122_c() });
}

S21PacketChunkData::S21PacketChunkData(Chunk* chunkIn, bool fullChunk, int sectionMask)
    : chunkX(chunkIn->xPosition)
    , chunkZ(chunkIn->zPosition)
    , extractedData(getExtractedData(chunkIn, fullChunk, !chunkIn->getWorld()->provider->getHasNoSky(), sectionMask))
    , field_149279_g(fullChunk)
{
}

int S21PacketChunkData::func_180737_a(int sections, bool hasSky, bool fullChunk)
{
    const int i = sections * 2 * 16 * 16 * 16;
    const int j = sections * 16 * 16 * 16 / 2;
    const int k = hasSky ? sections * 16 * 16 * 16 / 2 : 0;
    const int l = fullChunk ? 256 : 0;
    return i + j + k + l;
}

S21PacketChunkData::Extracted S21PacketChunkData::getExtractedData(Chunk* chunk, bool fullChunk, bool hasSky, int sectionMask)
{
    auto& storageArray = chunk->getBlockStorageArray();
    Extracted extracted;
    std::vector<ExtendedBlockStorage*> list;

    for (size_t i = 0; i < storageArray.size(); ++i) {
        ExtendedBlockStorage* storage = storageArray[i].get();
        if (storage != nullptr && (!fullChunk || !storage->isEmpty()) && (sectionMask & (1 << i)) != 0) {
            extracted.dataSize |= 1 << i;
            list.push_back(storage);
        }
    }

    extracted.data.resize(static_cast<size_t>(func_180737_a(std::popcount(static_cast<uint32_t>(extracted.dataSize)), hasSky, fullChunk)));
    int j = 0;

    for (ExtendedBlockStorage* storage : list) {
        for (uint16_t c0 : storage->getData()) {
            extracted.data[static_cast<size_t>(j++)] = static_cast<uint8_t>(c0 & 255);
            extracted.data[static_cast<size_t>(j++)] = static_cast<uint8_t>(c0 >> 8 & 255);
        }
    }

    for (ExtendedBlockStorage* storage : list)
        j = appendBytes(storage->getBlocklightArray()->getData(), extracted.data, j);

    if (hasSky) {
        for (ExtendedBlockStorage* storage : list)
            j = appendBytes(storage->getSkylightArray()->getData(), extracted.data, j);
    }

    if (fullChunk)
        appendBytes(chunk->getBiomeArray(), extracted.data, j);

    return extracted;
}

S22PacketMultiBlockChange::S22PacketMultiBlockChange(int count, const int16_t* crammedPositionsIn, Chunk* chunkIn)
    : chunkPosCoord(chunkIn->xPosition, chunkIn->zPosition)
{
    changedBlocks.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const int16_t crammed = crammedPositionsIn[i];
        // Java: BlockUpdateData.getPos -- chunk-relative x/z in the high byte, y in the low one.
        const BlockPos pos = chunkPosCoord.getBlock(crammed >> 12 & 15, crammed & 255, crammed >> 8 & 15);
        changedBlocks.push_back(BlockUpdateData { crammed, chunkIn->getBlockState(pos), pos });
    }
}

S23PacketBlockChange::S23PacketBlockChange(World* worldIn, const BlockPos& blockPositionIn)
    : blockState(worldIn->getBlockState(blockPositionIn))
    , blockPosition(blockPositionIn)
{
}

S26PacketMapChunkBulk::S26PacketMapChunkBulk(const std::vector<Chunk*>& chunks)
    : isOverworld(!chunks.front()->getWorld()->provider->getHasNoSky())
{
    for (Chunk* chunk : chunks) {
        chunksData.push_back(S21PacketChunkData::getExtractedData(chunk, true, isOverworld, 65535));
        xPositions.push_back(chunk->xPosition);
        zPositions.push_back(chunk->zPosition);
    }
}

S29PacketSoundEffect::S29PacketSoundEffect(std::string soundNameIn, double soundX, double soundY, double soundZ, float volume, float pitch)
    : soundName(std::move(soundNameIn))
    , posX(java::d2i(soundX * 8.0))
    , posY(java::d2i(soundY * 8.0))
    , posZ(java::d2i(soundZ * 8.0))
    , soundVolume(volume)
    , soundPitch(java::f2i(pitch * 63.0f))
{
}

S2CPacketSpawnGlobalEntity::S2CPacketSpawnGlobalEntity(Entity* entityIn, int typeIn)
    : entityId(entityIn->getEntityId())
    , x(packPosition(entityIn->posX))
    , y(packPosition(entityIn->posY))
    , z(packPosition(entityIn->posZ))
    , type(typeIn)
{
}

S2FPacketSetSlot::S2FPacketSetSlot(int windowIdIn, int slotIn, const ItemStack* itemIn)
    : windowId(windowIdIn)
    , slot(slotIn)
    , item(copyStack(itemIn))
{
}

S30PacketWindowItems::S30PacketWindowItems(int windowIdIn, const std::vector<std::shared_ptr<ItemStack>>& stacks)
    : windowId(windowIdIn)
{
    itemStacks.reserve(stacks.size());
    for (const std::shared_ptr<ItemStack>& itemstack : stacks)
        itemStacks.push_back(copyStack(itemstack.get()));
}

S38PacketPlayerListItem::S38PacketPlayerListItem(Action actionIn, const std::vector<EntityPlayerMP*>& playersIn)
    : action(actionIn)
{
    for (EntityPlayerMP* entityplayermp : playersIn) {
        players.push_back(AddPlayerData { std::make_shared<GameProfile>(entityplayermp->getGameProfile()), entityplayermp->ping,
            entityplayermp->theItemInWorldManager->getGameType(), entityplayermp->getTabListDisplayName() });
    }
}

S39PacketPlayerAbilities::S39PacketPlayerAbilities(const PlayerCapabilities& capabilities)
{
    setInvulnerable(capabilities.disableDamage);
    setFlying(capabilities.isFlying);
    setAllowFlying(capabilities.allowFlying);
    setCreativeMode(capabilities.isCreativeMode);
    setFlySpeed(capabilities.getFlySpeed());
    setWalkSpeed(capabilities.getWalkSpeed());
}

S3FPacketCustomPayload::S3FPacketCustomPayload(std::string channelName, std::vector<uint8_t> dataIn)
    : channel(std::move(channelName))
    , data(std::move(dataIn))
{
    if (data.size() > 1048576)
        throw std::invalid_argument("Payload may not be larger than 1048576 bytes");
}

S42PacketCombatEvent::S42PacketCombatEvent(CombatTracker& combatTrackerIn, Event combatEventType)
    : eventType(combatEventType)
{
    EntityLivingBase* entitylivingbase = combatTrackerIn.func_94550_c();

    switch (combatEventType) {
    case Event::END_COMBAT:
        field_179772_d = combatTrackerIn.func_180134_f();
        field_179775_c = entitylivingbase == nullptr ? -1 : entitylivingbase->getEntityId();
        break;
    case Event::ENTITY_DIED:
        field_179774_b = combatTrackerIn.getFighter()->getEntityId();
        field_179775_c = entitylivingbase == nullptr ? -1 : entitylivingbase->getEntityId();
        deathMessage = combatTrackerIn.getDeathMessage()->getUnformattedText();
        break;
    default:
        break;
    }
}

S43PacketCamera::S43PacketCamera(Entity* entityIn)
    : entityId(entityIn->getEntityId())
{
}

Entity* S43PacketCamera::getEntity(World* worldIn) const
{
    return worldIn->getEntityByID(entityId);
}

S44PacketWorldBorder::S44PacketWorldBorder(const WorldBorder& border, Action actionIn)
    : action(actionIn)
    , size(border.getSize())
    , centerX(border.getCenterX())
    , centerZ(border.getCenterZ())
    , targetSize(border.getTargetSize())
    , diameter(const_cast<WorldBorder&>(border).getDiameter())
    , timeUntilTarget(border.getTimeUntilTarget())
    , warningTime(border.getWarningTime())
    , warningDistance(border.getWarningDistance())
{
}

void S44PacketWorldBorder::func_179788_a(WorldBorder& border) const
{
    switch (action) {
    case Action::SET_SIZE:
        border.setTransition(targetSize);
        break;
    case Action::LERP_SIZE:
        border.setTransition(diameter, targetSize, timeUntilTarget);
        break;
    case Action::SET_CENTER:
        border.setCenter(centerX, centerZ);
        break;
    case Action::SET_WARNING_BLOCKS:
        border.setWarningDistance(warningDistance);
        break;
    case Action::SET_WARNING_TIME:
        border.setWarningTime(warningTime);
        break;
    case Action::INITIALIZE:
        border.setCenter(centerX, centerZ);
        if (timeUntilTarget > 0)
            border.setTransition(diameter, targetSize, timeUntilTarget);
        else
            border.setTransition(targetSize);
        border.setSize(size);
        border.setWarningDistance(warningDistance);
        border.setWarningTime(warningTime);
        break;
    }
}

S48PacketResourcePackSend::S48PacketResourcePackSend(std::string urlIn, std::string hashIn)
    : url(std::move(urlIn))
    , hash(std::move(hashIn))
{
    if (hash.size() > 40)
        throw std::invalid_argument("Hash is too long (max 40, was " + std::to_string(hash.size()) + ")");
}

Entity* S49PacketUpdateEntityNBT::getEntity(World* worldIn) const
{
    return worldIn->getEntityByID(entityId);
}

} // namespace mc
