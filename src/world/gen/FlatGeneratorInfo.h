#pragma once

// Original: net/minecraft/world/gen/FlatGeneratorInfo.java and FlatLayerInfo.java -- the superflat generator
// options ("3;minecraft:bedrock,2*minecraft:dirt,minecraft:grass;1;village").
// The feature maps are HashMaps in Java; JavaHashMap keeps their iteration order for toString().

#include "core/JavaHashMap.h"

#include <optional>
#include <string>
#include <vector>

namespace mc {

class Block;
class IBlockState;

class FlatLayerInfo {
public:
    FlatLayerInfo(int height, const Block* layerMaterialIn)
        : FlatLayerInfo(3, height, layerMaterialIn)
    {
    }
    FlatLayerInfo(int version, int height, const Block* layerMaterialIn);
    FlatLayerInfo(int version, int height, const Block* layerMaterialIn, int meta);

    int getLayerCount() const { return layerCount; }
    const IBlockState* getLayerMaterial() const { return layerMaterial; }
    int getMinY() const { return layerMinimumY; }
    void setMinY(int minY) { layerMinimumY = minY; }
    std::string toString() const;

private:
    int field_175902_a;
    const IBlockState* layerMaterial;
    int layerCount = 1;
    int layerMinimumY = 0;
};

class FlatGeneratorInfo {
public:
    using Options = JavaHashMap<std::string, std::string>;

    int getBiome() const { return biomeToUse; }
    void setBiome(int biome) { biomeToUse = biome; }
    JavaHashMap<std::string, Options>& getWorldFeatures() { return worldFeatures; }
    const JavaHashMap<std::string, Options>& getWorldFeatures() const { return worldFeatures; }
    std::vector<FlatLayerInfo>& getFlatLayers() { return flatLayers; }
    const std::vector<FlatLayerInfo>& getFlatLayers() const { return flatLayers; }
    void func_82645_d();
    std::string toString() const;

    static FlatGeneratorInfo createFlatGeneratorFromString(const std::string& flatGeneratorSettings);
    static FlatGeneratorInfo getDefaultFlatGenerator();

private:
    std::vector<FlatLayerInfo> flatLayers;
    JavaHashMap<std::string, Options> worldFeatures;
    int biomeToUse = 0;
};

} // namespace mc
