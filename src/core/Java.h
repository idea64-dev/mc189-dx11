#pragma once

// Java language semantics that C++ does not give for free. Gameplay and world generation depend on them
// bit for bit: two's-complement wrapping, masked shift counts, saturating float->int casts, String.hashCode.

#include <bit>
#include <cstdint>
#include <limits>
#include <string_view>

namespace mc::java {

// --- Wrapping integer arithmetic (signed overflow is undefined in C++, defined in Java) ---

constexpr int32_t add(int32_t a, int32_t b) { return static_cast<int32_t>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b)); }
constexpr int32_t sub(int32_t a, int32_t b) { return static_cast<int32_t>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b)); }
constexpr int32_t mul(int32_t a, int32_t b) { return static_cast<int32_t>(static_cast<uint32_t>(a) * static_cast<uint32_t>(b)); }
constexpr int32_t neg(int32_t a) { return static_cast<int32_t>(0u - static_cast<uint32_t>(a)); }

constexpr int64_t add(int64_t a, int64_t b) { return static_cast<int64_t>(static_cast<uint64_t>(a) + static_cast<uint64_t>(b)); }
constexpr int64_t sub(int64_t a, int64_t b) { return static_cast<int64_t>(static_cast<uint64_t>(a) - static_cast<uint64_t>(b)); }
constexpr int64_t mul(int64_t a, int64_t b) { return static_cast<int64_t>(static_cast<uint64_t>(a) * static_cast<uint64_t>(b)); }
constexpr int64_t neg(int64_t a) { return static_cast<int64_t>(0ull - static_cast<uint64_t>(a)); }

// Java division: Integer.MIN_VALUE / -1 == Integer.MIN_VALUE (C++ traps). Division by zero is the caller's problem.
constexpr int32_t div(int32_t a, int32_t b) { return (b == -1) ? neg(a) : a / b; }
constexpr int32_t rem(int32_t a, int32_t b) { return (b == -1) ? 0 : a % b; }
constexpr int64_t div(int64_t a, int64_t b) { return (b == -1) ? neg(a) : a / b; }
constexpr int64_t rem(int64_t a, int64_t b) { return (b == -1) ? 0 : a % b; }

// --- Shifts: Java masks the count (x << 33 == x << 1 for int); >>> is the logical shift ---

constexpr int32_t shl(int32_t value, int count) { return static_cast<int32_t>(static_cast<uint32_t>(value) << (count & 31)); }
constexpr int32_t shr(int32_t value, int count) { return value >> (count & 31); }
constexpr int32_t ushr(int32_t value, int count) { return static_cast<int32_t>(static_cast<uint32_t>(value) >> (count & 31)); }
constexpr int64_t shl(int64_t value, int count) { return static_cast<int64_t>(static_cast<uint64_t>(value) << (count & 63)); }
constexpr int64_t shr(int64_t value, int count) { return value >> (count & 63); }
constexpr int64_t ushr(int64_t value, int count) { return static_cast<int64_t>(static_cast<uint64_t>(value) >> (count & 63)); }

// --- Narrowing casts with Java rules: NaN -> 0, out of range saturates (C++ leaves these undefined) ---

constexpr int32_t f2i(float value)
{
    if (value != value)
        return 0;
    if (value >= 2147483648.0f)
        return std::numeric_limits<int32_t>::max();
    if (value <= -2147483648.0f)
        return std::numeric_limits<int32_t>::min();
    return static_cast<int32_t>(value);
}

constexpr int32_t d2i(double value)
{
    if (value != value)
        return 0;
    if (value >= 2147483647.0)
        return std::numeric_limits<int32_t>::max();
    if (value <= -2147483648.0)
        return std::numeric_limits<int32_t>::min();
    return static_cast<int32_t>(value);
}

constexpr int64_t f2l(float value)
{
    if (value != value)
        return 0;
    if (value >= 9223372036854775808.0f)
        return std::numeric_limits<int64_t>::max();
    if (value <= -9223372036854775808.0f)
        return std::numeric_limits<int64_t>::min();
    return static_cast<int64_t>(value);
}

constexpr int64_t d2l(double value)
{
    if (value != value)
        return 0;
    if (value >= 9223372036854775808.0)
        return std::numeric_limits<int64_t>::max();
    if (value <= -9223372036854775808.0)
        return std::numeric_limits<int64_t>::min();
    return static_cast<int64_t>(value);
}

// (int) of a long, (short)/(byte) of an int: plain two's-complement truncation (well-defined since C++20).
constexpr int32_t l2i(int64_t value) { return static_cast<int32_t>(value); }

// --- Float bit access ---

inline int32_t floatToRawIntBits(float value) { return std::bit_cast<int32_t>(value); }
inline float intBitsToFloat(int32_t bits) { return std::bit_cast<float>(bits); }
inline int64_t doubleToRawLongBits(double value) { return std::bit_cast<int64_t>(value); }
inline double longBitsToDouble(int64_t bits) { return std::bit_cast<double>(bits); }

// --- String.hashCode() over UTF-16 code units ---

constexpr int32_t hashCode(std::u16string_view text)
{
    int32_t hash = 0;
    for (const char16_t unit : text)
        hash = add(mul(31, hash), static_cast<int32_t>(unit));
    return hash;
}

// Same, for UTF-8 input (decoded to UTF-16 code units on the fly, surrogate pairs included).
int32_t hashCode(std::string_view utf8);

} // namespace mc::java
