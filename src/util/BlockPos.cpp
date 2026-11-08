#include "util/BlockPos.h"

#include "util/MathHelper.h"
#include "util/Random.h"
#include "util/Vec3.h"

#include <algorithm>
#include <cctype>
#include <limits>

namespace mc {

// --- Vec3i ---

Vec3i::Vec3i(double xIn, double yIn, double zIn)
    : Vec3i(MathHelper::floor_double(xIn), MathHelper::floor_double(yIn), MathHelper::floor_double(zIn))
{
}

Vec3i Vec3i::crossProduct(const Vec3i& vec) const
{
    return Vec3i(java::sub(java::mul(y, vec.z), java::mul(z, vec.y)), java::sub(java::mul(z, vec.x), java::mul(x, vec.z)),
        java::sub(java::mul(x, vec.y), java::mul(y, vec.x)));
}

double Vec3i::distanceSq(double toX, double toY, double toZ) const
{
    const double d0 = static_cast<double>(x) - toX;
    const double d1 = static_cast<double>(y) - toY;
    const double d2 = static_cast<double>(z) - toZ;
    return d0 * d0 + d1 * d1 + d2 * d2;
}

double Vec3i::distanceSqToCenter(double xIn, double yIn, double zIn) const
{
    const double d0 = static_cast<double>(x) + 0.5 - xIn;
    const double d1 = static_cast<double>(y) + 0.5 - yIn;
    const double d2 = static_cast<double>(z) + 0.5 - zIn;
    return d0 * d0 + d1 * d1 + d2 * d2;
}

std::string Vec3i::toString() const
{
    return "Vec3i{x=" + std::to_string(x) + ", y=" + std::to_string(y) + ", z=" + std::to_string(z) + "}";
}

// --- BlockPos ---

namespace {

constexpr int NUM_X_BITS = 26; // 1 + log2(roundUpToPowerOfTwo(30000000))
constexpr int NUM_Z_BITS = NUM_X_BITS;
constexpr int NUM_Y_BITS = 64 - NUM_X_BITS - NUM_Z_BITS;
constexpr int Y_SHIFT = NUM_Z_BITS;
constexpr int X_SHIFT = Y_SHIFT + NUM_Y_BITS;
constexpr int64_t X_MASK = (1LL << NUM_X_BITS) - 1;
constexpr int64_t Y_MASK = (1LL << NUM_Y_BITS) - 1;
constexpr int64_t Z_MASK = (1LL << NUM_Z_BITS) - 1;

} // namespace

BlockPos::BlockPos(const Vec3& source)
    : BlockPos(source.xCoord, source.yCoord, source.zCoord)
{
}

BlockPos BlockPos::add(double dx, double dy, double dz) const
{
    return dx == 0.0 && dy == 0.0 && dz == 0.0 ? *this : BlockPos(static_cast<double>(x) + dx, static_cast<double>(y) + dy, static_cast<double>(z) + dz);
}

int64_t BlockPos::toLong() const
{
    return java::shl(static_cast<int64_t>(x) & X_MASK, X_SHIFT) | java::shl(static_cast<int64_t>(y) & Y_MASK, Y_SHIFT) | (static_cast<int64_t>(z) & Z_MASK);
}

BlockPos BlockPos::fromLong(int64_t serialized)
{
    const auto i = static_cast<int32_t>(java::shr(java::shl(serialized, 64 - X_SHIFT - NUM_X_BITS), 64 - NUM_X_BITS));
    const auto j = static_cast<int32_t>(java::shr(java::shl(serialized, 64 - Y_SHIFT - NUM_Y_BITS), 64 - NUM_Y_BITS));
    const auto k = static_cast<int32_t>(java::shr(java::shl(serialized, 64 - NUM_Z_BITS), 64 - NUM_Z_BITS));
    return BlockPos(i, j, k);
}

std::string BlockPos::toString() const
{
    return "BlockPos{x=" + std::to_string(x) + ", y=" + std::to_string(y) + ", z=" + std::to_string(z) + "}";
}

BlockPosBox::BlockPosBox(const BlockPos& from, const BlockPos& to)
    : min(std::min(from.x, to.x), std::min(from.y, to.y), std::min(from.z, to.z))
    , max(std::max(from.x, to.x), std::max(from.y, to.y), std::max(from.z, to.z))
{
}

BlockPosBox::iterator& BlockPosBox::iterator::operator++()
{
    if (pos == range->max) {
        done = true;
        return *this;
    }
    if (pos.x < range->max.x) {
        ++pos.x;
    } else if (pos.y < range->max.y) {
        pos.x = range->min.x;
        ++pos.y;
    } else if (pos.z < range->max.z) {
        pos.x = range->min.x;
        pos.y = range->min.y;
        ++pos.z;
    }
    return *this;
}

// --- EnumFacing (needs Vec3i and Random) ---

EnumFacing rotateY(EnumFacing facing)
{
    switch (facing) {
    case EnumFacing::NORTH: return EnumFacing::EAST;
    case EnumFacing::EAST: return EnumFacing::SOUTH;
    case EnumFacing::SOUTH: return EnumFacing::WEST;
    case EnumFacing::WEST: return EnumFacing::NORTH;
    default: throw std::logic_error("Unable to get Y-rotated facing of " + std::string(getName(facing)));
    }
}

EnumFacing rotateYCCW(EnumFacing facing)
{
    switch (facing) {
    case EnumFacing::NORTH: return EnumFacing::WEST;
    case EnumFacing::EAST: return EnumFacing::NORTH;
    case EnumFacing::SOUTH: return EnumFacing::EAST;
    case EnumFacing::WEST: return EnumFacing::SOUTH;
    default: throw std::logic_error("Unable to get CCW facing of " + std::string(getName(facing)));
    }
}

EnumFacing rotateAround(EnumFacing facing, Axis axis)
{
    switch (axis) {
    case Axis::X:
        if (facing == EnumFacing::WEST || facing == EnumFacing::EAST)
            return facing;
        switch (facing) {
        case EnumFacing::NORTH: return EnumFacing::DOWN;
        case EnumFacing::SOUTH: return EnumFacing::UP;
        case EnumFacing::UP: return EnumFacing::NORTH;
        default: return EnumFacing::SOUTH; // DOWN
        }
    case Axis::Y:
        return facing == EnumFacing::UP || facing == EnumFacing::DOWN ? facing : rotateY(facing);
    case Axis::Z:
        if (facing == EnumFacing::NORTH || facing == EnumFacing::SOUTH)
            return facing;
        switch (facing) {
        case EnumFacing::EAST: return EnumFacing::DOWN;
        case EnumFacing::WEST: return EnumFacing::UP;
        case EnumFacing::UP: return EnumFacing::EAST;
        default: return EnumFacing::WEST; // DOWN
        }
    }
    throw std::logic_error("Unable to get CW facing for axis");
}

EnumFacing getFront(int index)
{
    return kFacings[static_cast<size_t>(MathHelper::abs_int(java::rem(index, 6)))];
}

EnumFacing getHorizontal(int index)
{
    return kHorizontals[static_cast<size_t>(MathHelper::abs_int(java::rem(index, 4)))];
}

EnumFacing fromAngle(double angle)
{
    return getHorizontal(MathHelper::floor_double(angle / 90.0 + 0.5) & 3);
}

EnumFacing randomFacing(Random& rand)
{
    return kFacings[static_cast<size_t>(rand.nextInt(6))];
}

EnumFacing getFacingFromVector(float vx, float vy, float vz)
{
    EnumFacing enumfacing = EnumFacing::NORTH;
    float f = std::numeric_limits<float>::denorm_min(); // Java's Float.MIN_VALUE
    for (const EnumFacing facing : kFacings) {
        const float f1 = vx * static_cast<float>(getFrontOffsetX(facing)) + vy * static_cast<float>(getFrontOffsetY(facing))
            + vz * static_cast<float>(getFrontOffsetZ(facing));
        if (f1 > f) {
            f = f1;
            enumfacing = facing;
        }
    }
    return enumfacing;
}

EnumFacing getFacingFromAxis(AxisDirection direction, Axis axis)
{
    for (const EnumFacing facing : kFacings) {
        if (getAxisDirection(facing) == direction && getAxis(facing) == axis)
            return facing;
    }
    throw std::invalid_argument("No such direction");
}

namespace {

std::string lower(std::string_view name)
{
    std::string s(name);
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

} // namespace

std::optional<EnumFacing> facingByName(std::string_view name)
{
    const std::string s = lower(name);
    for (const EnumFacing facing : kFacings) {
        if (getName(facing) == s)
            return facing;
    }
    return std::nullopt;
}

std::optional<Axis> axisByName(std::string_view name)
{
    const std::string s = lower(name);
    for (const Axis axis : kAxes) {
        if (getName(axis) == s)
            return axis;
    }
    return std::nullopt;
}

Vec3i getDirectionVec(EnumFacing facing)
{
    return Vec3i(getFrontOffsetX(facing), getFrontOffsetY(facing), getFrontOffsetZ(facing));
}

std::array<EnumFacing, 4> horizontalFacings()
{
    return { EnumFacing::NORTH, EnumFacing::EAST, EnumFacing::SOUTH, EnumFacing::WEST };
}

EnumFacing randomFacing(Plane plane, Random& rand)
{
    if (plane == Plane::HORIZONTAL)
        return horizontalFacings()[static_cast<size_t>(rand.nextInt(4))];
    constexpr EnumFacing vertical[2] { EnumFacing::UP, EnumFacing::DOWN };
    return vertical[rand.nextInt(2)];
}

} // namespace mc
