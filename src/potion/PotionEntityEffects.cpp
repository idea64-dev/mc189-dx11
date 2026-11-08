// Original: net/minecraft/potion/Potion.java (performEffect, isReady, the attribute modifiers), PotionAttackDamage.java,
// PotionHealthBoost.java, PotionAbsorption.java and PotionEffect.onUpdate / performEffect.

#include "potion/Potion.h"

#include "entity/EntityLivingBase.h"
#include "entity/SharedMonsterAttributes.h"
#include "entity/player/EntityPlayer.h"
#include "util/DamageSource.h"
#include "util/FoodStats.h"
#include "world/World.h"

#include <algorithm>

namespace mc {

void Potion::performEffect(EntityLivingBase* entityLivingBaseIn, int amplifier) const
{
    if (id == regeneration->id) {
        if (entityLivingBaseIn->getHealth() < entityLivingBaseIn->getMaxHealth())
            entityLivingBaseIn->heal(1.0f);
    } else if (id == poison->id) {
        if (entityLivingBaseIn->getHealth() > 1.0f)
            entityLivingBaseIn->attackEntityFrom(DamageSource::magic, 1.0f);
    } else if (id == wither->id) {
        entityLivingBaseIn->attackEntityFrom(DamageSource::wither, 1.0f);
    } else if (id == hunger->id && dynamic_cast<EntityPlayer*>(entityLivingBaseIn)) {
        static_cast<EntityPlayer*>(entityLivingBaseIn)->addExhaustion(0.025f * static_cast<float>(amplifier + 1));
    } else if (id == saturation->id && dynamic_cast<EntityPlayer*>(entityLivingBaseIn)) {
        if (!entityLivingBaseIn->worldObj->isRemote)
            static_cast<EntityPlayer*>(entityLivingBaseIn)->getFoodStats().addStats(amplifier + 1, 1.0f);
    } else if ((id != heal->id || entityLivingBaseIn->isEntityUndead()) && (id != harm->id || !entityLivingBaseIn->isEntityUndead())) {
        if ((id == harm->id && !entityLivingBaseIn->isEntityUndead()) || (id == heal->id && entityLivingBaseIn->isEntityUndead()))
            entityLivingBaseIn->attackEntityFrom(DamageSource::magic, static_cast<float>(6 << amplifier));
    } else {
        entityLivingBaseIn->heal(static_cast<float>(std::max(4 << amplifier, 0)));
    }
}

bool Potion::isReady(int duration, int amplifier) const
{
    if (id == regeneration->id) {
        const int k = 50 >> amplifier;
        return k > 0 ? duration % k == 0 : true;
    }
    if (id == poison->id) {
        const int j = 25 >> amplifier;
        return j > 0 ? duration % j == 0 : true;
    }
    if (id == wither->id) {
        const int i = 40 >> amplifier;
        return i > 0 ? duration % i == 0 : true;
    }
    return id == hunger->id;
}

const AttributeModifier* Potion::getAttributeModifier(std::string* attributeName) const
{
    // The registerPotionAttributeModifier calls of the static initializer.
    struct Registration {
        int potionId;
        const RangedAttribute& (*attribute)();
        AttributeModifier modifier;
    };
    static const std::vector<Registration> registrations = [] {
        std::vector<Registration> list;
        const auto add = [&list](const Potion* potion, const RangedAttribute& (*attribute)(), const char* uuid, double amount, int operation) {
            list.push_back({ potion->id, attribute, AttributeModifier(*UUID::fromString(uuid), potion->getName(), amount, operation) });
        };
        add(moveSpeed, &SharedMonsterAttributes::movementSpeed, "91AEAA56-376B-4498-935B-2F7F68070635", 0.20000000298023224, 2);
        add(moveSlowdown, &SharedMonsterAttributes::movementSpeed, "7107DE5E-7CE8-4030-940E-514C1F160890", -0.15000000596046448, 2);
        add(damageBoost, &SharedMonsterAttributes::attackDamage, "648D7064-6A60-4F59-8ABE-C2C23A6DD7A9", 2.5, 2);
        add(weakness, &SharedMonsterAttributes::attackDamage, "22653B89-116E-49DC-9B6B-9971489B5BE5", 2.0, 0);
        add(healthBoost, &SharedMonsterAttributes::maxHealth, "5D6F0BA2-1186-46AC-B896-C61C5CEE99CC", 4.0, 0);
        return list;
    }();
    for (const Registration& r : registrations) {
        if (r.potionId == id) {
            if (attributeName)
                *attributeName = r.attribute().getAttributeUnlocalizedName();
            return &r.modifier;
        }
    }
    return nullptr;
}

void Potion::removeAttributesModifiersFromEntity(EntityLivingBase* entityLivingBaseIn, BaseAttributeMap& attributeMap, int amplifier) const
{
    if (kind == Kind::ABSORPTION)
        entityLivingBaseIn->setAbsorptionAmount(entityLivingBaseIn->getAbsorptionAmount() - static_cast<float>(4 * (amplifier + 1)));
    std::string attributeName;
    if (const AttributeModifier* modifier = getAttributeModifier(&attributeName)) {
        if (IAttributeInstance* instance = attributeMap.getAttributeInstanceByName(attributeName))
            instance->removeModifier(*modifier);
    }
    if (kind == Kind::HEALTH_BOOST && entityLivingBaseIn->getHealth() > entityLivingBaseIn->getMaxHealth())
        entityLivingBaseIn->setHealth(entityLivingBaseIn->getMaxHealth());
}

void Potion::applyAttributesModifiersToEntity(EntityLivingBase* entityLivingBaseIn, BaseAttributeMap& attributeMap, int amplifier) const
{
    if (kind == Kind::ABSORPTION)
        entityLivingBaseIn->setAbsorptionAmount(entityLivingBaseIn->getAbsorptionAmount() + static_cast<float>(4 * (amplifier + 1)));
    std::string attributeName;
    if (const AttributeModifier* modifier = getAttributeModifier(&attributeName)) {
        if (IAttributeInstance* instance = attributeMap.getAttributeInstanceByName(attributeName)) {
            instance->removeModifier(*modifier);
            instance->applyModifier(AttributeModifier(modifier->getID(), getName() + " " + std::to_string(amplifier),
                getAttributeModifierAmount(amplifier, *modifier), modifier->getOperation()));
        }
    }
}

double Potion::getAttributeModifierAmount(int amplifier, const AttributeModifier& modifier) const
{
    if (kind == Kind::ATTACK_DAMAGE)
        return id == weakness->id ? static_cast<double>(-0.5f * static_cast<float>(amplifier + 1)) : 1.3 * static_cast<double>(amplifier + 1);
    return modifier.getAmount() * static_cast<double>(amplifier + 1);
}

bool PotionEffect::onUpdate(EntityLivingBase* entityIn)
{
    if (duration > 0) {
        if (Potion::potionTypes[static_cast<size_t>(potionID)]->isReady(duration, amplifier))
            performEffect(entityIn);
        --duration;
    }
    return duration > 0;
}

void PotionEffect::performEffect(EntityLivingBase* entityIn)
{
    if (duration > 0)
        Potion::potionTypes[static_cast<size_t>(potionID)]->performEffect(entityIn, amplifier);
}

} // namespace mc
