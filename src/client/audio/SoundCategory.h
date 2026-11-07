#pragma once

// Original: net/minecraft/client/audio/SoundCategory.java

#include <array>
#include <string_view>

namespace mc {

enum class SoundCategory { MASTER, MUSIC, RECORDS, WEATHER, BLOCKS, MOBS, ANIMALS, PLAYERS, AMBIENT };

inline constexpr std::array<SoundCategory, 9> kSoundCategories { SoundCategory::MASTER, SoundCategory::MUSIC, SoundCategory::RECORDS,
    SoundCategory::WEATHER, SoundCategory::BLOCKS, SoundCategory::MOBS, SoundCategory::ANIMALS, SoundCategory::PLAYERS, SoundCategory::AMBIENT };

constexpr std::string_view getCategoryName(SoundCategory category)
{
    switch (category) {
    case SoundCategory::MASTER: return "master";
    case SoundCategory::MUSIC: return "music";
    case SoundCategory::RECORDS: return "record";
    case SoundCategory::WEATHER: return "weather";
    case SoundCategory::BLOCKS: return "block";
    case SoundCategory::MOBS: return "hostile";
    case SoundCategory::ANIMALS: return "neutral";
    case SoundCategory::PLAYERS: return "player";
    case SoundCategory::AMBIENT: return "ambient";
    }
    return "";
}

constexpr int getCategoryId(SoundCategory category)
{
    return static_cast<int>(category);
}

} // namespace mc
