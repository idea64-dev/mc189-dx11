#include "world/chunk/storage/AnvilChunkLoader.h"

#include "core/DataStream.h"
#include "core/Log.h"
#include "nbt/CompressedStreamTools.h"
#include "nbt/NBTTagCompound.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/chunk/Chunk.h"
#include "world/chunk/storage/RegionFile.h"

namespace mc {

AnvilChunkLoader::AnvilChunkLoader(std::filesystem::path chunkSaveLocationIn)
    : chunkSaveLocation(std::move(chunkSaveLocationIn))
{
}

AnvilChunkLoader::~AnvilChunkLoader()
{
    // Java keeps a queued loader alive until the IO thread has written it; here the owner goes away, so write
    // what is left now.
    ThreadedFileIOBase::getThreadedIOInstance().remove(this);
    while (writeNextIO()) {
    }
}

std::unique_ptr<Chunk> AnvilChunkLoader::loadChunk(World* worldIn, int x, int z)
{
    const ChunkCoordIntPair chunkcoordintpair(x, z);
    std::shared_ptr<NBTTagCompound> nbttagcompound;
    {
        std::lock_guard guard(pendingLock);
        if (const auto it = chunksToRemove.find(chunkcoordintpair); it != chunksToRemove.end())
            nbttagcompound = it->second;
    }
    if (!nbttagcompound) {
        const auto data = RegionFileCache::getChunkData(chunkSaveLocation, x, z);
        if (!data)
            return nullptr;
        DataInput input(*data);
        NBTSizeTracker tracker = NBTSizeTracker::infinite();
        nbttagcompound = CompressedStreamTools::read(input, tracker);
    }
    return checkedReadChunkFromNBT(worldIn, x, z, nbttagcompound);
}

std::unique_ptr<Chunk> AnvilChunkLoader::checkedReadChunkFromNBT(World* worldIn, int x, int z, const std::shared_ptr<NBTTagCompound>& nbt)
{
    if (!nbt->hasKey("Level", 10)) {
        Log::error("Chunk file at {},{} is missing level data, skipping", x, z);
        return nullptr;
    }
    const auto nbttagcompound = nbt->getCompoundTag("Level");
    if (!nbttagcompound->hasKey("Sections", 9)) {
        Log::error("Chunk file at {},{} is missing block data, skipping", x, z);
        return nullptr;
    }
    std::unique_ptr<Chunk> chunk = readChunkFromNBT(worldIn, *nbttagcompound);
    if (!chunk->isAtLocation(x, z)) {
        Log::error("Chunk file at {},{} is in the wrong location; relocating. (Expected {}, {}, got {}, {})", x, z, x, z, chunk->xPosition, chunk->zPosition);
        nbttagcompound->setInteger("xPos", x);
        nbttagcompound->setInteger("zPos", z);
        chunk = readChunkFromNBT(worldIn, *nbttagcompound);
    }
    return chunk;
}

void AnvilChunkLoader::saveChunk(World* worldIn, Chunk* chunkIn)
{
    worldIn->checkSessionLock();
    try {
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        nbttagcompound->setTag("Level", writeChunkToNBT(*chunkIn, *worldIn));
        addChunkToPending(chunkIn->getChunkCoordIntPair(), nbttagcompound);
    } catch (const std::exception& e) {
        Log::error("Failed to save chunk: {}", e.what());
    }
}

void AnvilChunkLoader::addChunkToPending(const ChunkCoordIntPair& pos, std::shared_ptr<NBTTagCompound> nbt)
{
    {
        std::lock_guard guard(pendingLock);
        if (!pendingAnvilChunksCoordinates.contains(pos)) {
            if (chunksToRemove.insert_or_assign(pos, std::move(nbt)).second)
                chunkOrder.push_back(pos);
        }
    }
    ThreadedFileIOBase::getThreadedIOInstance().queueIO(this);
}

bool AnvilChunkLoader::writeNextIO()
{
    ChunkCoordIntPair chunkcoordintpair(0, 0);
    std::shared_ptr<NBTTagCompound> nbttagcompound;
    {
        std::lock_guard guard(pendingLock);
        if (chunksToRemove.empty()) {
            if (field_183014_e)
                Log::info("ThreadedAnvilChunkStorage ({}): All chunks are saved", chunkSaveLocation.filename().string());
            return false;
        }
        chunkcoordintpair = chunkOrder.front();
        chunkOrder.pop_front();
        pendingAnvilChunksCoordinates.insert(chunkcoordintpair);
        nbttagcompound = std::move(chunksToRemove.at(chunkcoordintpair));
        chunksToRemove.erase(chunkcoordintpair);
    }
    try {
        writeChunkNBT(chunkcoordintpair, *nbttagcompound);
    } catch (const std::exception& e) {
        Log::error("Failed to save chunk: {}", e.what());
    }
    std::lock_guard guard(pendingLock);
    pendingAnvilChunksCoordinates.erase(chunkcoordintpair);
    return true;
}

void AnvilChunkLoader::writeChunkNBT(const ChunkCoordIntPair& pos, const NBTTagCompound& nbt)
{
    DataOutput output;
    CompressedStreamTools::write(nbt, output);
    RegionFileCache::writeChunkData(chunkSaveLocation, pos.chunkXPos, pos.chunkZPos, output.getBytes());
}

void AnvilChunkLoader::saveExtraData()
{
    field_183014_e = true;
    while (writeNextIO()) {
    }
    field_183014_e = false;
}

std::shared_ptr<NBTTagCompound> AnvilChunkLoader::writeChunkToNBT(Chunk& chunkIn, World& worldIn)
{
    auto level = std::make_shared<NBTTagCompound>();
    level->setByte("V", 1);
    level->setInteger("xPos", chunkIn.xPosition);
    level->setInteger("zPos", chunkIn.zPosition);
    level->setLong("LastUpdate", worldIn.getTotalWorldTime());
    level->setIntArray("HeightMap", std::vector<int32_t>(chunkIn.getHeightMap().begin(), chunkIn.getHeightMap().end()));
    level->setBoolean("TerrainPopulated", chunkIn.isTerrainPopulated());
    level->setBoolean("LightPopulated", chunkIn.isLightPopulated());
    level->setLong("InhabitedTime", chunkIn.getInhabitedTime());
    auto nbttaglist = std::make_shared<NBTTagList>();
    const bool flag = !worldIn.provider->getHasNoSky();
    for (const auto& storage : chunkIn.getBlockStorageArray()) {
        if (!storage)
            continue;
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        nbttagcompound->setByte("Y", static_cast<int8_t>(storage->getYLocation() >> 4 & 255));
        const auto& data = storage->getData();
        std::vector<uint8_t> abyte(data.size());
        NibbleArray nibblearray;
        std::unique_ptr<NibbleArray> nibblearray1;
        for (size_t i = 0; i < data.size(); ++i) {
            const int c0 = data[i];
            const int j = static_cast<int>(i) & 15;
            const int k = static_cast<int>(i) >> 8 & 15;
            const int l = static_cast<int>(i) >> 4 & 15;
            if (c0 >> 12 != 0) {
                if (!nibblearray1)
                    nibblearray1 = std::make_unique<NibbleArray>();
                nibblearray1->set(j, k, l, c0 >> 12);
            }
            abyte[i] = static_cast<uint8_t>(c0 >> 4 & 255);
            nibblearray.set(j, k, l, c0 & 15);
        }
        nbttagcompound->setByteArray("Blocks", std::move(abyte));
        nbttagcompound->setByteArray("Data", std::vector<uint8_t>(nibblearray.getData().begin(), nibblearray.getData().end()));
        if (nibblearray1)
            nbttagcompound->setByteArray("Add", std::vector<uint8_t>(nibblearray1->getData().begin(), nibblearray1->getData().end()));
        const auto& blockLight = storage->getBlocklightArray()->getData();
        nbttagcompound->setByteArray("BlockLight", std::vector<uint8_t>(blockLight.begin(), blockLight.end()));
        if (flag) {
            const auto& skyLight = storage->getSkylightArray()->getData();
            nbttagcompound->setByteArray("SkyLight", std::vector<uint8_t>(skyLight.begin(), skyLight.end()));
        } else {
            nbttagcompound->setByteArray("SkyLight", std::vector<uint8_t>(blockLight.size()));
        }
        nbttaglist->appendTag(nbttagcompound);
    }
    level->setTag("Sections", nbttaglist);
    level->setByteArray("Biomes", std::vector<uint8_t>(chunkIn.getBiomeArray().begin(), chunkIn.getBiomeArray().end()));

    // Entities, tile entities and scheduled ticks: written back as they were read (see Chunk.h).
    chunkIn.setHasEntities(false);
    auto entities = std::make_shared<NBTTagList>();
    for (const auto& entity : chunkIn.getPendingEntityNBT()) {
        chunkIn.setHasEntities(true);
        entities->appendTag(entity->copy());
    }
    level->setTag("Entities", entities);
    auto tileEntities = std::make_shared<NBTTagList>();
    for (const auto& tileEntity : chunkIn.getPendingTileEntityNBT())
        tileEntities->appendTag(tileEntity->copy());
    level->setTag("TileEntities", tileEntities);
    if (!chunkIn.getPendingTileTickNBT().empty()) {
        auto ticks = std::make_shared<NBTTagList>();
        for (const auto& tick : chunkIn.getPendingTileTickNBT())
            ticks->appendTag(tick->copy());
        level->setTag("TileTicks", ticks);
    }
    return level;
}

std::unique_ptr<Chunk> AnvilChunkLoader::readChunkFromNBT(World* worldIn, const NBTTagCompound& level)
{
    const int i = level.getInteger("xPos");
    const int j = level.getInteger("zPos");
    auto chunk = std::make_unique<Chunk>(worldIn, i, j);
    const std::vector<int32_t> heightMap = level.getIntArray("HeightMap");
    chunk->setHeightMap(heightMap);
    chunk->setTerrainPopulated(level.getBoolean("TerrainPopulated"));
    chunk->setLightPopulated(level.getBoolean("LightPopulated"));
    chunk->setInhabitedTime(level.getLong("InhabitedTime"));
    const auto nbttaglist = level.getTagList("Sections", 10);
    std::array<std::unique_ptr<ExtendedBlockStorage>, 16> sections;
    const bool flag = !worldIn->provider->getHasNoSky();
    for (int l = 0; l < nbttaglist->tagCount(); ++l) {
        const auto nbttagcompound = nbttaglist->getCompoundTagAt(l);
        const int i1 = nbttagcompound->getByte("Y");
        auto storage = std::make_unique<ExtendedBlockStorage>(i1 << 4, flag);
        const std::vector<uint8_t> abyte = nbttagcompound->getByteArray("Blocks");
        const NibbleArray nibblearray(nbttagcompound->getByteArray("Data"));
        std::unique_ptr<NibbleArray> nibblearray1;
        if (nbttagcompound->hasKey("Add", 7))
            nibblearray1 = std::make_unique<NibbleArray>(nbttagcompound->getByteArray("Add"));
        // Java sizes the id array by the Blocks array; anything but 4096 entries fails in setData or later.
        if (abyte.size() != 4096)
            throw std::out_of_range("Blocks array of " + std::to_string(abyte.size()) + " bytes");
        std::array<uint16_t, 4096> achar {};
        for (int j1 = 0; j1 < 4096; ++j1) {
            const int k1 = j1 & 15;
            const int l1 = j1 >> 8 & 15;
            const int i2 = j1 >> 4 & 15;
            const int j2 = nibblearray1 ? nibblearray1->get(k1, l1, i2) : 0;
            achar[static_cast<size_t>(j1)] = static_cast<uint16_t>(j2 << 12 | (abyte[static_cast<size_t>(j1)] & 255) << 4 | nibblearray.get(k1, l1, i2));
        }
        storage->setData(achar);
        storage->setBlocklightArray(std::make_unique<NibbleArray>(nbttagcompound->getByteArray("BlockLight")));
        if (flag)
            storage->setSkylightArray(std::make_unique<NibbleArray>(nbttagcompound->getByteArray("SkyLight")));
        storage->removeInvalidBlocks();
        sections.at(static_cast<size_t>(i1)) = std::move(storage);
    }
    chunk->setStorageArrays(std::move(sections));
    if (level.hasKey("Biomes", 7)) {
        const std::vector<uint8_t> biomes = level.getByteArray("Biomes");
        chunk->setBiomeArray(biomes);
    }
    const auto entities = level.getTagList("Entities", 10);
    for (int k2 = 0; k2 < entities->tagCount(); ++k2) {
        // Java: EntityList.createEntityFromNBT and the riding chain -- stage 10.
        chunk->setHasEntities(true);
        chunk->getPendingEntityNBT().push_back(entities->getCompoundTagAt(k2));
    }
    const auto tileEntities = level.getTagList("TileEntities", 10);
    for (int l2 = 0; l2 < tileEntities->tagCount(); ++l2) {
        // Java: TileEntity.createAndLoadEntity -- stage 9.
        chunk->getPendingTileEntityNBT().push_back(tileEntities->getCompoundTagAt(l2));
    }
    if (level.hasKey("TileTicks", 9)) {
        const auto ticks = level.getTagList("TileTicks", 10);
        for (int i3 = 0; i3 < ticks->tagCount(); ++i3) {
            // Java: worldIn.scheduleBlockUpdate -- stage 9.
            chunk->getPendingTileTickNBT().push_back(ticks->getCompoundTagAt(i3));
        }
    }
    return chunk;
}

} // namespace mc
