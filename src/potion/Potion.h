#pragma once

// Original: net/minecraft/potion/Potion.java with PotionHealth, PotionAttackDamage, PotionHealthBoost and
// PotionAbsorption, and net/minecraft/potion/PotionEffect.java.
// The effects on living entities (performEffect, isReady, the attribute modifiers) are in PotionEntityEffects.cpp;
// affectEntity (splash potions) comes with the thrown potions (stage 10).

#include "util/ResourceLocation.h"

#include <array>
#include <memory>
#include <string>

namespace mc {

class AttributeModifier;
class BaseAttributeMap;
class EntityLivingBase;
class NBTTagCompound;

class Potion {
public:
    // The subclasses; only isInstant() differs among what is ported so far.
    enum class Kind { NORMAL, HEALTH, ATTACK_DAMAGE, HEALTH_BOOST, ABSORPTION };

    static std::array<const Potion*, 32> potionTypes;
    static const Potion *moveSpeed, *moveSlowdown, *digSpeed, *digSlowdown, *damageBoost, *heal, *harm, *jump, *confusion, *regeneration,
        *resistance, *fireResistance, *waterBreathing, *invisibility, *blindness, *nightVision, *hunger, *weakness, *poison, *wither,
        *healthBoost, *absorption, *saturation;

    static const Potion* getPotionFromResourceLocation(const std::string& location);

    const int id;

    int getId() const { return id; }
    const std::string& getName() const { return name; }
    bool hasStatusIcon() const { return statusIconIndex >= 0; }
    int getStatusIconIndex() const { return statusIconIndex; }
    bool isBadEffect() const { return badEffect; }
    double getEffectiveness() const { return effectiveness; }
    bool isUsable() const { return usable; }
    int getLiquidColor() const { return liquidColor; }
    bool isInstant() const { return kind == Kind::HEALTH; }
    void performEffect(EntityLivingBase* entityLivingBaseIn, int amplifier) const;
    bool isReady(int duration, int amplifier) const;
    // Java: getAttributeModifierMap -- the one attribute modifier the potion registers, if any: the attribute name
    // and the base modifier.
    const AttributeModifier* getAttributeModifier(std::string* attributeName = nullptr) const;
    void removeAttributesModifiersFromEntity(EntityLivingBase* entityLivingBaseIn, BaseAttributeMap& attributeMap, int amplifier) const;
    void applyAttributesModifiersToEntity(EntityLivingBase* entityLivingBaseIn, BaseAttributeMap& attributeMap, int amplifier) const;
    double getAttributeModifierAmount(int amplifier, const AttributeModifier& modifier) const;

private:
    struct Init;

    Potion(int potionID, const ResourceLocation& location, bool badEffectIn, int potionColor, Kind kindIn);

    Kind kind;
    bool badEffect;
    int liquidColor;
    std::string name;
    int statusIconIndex = -1;
    double effectiveness;
    bool usable = false;
};

class PotionEffect {
public:
    PotionEffect(int id, int effectDuration, int effectAmplifier = 0, bool ambient = false, bool showParticlesIn = true)
        : potionID(id)
        , duration(effectDuration)
        , amplifier(effectAmplifier)
        , isAmbient(ambient)
        , showParticles(showParticlesIn)
    {
    }

    // Java's copy constructor leaves the splash and max-duration flags false.
    static PotionEffect copyOf(const PotionEffect& other) { return PotionEffect(other.potionID, other.duration, other.amplifier, other.isAmbient, other.showParticles); }

    void combine(const PotionEffect& other);
    int getPotionID() const { return potionID; }
    int getDuration() const { return duration; }
    int getAmplifier() const { return amplifier; }
    void setSplashPotion(bool splashPotion) { isSplashPotion = splashPotion; }
    bool getIsAmbient() const { return isAmbient; }
    bool getIsShowParticles() const { return showParticles; }
    // Ticks the effect on the entity; false once it ran out.
    bool onUpdate(EntityLivingBase* entityIn);
    void performEffect(EntityLivingBase* entityIn);
    std::string getEffectName() const { return Potion::potionTypes[static_cast<size_t>(potionID)]->getName(); }
    int hashCode() const { return potionID; }
    std::string toString() const;
    bool operator==(const PotionEffect& other) const
    {
        return potionID == other.potionID && amplifier == other.amplifier && duration == other.duration && isSplashPotion == other.isSplashPotion && isAmbient == other.isAmbient;
    }
    void writeCustomPotionEffectToNBT(NBTTagCompound& nbt) const;
    // Java returns null for unknown ids.
    static std::unique_ptr<PotionEffect> readCustomPotionEffectFromNBT(const NBTTagCompound& nbt);
    void setPotionDurationMax(bool maxDuration) { isPotionDurationMax = maxDuration; }
    bool getIsPotionDurationMax() const { return isPotionDurationMax; }

private:
    int potionID;
    int duration;
    int amplifier;
    bool isSplashPotion = false;
    bool isAmbient;
    bool isPotionDurationMax = false;
    bool showParticles;
};

} // namespace mc
