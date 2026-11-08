#include "util/DamageSource.h"

#include "client/resources/Locale.h"
#include "entity/EntityLivingBase.h"
#include "entity/player/EntityPlayer.h"
#include "entity/player/PlayerCapabilities.h"
#include "item/ItemStack.h"
#include "util/ChatComponent.h"

namespace mc {

DamageSource DamageSource::inFire = std::move(DamageSource("inFire").setFireDamage());
DamageSource DamageSource::lightningBolt("lightningBolt");
DamageSource DamageSource::onFire = std::move(DamageSource("onFire").setDamageBypassesArmor().setFireDamage());
DamageSource DamageSource::lava = std::move(DamageSource("lava").setFireDamage());
DamageSource DamageSource::inWall = std::move(DamageSource("inWall").setDamageBypassesArmor());
DamageSource DamageSource::drown = std::move(DamageSource("drown").setDamageBypassesArmor());
DamageSource DamageSource::starve = std::move(DamageSource("starve").setDamageBypassesArmor().setDamageIsAbsolute());
DamageSource DamageSource::cactus("cactus");
DamageSource DamageSource::fall = std::move(DamageSource("fall").setDamageBypassesArmor());
DamageSource DamageSource::outOfWorld = std::move(DamageSource("outOfWorld").setDamageBypassesArmor().setDamageAllowedInCreativeMode());
DamageSource DamageSource::generic = std::move(DamageSource("generic").setDamageBypassesArmor());
DamageSource DamageSource::magic = std::move(DamageSource("magic").setDamageBypassesArmor().setMagicDamage());
DamageSource DamageSource::wither = std::move(DamageSource("wither").setDamageBypassesArmor());
DamageSource DamageSource::anvil("anvil");
DamageSource DamageSource::fallingBlock("fallingBlock");

namespace {

// Java: ItemStack.getChatComponent -- the display name in brackets with the item hover event; the hover comes
// with the chat (stage 13).
std::shared_ptr<IChatComponent> itemChatComponent(const ItemStack& stack)
{
    auto component = std::make_shared<ChatComponentText>(stack.getDisplayName());
    component->getChatStyle()->setItalic(true);
    auto bracketed = std::make_shared<ChatComponentText>("[");
    bracketed->appendSibling(component);
    bracketed->appendText("]");
    return bracketed;
}

} // namespace

bool DamageSource::isStaticSource() const
{
    for (const DamageSource* s : { &inFire, &lightningBolt, &onFire, &lava, &inWall, &drown, &starve, &cactus, &fall, &outOfWorld, &generic, &magic, &wither,
             &anvil, &fallingBlock }) {
        if (s == this)
            return true;
    }
    return false;
}

std::shared_ptr<DamageSource> DamageSource::causeMobDamage(EntityLivingBase* mob)
{
    return std::make_shared<EntityDamageSource>("mob", mob);
}

std::shared_ptr<DamageSource> DamageSource::causePlayerDamage(EntityPlayer* player)
{
    return std::make_shared<EntityDamageSource>("player", player);
}

std::shared_ptr<DamageSource> DamageSource::causeArrowDamage(Entity* arrow, Entity* indirectEntityIn)
{
    auto source = std::make_shared<EntityDamageSourceIndirect>("arrow", arrow, indirectEntityIn);
    source->setProjectile();
    return source;
}

std::shared_ptr<DamageSource> DamageSource::causeFireballDamage(Entity* fireball, Entity* indirectEntityIn)
{
    auto source = !indirectEntityIn ? std::make_shared<EntityDamageSourceIndirect>("onFire", fireball, fireball)
                                    : std::make_shared<EntityDamageSourceIndirect>("fireball", fireball, indirectEntityIn);
    source->setFireDamage().setProjectile();
    return source;
}

std::shared_ptr<DamageSource> DamageSource::causeThrownDamage(Entity* source, Entity* indirectEntityIn)
{
    auto result = std::make_shared<EntityDamageSourceIndirect>("thrown", source, indirectEntityIn);
    result->setProjectile();
    return result;
}

std::shared_ptr<DamageSource> DamageSource::causeIndirectMagicDamage(Entity* source, Entity* indirectEntityIn)
{
    auto result = std::make_shared<EntityDamageSourceIndirect>("indirectMagic", source, indirectEntityIn);
    result->setDamageBypassesArmor().setMagicDamage();
    return result;
}

std::shared_ptr<DamageSource> DamageSource::causeThornsDamage(Entity* source)
{
    auto result = std::make_shared<EntityDamageSource>("thorns", source);
    result->setIsThornsDamage();
    result->setMagicDamage();
    return result;
}

std::shared_ptr<DamageSource> DamageSource::setExplosionSource(EntityLivingBase* placedBy)
{
    if (placedBy) {
        auto result = std::make_shared<EntityDamageSource>("explosion.player", placedBy);
        result->setDifficultyScaled().setExplosion();
        return result;
    }
    auto result = std::make_shared<DamageSource>("explosion");
    result->setDifficultyScaled().setExplosion();
    return result;
}

std::shared_ptr<IChatComponent> DamageSource::getDeathMessage(EntityLivingBase* entityLivingBaseIn) const
{
    EntityLivingBase* entitylivingbase = entityLivingBaseIn->getAttackingEntity();
    const std::string s = "death.attack." + damageType;
    const std::string s1 = s + ".player";
    if (entitylivingbase && StatCollector::canTranslate(s1))
        return std::make_shared<ChatComponentTranslation>(s1, std::vector<ChatComponentTranslation::Argument> { entityLivingBaseIn->getDisplayName(), entitylivingbase->getDisplayName() });
    return std::make_shared<ChatComponentTranslation>(s, std::vector<ChatComponentTranslation::Argument> { entityLivingBaseIn->getDisplayName() });
}

bool DamageSource::isCreativePlayer() const
{
    const auto* player = dynamic_cast<const EntityPlayer*>(getEntity());
    return player && player->capabilities.isCreativeMode;
}

std::shared_ptr<IChatComponent> EntityDamageSource::getDeathMessage(EntityLivingBase* entityLivingBaseIn) const
{
    auto* living = dynamic_cast<EntityLivingBase*>(damageSourceEntity);
    const std::shared_ptr<ItemStack> itemstack = living ? living->getHeldItem() : nullptr;
    const std::string s = "death.attack." + damageType;
    const std::string s1 = s + ".item";
    if (itemstack && itemstack->hasDisplayName() && StatCollector::canTranslate(s1)) {
        return std::make_shared<ChatComponentTranslation>(s1, std::vector<ChatComponentTranslation::Argument> {
                                                                   entityLivingBaseIn->getDisplayName(), damageSourceEntity->getDisplayName(), itemChatComponent(*itemstack) });
    }
    return std::make_shared<ChatComponentTranslation>(s, std::vector<ChatComponentTranslation::Argument> { entityLivingBaseIn->getDisplayName(), damageSourceEntity->getDisplayName() });
}

bool EntityDamageSource::isDifficultyScaled() const
{
    return damageSourceEntity && dynamic_cast<EntityLivingBase*>(damageSourceEntity) && !dynamic_cast<EntityPlayer*>(damageSourceEntity);
}

std::shared_ptr<IChatComponent> EntityDamageSourceIndirect::getDeathMessage(EntityLivingBase* entityLivingBaseIn) const
{
    const std::shared_ptr<IChatComponent> ichatcomponent = !indirectEntity ? damageSourceEntity->getDisplayName() : indirectEntity->getDisplayName();
    auto* living = dynamic_cast<EntityLivingBase*>(indirectEntity);
    const std::shared_ptr<ItemStack> itemstack = living ? living->getHeldItem() : nullptr;
    const std::string s = "death.attack." + damageType;
    const std::string s1 = s + ".item";
    if (itemstack && itemstack->hasDisplayName() && StatCollector::canTranslate(s1)) {
        return std::make_shared<ChatComponentTranslation>(
            s1, std::vector<ChatComponentTranslation::Argument> { entityLivingBaseIn->getDisplayName(), ichatcomponent, itemChatComponent(*itemstack) });
    }
    return std::make_shared<ChatComponentTranslation>(s, std::vector<ChatComponentTranslation::Argument> { entityLivingBaseIn->getDisplayName(), ichatcomponent });
}

} // namespace mc
