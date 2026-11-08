#pragma once

// Original: net/minecraft/world/gen/layer/GenLayer.java -- the biome layer stack. The layers themselves
// (GenLayerIsland ... GenLayerVoronoiZoom) are in GenLayer.cpp.
// Java hands out arrays from IntCache, reused between calls without clearing; every layer writes each cell it
// returns, so fresh vectors give the same values.

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class WorldType;

class GenLayer {
public:
    virtual ~GenLayer() = default;

    // Java: initializeAllBiomeGenerators -- {biomes at 1:4, biomes at 1:1, biomes at 1:4}.
    static std::array<std::shared_ptr<GenLayer>, 3> initializeAllBiomeGenerators(int64_t seed, const WorldType* worldType, const std::string& options);

    explicit GenLayer(int64_t seed);

    virtual void initWorldGenSeed(int64_t seed);
    void initChunkSeed(int64_t x, int64_t z);
    virtual std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) = 0;

protected:
    int nextInt(int bound);
    static bool biomesEqualOrMesaPlateau(int biomeIDA, int biomeIDB);
    static bool isBiomeOceanic(int id);
    int selectRandom2(int a, int b) { return nextInt(2) == 0 ? a : b; }
    int selectRandom4(int a, int b, int c, int d);
    virtual int selectModeOrRandom(int a, int b, int c, int d);

    std::shared_ptr<GenLayer> parent;
    int64_t baseSeed;

private:
    int64_t worldGenSeed = 0;
    int64_t chunkSeed = 0;
};

} // namespace mc
