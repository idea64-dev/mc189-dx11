#include "util/MathHelper.h"

#include "core/Java.h"
#include "util/Random.h"

#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>

namespace mc {

namespace {

namespace atan_tables {
#include "util/MathHelperTables.inc"
}

struct Tables {
    std::array<float, 65536> sin {};
    std::array<double, 257> asin {};
    std::array<double, 257> cosOfAsin {};
    double atanOffset = 0.0;

    Tables()
    {
        for (int i = 0; i < 65536; ++i)
            sin[static_cast<size_t>(i)] = static_cast<float>(std::sin(static_cast<double>(i) * std::numbers::pi * 2.0 / 65536.0));

        // Java fills these with Math.asin/Math.cos, whose last bit differs from the C runtime for some entries,
        // so the values come from a Java 8 run (tools/reference/generate.bat).
        atanOffset = java::longBitsToDouble(4805340802404319232LL);
        for (size_t j = 0; j < 257; ++j) {
            asin[j] = java::longBitsToDouble(atan_tables::kAsinTableBits[j]);
            cosOfAsin[j] = java::longBitsToDouble(atan_tables::kCosOfAsinTableBits[j]);
        }
    }
};

const Tables& tables()
{
    static const Tables instance;
    return instance;
}

constexpr std::array<int, 32> multiplyDeBruijnBitPosition = {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

// Integer.parseInt(text): optional sign, decimal digits only, no surrounding whitespace.
bool parseJavaInt(std::string_view text, int& out)
{
    if (!text.empty() && text.front() == '+') {
        text.remove_prefix(1);
        if (!text.empty() && (text.front() == '-' || text.front() == '+'))
            return false;
    }
    if (text.empty())
        return false;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), out, 10);
    return result.ec == std::errc() && result.ptr == text.data() + text.size();
}

// Double.parseDouble(text) for decimal input: trims whitespace, accepts "Infinity"/"NaN" and a f/F/d/D suffix.
bool parseJavaDouble(std::string_view text, double& out)
{
    while (!text.empty() && static_cast<unsigned char>(text.front()) <= ' ')
        text.remove_prefix(1);
    while (!text.empty() && static_cast<unsigned char>(text.back()) <= ' ')
        text.remove_suffix(1);

    bool negative = false;
    if (!text.empty() && (text.front() == '+' || text.front() == '-')) {
        negative = text.front() == '-';
        text.remove_prefix(1);
    }
    if (text == "Infinity") {
        out = negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
        return true;
    }
    if (text == "NaN") {
        out = std::numeric_limits<double>::quiet_NaN();
        return true;
    }
    if (!text.empty() && (text.back() == 'f' || text.back() == 'F' || text.back() == 'd' || text.back() == 'D'))
        text.remove_suffix(1);
    // from_chars would also take "inf"/"nan" spellings that Java rejects.
    if (text.empty() || !(std::isdigit(static_cast<unsigned char>(text.front())) || text.front() == '.'))
        return false;

    double value = 0.0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, std::chars_format::general);
    if (result.ptr != text.data() + text.size())
        return false;
    if (result.ec == std::errc::result_out_of_range)
        value = std::abs(value) < 1.0 ? 0.0 : std::numeric_limits<double>::infinity(); // Java rounds, never throws
    else if (result.ec != std::errc())
        return false;
    out = negative ? -value : value;
    return true;
}

// Math.max(double, double): NaN wins, +0.0 beats -0.0.
double javaMax(double a, double b)
{
    if (a != a)
        return a;
    if (a == 0.0 && b == 0.0 && std::signbit(a))
        return b;
    return a >= b ? a : b;
}

} // namespace

const float MathHelper::SQRT_2 = MathHelper::sqrt_float(2.0f);

float MathHelper::sin(float value)
{
    return tables().sin[static_cast<size_t>(java::f2i(value * 10430.378f) & 65535)];
}

float MathHelper::cos(float value)
{
    return tables().sin[static_cast<size_t>(java::f2i(value * 10430.378f + 16384.0f) & 65535)];
}

float MathHelper::sinTableEntry(int index)
{
    return tables().sin[static_cast<size_t>(index & 65535)];
}

float MathHelper::sqrt_float(float value)
{
    return static_cast<float>(std::sqrt(static_cast<double>(value)));
}

float MathHelper::sqrt_double(double value)
{
    return static_cast<float>(std::sqrt(value));
}

int MathHelper::floor_float(float value)
{
    const int i = java::f2i(value);
    return value < static_cast<float>(i) ? i - 1 : i;
}

int MathHelper::truncateDoubleToInt(double value)
{
    return java::sub(java::d2i(value + 1024.0), 1024);
}

int MathHelper::floor_double(double value)
{
    const int i = java::d2i(value);
    return value < static_cast<double>(i) ? i - 1 : i;
}

int64_t MathHelper::floor_double_long(double value)
{
    const int64_t i = java::d2l(value);
    return value < static_cast<double>(i) ? i - 1 : i;
}

int MathHelper::func_154353_e(double value)
{
    return java::d2i(value >= 0.0 ? value : -value + 1.0);
}

float MathHelper::abs(float value)
{
    // Java: value >= 0.0F ? value : -value, which returns -0.0F unchanged. The optimizer rewrites that
    // expression as fabs() and loses the sign, so spell it out on the bits.
    const auto bits = std::bit_cast<uint32_t>(value);
    if (bits == 0x80000000u)
        return value;
    return std::bit_cast<float>(bits & 0x7FFFFFFFu);
}

int MathHelper::abs_int(int value)
{
    return value >= 0 ? value : java::neg(value);
}

int MathHelper::ceiling_float_int(float value)
{
    const int i = java::f2i(value);
    return value > static_cast<float>(i) ? i + 1 : i;
}

int MathHelper::ceiling_double_int(double value)
{
    const int i = java::d2i(value);
    return value > static_cast<double>(i) ? i + 1 : i;
}

double MathHelper::denormalizeClamp(double lowerBnd, double upperBnd, double slide)
{
    return slide < 0.0 ? lowerBnd : (slide > 1.0 ? upperBnd : lowerBnd + (upperBnd - lowerBnd) * slide);
}

double MathHelper::abs_max(double a, double b)
{
    if (a < 0.0)
        a = -a;
    if (b < 0.0)
        b = -b;
    return a > b ? a : b;
}

int MathHelper::bucketInt(int value, int bucketSize)
{
    return value < 0 ? java::sub(java::neg(java::div(java::sub(java::neg(value), 1), bucketSize)), 1) : java::div(value, bucketSize);
}

int MathHelper::getRandomIntegerInRange(Random& rand, int min, int max)
{
    return min >= max ? min : java::add(rand.nextInt(java::add(java::sub(max, min), 1)), min);
}

float MathHelper::randomFloatClamp(Random& rand, float min, float max)
{
    return min >= max ? min : rand.nextFloat() * (max - min) + min;
}

double MathHelper::getRandomDoubleInRange(Random& rand, double min, double max)
{
    return min >= max ? min : rand.nextDouble() * (max - min) + min;
}

double MathHelper::average(std::span<const int64_t> values)
{
    int64_t sum = 0;
    for (const int64_t value : values)
        sum = java::add(sum, value);
    return static_cast<double>(sum) / static_cast<double>(values.size());
}

bool MathHelper::epsilonEquals(float a, float b)
{
    return abs(b - a) < 1.0e-5f;
}

int MathHelper::normalizeAngle(int angle, int modulus)
{
    return java::rem(java::add(java::rem(angle, modulus), modulus), modulus);
}

float MathHelper::wrapAngleTo180_float(float value)
{
    value = std::fmod(value, 360.0f);
    if (value >= 180.0f)
        value -= 360.0f;
    if (value < -180.0f)
        value += 360.0f;
    return value;
}

double MathHelper::wrapAngleTo180_double(double value)
{
    value = std::fmod(value, 360.0);
    if (value >= 180.0)
        value -= 360.0;
    if (value < -180.0)
        value += 360.0;
    return value;
}

int MathHelper::parseIntWithDefault(std::string_view text, int defaultValue)
{
    int value = 0;
    return parseJavaInt(text, value) ? value : defaultValue;
}

int MathHelper::parseIntWithDefaultAndMax(std::string_view text, int defaultValue, int max)
{
    // Despite the name, Java returns Math.max(max, value).
    const int value = parseIntWithDefault(text, defaultValue);
    return max > value ? max : value;
}

double MathHelper::parseDoubleWithDefault(std::string_view text, double defaultValue)
{
    double value = 0.0;
    return parseJavaDouble(text, value) ? value : defaultValue;
}

double MathHelper::parseDoubleWithDefaultAndMax(std::string_view text, double defaultValue, double max)
{
    return javaMax(max, parseDoubleWithDefault(text, defaultValue));
}

int MathHelper::roundUpToPowerOfTwo(int value)
{
    int i = java::sub(value, 1);
    i = i | (i >> 1);
    i = i | (i >> 2);
    i = i | (i >> 4);
    i = i | (i >> 8);
    i = i | (i >> 16);
    return java::add(i, 1);
}

int MathHelper::calculateLogBaseTwoDeBruijn(int value)
{
    value = isPowerOfTwo(value) ? value : roundUpToPowerOfTwo(value);
    return multiplyDeBruijnBitPosition[static_cast<size_t>(java::l2i(java::mul(static_cast<int64_t>(value), 125613361LL) >> 27) & 31)];
}

int MathHelper::calculateLogBaseTwo(int value)
{
    return calculateLogBaseTwoDeBruijn(value) - (isPowerOfTwo(value) ? 0 : 1);
}

int MathHelper::roundUp(int number, int interval)
{
    if (interval == 0)
        return 0;
    if (number == 0)
        return interval;

    if (number < 0)
        interval = java::mul(interval, -1);

    const int i = java::rem(number, interval);
    return i == 0 ? number : java::sub(java::add(number, interval), i);
}

int MathHelper::func_180183_b(float red, float green, float blue)
{
    return func_180181_b(floor_float(red * 255.0f), floor_float(green * 255.0f), floor_float(blue * 255.0f));
}

int MathHelper::func_180181_b(int red, int green, int blue)
{
    int color = java::add(java::shl(red, 8), green);
    color = java::add(java::shl(color, 8), blue);
    return color;
}

int MathHelper::func_180188_d(int color1, int color2)
{
    const int i = (color1 & 16711680) >> 16;
    const int j = (color2 & 16711680) >> 16;
    const int k = (color1 & 65280) >> 8;
    const int l = (color2 & 65280) >> 8;
    const int i1 = color1 & 255;
    const int j1 = color2 & 255;
    const int k1 = java::f2i(static_cast<float>(i) * static_cast<float>(j) / 255.0f);
    const int l1 = java::f2i(static_cast<float>(k) * static_cast<float>(l) / 255.0f);
    const int i2 = java::f2i(static_cast<float>(i1) * static_cast<float>(j1) / 255.0f);
    return (color1 & -16777216) | (k1 << 16) | (l1 << 8) | i2;
}

double MathHelper::func_181162_h(double value)
{
    return value - std::floor(value);
}

int64_t MathHelper::getCoordinateRandom(int x, int y, int z)
{
    int64_t i = static_cast<int64_t>(java::mul(x, 3129871)) ^ java::mul(static_cast<int64_t>(z), 116129781LL) ^ static_cast<int64_t>(y);
    i = java::add(java::mul(java::mul(i, i), 42317861LL), java::mul(i, 11LL));
    return i;
}

double MathHelper::func_181160_c(double value, double min, double max)
{
    return (value - min) / (max - min);
}

double MathHelper::atan2(double y, double x)
{
    const double d0 = x * x + y * y;
    if (d0 != d0)
        return std::numeric_limits<double>::quiet_NaN();

    const bool flag = y < 0.0;
    if (flag)
        y = -y;
    const bool flag1 = x < 0.0;
    if (flag1)
        x = -x;
    const bool flag2 = y > x;
    if (flag2) {
        const double d1 = x;
        x = y;
        y = d1;
    }

    const Tables& t = tables();
    const double d9 = func_181161_i(d0);
    x = x * d9;
    y = y * d9;
    const double d2 = t.atanOffset + y;
    const int i = java::l2i(java::doubleToRawLongBits(d2));
    const double d3 = t.asin[static_cast<size_t>(i)];
    const double d4 = t.cosOfAsin[static_cast<size_t>(i)];
    const double d5 = d2 - t.atanOffset;
    const double d6 = y * d4 - x * d5;
    const double d7 = (6.0 + d6 * d6) * d6 * 0.16666666666666666;
    double d8 = d3 + d7;

    if (flag2)
        d8 = (std::numbers::pi / 2.0) - d8;
    if (flag1)
        d8 = std::numbers::pi - d8;
    if (flag)
        d8 = -d8;
    return d8;
}

double MathHelper::func_181161_i(double value)
{
    const double d0 = 0.5 * value;
    int64_t i = java::doubleToRawLongBits(value);
    i = 6910469410427058090LL - (i >> 1);
    value = java::longBitsToDouble(i);
    value = value * (1.5 - d0 * value * value);
    return value;
}

int MathHelper::hsvToRGB(float hue, float saturation, float value)
{
    const int i = java::rem(java::f2i(hue * 6.0f), 6);
    const float f = hue * 6.0f - static_cast<float>(i);
    const float f1 = value * (1.0f - saturation);
    const float f2 = value * (1.0f - f * saturation);
    const float f3 = value * (1.0f - (1.0f - f) * saturation);
    float f4;
    float f5;
    float f6;

    switch (i) {
    case 0: f4 = value; f5 = f3; f6 = f1; break;
    case 1: f4 = f2; f5 = value; f6 = f1; break;
    case 2: f4 = f1; f5 = value; f6 = f3; break;
    case 3: f4 = f1; f5 = f2; f6 = value; break;
    case 4: f4 = f3; f5 = f1; f6 = value; break;
    case 5: f4 = value; f5 = f1; f6 = f2; break;
    default:
        throw std::runtime_error("Something went wrong when converting from HSV to RGB. Input was "
            + std::to_string(hue) + ", " + std::to_string(saturation) + ", " + std::to_string(value));
    }

    const int j = clamp_int(java::f2i(f4 * 255.0f), 0, 255);
    const int k = clamp_int(java::f2i(f5 * 255.0f), 0, 255);
    const int l = clamp_int(java::f2i(f6 * 255.0f), 0, 255);
    return j << 16 | k << 8 | l;
}

} // namespace mc
