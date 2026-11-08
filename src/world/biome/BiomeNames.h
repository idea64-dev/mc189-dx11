#pragma once

// The names of BiomeGenBase.getBiomeGenArray() by id (biomeName), for the customized world screen. The biomes
// themselves come with world generation (stage 6), which replaces this table. Generated from the MCP code.

#include <string_view>

namespace mc {

// Null for ids without a biome.
constexpr const char* biomeNameById(int id)
{
    switch (id) {
    case 0: return "Ocean";
    case 1: return "Plains";
    case 2: return "Desert";
    case 3: return "Extreme Hills";
    case 4: return "Forest";
    case 5: return "Taiga";
    case 6: return "Swampland";
    case 7: return "River";
    case 8: return "Hell";
    case 9: return "The End";
    case 10: return "FrozenOcean";
    case 11: return "FrozenRiver";
    case 12: return "Ice Plains";
    case 13: return "Ice Mountains";
    case 14: return "MushroomIsland";
    case 15: return "MushroomIslandShore";
    case 16: return "Beach";
    case 17: return "DesertHills";
    case 18: return "ForestHills";
    case 19: return "TaigaHills";
    case 20: return "Extreme Hills Edge";
    case 21: return "Jungle";
    case 22: return "JungleHills";
    case 23: return "JungleEdge";
    case 24: return "Deep Ocean";
    case 25: return "Stone Beach";
    case 26: return "Cold Beach";
    case 27: return "Birch Forest";
    case 28: return "Birch Forest Hills";
    case 29: return "Roofed Forest";
    case 30: return "Cold Taiga";
    case 31: return "Cold Taiga Hills";
    case 32: return "Mega Taiga";
    case 33: return "Mega Taiga Hills";
    case 34: return "Extreme Hills+";
    case 35: return "Savanna";
    case 36: return "Savanna Plateau";
    case 37: return "Mesa";
    case 38: return "Mesa Plateau F";
    case 39: return "Mesa Plateau";
    case 129: return "Sunflower Plains";
    case 130: return "Desert M";
    case 131: return "Extreme Hills M";
    case 132: return "Flower Forest";
    case 133: return "Taiga M";
    case 134: return "Swampland M";
    case 140: return "Ice Plains Spikes";
    case 149: return "Jungle M";
    case 151: return "JungleEdge M";
    case 155: return "Birch Forest M";
    case 156: return "Birch Forest Hills M";
    case 157: return "Roofed Forest M";
    case 158: return "Cold Taiga M";
    case 160: return "Mega Spruce Taiga";
    case 161: return "Redwood Taiga Hills M";
    case 162: return "Extreme Hills+ M";
    case 163: return "Savanna M";
    case 164: return "Savanna Plateau M";
    case 165: return "Mesa (Bryce)";
    case 166: return "Mesa Plateau F M";
    case 167: return "Mesa Plateau M";
    default: return nullptr;
    }
}

// BiomeGenBase.hell.biomeID
inline constexpr int kHellBiomeId = 8;

} // namespace mc
