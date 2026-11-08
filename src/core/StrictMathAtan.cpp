// fdlibm 5.3 s_atan.c and e_atan2.c (as in java.lang.StrictMath), translated literally.

#include "core/StrictMath.h"

#include <bit>
#include <cstdint>

namespace mc {

namespace {

int32_t hiWord(double x)
{
    return static_cast<int32_t>(std::bit_cast<uint64_t>(x) >> 32);
}

uint32_t loWord(double x)
{
    return static_cast<uint32_t>(std::bit_cast<uint64_t>(x));
}

double withHiWord(double x, int32_t hi)
{
    return std::bit_cast<double>((static_cast<uint64_t>(static_cast<uint32_t>(hi)) << 32) | loWord(x));
}

constexpr double atanhi[] {
    4.63647609000806093515e-01, // atan(0.5)hi
    7.85398163397448278999e-01, // atan(1.0)hi
    9.82793723247329054082e-01, // atan(1.5)hi
    1.57079632679489655800e+00, // atan(inf)hi
};

constexpr double atanlo[] {
    2.26987774529616870924e-17,
    3.06161699786838301793e-17,
    1.39033110312309984516e-17,
    6.12323399573676603587e-17,
};

constexpr double aT[] {
    3.33333333333329318027e-01,
    -1.99999999998764832476e-01,
    1.42857142725034663711e-01,
    -1.11111104054623557880e-01,
    9.09088713343650656196e-02,
    -7.69187620504482999495e-02,
    6.66107313738753120669e-02,
    -5.83357013379057348645e-02,
    4.97687799461593236017e-02,
    -3.65315727442169155270e-02,
    1.62858201153657823623e-02,
};

constexpr double one = 1.0;
constexpr double huge = 1.0e300;
constexpr double tiny = 1.0e-300;
constexpr double zero = 0.0;
constexpr double pi_o_4 = 7.8539816339744827900E-01;
constexpr double pi_o_2 = 1.5707963267948965580E+00;
constexpr double pi = 3.1415926535897931160E+00;
constexpr double pi_lo = 1.2246467991473531772E-16;

} // namespace

double StrictMath::atan(double x)
{
    const int32_t hx = hiWord(x);
    const int32_t ix = hx & 0x7fffffff;
    int id;

    if (ix >= 0x44100000) { // |x| >= 2^66
        if (ix > 0x7ff00000 || (ix == 0x7ff00000 && loWord(x) != 0))
            return x + x; // NaN
        if (hx > 0)
            return atanhi[3] + atanlo[3];
        return -atanhi[3] - atanlo[3];
    }
    if (ix < 0x3fdc0000) { // |x| < 0.4375
        if (ix < 0x3e200000) { // |x| < 2^-29
            if (huge + x > one)
                return x;
        }
        id = -1;
    } else {
        x = std::fabs(x);
        if (ix < 0x3ff30000) { // |x| < 1.1875
            if (ix < 0x3fe60000) { // 7/16 <= |x| < 11/16
                id = 0;
                x = (2.0 * x - one) / (2.0 + x);
            } else { // 11/16 <= |x| < 19/16
                id = 1;
                x = (x - one) / (x + one);
            }
        } else {
            if (ix < 0x40038000) { // |x| < 2.4375
                id = 2;
                x = (x - 1.5) / (one + 1.5 * x);
            } else { // 2.4375 <= |x| < 2^66
                id = 3;
                x = -1.0 / x;
            }
        }
    }

    const double z = x * x;
    const double w = z * z;
    const double s1 = z * (aT[0] + w * (aT[2] + w * (aT[4] + w * (aT[6] + w * (aT[8] + w * aT[10])))));
    const double s2 = w * (aT[1] + w * (aT[3] + w * (aT[5] + w * (aT[7] + w * aT[9]))));
    if (id < 0)
        return x - x * (s1 + s2);
    const double r = atanhi[id] - ((x * (s1 + s2) - atanlo[id]) - x);
    return hx < 0 ? -r : r;
}

double StrictMath::atan2(double y, double x)
{
    const int32_t hx = hiWord(x);
    const int32_t ix = hx & 0x7fffffff;
    const uint32_t lx = loWord(x);
    const int32_t hy = hiWord(y);
    const int32_t iy = hy & 0x7fffffff;
    const uint32_t ly = loWord(y);

    if ((static_cast<uint32_t>(ix) | ((lx | (0u - lx)) >> 31)) > 0x7ff00000u || (static_cast<uint32_t>(iy) | ((ly | (0u - ly)) >> 31)) > 0x7ff00000u)
        return x + y; // NaN
    if (((static_cast<uint32_t>(hx) - 0x3ff00000u) | lx) == 0)
        return atan(y); // x = 1.0
    const int m = ((hy >> 31) & 1) | ((hx >> 30) & 2); // 2*sign(x)+sign(y)

    if ((static_cast<uint32_t>(iy) | ly) == 0) { // y = 0
        switch (m) {
        case 0:
        case 1: return y;
        case 2: return pi + tiny;
        default: return -pi - tiny;
        }
    }
    if ((static_cast<uint32_t>(ix) | lx) == 0) // x = 0
        return hy < 0 ? -pi_o_2 - tiny : pi_o_2 + tiny;
    if (ix == 0x7ff00000) { // x is INF
        if (iy == 0x7ff00000) {
            switch (m) {
            case 0: return pi_o_4 + tiny;
            case 1: return -pi_o_4 - tiny;
            case 2: return 3.0 * pi_o_4 + tiny;
            default: return -3.0 * pi_o_4 - tiny;
            }
        }
        switch (m) {
        case 0: return zero;
        case 1: return -zero;
        case 2: return pi + tiny;
        default: return -pi - tiny;
        }
    }
    if (iy == 0x7ff00000) // y is INF
        return hy < 0 ? -pi_o_2 - tiny : pi_o_2 + tiny;

    // compute y/x
    const int k = (iy - ix) >> 20;
    double z;
    if (k > 60)
        z = pi_o_2 + 0.5 * pi_lo; // |y/x| > 2^60
    else if (hx < 0 && k < -60)
        z = 0.0; // |y|/x < -2^60
    else
        z = atan(std::fabs(y / x));
    switch (m) {
    case 0: return z;
    case 1: return withHiWord(z, hiWord(z) ^ static_cast<int32_t>(0x80000000u));
    case 2: return pi - (z - pi_lo);
    default: return (z - pi_lo) - pi;
    }
}

} // namespace mc
