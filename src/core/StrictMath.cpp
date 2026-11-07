#include "core/StrictMath.h"

#include <bit>
#include <cstdint>
#include <limits>

namespace mc {

namespace {

int32_t highWord(double x) { return static_cast<int32_t>(std::bit_cast<uint64_t>(x) >> 32); }
uint32_t lowWord(double x) { return static_cast<uint32_t>(std::bit_cast<uint64_t>(x)); }

double withHighWord(double x, int32_t high)
{
    const uint64_t bits = (static_cast<uint64_t>(static_cast<uint32_t>(high)) << 32) | lowWord(x);
    return std::bit_cast<double>(bits);
}

constexpr double ln2_hi = 6.93147180369123816490e-01; // 3fe62e42 fee00000
constexpr double ln2_lo = 1.90821492927058770002e-10; // 3dea39ef 35793c76
constexpr double two54 = 1.80143985094819840000e+16;  // 43500000 00000000
constexpr double Lg1 = 6.666666666666735130e-01;      // 3FE55555 55555593
constexpr double Lg2 = 3.999999999940941908e-01;      // 3FD99999 9997FA04
constexpr double Lg3 = 2.857142874366239149e-01;      // 3FD24924 94229359
constexpr double Lg4 = 2.222219843214978396e-01;      // 3FCC71C5 1D8E78AF
constexpr double Lg5 = 1.818357216161805012e-01;      // 3FC74664 96CB03DE
constexpr double Lg6 = 1.531383769920937332e-01;      // 3FC39A09 D078C69F
constexpr double Lg7 = 1.479819860511658591e-01;      // 3FC2F112 DF3E5244

} // namespace

// fdlibm e_log.c (__ieee754_log), as used by java.lang.StrictMath.log.
double StrictMath::log(double x)
{
    int32_t hx = highWord(x);
    const uint32_t lx = lowWord(x);
    int32_t k = 0;

    if (hx < 0x00100000) { // x < 2**-1022
        if (((hx & 0x7fffffff) | static_cast<int32_t>(lx)) == 0)
            return -std::numeric_limits<double>::infinity(); // log(+-0) = -inf
        if (hx < 0)
            return std::numeric_limits<double>::quiet_NaN(); // log(-#) = NaN
        k -= 54;
        x *= two54; // subnormal number, scale up x
        hx = highWord(x);
    }
    if (hx >= 0x7ff00000)
        return x + x;

    k += (hx >> 20) - 1023;
    hx &= 0x000fffff;
    int32_t i = (hx + 0x95f64) & 0x100000;
    x = withHighWord(x, hx | (i ^ 0x3ff00000)); // normalize x or x/2
    k += (i >> 20);
    const double f = x - 1.0;

    if ((0x000fffff & (2 + hx)) < 3) { // |f| < 2**-20
        if (f == 0.0) {
            if (k == 0)
                return 0.0;
            const double dk = static_cast<double>(k);
            return dk * ln2_hi + dk * ln2_lo;
        }
        const double R = f * f * (0.5 - 0.33333333333333333 * f);
        if (k == 0)
            return f - R;
        const double dk = static_cast<double>(k);
        return dk * ln2_hi - ((R - dk * ln2_lo) - f);
    }

    const double s = f / (2.0 + f);
    const double dk = static_cast<double>(k);
    const double z = s * s;
    i = hx - 0x6147a;
    const double w = z * z;
    const int32_t j = 0x6b851 - hx;
    const double t1 = w * (Lg2 + w * (Lg4 + w * Lg6));
    const double t2 = z * (Lg1 + w * (Lg3 + w * (Lg5 + w * Lg7)));
    i |= j;
    const double R = t2 + t1;

    if (i > 0) {
        const double hfsq = 0.5 * f * f;
        if (k == 0)
            return f - (hfsq - s * (hfsq + R));
        return dk * ln2_hi - ((hfsq - (s * (hfsq + R) + dk * ln2_lo)) - f);
    }
    if (k == 0)
        return f - s * (f - R);
    return dk * ln2_hi - ((s * (f - R) - dk * ln2_lo) - f);
}

} // namespace mc
