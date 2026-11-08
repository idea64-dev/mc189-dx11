#pragma once

// Original: net/minecraft/world/gen/NoiseGeneratorImproved.java, NoiseGeneratorOctaves.java,
// NoiseGeneratorPerlin.java and NoiseGeneratorSimplex.java -- the terrain noise, bit for bit (no fused
// multiply-add: /fp:precise).
// Java passes a nullable array that is reused when big enough; here an empty vector stands for null and is
// resized, a non-empty one is zeroed and reused.

#include "util/Random.h"

#include <array>
#include <vector>

namespace mc {

class NoiseGeneratorImproved {
public:
    explicit NoiseGeneratorImproved(Random& random);

    // Adds noise * (1 / noiseScale) to noiseArray, x outermost, then z, then y.
    void populateNoiseArray(std::vector<double>& noiseArray, double xOffset, double yOffset, double zOffset, int xSize, int ySize, int zSize, double xScale,
        double yScale, double zScale, double noiseScale) const;

    double xCoord;
    double yCoord;
    double zCoord;

private:
    std::array<int, 512> permutations {};
};

class NoiseGeneratorOctaves {
public:
    NoiseGeneratorOctaves(Random& seed, int octavesIn);

    std::vector<double>& generateNoiseOctaves(std::vector<double>& noiseArray, int xOffset, int yOffset, int zOffset, int xSize, int ySize, int zSize,
        double xScale, double yScale, double zScale) const;
    // The 2D form (y size 1 at y 10).
    std::vector<double>& generateNoiseOctaves(std::vector<double>& noiseArray, int xOffset, int zOffset, int xSize, int zSize, double xScale, double zScale,
        double unused) const;

private:
    std::vector<NoiseGeneratorImproved> generatorCollection;
};

class NoiseGeneratorSimplex {
public:
    explicit NoiseGeneratorSimplex(Random& random);

    // Java: func_151605_a -- one 2D sample.
    double getValue(double x, double y) const;
    // Java: func_151606_a -- adds a grid of samples times amplitude.
    void add(std::vector<double>& out, double xOffset, double zOffset, int xSize, int zSize, double xScale, double zScale, double amplitude) const;

    double field_151612_b;
    double field_151613_c;
    double field_151610_d;

private:
    std::array<int, 512> field_151608_f {};
};

class NoiseGeneratorPerlin {
public:
    NoiseGeneratorPerlin(Random& random, int levels);

    // Java: func_151601_a
    double getValue(double x, double z) const;
    // Java: func_151599_a / func_151600_a
    std::vector<double>& getRegion(std::vector<double>& out, double xOffset, double zOffset, int xSize, int zSize, double xScale, double zScale,
        double amplitudeScale, double frequencyScale = 0.5) const;

private:
    std::vector<NoiseGeneratorSimplex> field_151603_a;
};

} // namespace mc
