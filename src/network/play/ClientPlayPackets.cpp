#include "network/play/ClientPlayPackets.h"

#include "core/StringUtil.h"
#include "entity/Entity.h"
#include "entity/player/PlayerCapabilities.h"
#include "item/ItemStack.h"
#include "world/World.h"

#include <stdexcept>

namespace mc {

namespace {

std::shared_ptr<ItemStack> copyStack(const ItemStack* stack)
{
    return stack ? std::make_shared<ItemStack>(stack->copy()) : nullptr;
}

} // namespace

C01PacketChatMessage::C01PacketChatMessage(std::string messageIn)
    : message(std::move(messageIn))
{
    const std::u16string utf16 = toUtf16(message);
    if (utf16.size() > 100)
        message = fromUtf16(std::u16string_view(utf16).substr(0, 100));
}

C02PacketUseEntity::C02PacketUseEntity(Entity* entity, Action actionIn)
    : entityId(entity->getEntityId())
    , action(actionIn)
{
}

C02PacketUseEntity::C02PacketUseEntity(Entity* entity, const Vec3& hitVecIn)
    : C02PacketUseEntity(entity, Action::INTERACT_AT)
{
    hitVec = hitVecIn;
}

Entity* C02PacketUseEntity::getEntityFromWorld(World* worldIn) const
{
    return worldIn->getEntityByID(entityId);
}

C08PacketPlayerBlockPlacement::C08PacketPlayerBlockPlacement(const ItemStack* stackIn)
    : C08PacketPlayerBlockPlacement(BlockPos(-1, -1, -1), 255, stackIn, 0.0f, 0.0f, 0.0f)
{
}

C08PacketPlayerBlockPlacement::C08PacketPlayerBlockPlacement(
    const BlockPos& positionIn, int placedBlockDirectionIn, const ItemStack* stackIn, float facingXIn, float facingYIn, float facingZIn)
    : position(positionIn)
    , placedBlockDirection(placedBlockDirectionIn)
    , stack(copyStack(stackIn))
    , facingX(facingXIn)
    , facingY(facingYIn)
    , facingZ(facingZIn)
{
}

C0BPacketEntityAction::C0BPacketEntityAction(Entity* entity, Action actionIn, int auxDataIn)
    : entityID(entity->getEntityId())
    , action(actionIn)
    , auxData(auxDataIn)
{
}

C0EPacketClickWindow::C0EPacketClickWindow(int windowIdIn, int slotIdIn, int usedButtonIn, int modeIn, const ItemStack* clickedItemIn, int16_t actionNumberIn)
    : windowId(windowIdIn)
    , slotId(slotIdIn)
    , usedButton(usedButtonIn)
    , actionNumber(actionNumberIn)
    , clickedItem(copyStack(clickedItemIn))
    , mode(modeIn)
{
}

C10PacketCreativeInventoryAction::C10PacketCreativeInventoryAction(int slotIdIn, const ItemStack* stackIn)
    : slotId(slotIdIn)
    , stack(copyStack(stackIn))
{
}

C13PacketPlayerAbilities::C13PacketPlayerAbilities(const PlayerCapabilities& capabilities)
{
    setInvulnerable(capabilities.disableDamage);
    setFlying(capabilities.isFlying);
    setAllowFlying(capabilities.allowFlying);
    setCreativeMode(capabilities.isCreativeMode);
    setFlySpeed(capabilities.getFlySpeed());
    setWalkSpeed(capabilities.getWalkSpeed());
}

C17PacketCustomPayload::C17PacketCustomPayload(std::string channelIn, std::vector<uint8_t> dataIn)
    : channel(std::move(channelIn))
    , data(std::move(dataIn))
{
    if (data.size() > 32767)
        throw std::invalid_argument("Payload may not be larger than 32767 bytes");
}

} // namespace mc
