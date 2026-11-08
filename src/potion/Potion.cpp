#include "potion/Potion.h"

#include "core/Log.h"
#include "nbt/NBTTagCompound.h"

#include <unordered_map>
#include <vector>

namespace mc {

std::array<const Potion*, 32> Potion::potionTypes {};

namespace {

std::unordered_map<ResourceLocation, const Potion*>& potionsByLocation()
{
    static std::unordered_map<ResourceLocation, const Potion*> map;
    return map;
}

} // namespace

struct Potion::Init {
    static std::vector<std::unique_ptr<Potion>>& owned()
    {
        static std::vector<std::unique_ptr<Potion>> potions;
        return potions;
    }

    // new Potion(id, location, bad, color).setPotionName(name).setIconIndex(x, y)[.setEffectiveness(e)]; an icon
    // x of -1 skips setIconIndex, an effectiveness of 0 skips setEffectiveness.
    static const Potion* make(int id, const char* location, bool bad, int color, Kind kind, const char* name, int iconX = -1, int iconY = 0, double effectiveness = 0.0)
    {
        owned().push_back(std::unique_ptr<Potion>(new Potion(id, ResourceLocation(location), bad, color, kind)));
        Potion* potion = owned().back().get();
        potion->name = name;
        if (iconX >= 0)
            potion->statusIconIndex = iconX + iconY * 8;
        if (effectiveness != 0.0)
            potion->effectiveness = effectiveness;
        return potion;
    }
};

Potion::Potion(int potionID, const ResourceLocation& location, bool badEffectIn, int potionColor, Kind kindIn)
    : id(potionID)
    , kind(kindIn)
    , badEffect(badEffectIn)
    , liquidColor(potionColor)
    , effectiveness(badEffectIn ? 0.5 : 1.0)
{
    potionTypes[static_cast<size_t>(potionID)] = this;
    potionsByLocation()[location] = this;
}

// Definition order is initialization order within this file, as in the Java class.
const Potion* Potion::moveSpeed = Init::make(1, "speed", false, 8171462, Kind::NORMAL, "potion.moveSpeed", 0, 0);
const Potion* Potion::moveSlowdown = Init::make(2, "slowness", true, 5926017, Kind::NORMAL, "potion.moveSlowdown", 1, 0);
const Potion* Potion::digSpeed = Init::make(3, "haste", false, 14270531, Kind::NORMAL, "potion.digSpeed", 2, 0, 1.5);
const Potion* Potion::digSlowdown = Init::make(4, "mining_fatigue", true, 4866583, Kind::NORMAL, "potion.digSlowDown", 3, 0);
const Potion* Potion::damageBoost = Init::make(5, "strength", false, 9643043, Kind::ATTACK_DAMAGE, "potion.damageBoost", 4, 0);
const Potion* Potion::heal = Init::make(6, "instant_health", false, 16262179, Kind::HEALTH, "potion.heal");
const Potion* Potion::harm = Init::make(7, "instant_damage", true, 4393481, Kind::HEALTH, "potion.harm");
const Potion* Potion::jump = Init::make(8, "jump_boost", false, 2293580, Kind::NORMAL, "potion.jump", 2, 1);
const Potion* Potion::confusion = Init::make(9, "nausea", true, 5578058, Kind::NORMAL, "potion.confusion", 3, 1, 0.25);
const Potion* Potion::regeneration = Init::make(10, "regeneration", false, 13458603, Kind::NORMAL, "potion.regeneration", 7, 0, 0.25);
const Potion* Potion::resistance = Init::make(11, "resistance", false, 10044730, Kind::NORMAL, "potion.resistance", 6, 1);
const Potion* Potion::fireResistance = Init::make(12, "fire_resistance", false, 14981690, Kind::NORMAL, "potion.fireResistance", 7, 1);
const Potion* Potion::waterBreathing = Init::make(13, "water_breathing", false, 3035801, Kind::NORMAL, "potion.waterBreathing", 0, 2);
const Potion* Potion::invisibility = Init::make(14, "invisibility", false, 8356754, Kind::NORMAL, "potion.invisibility", 0, 1);
const Potion* Potion::blindness = Init::make(15, "blindness", true, 2039587, Kind::NORMAL, "potion.blindness", 5, 1, 0.25);
const Potion* Potion::nightVision = Init::make(16, "night_vision", false, 2039713, Kind::NORMAL, "potion.nightVision", 4, 1);
const Potion* Potion::hunger = Init::make(17, "hunger", true, 5797459, Kind::NORMAL, "potion.hunger", 1, 1);
const Potion* Potion::weakness = Init::make(18, "weakness", true, 4738376, Kind::ATTACK_DAMAGE, "potion.weakness", 5, 0);
const Potion* Potion::poison = Init::make(19, "poison", true, 5149489, Kind::NORMAL, "potion.poison", 6, 0, 0.25);
const Potion* Potion::wither = Init::make(20, "wither", true, 3484199, Kind::NORMAL, "potion.wither", 1, 2, 0.25);
const Potion* Potion::healthBoost = Init::make(21, "health_boost", false, 16284963, Kind::HEALTH_BOOST, "potion.healthBoost", 2, 2);
const Potion* Potion::absorption = Init::make(22, "absorption", false, 2445989, Kind::ABSORPTION, "potion.absorption", 2, 2);
const Potion* Potion::saturation = Init::make(23, "saturation", false, 16262179, Kind::HEALTH, "potion.saturation");

const Potion* Potion::getPotionFromResourceLocation(const std::string& location)
{
    const auto it = potionsByLocation().find(ResourceLocation(location));
    return it == potionsByLocation().end() ? nullptr : it->second;
}

void PotionEffect::combine(const PotionEffect& other)
{
    if (potionID != other.potionID)
        Log::warn("This method should only be called for matching effects!");
    if (other.amplifier > amplifier) {
        amplifier = other.amplifier;
        duration = other.duration;
    } else if (other.amplifier == amplifier && duration < other.duration) {
        duration = other.duration;
    } else if (!other.isAmbient && isAmbient) {
        isAmbient = other.isAmbient;
    }
    showParticles = other.showParticles;
}

std::string PotionEffect::toString() const
{
    std::string s;
    if (getAmplifier() > 0)
        s = getEffectName() + " x " + std::to_string(getAmplifier() + 1) + ", Duration: " + std::to_string(getDuration());
    else
        s = getEffectName() + ", Duration: " + std::to_string(getDuration());
    if (isSplashPotion)
        s += ", Splash: true";
    if (!showParticles)
        s += ", Particles: false";
    return Potion::potionTypes[static_cast<size_t>(potionID)]->isUsable() ? "(" + s + ")" : s;
}

void PotionEffect::writeCustomPotionEffectToNBT(NBTTagCompound& nbt) const
{
    nbt.setByte("Id", static_cast<int8_t>(getPotionID()));
    nbt.setByte("Amplifier", static_cast<int8_t>(getAmplifier()));
    nbt.setInteger("Duration", getDuration());
    nbt.setBoolean("Ambient", getIsAmbient());
    nbt.setBoolean("ShowParticles", getIsShowParticles());
}

std::unique_ptr<PotionEffect> PotionEffect::readCustomPotionEffectFromNBT(const NBTTagCompound& nbt)
{
    const int i = nbt.getByte("Id");
    if (i >= 0 && i < static_cast<int>(Potion::potionTypes.size()) && Potion::potionTypes[static_cast<size_t>(i)]) {
        const int j = nbt.getByte("Amplifier");
        const int k = nbt.getInteger("Duration");
        const bool flag = nbt.getBoolean("Ambient");
        bool flag1 = true;
        if (nbt.hasKey("ShowParticles", 1))
            flag1 = nbt.getBoolean("ShowParticles");
        return std::make_unique<PotionEffect>(i, k, j, flag, flag1);
    }
    return nullptr;
}

} // namespace mc
