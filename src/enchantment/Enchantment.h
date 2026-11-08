#pragma once

// Original: net/minecraft/enchantment/Enchantment.java, EnumEnchantmentType.java, EnchantmentData.java and the
// Enchantment subclasses (EnchantmentProtection ... EnchantmentFishingSpeed).
// Ported: ids, names, weights, enchantability ranges and the compatibility rules -- everything enchanting and
// loot generation needs. The combat hooks (calcModifierDamage with DamageSource, calcDamageByCreature with
// EnumCreatureAttribute, onEntityDamaged, onUserHurt) come with entities (stage 10).

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

class Item;
class ItemStack;
class Random;

enum class EnumEnchantmentType { ALL, ARMOR, ARMOR_FEET, ARMOR_LEGS, ARMOR_TORSO, ARMOR_HEAD, WEAPON, DIGGER, FISHING_ROD, BREAKABLE, BOW };

// Java: EnumEnchantmentType.canEnchantItem
bool canEnchantItem(EnumEnchantmentType type, const Item* item);

class Enchantment {
public:
    virtual ~Enchantment() = default;
    Enchantment(const Enchantment&) = delete;
    Enchantment& operator=(const Enchantment&) = delete;

    // Java: the static initializer; idempotent. Call after the items are registered (from Bootstrap).
    static void registerEnchantments();

    // Null for ids without an enchantment.
    static Enchantment* getEnchantmentById(int enchID);
    // Java: getEnchantmentByLocation -- by registry name ("sharpness" or "minecraft:sharpness"); null if unknown.
    static Enchantment* getEnchantmentByLocation(const std::string& location);
    // Java: enchantmentsBookList -- every enchantment, by id.
    static const std::vector<Enchantment*>& getEnchantmentsBookList() { return enchantmentsBookList; }

    int getWeight() const { return weight; }
    virtual int getMinLevel() const { return 1; }
    virtual int getMaxLevel() const { return 1; }
    virtual int getMinEnchantability(int enchantmentLevel) const { return 1 + enchantmentLevel * 10; }
    virtual int getMaxEnchantability(int enchantmentLevel) const { return getMinEnchantability(enchantmentLevel) + 5; }
    virtual bool canApplyTogether(const Enchantment* ench) const { return this != ench; }
    Enchantment* setName(std::string enchName)
    {
        name = std::move(enchName);
        return this;
    }
    virtual std::string getName() const { return "enchantment." + name; }
    std::string getTranslatedName(int level) const;
    virtual bool canApply(const ItemStack& stack) const;

    const int effectId;
    EnumEnchantmentType type;

    static inline Enchantment* protection = nullptr;
    static inline Enchantment* fireProtection = nullptr;
    static inline Enchantment* featherFalling = nullptr;
    static inline Enchantment* blastProtection = nullptr;
    static inline Enchantment* projectileProtection = nullptr;
    static inline Enchantment* respiration = nullptr;
    static inline Enchantment* aquaAffinity = nullptr;
    static inline Enchantment* thorns = nullptr;
    static inline Enchantment* depthStrider = nullptr;
    static inline Enchantment* sharpness = nullptr;
    static inline Enchantment* smite = nullptr;
    static inline Enchantment* baneOfArthropods = nullptr;
    static inline Enchantment* knockback = nullptr;
    static inline Enchantment* fireAspect = nullptr;
    static inline Enchantment* looting = nullptr;
    static inline Enchantment* efficiency = nullptr;
    static inline Enchantment* silkTouch = nullptr;
    static inline Enchantment* unbreaking = nullptr;
    static inline Enchantment* fortune = nullptr;
    static inline Enchantment* power = nullptr;
    static inline Enchantment* punch = nullptr;
    static inline Enchantment* flame = nullptr;
    static inline Enchantment* infinity = nullptr;
    static inline Enchantment* luckOfTheSea = nullptr;
    static inline Enchantment* lure = nullptr;

protected:
    // Registers itself under enchID and "minecraft:" + location; throws on a duplicate id.
    Enchantment(int enchID, const std::string& location, int enchWeight, EnumEnchantmentType enchType);

    std::string name;

private:
    const int weight;

    static std::array<Enchantment*, 256> enchantmentsList;
    static std::vector<Enchantment*> enchantmentsBookList;
    static std::unordered_map<std::string, Enchantment*> locationEnchantments;
};

// Java: EnchantmentData (a WeightedRandom.Item).
struct EnchantmentData {
    EnchantmentData(Enchantment* enchantmentObj, int enchLevel);

    int itemWeight;
    Enchantment* enchantmentobj;
    int enchantmentLevel;
};

class EnchantmentProtection : public Enchantment {
public:
    EnchantmentProtection(int enchID, const std::string& location, int enchWeight, int protectionTypeIn);
    int getMinEnchantability(int level) const override;
    int getMaxEnchantability(int level) const override;
    int getMaxLevel() const override { return 4; }
    std::string getName() const override;
    bool canApplyTogether(const Enchantment* ench) const override;
    // Java: calcModifierDamage, getFireTimeForEntity, func_92092_a -- entities (stage 10).

    const int protectionType;
};

class EnchantmentDamage : public Enchantment {
public:
    EnchantmentDamage(int enchID, const std::string& location, int enchWeight, int classification);
    int getMinEnchantability(int level) const override;
    int getMaxEnchantability(int level) const override;
    int getMaxLevel() const override { return 5; }
    std::string getName() const override;
    bool canApplyTogether(const Enchantment* ench) const override;
    bool canApply(const ItemStack& stack) const override;
    // Java: calcDamageByCreature, onEntityDamaged -- entities (stage 10).

    const int damageType;
};

} // namespace mc
