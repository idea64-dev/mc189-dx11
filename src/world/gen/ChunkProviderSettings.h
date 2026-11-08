#pragma once

// Original: net/minecraft/world/gen/ChunkProviderSettings.java (with Factory and Serializer) -- the "customized"
// world type's generator options, kept as JSON in level.dat. The generator that reads them is stage 6.

#include <cstdint>
#include <string>
#include <string_view>

namespace mc {

class ChunkProviderSettings {
public:
    struct Factory {
        Factory() { func_177863_a(); }

        // Java: jsonToFactory -- defaults for an empty or unreadable string.
        static Factory jsonToFactory(std::string_view json);
        // Java: toString(), the Gson serialization.
        std::string toString() const;
        // Java: func_177863_a -- back to the defaults.
        void func_177863_a();
        bool operator==(const Factory& other) const;
        int32_t hashCode() const;

        float coordinateScale;
        float heightScale;
        float upperLimitScale;
        float lowerLimitScale;
        float depthNoiseScaleX;
        float depthNoiseScaleZ;
        float depthNoiseScaleExponent;
        float mainNoiseScaleX;
        float mainNoiseScaleY;
        float mainNoiseScaleZ;
        float baseSize;
        float stretchY;
        float biomeDepthWeight;
        float biomeDepthOffset;
        float biomeScaleWeight;
        float biomeScaleOffset;
        int seaLevel;
        bool useCaves;
        bool useDungeons;
        int dungeonChance;
        bool useStrongholds;
        bool useVillages;
        bool useMineShafts;
        bool useTemples;
        bool useMonuments;
        bool useRavines;
        bool useWaterLakes;
        int waterLakeChance;
        bool useLavaLakes;
        int lavaLakeChance;
        bool useLavaOceans;
        int fixedBiome;
        int biomeSize;
        int riverSize;
        int dirtSize;
        int dirtCount;
        int dirtMinHeight;
        int dirtMaxHeight;
        int gravelSize;
        int gravelCount;
        int gravelMinHeight;
        int gravelMaxHeight;
        int graniteSize;
        int graniteCount;
        int graniteMinHeight;
        int graniteMaxHeight;
        int dioriteSize;
        int dioriteCount;
        int dioriteMinHeight;
        int dioriteMaxHeight;
        int andesiteSize;
        int andesiteCount;
        int andesiteMinHeight;
        int andesiteMaxHeight;
        int coalSize;
        int coalCount;
        int coalMinHeight;
        int coalMaxHeight;
        int ironSize;
        int ironCount;
        int ironMinHeight;
        int ironMaxHeight;
        int goldSize;
        int goldCount;
        int goldMinHeight;
        int goldMaxHeight;
        int redstoneSize;
        int redstoneCount;
        int redstoneMinHeight;
        int redstoneMaxHeight;
        int diamondSize;
        int diamondCount;
        int diamondMinHeight;
        int diamondMaxHeight;
        int lapisSize;
        int lapisCount;
        int lapisCenterHeight;
        int lapisSpread;
    };

    // Java: the immutable settings func_177864_b() builds for the generator; the same values.
    explicit ChunkProviderSettings(const Factory& factory)
        : values(factory)
    {
    }

    const Factory values;
};

} // namespace mc
