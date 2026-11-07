#pragma once

// Original: net/minecraft/util/MathHelper.java
// Results match the JVM bit for bit (verified by tests/MathHelperTest.cpp against Java-generated values).
// Not ported yet: getPositionRandom(Vec3i) and getRandomUuid(Random) wait for BlockPos and UUID.

#include <cstdint>
#include <span>
#include <string_view>

namespace mc {

class Random;

class MathHelper {
public:
    static const float SQRT_2;

    // Table lookups with 65536 steps per turn, not std::sin: entity animation and particles depend on it.
    static float sin(float value);
    static float cos(float value);

    static float sqrt_float(float value);
    static float sqrt_double(double value);

    static int floor_float(float value);
    static int truncateDoubleToInt(double value);
    static int floor_double(double value);
    static int64_t floor_double_long(double value);
    static int func_154353_e(double value);
    static float abs(float value);
    static int abs_int(int value);
    static int ceiling_float_int(float value);
    static int ceiling_double_int(double value);

    static int clamp_int(int num, int min, int max) { return num < min ? min : (num > max ? max : num); }
    static float clamp_float(float num, float min, float max) { return num < min ? min : (num > max ? max : num); }
    static double clamp_double(double num, double min, double max) { return num < min ? min : (num > max ? max : num); }
    static double denormalizeClamp(double lowerBnd, double upperBnd, double slide);
    static double abs_max(double a, double b);
    static int bucketInt(int value, int bucketSize);

    static int getRandomIntegerInRange(Random& rand, int min, int max);
    static float randomFloatClamp(Random& rand, float min, float max);
    static double getRandomDoubleInRange(Random& rand, double min, double max);

    static double average(std::span<const int64_t> values);
    static bool epsilonEquals(float a, float b);
    static int normalizeAngle(int angle, int modulus);
    static float wrapAngleTo180_float(float value);
    static double wrapAngleTo180_double(double value);

    // Integer.parseInt / Double.parseDouble with a fallback on any parse error.
    static int parseIntWithDefault(std::string_view text, int defaultValue);
    static int parseIntWithDefaultAndMax(std::string_view text, int defaultValue, int max);
    static double parseDoubleWithDefault(std::string_view text, double defaultValue);
    static double parseDoubleWithDefaultAndMax(std::string_view text, double defaultValue, double max);

    static int roundUpToPowerOfTwo(int value);
    static int calculateLogBaseTwo(int value);
    static int roundUp(int number, int interval);

    // Packs 0..1 floats into 0xRRGGBB.
    static int func_180183_b(float red, float green, float blue);
    // Packs 0..255 ints into 0xRRGGBB.
    static int func_180181_b(int red, int green, int blue);
    // Multiplies two ARGB colours channel-wise, keeping the alpha of the first.
    static int func_180188_d(int color1, int color2);
    // Fractional part: x - floor(x).
    static double func_181162_h(double value);

    static int64_t getCoordinateRandom(int x, int y, int z);
    // Inverse lerp: where value lies between min and max.
    static double func_181160_c(double value, double min, double max);
    // Fast atan2 from a lookup table; slightly different from std::atan2 and used for entity rotations.
    static double atan2(double y, double x);
    // Fast inverse square root.
    static double func_181161_i(double value);
    static int hsvToRGB(float hue, float saturation, float value);

    // Raw access for tests.
    static float sinTableEntry(int index);

private:
    static bool isPowerOfTwo(int value) { return value != 0 && (value & (value - 1)) == 0; }
    static int calculateLogBaseTwoDeBruijn(int value);
};

} // namespace mc
