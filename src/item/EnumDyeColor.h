#pragma once

// Original: net/minecraft/item/EnumDyeColor.java

#include "block/material/Material.h"
#include "block/properties/IProperty.h"
#include "core/EnumValues.h"

#include <string_view>

namespace mc {

enum class EnumDyeColor { WHITE, ORANGE, MAGENTA, LIGHT_BLUE, YELLOW, LIME, PINK, GRAY, SILVER, CYAN, PURPLE, BLUE, BROWN, GREEN, RED, BLACK };

struct EnumDyeColorInfo {
    int meta;
    int dyeDamage;
    std::string_view name;
    std::string_view unlocalizedName;
    const MapColor* mapColor;
    char chatColor; // EnumChatFormatting code
};

inline constexpr EnumDyeColorInfo kDyeColors[16] {
    { 0, 15, "white", "white", &MapColor::snowColor, 'f' },
    { 1, 14, "orange", "orange", &MapColor::adobeColor, '6' },
    { 2, 13, "magenta", "magenta", &MapColor::magentaColor, 'b' },
    { 3, 12, "light_blue", "lightBlue", &MapColor::lightBlueColor, '9' },
    { 4, 11, "yellow", "yellow", &MapColor::yellowColor, 'e' },
    { 5, 10, "lime", "lime", &MapColor::limeColor, 'a' },
    { 6, 9, "pink", "pink", &MapColor::pinkColor, 'd' },
    { 7, 8, "gray", "gray", &MapColor::grayColor, '8' },
    { 8, 7, "silver", "silver", &MapColor::silverColor, '7' },
    { 9, 6, "cyan", "cyan", &MapColor::cyanColor, '3' },
    { 10, 5, "purple", "purple", &MapColor::purpleColor, '5' },
    { 11, 4, "blue", "blue", &MapColor::blueColor, '1' },
    { 12, 3, "brown", "brown", &MapColor::brownColor, '6' },
    { 13, 2, "green", "green", &MapColor::greenColor, '2' },
    { 14, 1, "red", "red", &MapColor::redColor, '4' },
    { 15, 0, "black", "black", &MapColor::blackColor, '0' },
};

constexpr const EnumDyeColorInfo& info(EnumDyeColor color) { return kDyeColors[static_cast<int>(color)]; }
constexpr int getMetadata(EnumDyeColor color) { return info(color).meta; }
constexpr int getDyeDamage(EnumDyeColor color) { return info(color).dyeDamage; }

// Out-of-range values fall back to entry 0 of the lookup, as in Java (WHITE by meta, BLACK by dye damage).
inline EnumDyeColor dyeColorByMetadata(int meta)
{
    return meta < 0 || meta >= 16 ? EnumDyeColor::WHITE : static_cast<EnumDyeColor>(meta);
}

inline EnumDyeColor dyeColorByDyeDamage(int damage)
{
    return damage < 0 || damage >= 16 ? EnumDyeColor::BLACK : static_cast<EnumDyeColor>(15 - damage);
}

MC_ENUM_TRAITS(EnumDyeColor, 16, info(value).name)

} // namespace mc
