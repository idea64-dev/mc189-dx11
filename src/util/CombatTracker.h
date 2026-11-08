#pragma once

// Original: net/minecraft/util/CombatTracker.java and CombatEntry.java -- the recent damage of a living entity,
// for the death message and the combat events.
// Entries keep the static damage sources by identity (Java compares them with ==) and copies of the others.
// The entities those copies point at are only guaranteed alive while the fight lasts, as in Java's usage.

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class DamageSource;
class EntityLivingBase;
class IChatComponent;

class CombatEntry {
public:
    CombatEntry(const DamageSource& damageSrcIn, int ticks, float healthAmount, float damageAmount, std::optional<std::string> location, float fallDistanceIn);

    const DamageSource& getDamageSrc() const { return *damageSrc; }
    float func_94563_c() const { return damage; }
    bool isLivingDamageSrc() const;
    const std::optional<std::string>& func_94562_g() const { return field_94566_e; }
    std::shared_ptr<IChatComponent> getDamageSrcDisplayName() const;
    float getDamageAmount() const;

private:
    std::shared_ptr<DamageSource> keepAlive;
    const DamageSource* damageSrc;
    int field_94567_b;
    float damage;
    float health;
    std::optional<std::string> field_94566_e;
    float fallDistance;
};

class CombatTracker {
public:
    explicit CombatTracker(EntityLivingBase* fighterIn)
        : fighter(fighterIn)
    {
    }

    void func_94545_a();
    void trackDamage(const DamageSource& damageSrc, float healthIn, float damageAmount);
    std::shared_ptr<IChatComponent> getDeathMessage();
    EntityLivingBase* func_94550_c();
    int func_180134_f() const;
    void reset();
    EntityLivingBase* getFighter() const { return fighter; }

private:
    const CombatEntry* func_94544_f() const;
    static std::string func_94548_b(const CombatEntry& entry);

    std::vector<CombatEntry> combatEntries;
    EntityLivingBase* fighter;
    int field_94555_c = 0;
    int field_152775_d = 0;
    int field_152776_e = 0;
    bool field_94552_d = false;
    bool field_94553_e = false;
    std::optional<std::string> field_94551_f;
};

} // namespace mc
