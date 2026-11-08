#include "world/gen/layer/GenLayer.h"

#include "core/Java.h"
#include "world/WorldType.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/Biomes.h"
#include "world/gen/ChunkProviderSettings.h"

#include <algorithm>
#include <optional>

namespace mc {

namespace {

constexpr int64_t kMul = 6364136223846793005LL;
constexpr int64_t kAdd = 1442695040888963407LL;

// x *= x * 6364136223846793005L + 1442695040888963407L; x += addend
int64_t scramble(int64_t x, int64_t addend)
{
    x = java::mul(x, java::add(java::mul(x, kMul), kAdd));
    return java::add(x, addend);
}

int id(const BiomeGenBase* biome)
{
    return biome->biomeID;
}

} // namespace

GenLayer::GenLayer(int64_t seed)
    : baseSeed(seed)
{
    baseSeed = scramble(baseSeed, seed);
    baseSeed = scramble(baseSeed, seed);
    baseSeed = scramble(baseSeed, seed);
}

void GenLayer::initWorldGenSeed(int64_t seed)
{
    worldGenSeed = seed;
    if (parent)
        parent->initWorldGenSeed(seed);
    worldGenSeed = scramble(worldGenSeed, baseSeed);
    worldGenSeed = scramble(worldGenSeed, baseSeed);
    worldGenSeed = scramble(worldGenSeed, baseSeed);
}

void GenLayer::initChunkSeed(int64_t x, int64_t z)
{
    chunkSeed = worldGenSeed;
    chunkSeed = scramble(chunkSeed, x);
    chunkSeed = scramble(chunkSeed, z);
    chunkSeed = scramble(chunkSeed, x);
    chunkSeed = scramble(chunkSeed, z);
}

int GenLayer::nextInt(int bound)
{
    int i = static_cast<int>((chunkSeed >> 24) % static_cast<int64_t>(bound));
    if (i < 0)
        i += bound;
    chunkSeed = scramble(chunkSeed, worldGenSeed);
    return i;
}

bool GenLayer::biomesEqualOrMesaPlateau(int biomeIDA, int biomeIDB)
{
    if (biomeIDA == biomeIDB)
        return true;
    if (biomeIDA != id(BiomeGenBase::mesaPlateau_F) && biomeIDA != id(BiomeGenBase::mesaPlateau)) {
        const BiomeGenBase* biomegenbase = BiomeGenBase::getBiome(biomeIDA);
        const BiomeGenBase* biomegenbase1 = BiomeGenBase::getBiome(biomeIDB);
        return biomegenbase && biomegenbase1 ? biomegenbase->isEqualTo(biomegenbase1) : false;
    }
    return biomeIDB == id(BiomeGenBase::mesaPlateau_F) || biomeIDB == id(BiomeGenBase::mesaPlateau);
}

bool GenLayer::isBiomeOceanic(int value)
{
    return value == id(BiomeGenBase::ocean) || value == id(BiomeGenBase::deepOcean) || value == id(BiomeGenBase::frozenOcean);
}

int GenLayer::selectRandom4(int a, int b, int c, int d)
{
    const int values[4] { a, b, c, d };
    return values[nextInt(4)];
}

int GenLayer::selectModeOrRandom(int a, int b, int c, int d)
{
    return b == c && c == d ? b
        : (a == b && a == c ? a
        : (a == b && a == d ? a
        : (a == c && a == d ? a
        : (a == b && c != d ? a
        : (a == c && b != d ? a
        : (a == d && b != c ? a
        : (b == c && a != d ? b
        : (b == d && a != c ? b
        : (c == d && a != b ? c
        : selectRandom4(a, b, c, d))))))))));
}

namespace {

class GenLayerIsland : public GenLayer {
public:
    using GenLayer::GenLayer;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        std::vector<int> aint(static_cast<size_t>(areaWidth * areaHeight));
        for (int i = 0; i < areaHeight; ++i) {
            for (int j = 0; j < areaWidth; ++j) {
                initChunkSeed(areaX + j, areaY + i);
                aint[static_cast<size_t>(j + i * areaWidth)] = nextInt(10) == 0 ? 1 : 0;
            }
        }
        if (areaX > -areaWidth && areaX <= 0 && areaY > -areaHeight && areaY <= 0)
            aint[static_cast<size_t>(-areaX + -areaY * areaWidth)] = 1;
        return aint;
    }
};

class GenLayerZoom : public GenLayer {
public:
    GenLayerZoom(int64_t seed, std::shared_ptr<GenLayer> parentIn)
        : GenLayer(seed)
    {
        parent = std::move(parentIn);
    }

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const int i = areaX >> 1;
        const int j = areaY >> 1;
        const int k = (areaWidth >> 1) + 2;
        const int l = (areaHeight >> 1) + 2;
        const std::vector<int> aint = parent->getInts(i, j, k, l);
        const int i1 = (k - 1) << 1;
        const int j1 = (l - 1) << 1;
        std::vector<int> aint1(static_cast<size_t>(i1 * j1));
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int k1 = 0; k1 < l - 1; ++k1) {
            int l1 = (k1 << 1) * i1;
            int i2 = 0;
            int j2 = at(i2 + 0 + (k1 + 0) * k);
            for (int k2 = at(i2 + 0 + (k1 + 1) * k); i2 < k - 1; ++i2) {
                initChunkSeed(static_cast<int64_t>((i2 + i) << 1), static_cast<int64_t>((k1 + j) << 1));
                const int l2 = at(i2 + 1 + (k1 + 0) * k);
                const int i3 = at(i2 + 1 + (k1 + 1) * k);
                aint1[static_cast<size_t>(l1)] = j2;
                aint1[static_cast<size_t>(l1++ + i1)] = selectRandom2(j2, k2);
                aint1[static_cast<size_t>(l1)] = selectRandom2(j2, l2);
                aint1[static_cast<size_t>(l1++ + i1)] = selectModeOrRandom(j2, l2, k2, i3);
                j2 = l2;
                k2 = i3;
            }
        }
        std::vector<int> aint2(static_cast<size_t>(areaWidth * areaHeight));
        for (int j3 = 0; j3 < areaHeight; ++j3) {
            const auto src = aint1.begin() + ((j3 + (areaY & 1)) * i1 + (areaX & 1));
            std::copy(src, src + areaWidth, aint2.begin() + j3 * areaWidth);
        }
        return aint2;
    }

    static std::shared_ptr<GenLayer> magnify(int64_t seed, std::shared_ptr<GenLayer> layer, int count)
    {
        for (int i = 0; i < count; ++i)
            layer = std::make_shared<GenLayerZoom>(seed + i, layer);
        return layer;
    }
};

class GenLayerFuzzyZoom : public GenLayerZoom {
public:
    using GenLayerZoom::GenLayerZoom;

protected:
    int selectModeOrRandom(int a, int b, int c, int d) override { return selectRandom4(a, b, c, d); }
};

// The layers reading a 1-cell border around the area from their parent.
class GenLayerBordered : public GenLayer {
public:
    GenLayerBordered(int64_t seed, std::shared_ptr<GenLayer> parentIn)
        : GenLayer(seed)
    {
        parent = std::move(parentIn);
    }
};

class GenLayerAddIsland : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const int k = areaWidth + 2;
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, k, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int i1 = 0; i1 < areaHeight; ++i1) {
            for (int j1 = 0; j1 < areaWidth; ++j1) {
                const int k1 = at(j1 + 0 + (i1 + 0) * k);
                const int l1 = at(j1 + 2 + (i1 + 0) * k);
                const int i2 = at(j1 + 0 + (i1 + 2) * k);
                const int j2 = at(j1 + 2 + (i1 + 2) * k);
                const int k2 = at(j1 + 1 + (i1 + 1) * k);
                int& out = aint1[static_cast<size_t>(j1 + i1 * areaWidth)];
                initChunkSeed(j1 + areaX, i1 + areaY);
                if (k2 != 0 || (k1 == 0 && l1 == 0 && i2 == 0 && j2 == 0)) {
                    if (k2 > 0 && (k1 == 0 || l1 == 0 || i2 == 0 || j2 == 0)) {
                        if (nextInt(5) == 0)
                            out = k2 == 4 ? 4 : 0;
                        else
                            out = k2;
                    } else {
                        out = k2;
                    }
                } else {
                    int l2 = 1;
                    int i3 = 1;
                    if (k1 != 0 && nextInt(l2++) == 0)
                        i3 = k1;
                    if (l1 != 0 && nextInt(l2++) == 0)
                        i3 = l1;
                    if (i2 != 0 && nextInt(l2++) == 0)
                        i3 = i2;
                    if (j2 != 0 && nextInt(l2++) == 0)
                        i3 = j2;
                    if (nextInt(3) == 0)
                        out = i3;
                    else if (i3 == 4)
                        out = 4;
                    else
                        out = 0;
                }
            }
        }
        return aint1;
    }
};

class GenLayerRemoveTooMuchOcean : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const int k = areaWidth + 2;
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, k, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int i1 = 0; i1 < areaHeight; ++i1) {
            for (int j1 = 0; j1 < areaWidth; ++j1) {
                const int k1 = at(j1 + 1 + (i1 + 1 - 1) * k);
                const int l1 = at(j1 + 1 + 1 + (i1 + 1) * k);
                const int i2 = at(j1 + 1 - 1 + (i1 + 1) * k);
                const int j2 = at(j1 + 1 + (i1 + 1 + 1) * k);
                const int k2 = at(j1 + 1 + (i1 + 1) * k);
                aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = k2;
                initChunkSeed(j1 + areaX, i1 + areaY);
                if (k2 == 0 && k1 == 0 && l1 == 0 && i2 == 0 && j2 == 0 && nextInt(2) == 0)
                    aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = 1;
            }
        }
        return aint1;
    }
};

class GenLayerAddSnow : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const int k = areaWidth + 2;
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, k, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        for (int i1 = 0; i1 < areaHeight; ++i1) {
            for (int j1 = 0; j1 < areaWidth; ++j1) {
                const int k1 = aint[static_cast<size_t>(j1 + 1 + (i1 + 1) * k)];
                initChunkSeed(j1 + areaX, i1 + areaY);
                if (k1 == 0) {
                    aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = 0;
                } else {
                    int l1 = nextInt(6);
                    if (l1 == 0)
                        l1 = 4;
                    else if (l1 <= 1)
                        l1 = 3;
                    else
                        l1 = 1;
                    aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = l1;
                }
            }
        }
        return aint1;
    }
};

class GenLayerEdge : public GenLayer {
public:
    enum class Mode { COOL_WARM, HEAT_ICE, SPECIAL };

    GenLayerEdge(int64_t seed, std::shared_ptr<GenLayer> parentIn, Mode mode)
        : GenLayer(seed)
        , field_151627_c(mode)
    {
        parent = std::move(parentIn);
    }

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        switch (field_151627_c) {
        case Mode::HEAT_ICE:
            return getIntsHeatIce(areaX, areaY, areaWidth, areaHeight);
        case Mode::SPECIAL:
            return getIntsSpecial(areaX, areaY, areaWidth, areaHeight);
        case Mode::COOL_WARM:
        default:
            return getIntsCoolWarm(areaX, areaY, areaWidth, areaHeight);
        }
    }

private:
    std::vector<int> getIntsCoolWarm(int areaX, int areaY, int areaWidth, int areaHeight)
    {
        const int k = 1 + areaWidth + 1;
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, k, 1 + areaHeight + 1);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int i1 = 0; i1 < areaHeight; ++i1) {
            for (int j1 = 0; j1 < areaWidth; ++j1) {
                initChunkSeed(j1 + areaX, i1 + areaY);
                int k1 = at(j1 + 1 + (i1 + 1) * k);
                if (k1 == 1) {
                    const int l1 = at(j1 + 1 + (i1 + 1 - 1) * k);
                    const int i2 = at(j1 + 1 + 1 + (i1 + 1) * k);
                    const int j2 = at(j1 + 1 - 1 + (i1 + 1) * k);
                    const int k2 = at(j1 + 1 + (i1 + 1 + 1) * k);
                    const bool flag = l1 == 3 || i2 == 3 || j2 == 3 || k2 == 3;
                    const bool flag1 = l1 == 4 || i2 == 4 || j2 == 4 || k2 == 4;
                    if (flag || flag1)
                        k1 = 2;
                }
                aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = k1;
            }
        }
        return aint1;
    }

    std::vector<int> getIntsHeatIce(int areaX, int areaY, int areaWidth, int areaHeight)
    {
        const int k = 1 + areaWidth + 1;
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, k, 1 + areaHeight + 1);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int i1 = 0; i1 < areaHeight; ++i1) {
            for (int j1 = 0; j1 < areaWidth; ++j1) {
                int k1 = at(j1 + 1 + (i1 + 1) * k);
                if (k1 == 4) {
                    const int l1 = at(j1 + 1 + (i1 + 1 - 1) * k);
                    const int i2 = at(j1 + 1 + 1 + (i1 + 1) * k);
                    const int j2 = at(j1 + 1 - 1 + (i1 + 1) * k);
                    const int k2 = at(j1 + 1 + (i1 + 1 + 1) * k);
                    const bool flag = l1 == 2 || i2 == 2 || j2 == 2 || k2 == 2;
                    const bool flag1 = l1 == 1 || i2 == 1 || j2 == 1 || k2 == 1;
                    if (flag1 || flag)
                        k1 = 3;
                }
                aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = k1;
            }
        }
        return aint1;
    }

    std::vector<int> getIntsSpecial(int areaX, int areaY, int areaWidth, int areaHeight)
    {
        const std::vector<int> aint = parent->getInts(areaX, areaY, areaWidth, areaHeight);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        for (int i = 0; i < areaHeight; ++i) {
            for (int j = 0; j < areaWidth; ++j) {
                initChunkSeed(j + areaX, i + areaY);
                int k = aint[static_cast<size_t>(j + i * areaWidth)];
                if (k != 0 && nextInt(13) == 0)
                    k |= ((1 + nextInt(15)) << 8) & 3840;
                aint1[static_cast<size_t>(j + i * areaWidth)] = k;
            }
        }
        return aint1;
    }

    Mode field_151627_c;
};

class GenLayerAddMushroomIsland : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const int k = areaWidth + 2;
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, k, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int i1 = 0; i1 < areaHeight; ++i1) {
            for (int j1 = 0; j1 < areaWidth; ++j1) {
                const int k1 = at(j1 + 0 + (i1 + 0) * k);
                const int l1 = at(j1 + 2 + (i1 + 0) * k);
                const int i2 = at(j1 + 0 + (i1 + 2) * k);
                const int j2 = at(j1 + 2 + (i1 + 2) * k);
                const int k2 = at(j1 + 1 + (i1 + 1) * k);
                initChunkSeed(j1 + areaX, i1 + areaY);
                if (k2 == 0 && k1 == 0 && l1 == 0 && i2 == 0 && j2 == 0 && nextInt(100) == 0)
                    aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = id(BiomeGenBase::mushroomIsland);
                else
                    aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = k2;
            }
        }
        return aint1;
    }
};

class GenLayerDeepOcean : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const int k = areaWidth + 2;
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, k, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int i1 = 0; i1 < areaHeight; ++i1) {
            for (int j1 = 0; j1 < areaWidth; ++j1) {
                const int k1 = at(j1 + 1 + (i1 + 1 - 1) * k);
                const int l1 = at(j1 + 1 + 1 + (i1 + 1) * k);
                const int i2 = at(j1 + 1 - 1 + (i1 + 1) * k);
                const int j2 = at(j1 + 1 + (i1 + 1 + 1) * k);
                const int k2 = at(j1 + 1 + (i1 + 1) * k);
                int l2 = 0;
                if (k1 == 0)
                    ++l2;
                if (l1 == 0)
                    ++l2;
                if (i2 == 0)
                    ++l2;
                if (j2 == 0)
                    ++l2;
                if (k2 == 0 && l2 > 3)
                    aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = id(BiomeGenBase::deepOcean);
                else
                    aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = k2;
            }
        }
        return aint1;
    }
};

class GenLayerRiverInit : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const std::vector<int> aint = parent->getInts(areaX, areaY, areaWidth, areaHeight);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        for (int i = 0; i < areaHeight; ++i) {
            for (int j = 0; j < areaWidth; ++j) {
                initChunkSeed(j + areaX, i + areaY);
                const size_t index = static_cast<size_t>(j + i * areaWidth);
                aint1[index] = aint[index] > 0 ? nextInt(299999) + 2 : 0;
            }
        }
        return aint1;
    }
};

class GenLayerBiome : public GenLayer {
public:
    GenLayerBiome(int64_t seed, std::shared_ptr<GenLayer> parentIn, const WorldType* worldType, const std::string& options)
        : GenLayer(seed)
    {
        parent = std::move(parentIn);
        field_151623_c = { BiomeGenBase::desert, BiomeGenBase::desert, BiomeGenBase::desert, BiomeGenBase::savanna, BiomeGenBase::savanna,
            BiomeGenBase::plains };
        field_151621_d = { BiomeGenBase::forest, BiomeGenBase::roofedForest, BiomeGenBase::extremeHills, BiomeGenBase::plains,
            BiomeGenBase::birchForest, BiomeGenBase::swampland };
        field_151622_e = { BiomeGenBase::forest, BiomeGenBase::extremeHills, BiomeGenBase::taiga, BiomeGenBase::plains };
        field_151620_f = { BiomeGenBase::icePlains, BiomeGenBase::icePlains, BiomeGenBase::icePlains, BiomeGenBase::coldTaiga };
        if (worldType == &WorldType::DEFAULT_1_1) {
            field_151623_c = { BiomeGenBase::desert, BiomeGenBase::forest, BiomeGenBase::extremeHills, BiomeGenBase::swampland, BiomeGenBase::plains,
                BiomeGenBase::taiga };
        } else if (worldType == &WorldType::CUSTOMIZED) {
            field_175973_g = ChunkProviderSettings::Factory::jsonToFactory(options);
        }
    }

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const std::vector<int> aint = parent->getInts(areaX, areaY, areaWidth, areaHeight);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const auto pick = [this](const std::vector<BiomeGenBase*>& list) { return id(list[static_cast<size_t>(nextInt(static_cast<int>(list.size())))]); };
        for (int i = 0; i < areaHeight; ++i) {
            for (int j = 0; j < areaWidth; ++j) {
                initChunkSeed(j + areaX, i + areaY);
                int k = aint[static_cast<size_t>(j + i * areaWidth)];
                const int l = (k & 3840) >> 8;
                k = k & -3841;
                int& out = aint1[static_cast<size_t>(j + i * areaWidth)];
                if (field_175973_g && field_175973_g->fixedBiome >= 0)
                    out = field_175973_g->fixedBiome;
                else if (isBiomeOceanic(k))
                    out = k;
                else if (k == id(BiomeGenBase::mushroomIsland))
                    out = k;
                else if (k == 1) {
                    if (l > 0)
                        out = nextInt(3) == 0 ? id(BiomeGenBase::mesaPlateau) : id(BiomeGenBase::mesaPlateau_F);
                    else
                        out = pick(field_151623_c);
                } else if (k == 2) {
                    out = l > 0 ? id(BiomeGenBase::jungle) : pick(field_151621_d);
                } else if (k == 3) {
                    out = l > 0 ? id(BiomeGenBase::megaTaiga) : pick(field_151622_e);
                } else if (k == 4) {
                    out = pick(field_151620_f);
                } else {
                    out = id(BiomeGenBase::mushroomIsland);
                }
            }
        }
        return aint1;
    }

private:
    std::vector<BiomeGenBase*> field_151623_c;
    std::vector<BiomeGenBase*> field_151621_d;
    std::vector<BiomeGenBase*> field_151622_e;
    std::vector<BiomeGenBase*> field_151620_f;
    std::optional<ChunkProviderSettings::Factory> field_175973_g;
};

class GenLayerBiomeEdge : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, areaWidth + 2, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const int w = areaWidth + 2;
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int i = 0; i < areaHeight; ++i) {
            for (int j = 0; j < areaWidth; ++j) {
                initChunkSeed(j + areaX, i + areaY);
                const int k = at(j + 1 + (i + 1) * w);
                if (!replaceBiomeEdgeIfNecessary(aint, aint1, j, i, areaWidth, k, id(BiomeGenBase::extremeHills), id(BiomeGenBase::extremeHillsEdge))
                    && !replaceBiomeEdge(aint, aint1, j, i, areaWidth, k, id(BiomeGenBase::mesaPlateau_F), id(BiomeGenBase::mesa))
                    && !replaceBiomeEdge(aint, aint1, j, i, areaWidth, k, id(BiomeGenBase::mesaPlateau), id(BiomeGenBase::mesa))
                    && !replaceBiomeEdge(aint, aint1, j, i, areaWidth, k, id(BiomeGenBase::megaTaiga), id(BiomeGenBase::taiga))) {
                    int& out = aint1[static_cast<size_t>(j + i * areaWidth)];
                    const int n = at(j + 1 + (i + 1 - 1) * w);
                    const int e = at(j + 1 + 1 + (i + 1) * w);
                    const int wv = at(j + 1 - 1 + (i + 1) * w);
                    const int s = at(j + 1 + (i + 1 + 1) * w);
                    if (k == id(BiomeGenBase::desert)) {
                        const int ice = id(BiomeGenBase::icePlains);
                        out = n != ice && e != ice && wv != ice && s != ice ? k : id(BiomeGenBase::extremeHillsPlus);
                    } else if (k == id(BiomeGenBase::swampland)) {
                        const int desert = id(BiomeGenBase::desert);
                        const int coldTaiga = id(BiomeGenBase::coldTaiga);
                        const int ice = id(BiomeGenBase::icePlains);
                        if (n != desert && e != desert && wv != desert && s != desert && n != coldTaiga && e != coldTaiga && wv != coldTaiga
                            && s != coldTaiga && n != ice && e != ice && wv != ice && s != ice) {
                            const int jungle = id(BiomeGenBase::jungle);
                            out = n != jungle && s != jungle && e != jungle && wv != jungle ? k : id(BiomeGenBase::jungleEdge);
                        } else {
                            out = id(BiomeGenBase::plains);
                        }
                    } else {
                        out = k;
                    }
                }
            }
        }
        return aint1;
    }

private:
    bool replaceBiomeEdgeIfNecessary(const std::vector<int>& in, std::vector<int>& out, int x, int y, int width, int value, int biome, int edge)
    {
        if (!biomesEqualOrMesaPlateau(value, biome))
            return false;
        const int w = width + 2;
        const int i = in[static_cast<size_t>(x + 1 + (y + 1 - 1) * w)];
        const int j = in[static_cast<size_t>(x + 1 + 1 + (y + 1) * w)];
        const int k = in[static_cast<size_t>(x + 1 - 1 + (y + 1) * w)];
        const int l = in[static_cast<size_t>(x + 1 + (y + 1 + 1) * w)];
        if (canBiomesBeNeighbors(i, biome) && canBiomesBeNeighbors(j, biome) && canBiomesBeNeighbors(k, biome) && canBiomesBeNeighbors(l, biome))
            out[static_cast<size_t>(x + y * width)] = value;
        else
            out[static_cast<size_t>(x + y * width)] = edge;
        return true;
    }

    bool replaceBiomeEdge(const std::vector<int>& in, std::vector<int>& out, int x, int y, int width, int value, int biome, int edge)
    {
        if (value != biome)
            return false;
        const int w = width + 2;
        const int i = in[static_cast<size_t>(x + 1 + (y + 1 - 1) * w)];
        const int j = in[static_cast<size_t>(x + 1 + 1 + (y + 1) * w)];
        const int k = in[static_cast<size_t>(x + 1 - 1 + (y + 1) * w)];
        const int l = in[static_cast<size_t>(x + 1 + (y + 1 + 1) * w)];
        if (biomesEqualOrMesaPlateau(i, biome) && biomesEqualOrMesaPlateau(j, biome) && biomesEqualOrMesaPlateau(k, biome)
            && biomesEqualOrMesaPlateau(l, biome))
            out[static_cast<size_t>(x + y * width)] = value;
        else
            out[static_cast<size_t>(x + y * width)] = edge;
        return true;
    }

    bool canBiomesBeNeighbors(int a, int b)
    {
        if (biomesEqualOrMesaPlateau(a, b))
            return true;
        const BiomeGenBase* biomegenbase = BiomeGenBase::getBiome(a);
        const BiomeGenBase* biomegenbase1 = BiomeGenBase::getBiome(b);
        if (biomegenbase && biomegenbase1) {
            const auto t = biomegenbase->getTempCategory();
            const auto t1 = biomegenbase1->getTempCategory();
            return t == t1 || t == BiomeGenBase::TempCategory::MEDIUM || t1 == BiomeGenBase::TempCategory::MEDIUM;
        }
        return false;
    }
};

class GenLayerHills : public GenLayer {
public:
    GenLayerHills(int64_t seed, std::shared_ptr<GenLayer> parentIn, std::shared_ptr<GenLayer> riverLayer)
        : GenLayer(seed)
        , field_151628_d(std::move(riverLayer))
    {
        parent = std::move(parentIn);
    }

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, areaWidth + 2, areaHeight + 2);
        const std::vector<int> aint1 = field_151628_d->getInts(areaX - 1, areaY - 1, areaWidth + 2, areaHeight + 2);
        std::vector<int> aint2(static_cast<size_t>(areaWidth * areaHeight));
        const int w = areaWidth + 2;
        for (int i = 0; i < areaHeight; ++i) {
            for (int j = 0; j < areaWidth; ++j) {
                initChunkSeed(j + areaX, i + areaY);
                const int k = aint[static_cast<size_t>(j + 1 + (i + 1) * w)];
                const int l = aint1[static_cast<size_t>(j + 1 + (i + 1) * w)];
                const bool flag = (l - 2) % 29 == 0;
                int& out = aint2[static_cast<size_t>(j + i * areaWidth)];
                if (k != 0 && l >= 2 && (l - 2) % 29 == 1 && k < 128) {
                    out = BiomeGenBase::getBiome(k + 128) ? k + 128 : k;
                } else if (nextInt(3) != 0 && !flag) {
                    out = k;
                } else {
                    int i1 = k;
                    if (k == id(BiomeGenBase::desert))
                        i1 = id(BiomeGenBase::desertHills);
                    else if (k == id(BiomeGenBase::forest))
                        i1 = id(BiomeGenBase::forestHills);
                    else if (k == id(BiomeGenBase::birchForest))
                        i1 = id(BiomeGenBase::birchForestHills);
                    else if (k == id(BiomeGenBase::roofedForest))
                        i1 = id(BiomeGenBase::plains);
                    else if (k == id(BiomeGenBase::taiga))
                        i1 = id(BiomeGenBase::taigaHills);
                    else if (k == id(BiomeGenBase::megaTaiga))
                        i1 = id(BiomeGenBase::megaTaigaHills);
                    else if (k == id(BiomeGenBase::coldTaiga))
                        i1 = id(BiomeGenBase::coldTaigaHills);
                    else if (k == id(BiomeGenBase::plains))
                        i1 = nextInt(3) == 0 ? id(BiomeGenBase::forestHills) : id(BiomeGenBase::forest);
                    else if (k == id(BiomeGenBase::icePlains))
                        i1 = id(BiomeGenBase::iceMountains);
                    else if (k == id(BiomeGenBase::jungle))
                        i1 = id(BiomeGenBase::jungleHills);
                    else if (k == id(BiomeGenBase::ocean))
                        i1 = id(BiomeGenBase::deepOcean);
                    else if (k == id(BiomeGenBase::extremeHills))
                        i1 = id(BiomeGenBase::extremeHillsPlus);
                    else if (k == id(BiomeGenBase::savanna))
                        i1 = id(BiomeGenBase::savannaPlateau);
                    else if (biomesEqualOrMesaPlateau(k, id(BiomeGenBase::mesaPlateau_F)))
                        i1 = id(BiomeGenBase::mesa);
                    else if (k == id(BiomeGenBase::deepOcean) && nextInt(3) == 0)
                        i1 = nextInt(2) == 0 ? id(BiomeGenBase::plains) : id(BiomeGenBase::forest);
                    if (flag && i1 != k) {
                        if (BiomeGenBase::getBiome(i1 + 128))
                            i1 += 128;
                        else
                            i1 = k;
                    }
                    if (i1 == k) {
                        out = k;
                    } else {
                        const int k2 = aint[static_cast<size_t>(j + 1 + (i + 1 - 1) * w)];
                        const int k1 = aint[static_cast<size_t>(j + 1 + 1 + (i + 1) * w)];
                        const int l1 = aint[static_cast<size_t>(j + 1 - 1 + (i + 1) * w)];
                        const int i2 = aint[static_cast<size_t>(j + 1 + (i + 1 + 1) * w)];
                        int j2 = 0;
                        if (biomesEqualOrMesaPlateau(k2, k))
                            ++j2;
                        if (biomesEqualOrMesaPlateau(k1, k))
                            ++j2;
                        if (biomesEqualOrMesaPlateau(l1, k))
                            ++j2;
                        if (biomesEqualOrMesaPlateau(i2, k))
                            ++j2;
                        out = j2 >= 3 ? i1 : k;
                    }
                }
            }
        }
        return aint2;
    }

private:
    std::shared_ptr<GenLayer> field_151628_d;
};

class GenLayerRiver : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const int k = areaWidth + 2;
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, k, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const auto at = [&](int index) { return func_151630_c(aint[static_cast<size_t>(index)]); };
        for (int i1 = 0; i1 < areaHeight; ++i1) {
            for (int j1 = 0; j1 < areaWidth; ++j1) {
                const int k1 = at(j1 + 0 + (i1 + 1) * k);
                const int l1 = at(j1 + 2 + (i1 + 1) * k);
                const int i2 = at(j1 + 1 + (i1 + 0) * k);
                const int j2 = at(j1 + 1 + (i1 + 2) * k);
                const int k2 = at(j1 + 1 + (i1 + 1) * k);
                if (k2 == k1 && k2 == i2 && k2 == l1 && k2 == j2)
                    aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = -1;
                else
                    aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = id(BiomeGenBase::river);
            }
        }
        return aint1;
    }

private:
    static int func_151630_c(int value) { return value >= 2 ? 2 + (value & 1) : value; }
};

class GenLayerSmooth : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const int k = areaWidth + 2;
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, k, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int i1 = 0; i1 < areaHeight; ++i1) {
            for (int j1 = 0; j1 < areaWidth; ++j1) {
                const int k1 = at(j1 + 0 + (i1 + 1) * k);
                const int l1 = at(j1 + 2 + (i1 + 1) * k);
                const int i2 = at(j1 + 1 + (i1 + 0) * k);
                const int j2 = at(j1 + 1 + (i1 + 2) * k);
                int k2 = at(j1 + 1 + (i1 + 1) * k);
                if (k1 == l1 && i2 == j2) {
                    initChunkSeed(j1 + areaX, i1 + areaY);
                    k2 = nextInt(2) == 0 ? k1 : i2;
                } else {
                    if (k1 == l1)
                        k2 = k1;
                    if (i2 == j2)
                        k2 = i2;
                }
                aint1[static_cast<size_t>(j1 + i1 * areaWidth)] = k2;
            }
        }
        return aint1;
    }
};

class GenLayerRareBiome : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, areaWidth + 2, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        for (int i = 0; i < areaHeight; ++i) {
            for (int j = 0; j < areaWidth; ++j) {
                initChunkSeed(j + areaX, i + areaY);
                const int k = aint[static_cast<size_t>(j + 1 + (i + 1) * (areaWidth + 2))];
                if (nextInt(57) == 0 && k == id(BiomeGenBase::plains))
                    aint1[static_cast<size_t>(j + i * areaWidth)] = id(BiomeGenBase::plains) + 128;
                else
                    aint1[static_cast<size_t>(j + i * areaWidth)] = k;
            }
        }
        return aint1;
    }
};

class GenLayerShore : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const std::vector<int> aint = parent->getInts(areaX - 1, areaY - 1, areaWidth + 2, areaHeight + 2);
        std::vector<int> aint1(static_cast<size_t>(areaWidth * areaHeight));
        const int w = areaWidth + 2;
        const auto at = [&](int index) { return aint[static_cast<size_t>(index)]; };
        for (int i = 0; i < areaHeight; ++i) {
            for (int j = 0; j < areaWidth; ++j) {
                initChunkSeed(j + areaX, i + areaY);
                const int k = at(j + 1 + (i + 1) * w);
                const BiomeGenBase* biomegenbase = BiomeGenBase::getBiome(k);
                int& out = aint1[static_cast<size_t>(j + i * areaWidth)];
                const int n = at(j + 1 + (i + 1 - 1) * w);
                const int e = at(j + 1 + 1 + (i + 1) * w);
                const int wv = at(j + 1 - 1 + (i + 1) * w);
                const int s = at(j + 1 + (i + 1 + 1) * w);
                if (k == id(BiomeGenBase::mushroomIsland)) {
                    const int ocean = id(BiomeGenBase::ocean);
                    out = n != ocean && e != ocean && wv != ocean && s != ocean ? k : id(BiomeGenBase::mushroomIslandShore);
                } else if (biomegenbase && biomegenbase->getBiomeClass() == typeid(BiomeGenJungle)) {
                    if (func_151631_c(n) && func_151631_c(e) && func_151631_c(wv) && func_151631_c(s))
                        out = !isBiomeOceanic(n) && !isBiomeOceanic(e) && !isBiomeOceanic(wv) && !isBiomeOceanic(s) ? k : id(BiomeGenBase::beach);
                    else
                        out = id(BiomeGenBase::jungleEdge);
                } else if (k != id(BiomeGenBase::extremeHills) && k != id(BiomeGenBase::extremeHillsPlus) && k != id(BiomeGenBase::extremeHillsEdge)) {
                    if (biomegenbase && biomegenbase->isSnowyBiome()) {
                        func_151632_a(aint, aint1, j, i, areaWidth, k, id(BiomeGenBase::coldBeach));
                    } else if (k != id(BiomeGenBase::mesa) && k != id(BiomeGenBase::mesaPlateau_F)) {
                        if (k != id(BiomeGenBase::ocean) && k != id(BiomeGenBase::deepOcean) && k != id(BiomeGenBase::river) && k != id(BiomeGenBase::swampland))
                            out = !isBiomeOceanic(n) && !isBiomeOceanic(e) && !isBiomeOceanic(wv) && !isBiomeOceanic(s) ? k : id(BiomeGenBase::beach);
                        else
                            out = k;
                    } else {
                        if (!isBiomeOceanic(n) && !isBiomeOceanic(e) && !isBiomeOceanic(wv) && !isBiomeOceanic(s))
                            out = func_151633_d(n) && func_151633_d(e) && func_151633_d(wv) && func_151633_d(s) ? k : id(BiomeGenBase::desert);
                        else
                            out = k;
                    }
                } else {
                    func_151632_a(aint, aint1, j, i, areaWidth, k, id(BiomeGenBase::stoneBeach));
                }
            }
        }
        return aint1;
    }

private:
    static void func_151632_a(const std::vector<int>& in, std::vector<int>& out, int x, int y, int width, int value, int shore)
    {
        if (isBiomeOceanic(value)) {
            out[static_cast<size_t>(x + y * width)] = value;
            return;
        }
        const int w = width + 2;
        const int i = in[static_cast<size_t>(x + 1 + (y + 1 - 1) * w)];
        const int j = in[static_cast<size_t>(x + 1 + 1 + (y + 1) * w)];
        const int k = in[static_cast<size_t>(x + 1 - 1 + (y + 1) * w)];
        const int l = in[static_cast<size_t>(x + 1 + (y + 1 + 1) * w)];
        out[static_cast<size_t>(x + y * width)] = !isBiomeOceanic(i) && !isBiomeOceanic(j) && !isBiomeOceanic(k) && !isBiomeOceanic(l) ? value : shore;
    }

    static bool func_151631_c(int value)
    {
        const BiomeGenBase* biome = BiomeGenBase::getBiome(value);
        return biome && biome->getBiomeClass() == typeid(BiomeGenJungle)
            ? true
            : value == id(BiomeGenBase::jungleEdge) || value == id(BiomeGenBase::jungle) || value == id(BiomeGenBase::jungleHills)
                || value == id(BiomeGenBase::forest) || value == id(BiomeGenBase::taiga) || isBiomeOceanic(value);
    }

    static bool func_151633_d(int value) { return dynamic_cast<const BiomeGenMesa*>(BiomeGenBase::getBiome(value)) != nullptr; }
};

class GenLayerRiverMix : public GenLayer {
public:
    GenLayerRiverMix(int64_t seed, std::shared_ptr<GenLayer> biomes, std::shared_ptr<GenLayer> rivers)
        : GenLayer(seed)
        , biomePatternGeneratorChain(std::move(biomes))
        , riverPatternGeneratorChain(std::move(rivers))
    {
    }

    void initWorldGenSeed(int64_t seed) override
    {
        biomePatternGeneratorChain->initWorldGenSeed(seed);
        riverPatternGeneratorChain->initWorldGenSeed(seed);
        GenLayer::initWorldGenSeed(seed);
    }

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        const std::vector<int> aint = biomePatternGeneratorChain->getInts(areaX, areaY, areaWidth, areaHeight);
        const std::vector<int> aint1 = riverPatternGeneratorChain->getInts(areaX, areaY, areaWidth, areaHeight);
        std::vector<int> aint2(static_cast<size_t>(areaWidth * areaHeight));
        for (size_t i = 0; i < aint2.size(); ++i) {
            if (aint[i] != id(BiomeGenBase::ocean) && aint[i] != id(BiomeGenBase::deepOcean)) {
                if (aint1[i] == id(BiomeGenBase::river)) {
                    if (aint[i] == id(BiomeGenBase::icePlains))
                        aint2[i] = id(BiomeGenBase::frozenRiver);
                    else if (aint[i] != id(BiomeGenBase::mushroomIsland) && aint[i] != id(BiomeGenBase::mushroomIslandShore))
                        aint2[i] = aint1[i] & 255;
                    else
                        aint2[i] = id(BiomeGenBase::mushroomIslandShore);
                } else {
                    aint2[i] = aint[i];
                }
            } else {
                aint2[i] = aint[i];
            }
        }
        return aint2;
    }

private:
    std::shared_ptr<GenLayer> biomePatternGeneratorChain;
    std::shared_ptr<GenLayer> riverPatternGeneratorChain;
};

class GenLayerVoronoiZoom : public GenLayerBordered {
public:
    using GenLayerBordered::GenLayerBordered;

    std::vector<int> getInts(int areaX, int areaY, int areaWidth, int areaHeight) override
    {
        areaX = areaX - 2;
        areaY = areaY - 2;
        const int i = areaX >> 2;
        const int j = areaY >> 2;
        const int k = (areaWidth >> 2) + 2;
        const int l = (areaHeight >> 2) + 2;
        const std::vector<int> aint = parent->getInts(i, j, k, l);
        const int i1 = (k - 1) << 2;
        const int j1 = (l - 1) << 2;
        std::vector<int> aint1(static_cast<size_t>(i1 * j1));
        const auto offset = [this]() { return (static_cast<double>(nextInt(1024)) / 1024.0 - 0.5) * 3.6; };
        for (int k1 = 0; k1 < l - 1; ++k1) {
            int l1 = 0;
            int i2 = aint[static_cast<size_t>(l1 + 0 + (k1 + 0) * k)];
            for (int j2 = aint[static_cast<size_t>(l1 + 0 + (k1 + 1) * k)]; l1 < k - 1; ++l1) {
                initChunkSeed(static_cast<int64_t>((l1 + i) << 2), static_cast<int64_t>((k1 + j) << 2));
                const double d1 = offset();
                const double d2 = offset();
                initChunkSeed(static_cast<int64_t>((l1 + i + 1) << 2), static_cast<int64_t>((k1 + j) << 2));
                const double d3 = offset() + 4.0;
                const double d4 = offset();
                initChunkSeed(static_cast<int64_t>((l1 + i) << 2), static_cast<int64_t>((k1 + j + 1) << 2));
                const double d5 = offset();
                const double d6 = offset() + 4.0;
                initChunkSeed(static_cast<int64_t>((l1 + i + 1) << 2), static_cast<int64_t>((k1 + j + 1) << 2));
                const double d7 = offset() + 4.0;
                const double d8 = offset() + 4.0;
                const int k2 = aint[static_cast<size_t>(l1 + 1 + (k1 + 0) * k)] & 255;
                const int l2 = aint[static_cast<size_t>(l1 + 1 + (k1 + 1) * k)] & 255;
                for (int i3 = 0; i3 < 4; ++i3) {
                    int j3 = ((k1 << 2) + i3) * i1 + (l1 << 2);
                    for (int k3 = 0; k3 < 4; ++k3) {
                        const double di = static_cast<double>(i3);
                        const double dk = static_cast<double>(k3);
                        const double d9 = (di - d2) * (di - d2) + (dk - d1) * (dk - d1);
                        const double d10 = (di - d4) * (di - d4) + (dk - d3) * (dk - d3);
                        const double d11 = (di - d6) * (di - d6) + (dk - d5) * (dk - d5);
                        const double d12 = (di - d8) * (di - d8) + (dk - d7) * (dk - d7);
                        if (d9 < d10 && d9 < d11 && d9 < d12)
                            aint1[static_cast<size_t>(j3++)] = i2;
                        else if (d10 < d9 && d10 < d11 && d10 < d12)
                            aint1[static_cast<size_t>(j3++)] = k2;
                        else if (d11 < d9 && d11 < d10 && d11 < d12)
                            aint1[static_cast<size_t>(j3++)] = j2;
                        else
                            aint1[static_cast<size_t>(j3++)] = l2;
                    }
                }
                i2 = k2;
                j2 = l2;
            }
        }
        std::vector<int> aint2(static_cast<size_t>(areaWidth * areaHeight));
        for (int l3 = 0; l3 < areaHeight; ++l3) {
            const auto src = aint1.begin() + ((l3 + (areaY & 3)) * i1 + (areaX & 3));
            std::copy(src, src + areaWidth, aint2.begin() + l3 * areaWidth);
        }
        return aint2;
    }
};

} // namespace

std::array<std::shared_ptr<GenLayer>, 3> GenLayer::initializeAllBiomeGenerators(int64_t seed, const WorldType* worldType, const std::string& options)
{
    using Edge = GenLayerEdge::Mode;
    std::shared_ptr<GenLayer> genlayer = std::make_shared<GenLayerIsland>(1);
    genlayer = std::make_shared<GenLayerFuzzyZoom>(2000, genlayer);
    std::shared_ptr<GenLayer> genlayeraddisland = std::make_shared<GenLayerAddIsland>(1, genlayer);
    std::shared_ptr<GenLayer> genlayerzoom = std::make_shared<GenLayerZoom>(2001, genlayeraddisland);
    std::shared_ptr<GenLayer> genlayeraddisland1 = std::make_shared<GenLayerAddIsland>(2, genlayerzoom);
    genlayeraddisland1 = std::make_shared<GenLayerAddIsland>(50, genlayeraddisland1);
    genlayeraddisland1 = std::make_shared<GenLayerAddIsland>(70, genlayeraddisland1);
    std::shared_ptr<GenLayer> genlayerremovetoomuchocean = std::make_shared<GenLayerRemoveTooMuchOcean>(2, genlayeraddisland1);
    std::shared_ptr<GenLayer> genlayeraddsnow = std::make_shared<GenLayerAddSnow>(2, genlayerremovetoomuchocean);
    std::shared_ptr<GenLayer> genlayeraddisland2 = std::make_shared<GenLayerAddIsland>(3, genlayeraddsnow);
    std::shared_ptr<GenLayer> genlayeredge = std::make_shared<GenLayerEdge>(2, genlayeraddisland2, Edge::COOL_WARM);
    genlayeredge = std::make_shared<GenLayerEdge>(2, genlayeredge, Edge::HEAT_ICE);
    genlayeredge = std::make_shared<GenLayerEdge>(3, genlayeredge, Edge::SPECIAL);
    std::shared_ptr<GenLayer> genlayerzoom1 = std::make_shared<GenLayerZoom>(2002, genlayeredge);
    genlayerzoom1 = std::make_shared<GenLayerZoom>(2003, genlayerzoom1);
    std::shared_ptr<GenLayer> genlayeraddisland3 = std::make_shared<GenLayerAddIsland>(4, genlayerzoom1);
    std::shared_ptr<GenLayer> genlayeraddmushroomisland = std::make_shared<GenLayerAddMushroomIsland>(5, genlayeraddisland3);
    std::shared_ptr<GenLayer> genlayerdeepocean = std::make_shared<GenLayerDeepOcean>(4, genlayeraddmushroomisland);
    std::shared_ptr<GenLayer> genlayer4 = GenLayerZoom::magnify(1000, genlayerdeepocean, 0);
    int i = 4;
    int j = i;
    if (worldType == &WorldType::CUSTOMIZED && !options.empty()) {
        const auto settings = ChunkProviderSettings::Factory::jsonToFactory(options);
        i = settings.biomeSize;
        j = settings.riverSize;
    }
    if (worldType == &WorldType::LARGE_BIOMES)
        i = 6;
    std::shared_ptr<GenLayer> lvt_8_1_ = GenLayerZoom::magnify(1000, genlayer4, 0);
    std::shared_ptr<GenLayer> genlayerriverinit = std::make_shared<GenLayerRiverInit>(100, lvt_8_1_);
    std::shared_ptr<GenLayer> lvt_9_1_ = std::make_shared<GenLayerBiome>(200, genlayer4, worldType, options);
    std::shared_ptr<GenLayer> genlayer6 = GenLayerZoom::magnify(1000, lvt_9_1_, 2);
    std::shared_ptr<GenLayer> genlayerbiomeedge = std::make_shared<GenLayerBiomeEdge>(1000, genlayer6);
    std::shared_ptr<GenLayer> lvt_10_1_ = GenLayerZoom::magnify(1000, genlayerriverinit, 2);
    std::shared_ptr<GenLayer> genlayerhills = std::make_shared<GenLayerHills>(1000, genlayerbiomeedge, lvt_10_1_);
    std::shared_ptr<GenLayer> genlayer5 = GenLayerZoom::magnify(1000, genlayerriverinit, 2);
    genlayer5 = GenLayerZoom::magnify(1000, genlayer5, j);
    std::shared_ptr<GenLayer> genlayerriver = std::make_shared<GenLayerRiver>(1, genlayer5);
    std::shared_ptr<GenLayer> genlayersmooth = std::make_shared<GenLayerSmooth>(1000, genlayerriver);
    genlayerhills = std::make_shared<GenLayerRareBiome>(1001, genlayerhills);
    for (int k = 0; k < i; ++k) {
        genlayerhills = std::make_shared<GenLayerZoom>(1000 + k, genlayerhills);
        if (k == 0)
            genlayerhills = std::make_shared<GenLayerAddIsland>(3, genlayerhills);
        if (k == 1 || i == 1)
            genlayerhills = std::make_shared<GenLayerShore>(1000, genlayerhills);
    }
    std::shared_ptr<GenLayer> genlayersmooth1 = std::make_shared<GenLayerSmooth>(1000, genlayerhills);
    std::shared_ptr<GenLayer> genlayerrivermix = std::make_shared<GenLayerRiverMix>(100, genlayersmooth1, genlayersmooth);
    std::shared_ptr<GenLayer> genlayer3 = std::make_shared<GenLayerVoronoiZoom>(10, genlayerrivermix);
    genlayerrivermix->initWorldGenSeed(seed);
    genlayer3->initWorldGenSeed(seed);
    return { genlayerrivermix, genlayer3, genlayerrivermix };
}

} // namespace mc
