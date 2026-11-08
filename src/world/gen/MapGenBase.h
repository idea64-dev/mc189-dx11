#pragma once

// Original: net/minecraft/world/gen/MapGenBase.java, MapGenCaves.java and MapGenRavine.java -- the carvers run
// over the chunks around the one being generated, each seeded from the world seed and its own position.

#include "util/Random.h"

#include <array>
#include <cstdint>

namespace mc {

class ChunkPrimer;
class IBlockState;
class IChunkProvider;
class World;

class MapGenBase {
public:
    virtual ~MapGenBase() = default;

    // chunkPrimerIn may be null (recreateStructures on load).
    virtual void generate(IChunkProvider* chunkProviderIn, World* worldIn, int x, int z, ChunkPrimer* chunkPrimerIn);

protected:
    virtual void recursiveGenerate(World*, int, int, int, int, ChunkPrimer*) { }

    int range = 8;
    Random rand;
    World* worldObj = nullptr;
};

class MapGenCaves : public MapGenBase {
protected:
    void func_180703_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z);
    void func_180702_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z, float width, float yaw, float pitch,
        int step, int steps, double heightScale);
    static bool func_175793_a(const IBlockState* state, const IBlockState* above);
    void recursiveGenerate(World* worldIn, int chunkX, int chunkZ, int originX, int originZ, ChunkPrimer* chunkPrimerIn) override;
};

class MapGenRavine : public MapGenBase {
protected:
    void func_180707_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z, float width, float yaw, float pitch,
        int step, int steps, double heightScale);
    void recursiveGenerate(World* worldIn, int chunkX, int chunkZ, int originX, int originZ, ChunkPrimer* chunkPrimerIn) override;

private:
    std::array<float, 1024> field_75046_d {};
};

// Java: MapGenCavesHell -- the Nether's carver.
class MapGenCavesHell : public MapGenBase {
protected:
    void func_180705_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z);
    void func_180704_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z, float width, float yaw, float pitch,
        int step, int steps, double heightScale);
    void recursiveGenerate(World* worldIn, int chunkX, int chunkZ, int originX, int originZ, ChunkPrimer* chunkPrimerIn) override;
};

} // namespace mc
