#include "enchantment/Enchantment.h"

#include "client/resources/Locale.h"
#include "init/Items.h"
#include "item/ItemArmor.h"
#include "item/ItemMisc.h"
#include "item/ItemStack.h"
#include "item/ItemTool.h"

#include <memory>
#include <stdexcept>

namespace mc {

std::array<Enchantment*, 256> Enchantment::enchantmentsList {};
std::vector<Enchantment*> Enchantment::enchantmentsBookList;
std::unordered_map<std::string, Enchantment*> Enchantment::locationEnchantments;

bool canEnchantItem(EnumEnchantmentType type, const Item* item)
{
    using T = EnumEnchantmentType;
    if (type == T::ALL)
        return true;
    if (type == T::BREAKABLE && item->isDamageable())
        return true;
    if (const auto* itemarmor = dynamic_cast<const ItemArmor*>(item)) {
        if (type == T::ARMOR)
            return true;
        return itemarmor->armorType == 0 ? type == T::ARMOR_HEAD
            : (itemarmor->armorType == 2 ? type == T::ARMOR_LEGS
            : (itemarmor->armorType == 1 ? type == T::ARMOR_TORSO : (itemarmor->armorType == 3 ? type == T::ARMOR_FEET : false)));
    }
    if (dynamic_cast<const ItemSword*>(item))
        return type == T::WEAPON;
    if (dynamic_cast<const ItemTool*>(item))
        return type == T::DIGGER;
    if (dynamic_cast<const ItemBow*>(item))
        return type == T::BOW;
    if (dynamic_cast<const ItemFishingRod*>(item))
        return type == T::FISHING_ROD;
    return false;
}

Enchantment::Enchantment(int enchID, const std::string& location, int enchWeight, EnumEnchantmentType enchType)
    : effectId(enchID)
    , type(enchType)
    , weight(enchWeight)
{
    if (enchantmentsList.at(static_cast<size_t>(enchID)))
        throw std::invalid_argument("Duplicate enchantment id!");
    enchantmentsList[static_cast<size_t>(enchID)] = this;
    locationEnchantments["minecraft:" + location] = this;
}

Enchantment* Enchantment::getEnchantmentById(int enchID)
{
    return enchID >= 0 && enchID < static_cast<int>(enchantmentsList.size()) ? enchantmentsList[static_cast<size_t>(enchID)] : nullptr;
}

Enchantment* Enchantment::getEnchantmentByLocation(const std::string& location)
{
    // ResourceLocation: no namespace means minecraft; the path is lower-cased like Java's.
    std::string key = location;
    for (char& c : key) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
    if (key.find(':') == std::string::npos)
        key = "minecraft:" + key;
    const auto it = locationEnchantments.find(key);
    return it == locationEnchantments.end() ? nullptr : it->second;
}

std::string Enchantment::getTranslatedName(int level) const
{
    const std::string s = StatCollector::translateToLocal(getName());
    return s + " " + StatCollector::translateToLocal("enchantment.level." + std::to_string(level));
}

bool Enchantment::canApply(const ItemStack& stack) const
{
    return canEnchantItem(type, stack.getItem());
}

EnchantmentData::EnchantmentData(Enchantment* enchantmentObj, int enchLevel)
    : itemWeight(enchantmentObj->getWeight())
    , enchantmentobj(enchantmentObj)
    , enchantmentLevel(enchLevel)
{
}

// --- Protection and Damage (the classes with state) ---

namespace {

constexpr const char* kProtectionName[5] { "all", "fire", "fall", "explosion", "projectile" };
constexpr int kProtectionBase[5] { 1, 10, 5, 5, 3 };
constexpr int kProtectionLevel[5] { 11, 8, 6, 8, 6 };
constexpr int kProtectionThreshold[5] { 20, 12, 10, 12, 15 };

constexpr const char* kDamageName[3] { "all", "undead", "arthropods" };
constexpr int kDamageBase[3] { 1, 5, 5 };
constexpr int kDamageLevel[3] { 11, 8, 8 };
constexpr int kDamageThreshold[3] { 20, 20, 20 };

// The base class's getMinEnchantability, which several subclasses call as super.getMinEnchantability.
constexpr int baseMinEnchantability(int level)
{
    return 1 + level * 10;
}

} // namespace

EnchantmentProtection::EnchantmentProtection(int enchID, const std::string& location, int enchWeight, int protectionTypeIn)
    : Enchantment(enchID, location, enchWeight, EnumEnchantmentType::ARMOR)
    , protectionType(protectionTypeIn)
{
    if (protectionTypeIn == 2)
        type = EnumEnchantmentType::ARMOR_FEET;
}

int EnchantmentProtection::getMinEnchantability(int level) const
{
    return kProtectionBase[protectionType] + (level - 1) * kProtectionLevel[protectionType];
}

int EnchantmentProtection::getMaxEnchantability(int level) const
{
    return getMinEnchantability(level) + kProtectionThreshold[protectionType];
}

std::string EnchantmentProtection::getName() const
{
    return std::string("enchantment.protect.") + kProtectionName[protectionType];
}

bool EnchantmentProtection::canApplyTogether(const Enchantment* ench) const
{
    if (const auto* other = dynamic_cast<const EnchantmentProtection*>(ench))
        return other->protectionType == protectionType ? false : protectionType == 2 || other->protectionType == 2;
    return Enchantment::canApplyTogether(ench);
}

EnchantmentDamage::EnchantmentDamage(int enchID, const std::string& location, int enchWeight, int classification)
    : Enchantment(enchID, location, enchWeight, EnumEnchantmentType::WEAPON)
    , damageType(classification)
{
}

int EnchantmentDamage::getMinEnchantability(int level) const
{
    return kDamageBase[damageType] + (level - 1) * kDamageLevel[damageType];
}

int EnchantmentDamage::getMaxEnchantability(int level) const
{
    return getMinEnchantability(level) + kDamageThreshold[damageType];
}

std::string EnchantmentDamage::getName() const
{
    return std::string("enchantment.damage.") + kDamageName[damageType];
}

bool EnchantmentDamage::canApplyTogether(const Enchantment* ench) const
{
    return !dynamic_cast<const EnchantmentDamage*>(ench);
}

bool EnchantmentDamage::canApply(const ItemStack& stack) const
{
    return dynamic_cast<const ItemAxe*>(stack.getItem()) ? true : Enchantment::canApply(stack);
}

// --- the stateless ones ---

namespace {

// A plain enchantment class: name, max level and the enchantability range as functions of the level.
class SimpleEnchantment : public Enchantment {
public:
    using Range = int (*)(int);

    SimpleEnchantment(int enchID, const std::string& location, int enchWeight, EnumEnchantmentType enchType, const char* nameIn, int maxLevel,
        Range minRange, Range maxRange)
        : Enchantment(enchID, location, enchWeight, enchType)
        , maxLevelValue(maxLevel)
        , minRangeFn(minRange)
        , maxRangeFn(maxRange)
    {
        setName(nameIn);
    }

    int getMaxLevel() const override { return maxLevelValue; }
    int getMinEnchantability(int level) const override { return minRangeFn(level); }
    int getMaxEnchantability(int level) const override { return maxRangeFn(level); }

private:
    int maxLevelValue;
    Range minRangeFn;
    Range maxRangeFn;
};

// EnchantmentDigging and EnchantmentUntouching also apply to shears.
class ShearsEnchantment : public SimpleEnchantment {
public:
    using SimpleEnchantment::SimpleEnchantment;
    bool canApply(const ItemStack& stack) const override { return stack.getItem() == Items::shears ? true : Enchantment::canApply(stack); }
};

class EnchantmentUntouching : public ShearsEnchantment {
public:
    using ShearsEnchantment::ShearsEnchantment;
    bool canApplyTogether(const Enchantment* ench) const override { return Enchantment::canApplyTogether(ench) && ench->effectId != Enchantment::fortune->effectId; }
};

class EnchantmentLootBonus : public SimpleEnchantment {
public:
    using SimpleEnchantment::SimpleEnchantment;
    bool canApplyTogether(const Enchantment* ench) const override { return Enchantment::canApplyTogether(ench) && ench->effectId != Enchantment::silkTouch->effectId; }
};

class EnchantmentDurability : public SimpleEnchantment {
public:
    using SimpleEnchantment::SimpleEnchantment;
    bool canApply(const ItemStack& stack) const override { return stack.isItemStackDamageable() ? true : Enchantment::canApply(stack); }
};

class EnchantmentThorns : public SimpleEnchantment {
public:
    using SimpleEnchantment::SimpleEnchantment;
    bool canApply(const ItemStack& stack) const override { return dynamic_cast<const ItemArmor*>(stack.getItem()) ? true : Enchantment::canApply(stack); }
    // Java: onUserHurt and the thorns damage helpers -- entities (stage 10).
};

std::vector<std::unique_ptr<Enchantment>>& ownedEnchantments()
{
    static std::vector<std::unique_ptr<Enchantment>> list;
    return list;
}

template <class T, class... Args>
Enchantment* make(Args&&... args)
{
    auto e = std::make_unique<T>(std::forward<Args>(args)...);
    Enchantment* raw = e.get();
    ownedEnchantments().push_back(std::move(e));
    return raw;
}

} // namespace

void Enchantment::registerEnchantments()
{
    if (protection)
        return;
    using T = EnumEnchantmentType;
    protection = make<EnchantmentProtection>(0, "protection", 10, 0);
    fireProtection = make<EnchantmentProtection>(1, "fire_protection", 5, 1);
    featherFalling = make<EnchantmentProtection>(2, "feather_falling", 5, 2);
    blastProtection = make<EnchantmentProtection>(3, "blast_protection", 2, 3);
    projectileProtection = make<EnchantmentProtection>(4, "projectile_protection", 5, 4);
    respiration = make<SimpleEnchantment>(5, "respiration", 2, T::ARMOR_HEAD, "oxygen", 3, //
        +[](int l) { return 10 * l; }, +[](int l) { return 10 * l + 30; });
    aquaAffinity = make<SimpleEnchantment>(6, "aqua_affinity", 2, T::ARMOR_HEAD, "waterWorker", 1, //
        +[](int) { return 1; }, +[](int) { return 1 + 40; });
    thorns = make<EnchantmentThorns>(7, "thorns", 1, T::ARMOR_TORSO, "thorns", 3, //
        +[](int l) { return 10 + 20 * (l - 1); }, +[](int l) { return baseMinEnchantability(l) + 50; });
    depthStrider = make<SimpleEnchantment>(8, "depth_strider", 2, T::ARMOR_FEET, "waterWalker", 3, //
        +[](int l) { return l * 10; }, +[](int l) { return l * 10 + 15; });
    sharpness = make<EnchantmentDamage>(16, "sharpness", 10, 0);
    smite = make<EnchantmentDamage>(17, "smite", 5, 1);
    baneOfArthropods = make<EnchantmentDamage>(18, "bane_of_arthropods", 5, 2);
    knockback = make<SimpleEnchantment>(19, "knockback", 5, T::WEAPON, "knockback", 2, //
        +[](int l) { return 5 + 20 * (l - 1); }, +[](int l) { return baseMinEnchantability(l) + 50; });
    fireAspect = make<SimpleEnchantment>(20, "fire_aspect", 2, T::WEAPON, "fire", 2, //
        +[](int l) { return 10 + 20 * (l - 1); }, +[](int l) { return baseMinEnchantability(l) + 50; });
    looting = make<EnchantmentLootBonus>(21, "looting", 2, T::WEAPON, "lootBonus", 3, //
        +[](int l) { return 15 + (l - 1) * 9; }, +[](int l) { return baseMinEnchantability(l) + 50; });
    efficiency = make<ShearsEnchantment>(32, "efficiency", 10, T::DIGGER, "digging", 5, //
        +[](int l) { return 1 + 10 * (l - 1); }, +[](int l) { return baseMinEnchantability(l) + 50; });
    silkTouch = make<EnchantmentUntouching>(33, "silk_touch", 1, T::DIGGER, "untouching", 1, //
        +[](int) { return 15; }, +[](int l) { return baseMinEnchantability(l) + 50; });
    unbreaking = make<EnchantmentDurability>(34, "unbreaking", 5, T::BREAKABLE, "durability", 3, //
        +[](int l) { return 5 + (l - 1) * 8; }, +[](int l) { return baseMinEnchantability(l) + 50; });
    fortune = make<EnchantmentLootBonus>(35, "fortune", 2, T::DIGGER, "lootBonusDigger", 3, //
        +[](int l) { return 15 + (l - 1) * 9; }, +[](int l) { return baseMinEnchantability(l) + 50; });
    power = make<SimpleEnchantment>(48, "power", 10, T::BOW, "arrowDamage", 5, //
        +[](int l) { return 1 + (l - 1) * 10; }, +[](int l) { return 1 + (l - 1) * 10 + 15; });
    punch = make<SimpleEnchantment>(49, "punch", 2, T::BOW, "arrowKnockback", 2, //
        +[](int l) { return 12 + (l - 1) * 20; }, +[](int l) { return 12 + (l - 1) * 20 + 25; });
    flame = make<SimpleEnchantment>(50, "flame", 2, T::BOW, "arrowFire", 1, //
        +[](int) { return 20; }, +[](int) { return 50; });
    infinity = make<SimpleEnchantment>(51, "infinity", 1, T::BOW, "arrowInfinite", 1, //
        +[](int) { return 20; }, +[](int) { return 50; });
    luckOfTheSea = make<EnchantmentLootBonus>(61, "luck_of_the_sea", 2, T::FISHING_ROD, "lootBonusFishing", 3, //
        +[](int l) { return 15 + (l - 1) * 9; }, +[](int l) { return baseMinEnchantability(l) + 50; });
    lure = make<SimpleEnchantment>(62, "lure", 2, T::FISHING_ROD, "fishingSpeed", 3, //
        +[](int l) { return 15 + (l - 1) * 9; }, +[](int l) { return baseMinEnchantability(l) + 50; });
    for (Enchantment* enchantment : enchantmentsList) {
        if (enchantment)
            enchantmentsBookList.push_back(enchantment);
    }
}

} // namespace mc
