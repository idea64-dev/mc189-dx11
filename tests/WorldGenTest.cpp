// World generation against the original: tests/reference/worldgen.txt is written by the MCP code
// (tools/reference/net/minecraft/world/WorldGenDump.java). For each case (seed, world type, generator options,
// dimension) a server-less world with a ChunkProviderServer-like cache loads the chunks around the origin in the
// order of MinecraftServer.initialWorldChunkLoad; every populated chunk's hash must match the dump.

#include "core/Java.h"
#include "init/Bootstrap.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/WorldSettings.h"
#include "world/WorldType.h"
#include "world/chunk/Chunk.h"
#include "world/chunk/IChunkProvider.h"
#include "world/storage/MapStorage.h"
#include "world/storage/WorldInfo.h"

#include "RefWorld.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace mc;

namespace {

constexpr int64_t kWorldRandSeed = 0x5DEECE66DLL;

const char* const kCustomized = "{\"coordinateScale\":738.41864, \"heightScale\":157.69133, \"upperLimitScale\":801.4267, \"lowerLimitScale\":1254.1643, "
                                "\"depthNoiseScaleX\":374.93652, \"depthNoiseScaleZ\":288.65228, \"depthNoiseScaleExponent\":1.2092624, "
                                "\"mainNoiseScaleX\":1355.9908, \"mainNoiseScaleY\":745.5343, \"mainNoiseScaleZ\":1183.464, \"baseSize\":1.8758626, "
                                "\"stretchY\":1.7137525, \"biomeDepthWeight\":1.7553768, \"biomeDepthOffset\":3.4701107, \"biomeScaleWeight\":1.0, "
                                "\"biomeScaleOffset\":2.535211, \"useCaves\":true, \"useDungeons\":true, \"dungeonChance\":8, \"useStrongholds\":true, "
                                "\"useVillages\":true, \"useMineShafts\":true, \"useTemples\":true, \"useRavines\":true, \"useWaterLakes\":true, "
                                "\"waterLakeChance\":4, \"useLavaLakes\":true, \"lavaLakeChance\":80, \"useLavaOceans\":false, \"seaLevel\":63 }";
const char* const kFlatCustom = "3;minecraft:bedrock,2*minecraft:dirt,minecraft:grass;1;village,decoration,lake";

struct Case {
    std::string name;
    int64_t seed;
    const WorldType* type;
    std::string options;
    int dimension;
};

// Keep in step with WorldGenDump.cases().
std::vector<Case> cases()
{
    return {
        { "default-0", 0, &WorldType::DEFAULT, "", 0 },
        { "default-12345", 12345, &WorldType::DEFAULT, "", 0 },
        { "default-big", -6017234389458431232LL, &WorldType::DEFAULT, "", 0 },
        { "default-text", static_cast<int64_t>(java::hashCode(std::string_view("MCP-919"))), &WorldType::DEFAULT, "", 0 },
        { "flat-default", 12345, &WorldType::FLAT, "", 0 },
        { "flat-custom", 12345, &WorldType::FLAT, kFlatCustom, 0 },
        { "large-biomes", 12345, &WorldType::LARGE_BIOMES, "", 0 },
        { "amplified", 12345, &WorldType::AMPLIFIED, "", 0 },
        { "customized", 12345, &WorldType::CUSTOMIZED, kCustomized, 0 },
        { "debug", 12345, &WorldType::DEBUG_WORLD, "", 0 },
        { "nether", 12345, &WorldType::DEFAULT, "", -1 },
        { "end", 12345, &WorldType::DEFAULT, "", 1 },
    };
}

class GenWorld;

// Java: ChunkProviderServer without a chunk loader -- missing chunks are generated, loaded and populated.
class GenChunkCache : public IChunkProvider {
public:
    explicit GenChunkCache(GenWorld& worldIn)
        : world(worldIn)
    {
    }

    Chunk* loadChunk(int x, int z);
    bool chunkExists(int x, int z) override { return chunks.contains(ChunkCoordIntPair::chunkXZ2Int(x, z)); }
    Chunk* provideChunk(int x, int z) override
    {
        const auto it = chunks.find(ChunkCoordIntPair::chunkXZ2Int(x, z));
        return it != chunks.end() ? it->second.get() : loadChunk(x, z);
    }
    Chunk* provideChunk(const BlockPos& pos) override { return provideChunk(pos.getX() >> 4, pos.getZ() >> 4); }
    void populate(IChunkProvider* p, int x, int z) override
    {
        Chunk* chunk = provideChunk(x, z);
        if (!chunk->isTerrainPopulated()) {
            chunk->func_150809_p();
            generator->populate(p, x, z);
            chunk->setChunkModified();
        }
    }
    bool populateChunk(IChunkProvider* p, Chunk* c, int x, int z) override
    {
        if (generator->populateChunk(p, c, x, z)) {
            provideChunk(x, z)->setChunkModified();
            return true;
        }
        return false;
    }
    bool saveChunks(bool, IProgressUpdate*) override { return true; }
    bool unloadQueuedChunks() override { return false; }
    bool canSave() override { return false; }
    std::string makeString() override { return "Gen"; }
    std::vector<BiomeGenBase::SpawnListEntry> getPossibleCreatures(EnumCreatureType t, const BlockPos& pos) override
    {
        return generator->getPossibleCreatures(t, pos);
    }
    std::optional<BlockPos> getStrongholdGen(World* w, const std::string& name, const BlockPos& pos) override
    {
        return generator->getStrongholdGen(w, name, pos);
    }
    int getLoadedChunkCount() override { return static_cast<int>(chunks.size()); }
    void recreateStructures(Chunk*, int, int) override { }
    void saveExtraData() override { }

    std::unique_ptr<IChunkProvider> generator;
    std::unordered_map<int64_t, std::unique_ptr<Chunk>> chunks;
    std::vector<Chunk*> loaded;

private:
    GenWorld& world;
};

class GenWorld : public World {
public:
    GenWorld(std::shared_ptr<WorldInfo> info, int dimension)
        : World(nullptr, std::move(info), WorldProvider::getProviderForDimension(dimension), profiler, false)
    {
        provider->registerWorld(this);
        mapStorage = std::make_shared<MapStorage>(nullptr);
        auto cache = std::make_unique<GenChunkCache>(*this);
        cache->generator = provider->createChunkGenerator();
        chunkProvider = std::move(cache);
        calculateInitialSkylight();
        rand.setSeed(kWorldRandSeed);
    }

    GenChunkCache& cache() { return static_cast<GenChunkCache&>(*chunkProvider); }
    // The Java dump discards spawned entities.
    bool spawnEntityInWorld(const std::shared_ptr<Entity>&) override { return false; }

protected:
    std::unique_ptr<IChunkProvider> createChunkProvider() override { return nullptr; }
    int getRenderDistanceChunks() override { return 8; }

private:
    static inline Profiler profiler;
};

Chunk* GenChunkCache::loadChunk(int x, int z)
{
    const int64_t key = ChunkCoordIntPair::chunkXZ2Int(x, z);
    if (const auto it = chunks.find(key); it != chunks.end())
        return it->second.get();
    std::unique_ptr<Chunk> generated = generator->generateChunk(x, z);
    REQUIRE(generated != nullptr);
    Chunk* chunk = generated.get();
    chunks.emplace(key, std::move(generated));
    loaded.push_back(chunk);
    chunk->onChunkLoad();
    chunk->populateChunk(this, this, x, z);
    return chunk;
}

// case name -> the dump lines of that case ("case ..." header first).
std::map<std::string, std::vector<std::string>> referenceCases(bool& found)
{
    std::map<std::string, std::vector<std::string>> result;
    std::ifstream file(std::filesystem::path(__FILE__).parent_path() / "reference" / "worldgen.txt");
    found = static_cast<bool>(file);
    std::string line;
    std::string current;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.starts_with("case ")) {
            std::istringstream in(line.substr(5));
            in >> current;
        }
        result[current].push_back(line);
    }
    return result;
}

std::vector<std::string> generate(const Case& c)
{
    WorldSettings settings(c.seed, GameType::SURVIVAL, true, false, c.type);
    settings.setWorldName(c.options);
    GenWorld world(std::make_shared<WorldInfo>(settings, "Gen"), c.dimension);

    for (int l1 = -64; l1 <= 64; l1 += 16) {
        for (int i2 = -64; i2 <= 64; i2 += 16)
            world.cache().loadChunk(l1 >> 4, i2 >> 4);
    }

    std::vector<Chunk*> populated;
    for (Chunk* chunk : world.cache().loaded) {
        if (chunk->isTerrainPopulated())
            populated.push_back(chunk);
    }
    std::sort(populated.begin(), populated.end(), [](const Chunk* a, const Chunk* b) {
        return a->xPosition != b->xPosition ? a->xPosition < b->xPosition : a->zPosition < b->zPosition;
    });

    std::vector<std::string> lines;
    lines.push_back("case " + c.name + " " + std::to_string(world.cache().loaded.size()) + " " + std::to_string(populated.size()));
    for (Chunk* chunk : populated)
        lines.push_back(std::to_string(chunk->xPosition) + " " + std::to_string(chunk->zPosition) + " " + std::to_string(test::hash(*chunk)));
    return lines;
}

} // namespace

TEST_CASE("Generated and populated chunks match the original")
{
    Bootstrap::registerAll();
    bool found = false;
    const auto expected = referenceCases(found);
    if (!found) {
        MESSAGE("tests/reference/worldgen.txt is missing; run tools/reference/generate_game.ps1");
        return;
    }

    for (const Case& c : cases()) {
        CAPTURE(c.name);
        const auto it = expected.find(c.name);
        REQUIRE(it != expected.end());
        const std::vector<std::string> actual = generate(c);
        const std::vector<std::string>& reference = it->second;
        CHECK(actual.front() == reference.front());

        int mismatches = 0;
        const size_t n = std::min(actual.size(), reference.size());
        for (size_t i = 1; i < n; ++i) {
            if (actual[i] != reference[i] && mismatches++ < 5)
                FAIL_CHECK("chunk mismatch: got '" << actual[i] << "', expected '" << reference[i] << "'");
        }
        if (mismatches > 5)
            FAIL_CHECK(mismatches << " chunks differ in total");
        CHECK(actual.size() == reference.size());
    }
}
