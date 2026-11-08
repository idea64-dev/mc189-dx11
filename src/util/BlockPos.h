#pragma once

// Original: net/minecraft/util/Vec3i.java and BlockPos.java (with MutableBlockPos)
// Value types here: Java's BlockPos is immutable and MutableBlockPos a reused instance; in C++ a BlockPos is a
// plain value and set() takes MutableBlockPos's role. Integer arithmetic wraps like Java's.

#include "core/Java.h"
#include "util/EnumFacing.h"

#include <compare>
#include <cstdint>
#include <string>

namespace mc {

struct Vec3i {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    constexpr Vec3i() = default;
    constexpr Vec3i(int32_t xIn, int32_t yIn, int32_t zIn)
        : x(xIn)
        , y(yIn)
        , z(zIn)
    {
    }
    // Floors each coordinate (MathHelper.floor_double).
    Vec3i(double xIn, double yIn, double zIn);

    constexpr int32_t getX() const { return x; }
    constexpr int32_t getY() const { return y; }
    constexpr int32_t getZ() const { return z; }

    constexpr bool operator==(const Vec3i& other) const { return x == other.x && y == other.y && z == other.z; }
    // Java: (y + z * 31) * 31 + x
    constexpr int32_t hashCode() const { return java::add(java::mul(java::add(y, java::mul(z, 31)), 31), x); }
    // Java: compareTo, by y, then z, then x (differences, which can overflow like Java's).
    constexpr int32_t compareTo(const Vec3i& other) const
    {
        return y == other.y ? (z == other.z ? java::sub(x, other.x) : java::sub(z, other.z)) : java::sub(y, other.y);
    }

    Vec3i crossProduct(const Vec3i& vec) const;
    double distanceSq(double toX, double toY, double toZ) const;
    double distanceSqToCenter(double xIn, double yIn, double zIn) const;
    double distanceSq(const Vec3i& to) const { return distanceSq(to.x, to.y, to.z); }
    // Guava's Objects.toStringHelper: "Vec3i{x=1, y=2, z=3}".
    std::string toString() const;
};

struct Vec3;

struct BlockPos : Vec3i {
    static const BlockPos ORIGIN;

    constexpr BlockPos() = default;
    constexpr BlockPos(int32_t xIn, int32_t yIn, int32_t zIn)
        : Vec3i(xIn, yIn, zIn)
    {
    }
    BlockPos(double xIn, double yIn, double zIn)
        : Vec3i(xIn, yIn, zIn)
    {
    }
    explicit constexpr BlockPos(const Vec3i& source)
        : Vec3i(source)
    {
    }
    explicit BlockPos(const Vec3& source);

    BlockPos add(double dx, double dy, double dz) const;
    constexpr BlockPos add(int32_t dx, int32_t dy, int32_t dz) const { return BlockPos(java::add(x, dx), java::add(y, dy), java::add(z, dz)); }
    constexpr BlockPos add(const Vec3i& vec) const { return add(vec.x, vec.y, vec.z); }
    constexpr BlockPos subtract(const Vec3i& vec) const { return BlockPos(java::sub(x, vec.x), java::sub(y, vec.y), java::sub(z, vec.z)); }
    constexpr BlockPos offset(EnumFacing facing, int32_t n = 1) const
    {
        return BlockPos(java::add(x, java::mul(getFrontOffsetX(facing), n)), java::add(y, java::mul(getFrontOffsetY(facing), n)),
            java::add(z, java::mul(getFrontOffsetZ(facing), n)));
    }
    constexpr BlockPos up(int32_t n = 1) const { return offset(EnumFacing::UP, n); }
    constexpr BlockPos down(int32_t n = 1) const { return offset(EnumFacing::DOWN, n); }
    constexpr BlockPos north(int32_t n = 1) const { return offset(EnumFacing::NORTH, n); }
    constexpr BlockPos south(int32_t n = 1) const { return offset(EnumFacing::SOUTH, n); }
    constexpr BlockPos west(int32_t n = 1) const { return offset(EnumFacing::WEST, n); }
    constexpr BlockPos east(int32_t n = 1) const { return offset(EnumFacing::EAST, n); }
    BlockPos crossProduct(const Vec3i& vec) const { return BlockPos(Vec3i::crossProduct(vec)); }
    // MutableBlockPos.set
    BlockPos& set(int32_t xIn, int32_t yIn, int32_t zIn)
    {
        x = xIn;
        y = yIn;
        z = zIn;
        return *this;
    }

    // 26 bits x, 12 bits y, 26 bits z.
    int64_t toLong() const;
    static BlockPos fromLong(int64_t serialized);
    std::string toString() const; // "BlockPos{x=1, y=2, z=3}"

    // Java: getAllInBox / getAllInBoxMutable -- x fastest, then y, then z, corners in any order.
    static class BlockPosBox getAllInBox(const BlockPos& from, const BlockPos& to);
};

inline constexpr BlockPos BlockPos::ORIGIN { 0, 0, 0 };

class BlockPosBox {
public:
    class iterator {
    public:
        const BlockPos& operator*() const { return pos; }
        const BlockPos* operator->() const { return &pos; }
        iterator& operator++();
        bool operator==(const iterator& other) const { return done == other.done && (done || pos == other.pos); }

    private:
        friend class BlockPosBox;
        iterator(const BlockPosBox* r, bool d)
            : range(r)
            , pos(r->min)
            , done(d)
        {
        }
        const BlockPosBox* range;
        BlockPos pos;
        bool done;
    };

    BlockPosBox(const BlockPos& from, const BlockPos& to);
    iterator begin() const { return iterator(this, false); }
    iterator end() const { return iterator(this, true); }

private:
    BlockPos min;
    BlockPos max;
};

inline BlockPosBox BlockPos::getAllInBox(const BlockPos& from, const BlockPos& to)
{
    return BlockPosBox(from, to);
}


} // namespace mc

template <>
struct std::hash<mc::BlockPos> {
    size_t operator()(const mc::BlockPos& pos) const noexcept { return static_cast<size_t>(pos.toLong()); }
};
