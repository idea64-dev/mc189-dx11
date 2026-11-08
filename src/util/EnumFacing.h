#pragma once

// Original: net/minecraft/util/EnumFacing.java (with Axis, AxisDirection and Plane)
// Java enums become enum classes with free functions: facing.getOpposite() is getOpposite(facing). The
// enumerators keep Java's ordinal order, so values() order and ordinals match.

#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace mc {

class Random;
struct Vec3i;

enum class EnumFacing { DOWN, UP, NORTH, SOUTH, WEST, EAST };

enum class Axis { X, Y, Z };
enum class AxisDirection { POSITIVE, NEGATIVE };
enum class Plane { HORIZONTAL, VERTICAL };

inline constexpr std::array<EnumFacing, 6> kFacings { EnumFacing::DOWN, EnumFacing::UP, EnumFacing::NORTH, EnumFacing::SOUTH,
    EnumFacing::WEST, EnumFacing::EAST };
// Java: HORIZONTALS, indexed by getHorizontalIndex (south, west, north, east).
inline constexpr std::array<EnumFacing, 4> kHorizontals { EnumFacing::SOUTH, EnumFacing::WEST, EnumFacing::NORTH, EnumFacing::EAST };
// Java: iterating EnumFacing.Plane.HORIZONTAL (its facings(): north, east, south, west).
inline constexpr std::array<EnumFacing, 4> kPlaneHorizontal { EnumFacing::NORTH, EnumFacing::EAST, EnumFacing::SOUTH, EnumFacing::WEST };
inline constexpr std::array<Axis, 3> kAxes { Axis::X, Axis::Y, Axis::Z };

constexpr int ordinal(EnumFacing facing) { return static_cast<int>(facing); }
// Same as the ordinal.
constexpr int getIndex(EnumFacing facing) { return static_cast<int>(facing); }

constexpr int getHorizontalIndex(EnumFacing facing)
{
    constexpr int table[6] { -1, -1, 2, 0, 1, 3 };
    return table[ordinal(facing)];
}

constexpr Axis getAxis(EnumFacing facing)
{
    constexpr Axis table[6] { Axis::Y, Axis::Y, Axis::Z, Axis::Z, Axis::X, Axis::X };
    return table[ordinal(facing)];
}

constexpr AxisDirection getAxisDirection(EnumFacing facing)
{
    return ordinal(facing) % 2 == 0 ? AxisDirection::NEGATIVE : AxisDirection::POSITIVE;
}

constexpr int getOffset(AxisDirection direction)
{
    return direction == AxisDirection::POSITIVE ? 1 : -1;
}

constexpr EnumFacing getOpposite(EnumFacing facing)
{
    constexpr EnumFacing table[6] { EnumFacing::UP, EnumFacing::DOWN, EnumFacing::SOUTH, EnumFacing::NORTH, EnumFacing::EAST, EnumFacing::WEST };
    return table[ordinal(facing)];
}

constexpr int getFrontOffsetX(EnumFacing facing) { return getAxis(facing) == Axis::X ? getOffset(getAxisDirection(facing)) : 0; }
constexpr int getFrontOffsetY(EnumFacing facing) { return getAxis(facing) == Axis::Y ? getOffset(getAxisDirection(facing)) : 0; }
constexpr int getFrontOffsetZ(EnumFacing facing) { return getAxis(facing) == Axis::Z ? getOffset(getAxisDirection(facing)) : 0; }

constexpr std::string_view getName(EnumFacing facing)
{
    constexpr std::string_view table[6] { "down", "up", "north", "south", "west", "east" };
    return table[ordinal(facing)];
}

constexpr std::string_view getName(Axis axis)
{
    constexpr std::string_view table[3] { "x", "y", "z" };
    return table[static_cast<int>(axis)];
}

// Java: AxisDirection.toString()
constexpr std::string_view getDescription(AxisDirection direction)
{
    return direction == AxisDirection::POSITIVE ? "Towards positive" : "Towards negative";
}

constexpr Plane getPlane(Axis axis) { return axis == Axis::Y ? Plane::VERTICAL : Plane::HORIZONTAL; }
constexpr bool isVertical(Axis axis) { return getPlane(axis) == Plane::VERTICAL; }
constexpr bool isHorizontal(Axis axis) { return getPlane(axis) == Plane::HORIZONTAL; }

// Clockwise around Y; throws for UP and DOWN like Java's IllegalStateException.
EnumFacing rotateY(EnumFacing facing);
EnumFacing rotateYCCW(EnumFacing facing);
EnumFacing rotateAround(EnumFacing facing, Axis axis);

// VALUES[abs(index % 6)]
EnumFacing getFront(int index);
// HORIZONTALS[abs(index % 4)]
EnumFacing getHorizontal(int index);
EnumFacing fromAngle(double angle);
EnumFacing randomFacing(Random& rand);
EnumFacing getFacingFromVector(float x, float y, float z);
EnumFacing getFacingFromAxis(AxisDirection direction, Axis axis);
// Lower-cased lookup; nullopt when unknown.
std::optional<EnumFacing> facingByName(std::string_view name);
std::optional<Axis> axisByName(std::string_view name);
Vec3i getDirectionVec(EnumFacing facing);

// Plane.facings(): horizontal is north, east, south, west; vertical is up, down.
std::array<EnumFacing, 4> horizontalFacings();
EnumFacing randomFacing(Plane plane, Random& rand);

} // namespace mc
