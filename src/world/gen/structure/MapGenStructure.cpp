#include "world/gen/structure/MapGenStructure.h"

#include "core/Log.h"
#include "nbt/NBTTagCompound.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/gen/structure/ComponentScatteredFeaturePieces.h"
#include "world/gen/structure/MapGenMineshaft.h"
#include "world/gen/structure/MapGenNetherBridge.h"
#include "world/gen/structure/MapGenScatteredFeature.h"
#include "world/gen/structure/MapGenStronghold.h"
#include "world/gen/structure/MapGenVillage.h"
#include "world/gen/structure/StructureMineshaftPieces.h"
#include "world/gen/structure/StructureNetherBridgePieces.h"
#include "world/gen/structure/StructureOceanMonument.h"
#include "world/gen/structure/StructureOceanMonumentPieces.h"
#include "world/gen/structure/StructureStrongholdPieces.h"
#include "world/gen/structure/StructureVillagePieces.h"

#include <cfloat>
#include <unordered_map>

namespace mc {

// --- MapGenStructureData ---

MapGenStructureData::MapGenStructureData(std::string name)
    : WorldSavedData(std::move(name))
    , tagCompound(std::make_shared<NBTTagCompound>())
{
}

void MapGenStructureData::readFromNBT(const NBTTagCompound& nbt)
{
    tagCompound = nbt.getCompoundTag("Features");
}

void MapGenStructureData::writeToNBT(NBTTagCompound& nbt) const
{
    nbt.setTag("Features", tagCompound);
}

void MapGenStructureData::writeInstance(std::shared_ptr<NBTTagCompound> tagCompoundIn, int chunkX, int chunkZ)
{
    tagCompound->setTag(formatChunkCoords(chunkX, chunkZ), std::move(tagCompoundIn));
}

// --- MapGenStructureIO ---

namespace {

std::unordered_map<std::string, MapGenStructureIO::StartFactory>& startNameToClassMap()
{
    static std::unordered_map<std::string, MapGenStructureIO::StartFactory> map;
    return map;
}

std::unordered_map<std::string, MapGenStructureIO::ComponentFactory>& componentNameToClassMap()
{
    static std::unordered_map<std::string, MapGenStructureIO::ComponentFactory> map;
    return map;
}

} // namespace

void MapGenStructureIO::registerAll()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    registerStructure("Mineshaft", [] { return std::make_shared<StructureMineshaftStart>(); });
    registerStructure("Village", [] { return std::make_shared<MapGenVillage::Start>(); });
    registerStructure("Fortress", [] { return std::make_shared<MapGenNetherBridge::Start>(); });
    registerStructure("Stronghold", [] { return std::make_shared<MapGenStronghold::Start>(); });
    registerStructure("Temple", [] { return std::make_shared<MapGenScatteredFeature::Start>(); });
    registerStructure("Monument", [] { return std::make_shared<StructureOceanMonument::StartMonument>(); });
    StructureMineshaftPieces::registerStructurePieces();
    StructureVillagePieces::registerVillagePieces();
    StructureNetherBridgePieces::registerNetherFortressPieces();
    StructureStrongholdPieces::registerStrongholdPieces();
    ComponentScatteredFeaturePieces::registerScatteredFeaturePieces();
    StructureOceanMonumentPieces::registerOceanMonumentPieces();
}

void MapGenStructureIO::registerStructure(const std::string& structureName, StartFactory factory)
{
    startNameToClassMap()[structureName] = std::move(factory);
}

void MapGenStructureIO::registerStructureComponent(const std::string& componentName, ComponentFactory factory)
{
    componentNameToClassMap()[componentName] = std::move(factory);
}

std::shared_ptr<StructureStart> MapGenStructureIO::getStructureStart(const NBTTagCompound& tagCompound, World* worldIn)
{
    registerAll();
    const std::string id = tagCompound.getString("id");
    const auto it = startNameToClassMap().find(id);
    if (it == startNameToClassMap().end()) {
        Log::warn("Skipping Structure with id {}", id);
        return nullptr;
    }
    std::shared_ptr<StructureStart> structurestart = it->second();
    structurestart->readStructureComponentsFromNBT(worldIn, tagCompound);
    return structurestart;
}

std::shared_ptr<StructureComponent> MapGenStructureIO::getStructureComponent(const NBTTagCompound& tagCompound, World* worldIn)
{
    registerAll();
    const std::string id = tagCompound.getString("id");
    const auto it = componentNameToClassMap().find(id);
    if (it == componentNameToClassMap().end()) {
        Log::warn("Skipping Piece with id {}", id);
        return nullptr;
    }
    std::shared_ptr<StructureComponent> structurecomponent = it->second();
    structurecomponent->readStructureBaseNBT(worldIn, tagCompound);
    return structurecomponent;
}

// --- MapGenStructure ---

void MapGenStructure::recursiveGenerate(World* worldIn, int chunkX, int chunkZ, int, int, ChunkPrimer*)
{
    initializeStructureData(worldIn);
    const int64_t key = ChunkCoordIntPair::chunkXZ2Int(chunkX, chunkZ);
    if (!structureMap.contains(key)) {
        rand.nextInt();
        if (canSpawnStructureAtCoords(chunkX, chunkZ)) {
            std::shared_ptr<StructureStart> structurestart = getStructureStart(chunkX, chunkZ);
            structureMap.put(key, structurestart);
            setStructureStart(chunkX, chunkZ, *structurestart);
        }
    }
}

bool MapGenStructure::generateStructure(World* worldIn, Random& randomIn, const ChunkCoordIntPair& chunkCoord)
{
    initializeStructureData(worldIn);
    const int i = (chunkCoord.chunkXPos << 4) + 8;
    const int j = (chunkCoord.chunkZPos << 4) + 8;
    bool flag = false;
    // A snapshot of the values: Java would throw if building a structure generated a chunk that adds a start.
    std::vector<std::shared_ptr<StructureStart>> starts;
    starts.reserve(structureMap.size());
    for (const auto& entry : structureMap)
        starts.push_back(entry.second);
    for (const auto& structurestart : starts) {
        if (structurestart->isSizeableStructure() && structurestart->func_175788_a(chunkCoord)
            && structurestart->getBoundingBox().intersectsWith(i, j, i + 15, j + 15)) {
            structurestart->generateStructure(worldIn, randomIn, StructureBoundingBox::ofXZ(i, j, i + 15, j + 15));
            structurestart->func_175787_b(chunkCoord);
            flag = true;
            setStructureStart(structurestart->getChunkPosX(), structurestart->getChunkPosZ(), *structurestart);
        }
    }
    return flag;
}

bool MapGenStructure::func_175795_b(const BlockPos& pos)
{
    initializeStructureData(worldObj);
    return func_175797_c(pos) != nullptr;
}

StructureStart* MapGenStructure::func_175797_c(const BlockPos& pos)
{
    for (const auto& entry : structureMap) {
        StructureStart* structurestart = entry.second.get();
        if (structurestart->isSizeableStructure() && structurestart->getBoundingBox().isVecInside(pos)) {
            for (const auto& structurecomponent : structurestart->getComponents()) {
                if (structurecomponent->getBoundingBox().isVecInside(pos))
                    return structurestart;
            }
        }
    }
    return nullptr;
}

bool MapGenStructure::isPositionInStructure(World* worldIn, const BlockPos& pos)
{
    initializeStructureData(worldIn);
    for (const auto& entry : structureMap) {
        if (entry.second->isSizeableStructure() && entry.second->getBoundingBox().isVecInside(pos))
            return true;
    }
    return false;
}

std::optional<BlockPos> MapGenStructure::getClosestStrongholdPos(World* worldIn, const BlockPos& pos)
{
    worldObj = worldIn;
    initializeStructureData(worldIn);
    rand.setSeed(worldIn->getSeed());
    const int64_t i = rand.nextLong();
    const int64_t j = rand.nextLong();
    const int64_t k = java::mul(static_cast<int64_t>(pos.getX() >> 4), i);
    const int64_t l = java::mul(static_cast<int64_t>(pos.getZ() >> 4), j);
    rand.setSeed(k ^ l ^ worldIn->getSeed());
    recursiveGenerate(worldIn, pos.getX() >> 4, pos.getZ() >> 4, 0, 0, nullptr);
    double d0 = DBL_MAX;
    std::optional<BlockPos> blockpos;
    for (const auto& entry : structureMap) {
        StructureStart* structurestart = entry.second.get();
        if (structurestart->isSizeableStructure()) {
            const StructureComponent* structurecomponent = structurestart->getComponents().at(0).get();
            const BlockPos blockpos1 = structurecomponent->getBoundingBoxCenter();
            const double d1 = blockpos1.distanceSq(pos);
            if (d1 < d0) {
                d0 = d1;
                blockpos = blockpos1;
            }
        }
    }
    if (blockpos)
        return blockpos;
    const auto list = getCoordList();
    if (!list)
        return std::nullopt;
    std::optional<BlockPos> blockpos2;
    for (const BlockPos& blockpos3 : *list) {
        const double d2 = blockpos3.distanceSq(pos);
        if (d2 < d0) {
            d0 = d2;
            blockpos2 = blockpos3;
        }
    }
    return blockpos2;
}

void MapGenStructure::initializeStructureData(World* worldIn)
{
    if (structureData)
        return;
    MapStorage* storage = worldIn ? worldIn->getMapStorage() : nullptr;
    // Java would throw without the storage (a client world); the data then stays in memory only.
    if (storage)
        structureData = storage->loadData<MapGenStructureData>(getStructureName());
    if (!structureData) {
        structureData = std::make_shared<MapGenStructureData>(getStructureName());
        if (storage)
            storage->setData(getStructureName(), structureData);
        return;
    }
    const NBTTagCompound& nbttagcompound = structureData->getTagCompound();
    for (const std::string& s : nbttagcompound.getKeySet()) {
        const auto nbtbase = nbttagcompound.getTag(s);
        if (nbtbase->getId() == 10) {
            const auto& nbttagcompound1 = static_cast<const NBTTagCompound&>(*nbtbase);
            if (nbttagcompound1.hasKey("ChunkX") && nbttagcompound1.hasKey("ChunkZ")) {
                const int i = nbttagcompound1.getInteger("ChunkX");
                const int j = nbttagcompound1.getInteger("ChunkZ");
                if (auto structurestart = MapGenStructureIO::getStructureStart(nbttagcompound1, worldIn))
                    structureMap.put(ChunkCoordIntPair::chunkXZ2Int(i, j), structurestart);
            }
        }
    }
}

void MapGenStructure::setStructureStart(int chunkX, int chunkZ, const StructureStart& start)
{
    structureData->writeInstance(start.writeStructureComponentsToNBT(chunkX, chunkZ), chunkX, chunkZ);
    structureData->markDirty();
}

} // namespace mc
