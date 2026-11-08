#include "world/gen/ChunkProviderServer.h"

#include "core/Log.h"
#include "world/ChunkCoordIntPair.h"
#include "world/WorldProvider.h"
#include "world/WorldServer.h"
#include "world/chunk/Chunk.h"
#include "world/storage/ISaveHandler.h"

#include <algorithm>

namespace mc {

ChunkProviderServer::ChunkProviderServer(WorldServer* worldIn, std::unique_ptr<IChunkLoader> chunkLoaderIn, std::unique_ptr<IChunkProvider> chunkGeneratorIn)
    : dummyChunk(std::make_unique<EmptyChunk>(worldIn, 0, 0))
    , serverChunkGenerator(std::move(chunkGeneratorIn))
    , chunkLoader(std::move(chunkLoaderIn))
    , worldObj(worldIn)
{
}

ChunkProviderServer::~ChunkProviderServer() = default;

bool ChunkProviderServer::chunkExists(int x, int z)
{
    return findLoaded(x, z) != nullptr;
}

void ChunkProviderServer::dropChunk(int x, int z)
{
    if (worldObj->provider->canRespawnHere()) {
        if (!worldObj->isSpawnChunk(x, z))
            droppedChunksSet.put(ChunkCoordIntPair::chunkXZ2Int(x, z), true);
    } else {
        droppedChunksSet.put(ChunkCoordIntPair::chunkXZ2Int(x, z), true);
    }
}

void ChunkProviderServer::unloadAllChunks()
{
    for (Chunk* chunk : loadedChunks)
        dropChunk(chunk->xPosition, chunk->zPosition);
}

Chunk* ChunkProviderServer::loadChunk(int chunkX, int chunkZ)
{
    const int64_t i = ChunkCoordIntPair::chunkXZ2Int(chunkX, chunkZ);
    droppedChunksSet.erase(i);
    if (const auto it = id2ChunkMap.find(i); it != id2ChunkMap.end())
        return it->second.get();

    std::unique_ptr<Chunk> loaded = loadChunkFromFile(chunkX, chunkZ);
    if (!loaded && serverChunkGenerator) {
        try {
            loaded = serverChunkGenerator->generateChunk(chunkX, chunkZ);
        } catch (const std::exception& e) {
            Log::error("Exception generating new chunk at {},{} ({}): {}", chunkX, chunkZ, serverChunkGenerator->makeString(), e.what());
            throw;
        }
    }

    Chunk* chunk;
    if (loaded) {
        chunk = loaded.get();
        id2ChunkMap.emplace(i, std::move(loaded));
    } else {
        // Java maps the shared dummy chunk; here it isn't owned by the map.
        chunk = dummyChunk.get();
        id2ChunkMap.emplace(i, nullptr);
    }
    loadedChunks.push_back(chunk);
    chunk->onChunkLoad();
    chunk->populateChunk(this, this, chunkX, chunkZ);
    return chunk;
}

Chunk* ChunkProviderServer::findLoaded(int x, int z)
{
    const int64_t key = ChunkCoordIntPair::chunkXZ2Int(x, z);
    CachedChunk& cached = chunkCache[static_cast<size_t>((x & 7) | ((z & 7) << 3))];
    if (cached.chunk && cached.key == key)
        return cached.chunk;
    const auto it = id2ChunkMap.find(key);
    if (it == id2ChunkMap.end())
        return nullptr;
    cached.key = key;
    cached.chunk = it->second ? it->second.get() : dummyChunk.get();
    return cached.chunk;
}

Chunk* ChunkProviderServer::provideChunk(int x, int z)
{
    if (Chunk* chunk = findLoaded(x, z))
        return chunk;
    return !worldObj->isFindingSpawnPoint() && !chunkLoadOverride ? dummyChunk.get() : loadChunk(x, z);
}

std::unique_ptr<Chunk> ChunkProviderServer::loadChunkFromFile(int x, int z)
{
    if (!chunkLoader)
        return nullptr;
    try {
        std::unique_ptr<Chunk> chunk = chunkLoader->loadChunk(worldObj, x, z);
        if (chunk) {
            chunk->setLastSaveTime(worldObj->getTotalWorldTime());
            if (serverChunkGenerator)
                serverChunkGenerator->recreateStructures(chunk.get(), x, z);
        }
        return chunk;
    } catch (const std::exception& e) {
        Log::error("Couldn't load chunk: {}", e.what());
        return nullptr;
    }
}

void ChunkProviderServer::saveChunkExtraData(Chunk* chunkIn)
{
    if (!chunkLoader)
        return;
    try {
        chunkLoader->saveExtraChunkData(worldObj, chunkIn);
    } catch (const std::exception& e) {
        Log::error("Couldn't save entities: {}", e.what());
    }
}

void ChunkProviderServer::saveChunkData(Chunk* chunkIn)
{
    if (!chunkLoader)
        return;
    try {
        chunkIn->setLastSaveTime(worldObj->getTotalWorldTime());
        chunkLoader->saveChunk(worldObj, chunkIn);
    } catch (const MinecraftException& e) {
        Log::error("Couldn't save chunk; already in use by another instance of Minecraft?: {}", e.what());
    } catch (const std::exception& e) {
        Log::error("Couldn't save chunk: {}", e.what());
    }
}

void ChunkProviderServer::populate(IChunkProvider* chunkProvider, int x, int z)
{
    Chunk* chunk = provideChunk(x, z);
    if (!chunk->isTerrainPopulated()) {
        chunk->func_150809_p();
        if (serverChunkGenerator) {
            serverChunkGenerator->populate(chunkProvider, x, z);
            chunk->setChunkModified();
        }
    }
}

bool ChunkProviderServer::populateChunk(IChunkProvider* chunkProvider, Chunk* chunkIn, int x, int z)
{
    if (serverChunkGenerator && serverChunkGenerator->populateChunk(chunkProvider, chunkIn, x, z)) {
        provideChunk(x, z)->setChunkModified();
        return true;
    }
    return false;
}

bool ChunkProviderServer::saveChunks(bool saveAllChunks, IProgressUpdate*)
{
    int i = 0;
    const std::vector<Chunk*> list = loadedChunks;

    for (Chunk* chunk : list) {
        if (saveAllChunks)
            saveChunkExtraData(chunk);

        if (chunk->needsSaving(saveAllChunks)) {
            saveChunkData(chunk);
            chunk->setModified(false);
            ++i;
            if (i == 24 && !saveAllChunks)
                return false;
        }
    }

    return true;
}

void ChunkProviderServer::saveExtraData()
{
    if (chunkLoader)
        chunkLoader->saveExtraData();
}

bool ChunkProviderServer::unloadQueuedChunks()
{
    if (!worldObj->disableLevelSaving) {
        for (int i = 0; i < 100; ++i) {
            if (!droppedChunksSet.empty()) {
                const int64_t key = droppedChunksSet.begin()->first;
                if (const auto it = id2ChunkMap.find(key); it != id2ChunkMap.end()) {
                    Chunk* chunk = it->second ? it->second.get() : dummyChunk.get();
                    chunk->onChunkUnload();
                    saveChunkData(chunk);
                    saveChunkExtraData(chunk);
                    // Java: loadedChunks.remove(chunk) -- the first occurrence (the dummy chunk may be listed often).
                    if (const auto pos = std::find(loadedChunks.begin(), loadedChunks.end(), chunk); pos != loadedChunks.end())
                        loadedChunks.erase(pos);
                    id2ChunkMap.erase(it);
                    chunkCache.fill({});
                }
                droppedChunksSet.erase(key);
            }
        }

        if (chunkLoader)
            chunkLoader->chunkTick();
    }

    return serverChunkGenerator->unloadQueuedChunks();
}

bool ChunkProviderServer::canSave()
{
    return !worldObj->disableLevelSaving;
}

std::string ChunkProviderServer::makeString()
{
    return "ServerChunkCache: " + std::to_string(id2ChunkMap.size()) + " Drop: " + std::to_string(droppedChunksSet.size());
}

std::vector<BiomeGenBase::SpawnListEntry> ChunkProviderServer::getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos)
{
    return serverChunkGenerator->getPossibleCreatures(creatureType, pos);
}

std::optional<BlockPos> ChunkProviderServer::getStrongholdGen(World* worldIn, const std::string& structureName, const BlockPos& position)
{
    return serverChunkGenerator->getStrongholdGen(worldIn, structureName, position);
}

} // namespace mc
