#pragma once

// A client-side world over a map of chunks, as tools/reference/net/minecraft/world/WorldDump.java builds it, and
// the chunk hash both sides compare.

#include "core/Java.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/chunk/Chunk.h"
#include "world/chunk/IChunkProvider.h"

#include <memory>
#include <unordered_map>

namespace mc::test {

inline int32_t hash(Chunk& chunk)
{
    int32_t h = 0;
    const auto mix = [&h](int32_t v) { h = java::add(java::mul(h, 31), v); };
    for (const int v : chunk.getHeightMap())
        mix(v);
    mix(chunk.getLowestHeight());
    mix(chunk.isLightPopulated() ? 1 : 0);
    mix(chunk.isTerrainPopulated() ? 1 : 0);
    for (const auto& s : chunk.getBlockStorageArray()) {
        if (!s) {
            mix(-1);
            continue;
        }
        for (const uint16_t c : s->getData())
            mix(c);
        for (const uint8_t b : s->getBlocklightArray()->getData())
            mix(b);
        for (const uint8_t b : s->getSkylightArray()->getData())
            mix(b);
        mix(s->isEmpty() ? 1 : 0);
        mix(s->getNeedsRandomTick() ? 1 : 0);
    }
    return h;
}

class MapChunkProvider : public IChunkProvider {
public:
    explicit MapChunkProvider(World* world)
        : blank(world, 0, 0)
    {
    }

    bool chunkExists(int x, int z) override { return chunks.contains(ChunkCoordIntPair::chunkXZ2Int(x, z)); }
    Chunk* provideChunk(int x, int z) override
    {
        const auto it = chunks.find(ChunkCoordIntPair::chunkXZ2Int(x, z));
        return it != chunks.end() ? it->second.get() : &blank;
    }
    Chunk* provideChunk(const BlockPos& pos) override { return provideChunk(pos.getX() >> 4, pos.getZ() >> 4); }
    void populate(IChunkProvider*, int, int) override { }
    bool populateChunk(IChunkProvider*, Chunk*, int, int) override { return false; }
    bool saveChunks(bool, IProgressUpdate*) override { return true; }
    bool unloadQueuedChunks() override { return false; }
    bool canSave() override { return false; }
    std::string makeString() override { return "Ref"; }
    int getLoadedChunkCount() override { return static_cast<int>(chunks.size()); }
    void recreateStructures(Chunk*, int, int) override { }
    void saveExtraData() override { }

    std::unordered_map<int64_t, std::unique_ptr<Chunk>> chunks;
    EmptyChunk blank;
};

class RefWorld : public World {
public:
    explicit RefWorld(std::shared_ptr<WorldInfo> info)
        : World(nullptr, std::move(info), WorldProvider::getProviderForDimension(0), profiler, true)
    {
        provider->registerWorld(this);
        chunkProvider = createChunkProvider();
        calculateInitialSkylight();
    }

    MapChunkProvider& chunks() { return static_cast<MapChunkProvider&>(*chunkProvider); }

protected:
    std::unique_ptr<IChunkProvider> createChunkProvider() override { return std::make_unique<MapChunkProvider>(this); }
    int getRenderDistanceChunks() override { return 8; }

private:
    static inline Profiler profiler;
};

} // namespace mc::test
