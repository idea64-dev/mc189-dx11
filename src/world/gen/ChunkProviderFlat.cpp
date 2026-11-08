#include "world/gen/ChunkProviderFlat.h"

#include "block/AllBlocks.h"
#include "core/Java.h"
#include "init/Blocks.h"
#include "world/ChunkCoordIntPair.h"
#include "world/World.h"
#include "world/biome/WorldChunkManager.h"
#include "world/chunk/Chunk.h"
#include "world/gen/feature/WorldGenerators.h"
#include "world/gen/structure/MapGenMineshaft.h"
#include "world/gen/structure/MapGenScatteredFeature.h"
#include "world/gen/structure/MapGenStronghold.h"
#include "world/gen/structure/MapGenVillage.h"
#include "world/gen/structure/StructureOceanMonument.h"

namespace mc {

ChunkProviderFlat::ChunkProviderFlat(World* worldIn, int64_t seed, bool generateStructures, const std::string& flatGeneratorSettings)
    : worldObj(worldIn)
    , random(seed)
    , flatWorldGenInfo(FlatGeneratorInfo::createFlatGeneratorFromString(flatGeneratorSettings))
{
    auto& map = flatWorldGenInfo.getWorldFeatures();
    if (generateStructures) {
        if (FlatGeneratorInfo::Options* map1 = map.find("village")) {
            if (!map1->contains("size"))
                map1->put("size", "1");
            structureGenerators.push_back(std::make_unique<MapGenVillage>(*map1));
        }
        if (const FlatGeneratorInfo::Options* options = map.find("biome_1"))
            structureGenerators.push_back(std::make_unique<MapGenScatteredFeature>(*options));
        if (const FlatGeneratorInfo::Options* options = map.find("mineshaft"))
            structureGenerators.push_back(std::make_unique<MapGenMineshaft>(*options));
        if (const FlatGeneratorInfo::Options* options = map.find("stronghold"))
            structureGenerators.push_back(std::make_unique<MapGenStronghold>(*options));
        if (const FlatGeneratorInfo::Options* options = map.find("oceanmonument"))
            structureGenerators.push_back(std::make_unique<StructureOceanMonument>(*options));
    }
    if (map.contains("lake"))
        waterLakeGenerator = std::make_unique<WorldGenLakes>(Blocks::water);
    if (map.contains("lava_lake"))
        lavaLakeGenerator = std::make_unique<WorldGenLakes>(Blocks::lava);
    hasDungeons = map.contains("dungeon");
    int j = 0;
    int k = 0;
    bool flag = true;
    for (const FlatLayerInfo& flatlayerinfo : flatWorldGenInfo.getFlatLayers()) {
        for (int i = flatlayerinfo.getMinY(); i < flatlayerinfo.getMinY() + flatlayerinfo.getLayerCount(); ++i) {
            const IBlockState* iblockstate = flatlayerinfo.getLayerMaterial();
            if (iblockstate->getBlock() != Blocks::air) {
                flag = false;
                cachedBlockIDs.at(static_cast<size_t>(i)) = iblockstate;
            }
        }
        if (flatlayerinfo.getLayerMaterial()->getBlock() == Blocks::air) {
            k += flatlayerinfo.getLayerCount();
        } else {
            j += flatlayerinfo.getLayerCount() + k;
            k = 0;
        }
    }
    worldIn->setSeaLevel(j);
    hasDecoration = flag ? false : map.contains("decoration");
}

ChunkProviderFlat::~ChunkProviderFlat() = default;

std::unique_ptr<Chunk> ChunkProviderFlat::generateChunk(int x, int z)
{
    auto chunkprimer = std::make_unique<ChunkPrimer>();
    for (size_t i = 0; i < cachedBlockIDs.size(); ++i) {
        const IBlockState* iblockstate = cachedBlockIDs[i];
        if (iblockstate) {
            for (int j = 0; j < 16; ++j) {
                for (int k = 0; k < 16; ++k)
                    chunkprimer->setBlockState(j, static_cast<int>(i), k, iblockstate);
            }
        }
    }
    for (const auto& mapgenbase : structureGenerators)
        mapgenbase->generate(this, worldObj, x, z, chunkprimer.get());
    auto chunk = std::make_unique<Chunk>(worldObj, *chunkprimer, x, z);
    std::vector<BiomeGenBase*> abiomegenbase;
    worldObj->getWorldChunkManager()->loadBlockGeneratorData(abiomegenbase, x * 16, z * 16, 16, 16);
    std::array<uint8_t, 256>& abyte = chunk->getBiomeArray();
    for (size_t l = 0; l < abyte.size(); ++l)
        abyte[l] = static_cast<uint8_t>(abiomegenbase[l]->biomeID);
    chunk->generateSkylightMap();
    return chunk;
}

Chunk* ChunkProviderFlat::provideChunk(int x, int z)
{
    lastProvided = generateChunk(x, z);
    return lastProvided.get();
}

void ChunkProviderFlat::populate(IChunkProvider*, int x, int z)
{
    const int i = x * 16;
    const int j = z * 16;
    const BlockPos blockpos(i, 0, j);
    BiomeGenBase* biomegenbase = worldObj->getBiomeGenForCoords(BlockPos(i + 16, 0, j + 16));
    bool flag = false;
    random.setSeed(worldObj->getSeed());
    const int64_t k = java::add(java::mul(random.nextLong() / 2LL, static_cast<int64_t>(2)), static_cast<int64_t>(1));
    const int64_t l = java::add(java::mul(random.nextLong() / 2LL, static_cast<int64_t>(2)), static_cast<int64_t>(1));
    random.setSeed(java::add(java::mul(static_cast<int64_t>(x), k), java::mul(static_cast<int64_t>(z), l)) ^ worldObj->getSeed());
    const ChunkCoordIntPair chunkcoordintpair(x, z);
    for (const auto& mapgenstructure : structureGenerators) {
        const bool flag1 = mapgenstructure->generateStructure(worldObj, random, chunkcoordintpair);
        if (dynamic_cast<MapGenVillage*>(mapgenstructure.get()))
            flag |= flag1;
    }
    // blockpos.add(random.nextInt(16) + 8, random.nextInt(y), random.nextInt(16) + 8), in Java's order.
    const auto randomPos = [this, &blockpos](auto&& yDraw) {
        const int dx = random.nextInt(16) + 8;
        const int dy = yDraw();
        const int dz = random.nextInt(16) + 8;
        return blockpos.add(dx, dy, dz);
    };
    if (waterLakeGenerator && !flag && random.nextInt(4) == 0)
        waterLakeGenerator->generate(worldObj, random, randomPos([this] { return random.nextInt(256); }));
    if (lavaLakeGenerator && !flag && random.nextInt(8) == 0) {
        const BlockPos blockpos1 = randomPos([this] {
            const int a = random.nextInt(248) + 8;
            return random.nextInt(a);
        });
        if (blockpos1.getY() < worldObj->getSeaLevel() || random.nextInt(10) == 0)
            lavaLakeGenerator->generate(worldObj, random, blockpos1);
    }
    if (hasDungeons) {
        for (int i1 = 0; i1 < 8; ++i1)
            WorldGenDungeons().generate(worldObj, random, randomPos([this] { return random.nextInt(256); }));
    }
    if (hasDecoration)
        biomegenbase->decorate(worldObj, random, blockpos);
}

std::vector<BiomeGenBase::SpawnListEntry> ChunkProviderFlat::getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos)
{
    return worldObj->getBiomeGenForCoords(pos)->getSpawnableList(creatureType);
}

std::optional<BlockPos> ChunkProviderFlat::getStrongholdGen(World* worldIn, const std::string& structureName, const BlockPos& position)
{
    if (structureName == "Stronghold") {
        for (const auto& mapgenstructure : structureGenerators) {
            if (dynamic_cast<MapGenStronghold*>(mapgenstructure.get()))
                return mapgenstructure->getClosestStrongholdPos(worldIn, position);
        }
    }
    return std::nullopt;
}

void ChunkProviderFlat::recreateStructures(Chunk*, int x, int z)
{
    for (const auto& mapgenstructure : structureGenerators)
        mapgenstructure->generate(this, worldObj, x, z, nullptr);
}

} // namespace mc
