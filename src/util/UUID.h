#pragma once

// java.util.UUID: two longs, Java's toString/fromString/hashCode/equals and nameUUIDFromBytes (version 3, MD5),
// plus MathHelper.getRandomUuid.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace mc {

class Random;

struct UUID {
    int64_t mostSigBits = 0;
    int64_t leastSigBits = 0;

    constexpr UUID() = default;
    constexpr UUID(int64_t most, int64_t least)
        : mostSigBits(most)
        , leastSigBits(least)
    {
    }

    int64_t getMostSignificantBits() const { return mostSigBits; }
    int64_t getLeastSignificantBits() const { return leastSigBits; }
    // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx", lower case.
    std::string toString() const;
    int32_t hashCode() const
    {
        const int64_t hilo = mostSigBits ^ leastSigBits;
        return static_cast<int32_t>(hilo >> 32) ^ static_cast<int32_t>(hilo);
    }
    constexpr bool operator==(const UUID&) const = default;
    constexpr auto operator<=>(const UUID&) const = default;

    // Java: UUID.fromString -- nullopt where Java throws IllegalArgumentException.
    static std::optional<UUID> fromString(std::string_view text);
    // Java: UUID.nameUUIDFromBytes
    static UUID nameUUIDFromBytes(std::string_view bytes);
    // Java: MathHelper.getRandomUuid -- a version 4 UUID from two nextLong draws.
    static UUID getRandomUuid(Random& rand);
};

} // namespace mc
