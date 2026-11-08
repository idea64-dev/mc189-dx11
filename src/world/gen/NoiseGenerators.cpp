#include "world/gen/NoiseGenerators.h"

#include "core/Java.h"
#include "util/MathHelper.h"

#include <algorithm>
#include <cmath>

namespace mc {

namespace {

constexpr double field_152381_e[16] { 1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0 };
constexpr double field_152382_f[16] { 1.0, 1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 1.0, -1.0 };
constexpr double field_152383_g[16] { 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 0.0, 1.0, 0.0, -1.0 };
constexpr double field_152384_h[16] { 1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0 };
constexpr double field_152385_i[16] { 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 0.0, 1.0, 0.0, -1.0 };

inline double lerp(double t, double a, double b)
{
    return a + t * (b - a);
}

inline double grad2(int hash, double x, double z)
{
    const int i = hash & 15;
    return field_152384_h[i] * x + field_152385_i[i] * z;
}

inline double grad(int hash, double x, double y, double z)
{
    const int i = hash & 15;
    return field_152381_e[i] * x + field_152382_f[i] * y + field_152383_g[i] * z;
}

void fillPermutations(std::array<int, 512>& p, Random& random)
{
    for (int i = 0; i < 256; ++i)
        p[static_cast<size_t>(i)] = i;
    for (int l = 0; l < 256; ++l) {
        const int j = random.nextInt(256 - l) + l;
        std::swap(p[static_cast<size_t>(l)], p[static_cast<size_t>(j)]);
        p[static_cast<size_t>(l + 256)] = p[static_cast<size_t>(l)];
    }
}

} // namespace

// --- NoiseGeneratorImproved ---

NoiseGeneratorImproved::NoiseGeneratorImproved(Random& random)
{
    xCoord = random.nextDouble() * 256.0;
    yCoord = random.nextDouble() * 256.0;
    zCoord = random.nextDouble() * 256.0;
    fillPermutations(permutations, random);
}

void NoiseGeneratorImproved::populateNoiseArray(std::vector<double>& noiseArray, double xOffset, double yOffset, double zOffset, int xSize, int ySize,
    int zSize, double xScale, double yScale, double zScale, double noiseScale) const
{
    const auto p = [this](int i) { return permutations[static_cast<size_t>(i)]; };
    if (ySize == 1) {
        int l5 = 0;
        const double d16 = 1.0 / noiseScale;
        for (int j2 = 0; j2 < xSize; ++j2) {
            double d17 = xOffset + static_cast<double>(j2) * xScale + xCoord;
            int i6 = java::d2i(d17);
            if (d17 < static_cast<double>(i6))
                --i6;
            const int k2 = i6 & 255;
            d17 = d17 - static_cast<double>(i6);
            const double d18 = d17 * d17 * d17 * (d17 * (d17 * 6.0 - 15.0) + 10.0);
            for (int j6 = 0; j6 < zSize; ++j6) {
                double d19 = zOffset + static_cast<double>(j6) * zScale + zCoord;
                int k6 = java::d2i(d19);
                if (d19 < static_cast<double>(k6))
                    --k6;
                const int l6 = k6 & 255;
                d19 = d19 - static_cast<double>(k6);
                const double d20 = d19 * d19 * d19 * (d19 * (d19 * 6.0 - 15.0) + 10.0);
                const int i5 = p(k2) + 0;
                const int j5 = p(i5) + l6;
                const int j = p(k2 + 1) + 0;
                const int k5 = p(j) + l6;
                const double d14 = lerp(d18, grad2(p(j5), d17, d19), grad(p(k5), d17 - 1.0, 0.0, d19));
                const double d15 = lerp(d18, grad(p(j5 + 1), d17, 0.0, d19 - 1.0), grad(p(k5 + 1), d17 - 1.0, 0.0, d19 - 1.0));
                const double d21 = lerp(d20, d14, d15);
                noiseArray[static_cast<size_t>(l5++)] += d21 * d16;
            }
        }
        return;
    }
    int i = 0;
    const double d0 = 1.0 / noiseScale;
    int k = -1;
    int i1 = 0;
    int j1 = 0;
    int l1 = 0;
    int i2 = 0;
    double d1 = 0.0;
    double d2 = 0.0;
    double d3 = 0.0;
    double d4 = 0.0;
    for (int l2 = 0; l2 < xSize; ++l2) {
        double d5 = xOffset + static_cast<double>(l2) * xScale + xCoord;
        int i3 = java::d2i(d5);
        if (d5 < static_cast<double>(i3))
            --i3;
        const int j3 = i3 & 255;
        d5 = d5 - static_cast<double>(i3);
        const double d6 = d5 * d5 * d5 * (d5 * (d5 * 6.0 - 15.0) + 10.0);
        for (int k3 = 0; k3 < zSize; ++k3) {
            double d7 = zOffset + static_cast<double>(k3) * zScale + zCoord;
            int l3 = java::d2i(d7);
            if (d7 < static_cast<double>(l3))
                --l3;
            const int i4 = l3 & 255;
            d7 = d7 - static_cast<double>(l3);
            const double d8 = d7 * d7 * d7 * (d7 * (d7 * 6.0 - 15.0) + 10.0);
            for (int j4 = 0; j4 < ySize; ++j4) {
                double d9 = yOffset + static_cast<double>(j4) * yScale + yCoord;
                int k4 = java::d2i(d9);
                if (d9 < static_cast<double>(k4))
                    --k4;
                const int l4 = k4 & 255;
                d9 = d9 - static_cast<double>(k4);
                const double d10 = d9 * d9 * d9 * (d9 * (d9 * 6.0 - 15.0) + 10.0);
                if (j4 == 0 || l4 != k) {
                    k = l4;
                    const int l = p(j3) + l4;
                    i1 = p(l) + i4;
                    j1 = p(l + 1) + i4;
                    const int k1 = p(j3 + 1) + l4;
                    l1 = p(k1) + i4;
                    i2 = p(k1 + 1) + i4;
                    d1 = lerp(d6, grad(p(i1), d5, d9, d7), grad(p(l1), d5 - 1.0, d9, d7));
                    d2 = lerp(d6, grad(p(j1), d5, d9 - 1.0, d7), grad(p(i2), d5 - 1.0, d9 - 1.0, d7));
                    d3 = lerp(d6, grad(p(i1 + 1), d5, d9, d7 - 1.0), grad(p(l1 + 1), d5 - 1.0, d9, d7 - 1.0));
                    d4 = lerp(d6, grad(p(j1 + 1), d5, d9 - 1.0, d7 - 1.0), grad(p(i2 + 1), d5 - 1.0, d9 - 1.0, d7 - 1.0));
                }
                const double d11 = lerp(d10, d1, d2);
                const double d12 = lerp(d10, d3, d4);
                const double d13 = lerp(d8, d11, d12);
                noiseArray[static_cast<size_t>(i++)] += d13 * d0;
            }
        }
    }
}

// --- NoiseGeneratorOctaves ---

NoiseGeneratorOctaves::NoiseGeneratorOctaves(Random& seed, int octavesIn)
{
    generatorCollection.reserve(static_cast<size_t>(octavesIn));
    for (int i = 0; i < octavesIn; ++i)
        generatorCollection.emplace_back(seed);
}

std::vector<double>& NoiseGeneratorOctaves::generateNoiseOctaves(std::vector<double>& noiseArray, int xOffset, int yOffset, int zOffset, int xSize, int ySize,
    int zSize, double xScale, double yScale, double zScale) const
{
    if (noiseArray.empty())
        noiseArray.assign(static_cast<size_t>(xSize * ySize * zSize), 0.0);
    else
        std::fill(noiseArray.begin(), noiseArray.end(), 0.0);
    double d3 = 1.0;
    for (const NoiseGeneratorImproved& generator : generatorCollection) {
        double d0 = static_cast<double>(xOffset) * d3 * xScale;
        const double d1 = static_cast<double>(yOffset) * d3 * yScale;
        double d2 = static_cast<double>(zOffset) * d3 * zScale;
        int64_t k = MathHelper::floor_double_long(d0);
        int64_t l = MathHelper::floor_double_long(d2);
        d0 = d0 - static_cast<double>(k);
        d2 = d2 - static_cast<double>(l);
        k = k % 16777216LL;
        l = l % 16777216LL;
        d0 = d0 + static_cast<double>(k);
        d2 = d2 + static_cast<double>(l);
        generator.populateNoiseArray(noiseArray, d0, d1, d2, xSize, ySize, zSize, xScale * d3, yScale * d3, zScale * d3, d3);
        d3 /= 2.0;
    }
    return noiseArray;
}

std::vector<double>& NoiseGeneratorOctaves::generateNoiseOctaves(std::vector<double>& noiseArray, int xOffset, int zOffset, int xSize, int zSize, double xScale,
    double zScale, double) const
{
    return generateNoiseOctaves(noiseArray, xOffset, 10, zOffset, xSize, 1, zSize, xScale, 1.0, zScale);
}

// --- NoiseGeneratorSimplex ---

namespace {

constexpr int field_151611_e[12][3] { { 1, 1, 0 }, { -1, 1, 0 }, { 1, -1, 0 }, { -1, -1, 0 }, { 1, 0, 1 }, { -1, 0, 1 }, { 1, 0, -1 }, { -1, 0, -1 },
    { 0, 1, 1 }, { 0, -1, 1 }, { 0, 1, -1 }, { 0, -1, -1 } };
const double field_151614_a = std::sqrt(3.0);
const double field_151609_g = 0.5 * (field_151614_a - 1.0);
const double field_151615_h = (3.0 - field_151614_a) / 6.0;

inline int fastFloor(double value)
{
    return value > 0.0 ? java::d2i(value) : java::d2i(value) - 1;
}

inline double dot(const int* g, double x, double y)
{
    return static_cast<double>(g[0]) * x + static_cast<double>(g[1]) * y;
}

} // namespace

NoiseGeneratorSimplex::NoiseGeneratorSimplex(Random& random)
{
    field_151612_b = random.nextDouble() * 256.0;
    field_151613_c = random.nextDouble() * 256.0;
    field_151610_d = random.nextDouble() * 256.0;
    fillPermutations(field_151608_f, random);
}

double NoiseGeneratorSimplex::getValue(double x, double y) const
{
    const auto p = [this](int i) { return field_151608_f[static_cast<size_t>(i)]; };
    const double d3 = 0.5 * (field_151614_a - 1.0);
    const double d4 = (x + y) * d3;
    const int i = fastFloor(x + d4);
    const int j = fastFloor(y + d4);
    const double d5 = (3.0 - field_151614_a) / 6.0;
    const double d6 = static_cast<double>(i + j) * d5;
    const double d7 = static_cast<double>(i) - d6;
    const double d8 = static_cast<double>(j) - d6;
    const double d9 = x - d7;
    const double d10 = y - d8;
    int k;
    int l;
    if (d9 > d10) {
        k = 1;
        l = 0;
    } else {
        k = 0;
        l = 1;
    }
    const double d11 = d9 - static_cast<double>(k) + d5;
    const double d12 = d10 - static_cast<double>(l) + d5;
    const double d13 = d9 - 1.0 + 2.0 * d5;
    const double d14 = d10 - 1.0 + 2.0 * d5;
    const int i1 = i & 255;
    const int j1 = j & 255;
    const int k1 = p(i1 + p(j1)) % 12;
    const int l1 = p(i1 + k + p(j1 + l)) % 12;
    const int i2 = p(i1 + 1 + p(j1 + 1)) % 12;
    double d15 = 0.5 - d9 * d9 - d10 * d10;
    double d0;
    if (d15 < 0.0) {
        d0 = 0.0;
    } else {
        d15 = d15 * d15;
        d0 = d15 * d15 * dot(field_151611_e[k1], d9, d10);
    }
    double d16 = 0.5 - d11 * d11 - d12 * d12;
    double d1;
    if (d16 < 0.0) {
        d1 = 0.0;
    } else {
        d16 = d16 * d16;
        d1 = d16 * d16 * dot(field_151611_e[l1], d11, d12);
    }
    double d17 = 0.5 - d13 * d13 - d14 * d14;
    double d2;
    if (d17 < 0.0) {
        d2 = 0.0;
    } else {
        d17 = d17 * d17;
        d2 = d17 * d17 * dot(field_151611_e[i2], d13, d14);
    }
    return 70.0 * (d0 + d1 + d2);
}

void NoiseGeneratorSimplex::add(std::vector<double>& out, double xOffset, double zOffset, int xSize, int zSize, double xScale, double zScale,
    double amplitude) const
{
    const auto p = [this](int i) { return field_151608_f[static_cast<size_t>(i)]; };
    int i = 0;
    for (int j = 0; j < zSize; ++j) {
        const double d0 = (zOffset + static_cast<double>(j)) * zScale + field_151613_c;
        for (int k = 0; k < xSize; ++k) {
            const double d1 = (xOffset + static_cast<double>(k)) * xScale + field_151612_b;
            const double d5 = (d1 + d0) * field_151609_g;
            const int l = fastFloor(d1 + d5);
            const int i1 = fastFloor(d0 + d5);
            const double d6 = static_cast<double>(l + i1) * field_151615_h;
            const double d7 = static_cast<double>(l) - d6;
            const double d8 = static_cast<double>(i1) - d6;
            const double d9 = d1 - d7;
            const double d10 = d0 - d8;
            int j1;
            int k1;
            if (d9 > d10) {
                j1 = 1;
                k1 = 0;
            } else {
                j1 = 0;
                k1 = 1;
            }
            const double d11 = d9 - static_cast<double>(j1) + field_151615_h;
            const double d12 = d10 - static_cast<double>(k1) + field_151615_h;
            const double d13 = d9 - 1.0 + 2.0 * field_151615_h;
            const double d14 = d10 - 1.0 + 2.0 * field_151615_h;
            const int l1 = l & 255;
            const int i2 = i1 & 255;
            const int j2 = p(l1 + p(i2)) % 12;
            const int k2 = p(l1 + j1 + p(i2 + k1)) % 12;
            const int l2 = p(l1 + 1 + p(i2 + 1)) % 12;
            double d15 = 0.5 - d9 * d9 - d10 * d10;
            double d2;
            if (d15 < 0.0) {
                d2 = 0.0;
            } else {
                d15 = d15 * d15;
                d2 = d15 * d15 * dot(field_151611_e[j2], d9, d10);
            }
            double d16 = 0.5 - d11 * d11 - d12 * d12;
            double d3;
            if (d16 < 0.0) {
                d3 = 0.0;
            } else {
                d16 = d16 * d16;
                d3 = d16 * d16 * dot(field_151611_e[k2], d11, d12);
            }
            double d17 = 0.5 - d13 * d13 - d14 * d14;
            double d4;
            if (d17 < 0.0) {
                d4 = 0.0;
            } else {
                d17 = d17 * d17;
                d4 = d17 * d17 * dot(field_151611_e[l2], d13, d14);
            }
            out[static_cast<size_t>(i++)] += 70.0 * (d2 + d3 + d4) * amplitude;
        }
    }
}

// --- NoiseGeneratorPerlin ---

NoiseGeneratorPerlin::NoiseGeneratorPerlin(Random& random, int levels)
{
    field_151603_a.reserve(static_cast<size_t>(levels));
    for (int i = 0; i < levels; ++i)
        field_151603_a.emplace_back(random);
}

double NoiseGeneratorPerlin::getValue(double x, double z) const
{
    double d0 = 0.0;
    double d1 = 1.0;
    for (const NoiseGeneratorSimplex& simplex : field_151603_a) {
        d0 += simplex.getValue(x * d1, z * d1) / d1;
        d1 /= 2.0;
    }
    return d0;
}

std::vector<double>& NoiseGeneratorPerlin::getRegion(std::vector<double>& out, double xOffset, double zOffset, int xSize, int zSize, double xScale,
    double zScale, double amplitudeScale, double frequencyScale) const
{
    if (!out.empty() && out.size() >= static_cast<size_t>(xSize * zSize))
        std::fill(out.begin(), out.end(), 0.0);
    else
        out.assign(static_cast<size_t>(xSize * zSize), 0.0);
    double d1 = 1.0;
    double d0 = 1.0;
    for (const NoiseGeneratorSimplex& simplex : field_151603_a) {
        simplex.add(out, xOffset, zOffset, xSize, zSize, xScale * d0 * d1, zScale * d0 * d1, 0.55 / d1);
        d0 *= amplitudeScale;
        d1 *= frequencyScale;
    }
    return out;
}

} // namespace mc
