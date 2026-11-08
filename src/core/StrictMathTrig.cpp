// fdlibm 5.3 sin and cos, as java.lang.StrictMath runs them: s_sin.c, s_cos.c, k_sin.c, k_cos.c,
// e_rem_pio2.c and k_rem_pio2.c (only the double-double precision the others use).

#include "core/StrictMath.h"

#include <bit>
#include <cmath>
#include <cstdint>

namespace mc {

namespace {

int32_t highWord(double x) { return static_cast<int32_t>(std::bit_cast<uint64_t>(x) >> 32); }
uint32_t lowWord(double x) { return static_cast<uint32_t>(std::bit_cast<uint64_t>(x)); }

double fromWords(int32_t high, uint32_t low)
{
    return std::bit_cast<double>((static_cast<uint64_t>(static_cast<uint32_t>(high)) << 32) | low);
}

// k_sin.c
double kernelSin(double x, double y, int iy)
{
    constexpr double half = 5.00000000000000000000e-01;
    constexpr double S1 = -1.66666666666666324348e-01; // 0xBFC55555, 0x55555549
    constexpr double S2 = 8.33333333332248946124e-03; // 0x3F811111, 0x1110F8A6
    constexpr double S3 = -1.98412698298579493134e-04; // 0xBF2A01A0, 0x19C161D5
    constexpr double S4 = 2.75573137070700676789e-06; // 0x3EC71DE3, 0x57B1FE7D
    constexpr double S5 = -2.50507602534068634195e-08; // 0xBE5AE5E6, 0x8A2B9CEB
    constexpr double S6 = 1.58969099521155010221e-10; // 0x3DE5D93A, 0x5ACFD57C
    const int32_t ix = highWord(x) & 0x7fffffff;
    if (ix < 0x3e400000) { // |x| < 2**-27
        if (static_cast<int32_t>(x) == 0)
            return x;
    }
    const double z = x * x;
    const double v = z * x;
    const double r = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
    if (iy == 0)
        return x + v * (S1 + z * r);
    return x - ((z * (half * y - v * r) - y) - v * S1);
}

// k_cos.c
double kernelCos(double x, double y)
{
    constexpr double one = 1.0;
    constexpr double C1 = 4.16666666666666019037e-02; // 0x3FA55555, 0x5555554C
    constexpr double C2 = -1.38888888888741095749e-03; // 0xBF56C16C, 0x16C15177
    constexpr double C3 = 2.48015872894767294178e-05; // 0x3EFA01A0, 0x19CB1590
    constexpr double C4 = -2.75573143513906633035e-07; // 0xBE927E4F, 0x809C52AD
    constexpr double C5 = 2.08757232129817482790e-09; // 0x3E21EE9E, 0xBDB4B1C4
    constexpr double C6 = -1.13596475577881948265e-11; // 0xBDA8FAE9, 0xBE8838D4
    const int32_t ix = highWord(x) & 0x7fffffff;
    if (ix < 0x3e400000) { // |x| < 2**-27
        if (static_cast<int32_t>(x) == 0)
            return one;
    }
    const double z = x * x;
    const double r = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
    if (ix < 0x3FD33333) // |x| < 0.3
        return one - (0.5 * z - (z * r - x * y));
    double qx;
    if (ix > 0x3fe90000) // x > 0.78125
        qx = 0.28125;
    else
        qx = fromWords(ix - 0x00200000, 0); // x/4
    const double hz = 0.5 * z - qx;
    const double a = one - qx;
    return a - (hz - (z * r - x * y));
}

constexpr int32_t two_over_pi[] = {
    0xA2F983, 0x6E4E44, 0x1529FC, 0x2757D1, 0xF534DD, 0xC0DB62, 0x95993C, 0x439041, 0xFE5163, 0xABDEBB, 0xC561B7, 0x246E3A,
    0x424DD2, 0xE00649, 0x2EEA09, 0xD1921C, 0xFE1DEB, 0x1CB129, 0xA73EE8, 0x8235F5, 0x2EBB44, 0x84E99C, 0x7026B4, 0x5F7E41,
    0x3991D6, 0x398353, 0x39F49C, 0x845F8B, 0xBDF928, 0x3B1FF8, 0x97FFDE, 0x05980F, 0xEF2F11, 0x8B5A0A, 0x6D1F6D, 0x367ECF,
    0x27CB09, 0xB74F46, 0x3F669E, 0x5FEA2D, 0x7527BA, 0xC7EBE5, 0xF17B3D, 0x0739F7, 0x8A5292, 0xEA6BFB, 0x5FB11F, 0x8D5D08,
    0x560330, 0x46FC7B, 0x6BABF0, 0xCFBC20, 0x9AF436, 0x1DA9E3, 0x91615E, 0xE61B08, 0x659985, 0x5F14A0, 0x68408D, 0xFFD880,
    0x4D7327, 0x310606, 0x1556CA, 0x73A8C9, 0x60E27B, 0xC08C6B,
};

constexpr int32_t npio2_hw[] = {
    0x3FF921FB, 0x400921FB, 0x4012D97C, 0x401921FB, 0x401F6A7A, 0x4022D97C, 0x4025FDBB, 0x402921FB, 0x402C463A, 0x402F6A7A, 0x4031475C,
    0x4032D97C, 0x40346B9C, 0x4035FDBB, 0x40378FDB, 0x403921FB, 0x403AB41B, 0x403C463A, 0x403DD85A, 0x403F6A7A, 0x40407E4C, 0x4041475C,
    0x4042106C, 0x4042D97C, 0x4043A28C, 0x40446B9C, 0x404534AC, 0x4045FDBB, 0x4046C6CB, 0x40478FDB, 0x404858EB, 0x404921FB,
};

constexpr double two24 = 1.67772160000000000000e+07; // 0x41700000, 0x00000000
constexpr double twon24 = 5.96046447753906250000e-08; // 0x3E700000, 0x00000000

// k_rem_pio2.c with prec = 2
int kernelRemPio2(const double* x, double* y, int e0, int nx)
{
    constexpr double PIo2[] = {
        1.57079625129699707031e+00, // 0x3FF921FB, 0x40000000
        7.54978941586159635335e-08, // 0x3E74442D, 0x00000000
        5.39030252995776476554e-15, // 0x3CF84698, 0x80000000
        3.28200341580791294123e-22, // 0x3B78CC51, 0x60000000
        1.27065575308067607349e-29, // 0x39F01B83, 0x80000000
        1.22933308981111328932e-36, // 0x387A2520, 0x40000000
        2.73370053816464559624e-44, // 0x36E38222, 0x80000000
        2.16741683877804819444e-51, // 0x3569F31D, 0x00000000
    };
    const int32_t* ipio2 = two_over_pi;
    int32_t iq[20] {};
    double f[20] {};
    double fq[20] {};
    double q[20] {};
    const int jk = 4; // init_jk[prec]
    const int jp = jk;

    const int jx = nx - 1;
    int jv = (e0 - 3) / 24;
    if (jv < 0)
        jv = 0;
    int q0 = e0 - 24 * (jv + 1);

    int j = jv - jx;
    const int m = jx + jk;
    for (int i = 0; i <= m; i++, j++)
        f[i] = j < 0 ? 0.0 : static_cast<double>(ipio2[j]);

    for (int i = 0; i <= jk; i++) {
        double fw = 0.0;
        for (j = 0; j <= jx; j++)
            fw += x[j] * f[jx + i - j];
        q[i] = fw;
    }

    int jz = jk;
    int n;
    int ih;
    double z;
    for (;;) { // recompute:
        // distill q[] into iq[] reversingly
        int i = 0;
        z = q[jz];
        for (j = jz; j > 0; i++, j--) {
            const double fw = static_cast<double>(static_cast<int32_t>(twon24 * z));
            iq[i] = static_cast<int32_t>(z - two24 * fw);
            z = q[j - 1] + fw;
        }

        // compute n
        z = std::ldexp(z, q0); // actual value of z
        z -= 8.0 * std::floor(z * 0.125); // trim off integer >= 8
        n = static_cast<int32_t>(z);
        z -= static_cast<double>(n);
        ih = 0;
        if (q0 > 0) { // need iq[jz-1] to determine n
            i = iq[jz - 1] >> (24 - q0);
            n += i;
            iq[jz - 1] -= i << (24 - q0);
            ih = iq[jz - 1] >> (23 - q0);
        } else if (q0 == 0) {
            ih = iq[jz - 1] >> 23;
        } else if (z >= 0.5) {
            ih = 2;
        }

        if (ih > 0) { // q > 0.5
            n += 1;
            int carry = 0;
            for (i = 0; i < jz; i++) { // compute 1-q
                j = iq[i];
                if (carry == 0) {
                    if (j != 0) {
                        carry = 1;
                        iq[i] = 0x1000000 - j;
                    }
                } else {
                    iq[i] = 0xffffff - j;
                }
            }
            if (q0 > 0) { // rare case: chance is 1 in 12
                switch (q0) {
                case 1: iq[jz - 1] &= 0x7fffff; break;
                case 2: iq[jz - 1] &= 0x3fffff; break;
                default: break;
                }
            }
            if (ih == 2) {
                z = 1.0 - z;
                if (carry != 0)
                    z -= std::ldexp(1.0, q0);
            }
        }

        // check if recomputation is needed
        if (z == 0.0) {
            j = 0;
            for (i = jz - 1; i >= jk; i--)
                j |= iq[i];
            if (j == 0) {
                int k = 1;
                while (iq[jk - k] == 0)
                    k++; // k = no. of terms needed
                for (i = jz + 1; i <= jz + k; i++) { // add q[jz+1] to q[jz+k]
                    f[jx + i] = static_cast<double>(ipio2[jv + i]);
                    double fw = 0.0;
                    for (j = 0; j <= jx; j++)
                        fw += x[j] * f[jx + i - j];
                    q[i] = fw;
                }
                jz += k;
                continue;
            }
        }
        break;
    }

    // chop off zero terms
    if (z == 0.0) {
        jz -= 1;
        q0 -= 24;
        while (iq[jz] == 0) {
            jz--;
            q0 -= 24;
        }
    } else { // break z into 24-bit if necessary
        z = std::ldexp(z, -q0);
        if (z >= two24) {
            const double fw = static_cast<double>(static_cast<int32_t>(twon24 * z));
            iq[jz] = static_cast<int32_t>(z - two24 * fw);
            jz += 1;
            q0 += 24;
            iq[jz] = static_cast<int32_t>(fw);
        } else {
            iq[jz] = static_cast<int32_t>(z);
        }
    }

    // convert integer "bit" chunk to floating-point value
    double fw = std::ldexp(1.0, q0);
    for (int i = jz; i >= 0; i--) {
        q[i] = fw * static_cast<double>(iq[i]);
        fw *= twon24;
    }

    // compute PIo2[0,...,jp]*q[jz,...,0]
    for (int i = jz; i >= 0; i--) {
        fw = 0.0;
        for (int k = 0; k <= jp && k <= jz - i; k++)
            fw += PIo2[k] * q[i + k];
        fq[jz - i] = fw;
    }

    // compress fq[] into y[]
    fw = 0.0;
    for (int i = jz; i >= 0; i--)
        fw += fq[i];
    y[0] = ih == 0 ? fw : -fw;
    fw = fq[0] - fw;
    for (int i = 1; i <= jz; i++)
        fw += fq[i];
    y[1] = ih == 0 ? fw : -fw;
    return n & 7;
}

// e_rem_pio2.c
int remPio2(double x, double* y)
{
    constexpr double half = 5.00000000000000000000e-01;
    constexpr double invpio2 = 6.36619772367581382433e-01; // 0x3FE45F30, 0x6DC9C883
    constexpr double pio2_1 = 1.57079632673412561417e+00; // 0x3FF921FB, 0x54400000
    constexpr double pio2_1t = 6.07710050650619224932e-11; // 0x3DD0B461, 0x1A626331
    constexpr double pio2_2 = 6.07710050630396597660e-11; // 0x3DD0B461, 0x1A600000
    constexpr double pio2_2t = 2.02226624879595063154e-21; // 0x3BA3198A, 0x2E037073
    constexpr double pio2_3 = 2.02226624871116645580e-21; // 0x3BA3198A, 0x2E000000
    constexpr double pio2_3t = 8.47842766036889956997e-32; // 0x397B839A, 0x252049C1

    const int32_t hx = highWord(x);
    const int32_t ix = hx & 0x7fffffff;
    if (ix <= 0x3fe921fb) { // |x| ~<= pi/4, no need for reduction
        y[0] = x;
        y[1] = 0;
        return 0;
    }
    if (ix < 0x4002d97c) { // |x| < 3pi/4, special case with n=+-1
        if (hx > 0) {
            double z = x - pio2_1;
            if (ix != 0x3ff921fb) { // 33+53 bit pi is good enough
                y[0] = z - pio2_1t;
                y[1] = (z - y[0]) - pio2_1t;
            } else { // near pi/2, use 33+33+53 bit pi
                z -= pio2_2;
                y[0] = z - pio2_2t;
                y[1] = (z - y[0]) - pio2_2t;
            }
            return 1;
        }
        double z = x + pio2_1;
        if (ix != 0x3ff921fb) {
            y[0] = z + pio2_1t;
            y[1] = (z - y[0]) + pio2_1t;
        } else {
            z += pio2_2;
            y[0] = z + pio2_2t;
            y[1] = (z - y[0]) + pio2_2t;
        }
        return -1;
    }
    if (ix <= 0x413921fb) { // |x| ~<= 2^19*(pi/2), medium size
        double t = std::fabs(x);
        const int32_t n = static_cast<int32_t>(t * invpio2 + half);
        const double fn = static_cast<double>(n);
        double r = t - fn * pio2_1;
        double w = fn * pio2_1t; // 1st round good to 85 bit
        if (n < 32 && ix != npio2_hw[n - 1]) {
            y[0] = r - w; // quick check no cancellation
        } else {
            const int32_t j = ix >> 20;
            y[0] = r - w;
            int32_t i = j - ((highWord(y[0]) >> 20) & 0x7ff);
            if (i > 16) { // 2nd iteration needed, good to 118
                t = r;
                w = fn * pio2_2;
                r = t - w;
                w = fn * pio2_2t - ((t - r) - w);
                y[0] = r - w;
                i = j - ((highWord(y[0]) >> 20) & 0x7ff);
                if (i > 49) { // 3rd iteration need, 151 bits acc
                    t = r;
                    w = fn * pio2_3;
                    r = t - w;
                    w = fn * pio2_3t - ((t - r) - w);
                    y[0] = r - w;
                }
            }
        }
        y[1] = (r - y[0]) - w;
        if (hx < 0) {
            y[0] = -y[0];
            y[1] = -y[1];
            return -n;
        }
        return n;
    }
    // all other (large) arguments
    if (ix >= 0x7ff00000) { // x is inf or NaN
        y[0] = y[1] = x - x;
        return 0;
    }
    // set z = scalbn(|x|,ilogb(x)-23)
    const int32_t e0 = (ix >> 20) - 1046; // e0 = ilogb(z)-23
    double z = fromWords(ix - (e0 << 20), lowWord(x));
    double tx[3];
    for (int i = 0; i < 2; i++) {
        tx[i] = static_cast<double>(static_cast<int32_t>(z));
        z = (z - tx[i]) * two24;
    }
    tx[2] = z;
    int nx = 3;
    while (tx[nx - 1] == 0.0)
        nx--; // skip zero term
    const int n = kernelRemPio2(tx, y, e0, nx);
    if (hx < 0) {
        y[0] = -y[0];
        y[1] = -y[1];
        return -n;
    }
    return n;
}

} // namespace

double StrictMath::sin(double x)
{
    const int32_t ix = highWord(x) & 0x7fffffff;
    if (ix <= 0x3fe921fb)
        return kernelSin(x, 0.0, 0);
    if (ix >= 0x7ff00000)
        return x - x;
    double y[2];
    const int n = remPio2(x, y);
    switch (n & 3) {
    case 0: return kernelSin(y[0], y[1], 1);
    case 1: return kernelCos(y[0], y[1]);
    case 2: return -kernelSin(y[0], y[1], 1);
    default: return -kernelCos(y[0], y[1]);
    }
}

double StrictMath::cos(double x)
{
    const int32_t ix = highWord(x) & 0x7fffffff;
    if (ix <= 0x3fe921fb)
        return kernelCos(x, 0.0);
    if (ix >= 0x7ff00000)
        return x - x;
    double y[2];
    const int n = remPio2(x, y);
    switch (n & 3) {
    case 0: return kernelCos(y[0], y[1]);
    case 1: return -kernelSin(y[0], y[1], 1);
    case 2: return -kernelCos(y[0], y[1]);
    default: return kernelSin(y[0], y[1], 1);
    }
}

} // namespace mc
