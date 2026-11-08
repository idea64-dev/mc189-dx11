#pragma once

// Original: net/minecraft/util/EnumParticleTypes.java -- the particle ids sent to IWorldAccess.spawnParticle.

#include <string_view>

namespace mc {

enum class EnumParticleTypes {
    EXPLOSION_NORMAL,
    EXPLOSION_LARGE,
    EXPLOSION_HUGE,
    FIREWORKS_SPARK,
    WATER_BUBBLE,
    WATER_SPLASH,
    WATER_WAKE,
    SUSPENDED,
    SUSPENDED_DEPTH,
    CRIT,
    CRIT_MAGIC,
    SMOKE_NORMAL,
    SMOKE_LARGE,
    SPELL,
    SPELL_INSTANT,
    SPELL_MOB,
    SPELL_MOB_AMBIENT,
    SPELL_WITCH,
    DRIP_WATER,
    DRIP_LAVA,
    VILLAGER_ANGRY,
    VILLAGER_HAPPY,
    TOWN_AURA,
    NOTE,
    PORTAL,
    ENCHANTMENT_TABLE,
    FLAME,
    LAVA,
    FOOTSTEP,
    CLOUD,
    REDSTONE,
    SNOWBALL,
    SNOW_SHOVEL,
    SLIME,
    HEART,
    BARRIER,
    ITEM_CRACK,
    BLOCK_CRACK,
    BLOCK_DUST,
    WATER_DROP,
    ITEM_TAKE,
    MOB_APPEARANCE,
};

// The ids equal the ordinals.
constexpr int getParticleID(EnumParticleTypes type) { return static_cast<int>(type); }

constexpr bool getShouldIgnoreRange(EnumParticleTypes type)
{
    return type == EnumParticleTypes::EXPLOSION_NORMAL || type == EnumParticleTypes::EXPLOSION_LARGE || type == EnumParticleTypes::EXPLOSION_HUGE
        || type == EnumParticleTypes::MOB_APPEARANCE;
}

constexpr std::string_view getParticleName(EnumParticleTypes type)
{
    constexpr std::string_view names[42] { "explode", "largeexplode", "hugeexplosion", "fireworksSpark", "bubble", "splash", "wake", "suspended",
        "depthsuspend", "crit", "magicCrit", "smoke", "largesmoke", "spell", "instantSpell", "mobSpell", "mobSpellAmbient", "witchMagic", "dripWater",
        "dripLava", "angryVillager", "happyVillager", "townaura", "note", "portal", "enchantmenttable", "flame", "lava", "footstep", "cloud", "reddust",
        "snowballpoof", "snowshovel", "slime", "heart", "barrier", "iconcrack_", "blockcrack_", "blockdust_", "droplet", "take", "mobappearance" };
    return names[static_cast<int>(type)];
}

// Extra int arguments: 2 for ITEM_CRACK, 1 for BLOCK_CRACK and BLOCK_DUST, 0 otherwise.
constexpr int getArgumentCount(EnumParticleTypes type)
{
    return type == EnumParticleTypes::ITEM_CRACK ? 2 : (type == EnumParticleTypes::BLOCK_CRACK || type == EnumParticleTypes::BLOCK_DUST ? 1 : 0);
}

} // namespace mc
