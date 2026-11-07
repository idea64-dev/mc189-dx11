// Bit-exact comparison with values produced by a real JVM (tests/reference/JavaReference.inc).
// The scripts here mirror tools/reference/GenerateReference.java call for call; keep them in sync.

#include "core/Java.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "util/ResourceLocation.h"

#include "reference/JavaReference.inc"

#include <doctest/doctest.h>

#include <cmath>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace mc;

namespace {

// Walks a reference array in order; floats/doubles compare by bits, except that any NaN equals any NaN.
class Expect {
public:
    explicit Expect(std::span<const int64_t> values)
        : values(values)
    {
    }

    void integer(int64_t actual, const char* what)
    {
        const int64_t expected = next();
        if (expected != actual)
            fail(what, std::to_string(expected), std::to_string(actual));
    }

    void floatBits(float actual, const char* what)
    {
        const auto expected = static_cast<int32_t>(next());
        const float expectedValue = java::intBitsToFloat(expected);
        if (std::isnan(expectedValue) && std::isnan(actual))
            return;
        if (expected != java::floatToRawIntBits(actual))
            fail(what, std::to_string(expectedValue), std::to_string(actual));
    }

    void doubleBits(double actual, const char* what)
    {
        const int64_t expected = next();
        const double expectedValue = java::longBitsToDouble(expected);
        if (std::isnan(expectedValue) && std::isnan(actual))
            return;
        if (expected != java::doubleToRawLongBits(actual))
            fail(what, formatDouble(expectedValue), formatDouble(actual));
    }

    void finish() { CHECK_MESSAGE(position == values.size(), "script consumed ", position, " of ", values.size(), " values"); }
    int failures() const { return failureCount; }

private:
    int64_t next()
    {
        REQUIRE_MESSAGE(position < values.size(), "reference data exhausted at ", position);
        return values[position++];
    }

    static std::string formatDouble(double value)
    {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.17g", value);
        return buffer;
    }

    void fail(const char* what, const std::string& expected, const std::string& actual)
    {
        ++failureCount;
        if (failureCount <= 20)
            FAIL_CHECK(std::string(what), " (value #", position - 1, "): expected ", expected, ", got ", actual);
    }

    std::span<const int64_t> values;
    size_t position = 0;
    int failureCount = 0;
};

uint64_t fnv(uint64_t hash, uint64_t value, int bytes)
{
    for (int i = 0; i < bytes; ++i) {
        hash ^= (value >> (8 * i)) & 0xFF;
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

constexpr uint64_t kFnvOffset = 0xCBF29CE484222325ULL;

std::string_view view(const char8_t* text)
{
    return { reinterpret_cast<const char*>(text) };
}

template <class T, size_t N>
std::vector<T> fromBits(const int64_t (&bits)[N])
{
    std::vector<T> result;
    for (const int64_t value : bits) {
        if constexpr (std::is_same_v<T, float>)
            result.push_back(java::intBitsToFloat(static_cast<int32_t>(value)));
        else
            result.push_back(java::longBitsToDouble(value));
    }
    return result;
}

} // namespace

TEST_CASE("java.util.Random matches the JVM")
{
    Expect expect(javaref::randomScript);

    for (const int64_t seed : javaref::randomSeeds) {
        CAPTURE(seed);
        Random r(seed);
        for (int i = 0; i < 8; ++i)
            expect.integer(r.nextInt(), "nextInt()");
        for (const int64_t bound : javaref::randomBounds)
            for (int i = 0; i < 4; ++i)
                expect.integer(r.nextInt(static_cast<int32_t>(bound)), "nextInt(bound)");
        for (int i = 0; i < 4; ++i)
            expect.integer(r.nextLong(), "nextLong()");
        for (int i = 0; i < 4; ++i)
            expect.floatBits(r.nextFloat(), "nextFloat()");
        for (int i = 0; i < 4; ++i)
            expect.doubleBits(r.nextDouble(), "nextDouble()");
        for (int i = 0; i < 4; ++i)
            expect.integer(r.nextBoolean() ? 1 : 0, "nextBoolean()");
        for (int i = 0; i < 7; ++i)
            expect.doubleBits(r.nextGaussian(), "nextGaussian()");
        r.setSeed(r.nextLong());
        expect.doubleBits(r.nextGaussian(), "nextGaussian() after setSeed");
        int8_t bytes[7];
        r.nextBytes(bytes);
        for (const int8_t b : bytes)
            expect.integer(b, "nextBytes()");
    }
    expect.finish();

    CHECK_THROWS(Random(0).nextInt(0));
}

TEST_CASE("Random stress hash matches the JVM")
{
    Random r(2024);
    uint64_t h = kFnvOffset;
    for (int i = 0; i < 20000; ++i) {
        h = fnv(h, static_cast<uint64_t>(java::doubleToRawLongBits(r.nextGaussian())), 8);
        h = fnv(h, static_cast<uint64_t>(r.nextInt(1 + (i % 1000))), 4);
        h = fnv(h, static_cast<uint64_t>(java::floatToRawIntBits(r.nextFloat())), 4);
        h = fnv(h, static_cast<uint64_t>(r.nextLong()), 8);
    }
    CHECK(h == javaref::randomStressHash);
}

TEST_CASE("MathHelper sin table matches the JVM")
{
    uint64_t h = kFnvOffset;
    for (int i = 0; i < 65536; ++i) {
        const float angle = static_cast<float>(i) / 10430.378f;
        h = fnv(h, static_cast<uint32_t>(java::floatToRawIntBits(MathHelper::sin(angle))), 4);
    }
    CHECK(h == javaref::sinTableHash);
}

TEST_CASE("MathHelper matches the JVM")
{
    Expect expect(javaref::mathScript);
    const std::vector<float> floats = fromBits<float>(javaref::floatInputBits);
    const std::vector<double> doubles = fromBits<double>(javaref::doubleInputBits);
    const std::vector<double> atanValues = fromBits<double>(javaref::atanInputBits);

    for (const float f : floats) {
        CAPTURE(f);
        expect.floatBits(MathHelper::sin(f), "sin");
        expect.floatBits(MathHelper::cos(f), "cos");
        expect.integer(MathHelper::floor_float(f), "floor_float");
        expect.integer(MathHelper::ceiling_float_int(f), "ceiling_float_int");
        expect.floatBits(MathHelper::wrapAngleTo180_float(f), "wrapAngleTo180_float");
        expect.floatBits(MathHelper::sqrt_float(f), "sqrt_float");
        expect.floatBits(MathHelper::abs(f), "abs");
    }
    for (const double d : doubles) {
        CAPTURE(d);
        expect.integer(MathHelper::floor_double(d), "floor_double");
        expect.integer(MathHelper::floor_double_long(d), "floor_double_long");
        expect.integer(MathHelper::ceiling_double_int(d), "ceiling_double_int");
        expect.integer(MathHelper::truncateDoubleToInt(d), "truncateDoubleToInt");
        expect.integer(MathHelper::func_154353_e(d), "func_154353_e");
        expect.doubleBits(MathHelper::wrapAngleTo180_double(d), "wrapAngleTo180_double");
        expect.doubleBits(MathHelper::func_181162_h(d), "func_181162_h");
        expect.doubleBits(MathHelper::func_181161_i(std::abs(d)), "func_181161_i");
        expect.floatBits(MathHelper::sqrt_double(d), "sqrt_double");
    }
    for (const double y : atanValues) {
        for (const double x : atanValues) {
            CAPTURE(y);
            CAPTURE(x);
            expect.doubleBits(MathHelper::atan2(y, x), "atan2");
        }
    }
    for (const int64_t value : javaref::intInputs) {
        const auto v = static_cast<int32_t>(value);
        CAPTURE(v);
        expect.integer(MathHelper::roundUpToPowerOfTwo(v), "roundUpToPowerOfTwo");
        expect.integer(MathHelper::calculateLogBaseTwo(v), "calculateLogBaseTwo");
        expect.integer(MathHelper::abs_int(v), "abs_int");
        expect.integer(MathHelper::bucketInt(v, 16), "bucketInt(16)");
        expect.integer(MathHelper::bucketInt(v, 3), "bucketInt(3)");
        expect.integer(MathHelper::normalizeAngle(v, 360), "normalizeAngle");
        expect.integer(MathHelper::roundUp(v, 16), "roundUp(16)");
        expect.integer(MathHelper::roundUp(v, -7), "roundUp(-7)");
        expect.integer(MathHelper::roundUp(v, 0), "roundUp(0)");
        expect.integer(MathHelper::func_180181_b(v, v >> 3, v ^ 0x55), "func_180181_b");
        for (const int64_t other : javaref::intInputs)
            expect.integer(MathHelper::func_180188_d(v, static_cast<int32_t>(other)), "func_180188_d");
    }

    const int coords[][3] = { { 0, 0, 0 }, { 1, 2, 3 }, { -1, -64, -1 }, { 30000000, 255, -30000000 },
        { INT32_MAX, INT32_MIN, INT32_MAX }, { 123, 45, -678 } };
    for (const auto& c : coords)
        expect.integer(MathHelper::getCoordinateRandom(c[0], c[1], c[2]), "getCoordinateRandom");

    const float unit[] = { 0.0f, 0.1f, 0.33f, 0.5f, 0.66f, 0.83f, 0.99f, 1.0f };
    for (const float h : unit)
        for (const float s : unit)
            for (const float v : unit)
                expect.integer(MathHelper::hsvToRGB(h, s, v), "hsvToRGB");
    for (const float r : unit)
        for (const float g : unit)
            expect.integer(MathHelper::func_180183_b(r, g, 1.0f - r), "func_180183_b");

    for (const char8_t* text : javaref::intStrings) {
        CAPTURE(std::string(view(text)));
        expect.integer(MathHelper::parseIntWithDefault(view(text), -999), "parseIntWithDefault");
        expect.integer(MathHelper::parseIntWithDefaultAndMax(view(text), -999, 10), "parseIntWithDefaultAndMax");
    }
    for (const char8_t* text : javaref::doubleStrings) {
        CAPTURE(std::string(view(text)));
        expect.doubleBits(MathHelper::parseDoubleWithDefault(view(text), -999.0), "parseDoubleWithDefault");
        expect.doubleBits(MathHelper::parseDoubleWithDefaultAndMax(view(text), -999.0, 2.0), "parseDoubleWithDefaultAndMax");
    }

    for (const float a : floats)
        for (const float b : { 0.0f, 1.0f, 1.000001f, 1.0001f })
            expect.integer(MathHelper::epsilonEquals(a, b) ? 1 : 0, "epsilonEquals");
    for (const double d : doubles) {
        CAPTURE(d);
        expect.doubleBits(MathHelper::denormalizeClamp(-5.0, 7.5, d), "denormalizeClamp");
        expect.doubleBits(MathHelper::abs_max(d, -2.0), "abs_max");
        expect.doubleBits(MathHelper::func_181160_c(d, -1.0, 3.0), "func_181160_c");
    }
    const int64_t averageInput[] = { 1, 2, 4, INT64_MAX };
    expect.doubleBits(MathHelper::average(averageInput), "average");

    Random r(5);
    for (int i = 0; i < 16; ++i) {
        expect.integer(MathHelper::getRandomIntegerInRange(r, -10, 10 + i), "getRandomIntegerInRange");
        expect.floatBits(MathHelper::randomFloatClamp(r, -1.5f, 2.5f + static_cast<float>(i)), "randomFloatClamp");
        expect.doubleBits(MathHelper::getRandomDoubleInRange(r, -3.0, 4.0 + i), "getRandomDoubleInRange");
    }
    expect.floatBits(MathHelper::SQRT_2, "SQRT_2");
    expect.finish();
}

TEST_CASE("String.hashCode matches the JVM")
{
    for (const auto& entry : javaref::stringHashes) {
        CAPTURE(std::string(view(entry.text)));
        CHECK(java::hashCode(view(entry.text)) == entry.hash);
    }
}

TEST_CASE("ResourceLocation parsing follows the original")
{
    CHECK(ResourceLocation("textures/gui/widgets.png").toString() == "minecraft:textures/gui/widgets.png");
    CHECK(ResourceLocation("Realms:textures/x.png").toString() == "realms:textures/x.png");
    CHECK(ResourceLocation(":path").toString() == "minecraft:path");
    // One-character domains are dropped by splitObjectName (index of ':' must be > 1).
    CHECK(ResourceLocation("a:path").toString() == "minecraft:path");
    CHECK(ResourceLocation("ab:path").toString() == "ab:path");
    CHECK(ResourceLocation("minecraft", "textures/a.png") == ResourceLocation("textures/a.png"));
    CHECK(ResourceLocation("textures/a.png").hashCode()
        == java::add(java::mul(31, java::hashCode(std::string_view("minecraft"))), java::hashCode(std::string_view("textures/a.png"))));
}

TEST_CASE("Java casts saturate instead of invoking undefined behaviour")
{
    CHECK(java::f2i(3.0e9f) == INT32_MAX);
    CHECK(java::f2i(-3.0e9f) == INT32_MIN);
    CHECK(java::f2i(NAN) == 0);
    CHECK(java::d2i(2147483647.9) == INT32_MAX);
    CHECK(java::d2l(1.0e30) == INT64_MAX);
    CHECK(java::div(INT32_MIN, -1) == INT32_MIN);
    CHECK(java::rem(INT32_MIN, -1) == 0);
    CHECK(java::ushr(-1, 28) == 15);
    CHECK(java::shl(1, 33) == 2);
}
