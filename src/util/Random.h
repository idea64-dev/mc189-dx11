#pragma once

// java.util.Random, bit-exact: the same seed yields the same sequence as the JVM.
// World generation, structure placement and many gameplay rolls depend on this.

#include <cstdint>
#include <span>

namespace mc {

class Random {
public:
    // Seeded from the clock like new Random(); the sequence is not reproducible.
    Random();
    explicit Random(int64_t seed);

    void setSeed(int64_t seed);

    int32_t nextInt();
    // Uniform in [0, bound). Java throws IllegalArgumentException for bound <= 0; so does this.
    int32_t nextInt(int32_t bound);
    int64_t nextLong();
    bool nextBoolean();
    float nextFloat();
    double nextDouble();
    double nextGaussian();
    void nextBytes(std::span<int8_t> bytes);

protected:
    int32_t next(int bits);

private:
    static constexpr uint64_t kMultiplier = 0x5DEECE66DULL;
    static constexpr uint64_t kAddend = 0xBULL;
    static constexpr uint64_t kMask = (1ULL << 48) - 1;

    uint64_t seed = 0;
    double nextNextGaussian = 0.0;
    bool haveNextNextGaussian = false;
};

} // namespace mc
