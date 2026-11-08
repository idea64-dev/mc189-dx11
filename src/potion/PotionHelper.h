#pragma once

// Original: net/minecraft/potion/PotionHelper.java -- the brewing bit language: ingredient strings flip bits of
// the potion damage value, requirement strings decide which effects a damage value carries.

#include "potion/Potion.h"

#include <string>
#include <string_view>
#include <vector>

namespace mc {

class PotionHelper {
public:
    static constexpr const char* sugarEffect = "-0+1-2-3&4-4+13";
    static constexpr const char* ghastTearEffect = "+0-1-2-3&4-4+13";
    static constexpr const char* spiderEyeEffect = "-0-1+2-3&4-4+13";
    static constexpr const char* fermentedSpiderEyeEffect = "-0+3-4+13";
    static constexpr const char* speckledMelonEffect = "+0-1+2-3&4-4+13";
    static constexpr const char* blazePowderEffect = "+0-1-2+3&4-4+13";
    static constexpr const char* magmaCreamEffect = "+0+1-2-3&4-4+13";
    static constexpr const char* redstoneEffect = "-5+6-7";
    static constexpr const char* glowstoneEffect = "+5-6-7";
    static constexpr const char* gunpowderEffect = "+14&13-13";
    static constexpr const char* goldenCarrotEffect = "-0+1+2-3+13&4-4";
    static constexpr const char* pufferfishEffect = "+0-1+2+3+13&4-4";
    static constexpr const char* rabbitFootEffect = "+0+1-2+3&4-4+13";

    static bool checkFlag(int value, int bit) { return (value & 1 << bit) != 0; }
    static int getPotionPrefixIndex(int dataValue) { return getPotionPrefixIndexFlags(dataValue, 5, 4, 3, 2, 1); }
    // An empty list stands for Java's null as well as an empty collection.
    static int calcPotionLiquidColor(const std::vector<PotionEffect>& effects);
    static bool getAreAmbient(const std::vector<PotionEffect>& effects);
    static int getLiquidColor(int dataValue, bool bypassCache);
    static const char* getPotionPrefix(int dataValue);
    // Java returns null instead of an empty list.
    static std::vector<PotionEffect> getPotionEffects(int dataValue, bool includeUsable);
    static int applyIngredient(int dataValue, std::string_view ingredient);
    static int getPotionPrefixIndexFlags(int value, int a, int b, int c, int d, int e);

private:
    static int parsePotionEffects(std::string_view text, int start, int end, int dataValue);
};

} // namespace mc
