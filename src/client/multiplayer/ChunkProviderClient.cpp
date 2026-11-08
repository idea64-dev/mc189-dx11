#include "client/multiplayer/ChunkProviderClient.h"

#include "core/JavaTime.h"
#include "core/Log.h"

#include <algorithm>

namespace mc {

ChunkProviderClient::ChunkProviderClient(World* worldIn)
    : blankChunk(worldIn, 0, 0)
    , worldObj(worldIn)
{
}

void ChunkProviderClient::unloadChunk(int x, int z)
{
    Chunk* chunk = provideChunk(x, z);
    if (!chunk->isEmpty())
        chunk->onChunkUnload();
    const auto it = std::find(chunkListing.begin(), chunkListing.end(), chunk);
    if (it != chunkListing.end())
        chunkListing.erase(it);
    // Erased last: the blank chunk is not in the map, a loaded one dies here.
    chunkMapping.erase(ChunkCoordIntPair::chunkXZ2Int(x, z));
}

Chunk* ChunkProviderClient::loadChunk(int chunkX, int chunkZ)
{
    auto chunk = std::make_unique<Chunk>(worldObj, chunkX, chunkZ);
    Chunk* raw = chunk.get();
    // Java: LongHashMap.add replaces an existing entry and the replaced chunk stays in chunkListing (still
    // ticked, a leak); here it is dropped with its map entry.
    auto& slot = chunkMapping[ChunkCoordIntPair::chunkXZ2Int(chunkX, chunkZ)];
    if (slot) {
        const auto it = std::find(chunkListing.begin(), chunkListing.end(), slot.get());
        if (it != chunkListing.end())
            chunkListing.erase(it);
    }
    slot = std::move(chunk);
    chunkListing.push_back(raw);
    raw->setChunkLoaded(true);
    return raw;
}

Chunk* ChunkProviderClient::provideChunk(int x, int z)
{
    const auto it = chunkMapping.find(ChunkCoordIntPair::chunkXZ2Int(x, z));
    return it == chunkMapping.end() ? static_cast<Chunk*>(&blankChunk) : it->second.get();
}

bool ChunkProviderClient::unloadQueuedChunks()
{
    const int64_t i = java::currentTimeMillis();
    for (Chunk* chunk : chunkListing)
        chunk->func_150804_b(java::currentTimeMillis() - i > 5);
    if (java::currentTimeMillis() - i > 100)
        Log::info("Warning: Clientside chunk ticking took {} ms", java::currentTimeMillis() - i);
    return false;
}

std::string ChunkProviderClient::makeString()
{
    return "MultiplayerChunkCache: " + std::to_string(chunkMapping.size()) + ", " + std::to_string(chunkListing.size());
}

} // namespace mc
