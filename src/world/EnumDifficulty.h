#pragma once

// Original: net/minecraft/world/EnumDifficulty.java

#include <stdexcept>
#include <string_view>

namespace mc {

enum class EnumDifficulty { PEACEFUL, EASY, NORMAL, HARD };

constexpr int getDifficultyId(EnumDifficulty difficulty)
{
    return static_cast<int>(difficulty);
}

constexpr std::string_view getDifficultyResourceKey(EnumDifficulty difficulty)
{
    switch (difficulty) {
    case EnumDifficulty::PEACEFUL: return "options.difficulty.peaceful";
    case EnumDifficulty::EASY: return "options.difficulty.easy";
    case EnumDifficulty::NORMAL: return "options.difficulty.normal";
    case EnumDifficulty::HARD: return "options.difficulty.hard";
    }
    return "";
}

// Java: difficultyEnums[id % length] -- a negative id throws.
inline EnumDifficulty getDifficultyEnum(int id)
{
    const int index = id % 4;
    if (index < 0)
        throw std::out_of_range("difficulty");
    return static_cast<EnumDifficulty>(index);
}

} // namespace mc
