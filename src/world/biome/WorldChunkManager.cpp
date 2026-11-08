#include "world/biome/WorldChunkManager.h"

#include "core/JavaTime.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/biome/BiomeGenBase.h"
#include "world/gen/layer/GenLayer.h"
#include "world/storage/WorldInfo.h"

#include <algorithm>

namespace mc {

namespace {

template <class T>
void ensureSize(std::vector<T>& list, int size)
{
    if (list.size() < static_cast<size_t>(size))
        list.assign(static_cast<size_t>(size), T {});
}

bool contains(const std::vector<BiomeGenBase*>& list, const BiomeGenBase* biome)
{
    return std::find(list.begin(), list.end(), biome) != list.end();
}

} // namespace

// --- BiomeCache ---

BiomeCache::Block::Block(WorldChunkManager& chunkManager, int x, int z)
    : rainfallValues(256)
    , biomes(256)
    , xPosition(x)
    , zPosition(z)
{
    chunkManager.getRainfall(rainfallValues, x << 4, z << 4, 16, 16);
    chunkManager.getBiomeGenAt(biomes, x << 4, z << 4, 16, 16, false);
}

BiomeCache::Block& BiomeCache::getBiomeCacheBlock(int x, int z)
{
    x = x >> 4;
    z = z >> 4;
    const int64_t i = (static_cast<int64_t>(x) & 4294967295LL) | (static_cast<int64_t>(z) & 4294967295LL) << 32;
    auto it = cacheMap.find(i);
    if (it == cacheMap.end()) {
        auto block = std::make_unique<Block>(chunkManager, x, z);
        cache.push_back(block.get());
        it = cacheMap.emplace(i, std::move(block)).first;
    }
    it->second->lastAccessTime = java::currentTimeMillis();
    return *it->second;
}

BiomeGenBase* BiomeCache::func_180284_a(int x, int z, BiomeGenBase* defaultBiome)
{
    BiomeGenBase* biomegenbase = getBiomeCacheBlock(x, z).getBiomeGenAt(x, z);
    return biomegenbase ? biomegenbase : defaultBiome;
}

void BiomeCache::cleanupCache()
{
    const int64_t i = java::currentTimeMillis();
    const int64_t j = i - lastCleanupTime;
    if (j > 7500LL || j < 0LL) {
        lastCleanupTime = i;
        for (size_t k = 0; k < cache.size(); ++k) {
            Block* block = cache[k];
            const int64_t l = i - block->lastAccessTime;
            if (l > 30000LL || l < 0LL) {
                cache.erase(cache.begin() + static_cast<std::ptrdiff_t>(k--));
                const int64_t i1 = (static_cast<int64_t>(block->xPosition) & 4294967295LL) | (static_cast<int64_t>(block->zPosition) & 4294967295LL) << 32;
                cacheMap.erase(i1);
            }
        }
    }
}

// --- WorldChunkManager ---

WorldChunkManager::WorldChunkManager()
    : biomeCache(*this)
    , biomesToSpawnIn { BiomeGenBase::forest, BiomeGenBase::plains, BiomeGenBase::taiga, BiomeGenBase::taigaHills, BiomeGenBase::forestHills,
        BiomeGenBase::jungle, BiomeGenBase::jungleHills }
{
}

WorldChunkManager::WorldChunkManager(int64_t seed, const WorldType* worldTypeIn, const std::string& options)
    : WorldChunkManager()
{
    generatorOptions = options;
    const auto agenlayer = GenLayer::initializeAllBiomeGenerators(seed, worldTypeIn, options);
    genBiomes = agenlayer[0];
    biomeIndexLayer = agenlayer[1];
}

WorldChunkManager::WorldChunkManager(World* worldIn)
    : WorldChunkManager(worldIn->getSeed(), worldIn->getWorldInfo().getTerrainType(), worldIn->getWorldInfo().getGeneratorOptions())
{
}

WorldChunkManager::~WorldChunkManager() = default;

BiomeGenBase* WorldChunkManager::getBiomeGenerator(const BlockPos& pos, BiomeGenBase* biomeGenBaseIn)
{
    return biomeCache.func_180284_a(pos.getX(), pos.getZ(), biomeGenBaseIn);
}

std::vector<float>& WorldChunkManager::getRainfall(std::vector<float>& listToReuse, int x, int z, int width, int length)
{
    ensureSize(listToReuse, width * length);
    const std::vector<int> aint = biomeIndexLayer->getInts(x, z, width, length);
    for (int i = 0; i < width * length; ++i) {
        float f = static_cast<float>(BiomeGenBase::getBiomeFromBiomeList(aint[static_cast<size_t>(i)], BiomeGenBase::field_180279_ad)->getIntRainfall()) / 65536.0f;
        if (f > 1.0f)
            f = 1.0f;
        listToReuse[static_cast<size_t>(i)] = f;
    }
    return listToReuse;
}

std::vector<BiomeGenBase*>& WorldChunkManager::getBiomesForGeneration(std::vector<BiomeGenBase*>& biomes, int x, int z, int width, int height)
{
    ensureSize(biomes, width * height);
    const std::vector<int> aint = genBiomes->getInts(x, z, width, height);
    for (int i = 0; i < width * height; ++i)
        biomes[static_cast<size_t>(i)] = BiomeGenBase::getBiomeFromBiomeList(aint[static_cast<size_t>(i)], BiomeGenBase::field_180279_ad);
    return biomes;
}

std::vector<BiomeGenBase*>& WorldChunkManager::loadBlockGeneratorData(std::vector<BiomeGenBase*>& oldBiomeList, int x, int z, int width, int depth)
{
    return getBiomeGenAt(oldBiomeList, x, z, width, depth, true);
}

std::vector<BiomeGenBase*>& WorldChunkManager::getBiomeGenAt(std::vector<BiomeGenBase*>& listToReuse, int x, int z, int width, int length, bool cacheFlag)
{
    ensureSize(listToReuse, width * length);
    if (cacheFlag && width == 16 && length == 16 && (x & 15) == 0 && (z & 15) == 0) {
        const std::vector<BiomeGenBase*>& abiomegenbase = biomeCache.getCachedBiomes(x, z);
        std::copy(abiomegenbase.begin(), abiomegenbase.begin() + width * length, listToReuse.begin());
        return listToReuse;
    }
    const std::vector<int> aint = biomeIndexLayer->getInts(x, z, width, length);
    for (int i = 0; i < width * length; ++i)
        listToReuse[static_cast<size_t>(i)] = BiomeGenBase::getBiomeFromBiomeList(aint[static_cast<size_t>(i)], BiomeGenBase::field_180279_ad);
    return listToReuse;
}

bool WorldChunkManager::areBiomesViable(int x, int z, int radius, const std::vector<BiomeGenBase*>& allowed)
{
    const int i = (x - radius) >> 2;
    const int j = (z - radius) >> 2;
    const int k = (x + radius) >> 2;
    const int l = (z + radius) >> 2;
    const int i1 = k - i + 1;
    const int j1 = l - j + 1;
    const std::vector<int> aint = genBiomes->getInts(i, j, i1, j1);
    for (int k1 = 0; k1 < i1 * j1; ++k1) {
        if (!contains(allowed, BiomeGenBase::getBiome(aint[static_cast<size_t>(k1)])))
            return false;
    }
    return true;
}

std::optional<BlockPos> WorldChunkManager::findBiomePosition(int x, int z, int range, const std::vector<BiomeGenBase*>& biomes, Random& random)
{
    const int i = (x - range) >> 2;
    const int j = (z - range) >> 2;
    const int k = (x + range) >> 2;
    const int l = (z + range) >> 2;
    const int i1 = k - i + 1;
    const int j1 = l - j + 1;
    const std::vector<int> aint = genBiomes->getInts(i, j, i1, j1);
    std::optional<BlockPos> blockpos;
    int k1 = 0;
    for (int l1 = 0; l1 < i1 * j1; ++l1) {
        const int i2 = (i + l1 % i1) << 2;
        const int j2 = (j + l1 / i1) << 2;
        const BiomeGenBase* biomegenbase = BiomeGenBase::getBiome(aint[static_cast<size_t>(l1)]);
        if (contains(biomes, biomegenbase) && (!blockpos || random.nextInt(k1 + 1) == 0)) {
            blockpos = BlockPos(i2, 0, j2);
            ++k1;
        }
    }
    return blockpos;
}

// --- WorldChunkManagerHell ---

std::vector<BiomeGenBase*>& WorldChunkManagerHell::getBiomesForGeneration(std::vector<BiomeGenBase*>& biomes, int, int, int width, int height)
{
    ensureSize(biomes, width * height);
    std::fill(biomes.begin(), biomes.begin() + width * height, biomeGenerator);
    return biomes;
}

std::vector<float>& WorldChunkManagerHell::getRainfall(std::vector<float>& listToReuse, int, int, int width, int length)
{
    ensureSize(listToReuse, width * length);
    std::fill(listToReuse.begin(), listToReuse.begin() + width * length, rainfall);
    return listToReuse;
}

std::vector<BiomeGenBase*>& WorldChunkManagerHell::loadBlockGeneratorData(std::vector<BiomeGenBase*>& oldBiomeList, int, int, int width, int depth)
{
    ensureSize(oldBiomeList, width * depth);
    std::fill(oldBiomeList.begin(), oldBiomeList.begin() + width * depth, biomeGenerator);
    return oldBiomeList;
}

std::vector<BiomeGenBase*>& WorldChunkManagerHell::getBiomeGenAt(std::vector<BiomeGenBase*>& listToReuse, int x, int z, int width, int length, bool)
{
    return loadBlockGeneratorData(listToReuse, x, z, width, length);
}

std::optional<BlockPos> WorldChunkManagerHell::findBiomePosition(int x, int z, int range, const std::vector<BiomeGenBase*>& biomes, Random& random)
{
    if (!contains(biomes, biomeGenerator))
        return std::nullopt;
    const int dx = random.nextInt(range * 2 + 1);
    const int dz = random.nextInt(range * 2 + 1);
    return BlockPos(x - range + dx, 0, z - range + dz);
}

bool WorldChunkManagerHell::areBiomesViable(int, int, int, const std::vector<BiomeGenBase*>& allowed)
{
    return contains(allowed, biomeGenerator);
}

} // namespace mc
