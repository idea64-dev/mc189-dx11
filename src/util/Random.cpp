#include "util/Random.h"

#include "core/Java.h"
#include "core/StrictMath.h"
#include "platform/Sys.h"

#include <algorithm>
#include <atomic>
#include <stdexcept>

namespace mc {

namespace {

// Random()'s seed: a seed uniquifier advanced by an LCG, XOR the current nano time (as in OpenJDK).
int64_t seedUniquifier()
{
    static std::atomic<int64_t> uniquifier { 8682522807148012LL };
    int64_t current = uniquifier.load();
    for (;;) {
        const int64_t next = java::mul(current, 1181783497276652981LL);
        if (uniquifier.compare_exchange_weak(current, next))
            return next;
    }
}

} // namespace

Random::Random()
    : Random(seedUniquifier() ^ Sys::nanoTime())
{
}

Random::Random(int64_t initialSeed)
{
    setSeed(initialSeed);
}

void Random::setSeed(int64_t newSeed)
{
    seed = (static_cast<uint64_t>(newSeed) ^ kMultiplier) & kMask;
    haveNextNextGaussian = false;
}

int32_t Random::next(int bits)
{
    seed = (seed * kMultiplier + kAddend) & kMask;
    return static_cast<int32_t>(static_cast<int64_t>(seed >> (48 - bits)));
}

int32_t Random::nextInt()
{
    return next(32);
}

int32_t Random::nextInt(int32_t bound)
{
    if (bound <= 0)
        throw std::invalid_argument("bound must be positive");

    if ((bound & -bound) == bound) // power of two
        return static_cast<int32_t>((static_cast<int64_t>(bound) * static_cast<int64_t>(next(31))) >> 31);

    int32_t bits;
    int32_t value;
    do {
        bits = next(31);
        value = bits % bound;
    } while (java::add(java::sub(bits, value), bound - 1) < 0);
    return value;
}

int64_t Random::nextLong()
{
    // Separate statements: C++ leaves the evaluation order of operands unspecified, Java does not.
    const int64_t high = next(32);
    const int64_t low = next(32);
    return java::add(java::shl(high, 32), low);
}

bool Random::nextBoolean()
{
    return next(1) != 0;
}

float Random::nextFloat()
{
    return static_cast<float>(next(24)) / static_cast<float>(1 << 24);
}

double Random::nextDouble()
{
    const int64_t high = next(26);
    const int64_t low = next(27);
    return static_cast<double>((high << 27) + low) * 0x1.0p-53;
}

double Random::nextGaussian()
{
    if (haveNextNextGaussian) {
        haveNextNextGaussian = false;
        return nextNextGaussian;
    }

    double v1;
    double v2;
    double s;
    do {
        v1 = 2.0 * nextDouble() - 1.0;
        v2 = 2.0 * nextDouble() - 1.0;
        s = v1 * v1 + v2 * v2;
    } while (s >= 1.0 || s == 0.0);

    const double multiplier = StrictMath::sqrt(-2.0 * StrictMath::log(s) / s);
    nextNextGaussian = v2 * multiplier;
    haveNextNextGaussian = true;
    return v1 * multiplier;
}

void Random::nextBytes(std::span<int8_t> bytes)
{
    for (size_t i = 0; i < bytes.size();) {
        for (int32_t rnd = nextInt(), n = std::min<int32_t>(static_cast<int32_t>(bytes.size() - i), 4); n-- > 0; rnd >>= 8)
            bytes[i++] = static_cast<int8_t>(rnd);
    }
}

} // namespace mc
