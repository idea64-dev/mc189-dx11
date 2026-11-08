#pragma once

// Helpers for Java enums ported as enum classes: values() in ordinal order, and EnumTraits for those used as
// block state properties.

#include <array>
#include <cstddef>

namespace mc {

template <class E, size_t N>
constexpr std::array<E, N> enumValues()
{
    std::array<E, N> values {};
    for (size_t i = 0; i < N; ++i)
        values[i] = static_cast<E>(i);
    return values;
}

template <class E>
constexpr int ordinalOf(E value)
{
    return static_cast<int>(value);
}

} // namespace mc

// Specializes mc::EnumTraits for enum E with N values; NAME is an expression of `value` giving getName().
#define MC_ENUM_TRAITS(E, N, NAME)                                                                       \
    template <>                                                                                          \
    struct mc::EnumTraits<E> {                                                                           \
        static constexpr std::array<E, N> all = mc::enumValues<E, N>();                                  \
        static constexpr std::span<const E> values() { return all; }                                     \
        static std::string_view getName(E value) { return NAME; }                                        \
    };
