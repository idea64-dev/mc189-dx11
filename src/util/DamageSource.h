#pragma once

// Original: net/minecraft/util/DamageSource.java, EntityDamageSource.java and EntityDamageSourceIndirect.java
// The fixed sources are statics; the cause* factories make new ones (shared, as Java references).

#include <memory>
#include <string>

namespace mc {

class Entity;
class EntityLivingBase;
class EntityPlayer;
class Explosion;
class IChatComponent;

class DamageSource {
public:
    static DamageSource inFire, lightningBolt, onFire, lava, inWall, drown, starve, cactus, fall, outOfWorld, generic, magic, wither, anvil, fallingBlock;

    static std::shared_ptr<DamageSource> causeMobDamage(EntityLivingBase* mob);
    static std::shared_ptr<DamageSource> causePlayerDamage(EntityPlayer* player);
    static std::shared_ptr<DamageSource> causeArrowDamage(Entity* arrow, Entity* indirectEntityIn);
    static std::shared_ptr<DamageSource> causeFireballDamage(Entity* fireball, Entity* indirectEntityIn);
    static std::shared_ptr<DamageSource> causeThrownDamage(Entity* source, Entity* indirectEntityIn);
    static std::shared_ptr<DamageSource> causeIndirectMagicDamage(Entity* source, Entity* indirectEntityIn);
    static std::shared_ptr<DamageSource> causeThornsDamage(Entity* source);
    // Java: setExplosionSource -- with the placer when known.
    static std::shared_ptr<DamageSource> setExplosionSource(EntityLivingBase* placedBy);

    explicit DamageSource(std::string damageTypeIn)
        : damageType(std::move(damageTypeIn))
    {
    }
    virtual ~DamageSource() = default;

    bool isProjectile() const { return projectile; }
    DamageSource& setProjectile()
    {
        projectile = true;
        return *this;
    }
    bool isExplosion() const { return explosion; }
    DamageSource& setExplosion()
    {
        explosion = true;
        return *this;
    }
    bool isUnblockable() const { return unblockable; }
    float getHungerDamage() const { return hungerDamage; }
    bool canHarmInCreative() const { return damageAllowedInCreativeMode; }
    bool isDamageAbsolute() const { return damageIsAbsolute; }
    virtual Entity* getSourceOfDamage() const { return getEntity(); }
    virtual Entity* getEntity() const { return nullptr; }
    virtual std::shared_ptr<IChatComponent> getDeathMessage(EntityLivingBase* entityLivingBaseIn) const;
    bool isFireDamage() const { return fireDamage; }
    const std::string& getDamageType() const { return damageType; }
    DamageSource& setDifficultyScaled()
    {
        difficultyScaled = true;
        return *this;
    }
    virtual bool isDifficultyScaled() const { return difficultyScaled; }
    bool isMagicDamage() const { return magicDamage; }
    DamageSource& setMagicDamage()
    {
        magicDamage = true;
        return *this;
    }
    bool isCreativePlayer() const;
    // A copy for holders that outlive the call (CombatTracker).
    virtual std::shared_ptr<DamageSource> clone() const { return std::make_shared<DamageSource>(*this); }
    // Whether this is one of the static sources, which callers compare by identity.
    bool isStaticSource() const;

    DamageSource& setDamageBypassesArmor()
    {
        unblockable = true;
        hungerDamage = 0.0f;
        return *this;
    }
    DamageSource& setDamageAllowedInCreativeMode()
    {
        damageAllowedInCreativeMode = true;
        return *this;
    }
    DamageSource& setDamageIsAbsolute()
    {
        damageIsAbsolute = true;
        hungerDamage = 0.0f;
        return *this;
    }
    DamageSource& setFireDamage()
    {
        fireDamage = true;
        return *this;
    }

    std::string damageType;

private:
    bool unblockable = false;
    bool damageAllowedInCreativeMode = false;
    bool damageIsAbsolute = false;
    float hungerDamage = 0.3f;
    bool fireDamage = false;
    bool projectile = false;
    bool difficultyScaled = false;
    bool magicDamage = false;
    bool explosion = false;
};

class EntityDamageSource : public DamageSource {
public:
    EntityDamageSource(std::string damageTypeIn, Entity* source)
        : DamageSource(std::move(damageTypeIn))
        , damageSourceEntity(source)
    {
    }

    EntityDamageSource& setIsThornsDamage()
    {
        isThornsDamage = true;
        return *this;
    }
    bool getIsThornsDamage() const { return isThornsDamage; }
    Entity* getEntity() const override { return damageSourceEntity; }
    std::shared_ptr<IChatComponent> getDeathMessage(EntityLivingBase* entityLivingBaseIn) const override;
    bool isDifficultyScaled() const override;
    std::shared_ptr<DamageSource> clone() const override { return std::make_shared<EntityDamageSource>(*this); }

protected:
    Entity* damageSourceEntity;

private:
    bool isThornsDamage = false;
};

class EntityDamageSourceIndirect : public EntityDamageSource {
public:
    EntityDamageSourceIndirect(std::string damageTypeIn, Entity* source, Entity* indirect)
        : EntityDamageSource(std::move(damageTypeIn), source)
        , indirectEntity(indirect)
    {
    }

    Entity* getSourceOfDamage() const override { return damageSourceEntity; }
    Entity* getEntity() const override { return indirectEntity; }
    std::shared_ptr<IChatComponent> getDeathMessage(EntityLivingBase* entityLivingBaseIn) const override;
    std::shared_ptr<DamageSource> clone() const override { return std::make_shared<EntityDamageSourceIndirect>(*this); }

private:
    Entity* indirectEntity;
};

} // namespace mc
