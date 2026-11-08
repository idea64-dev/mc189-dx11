#pragma once

// Original: net/minecraft/util/Vec3.java, AxisAlignedBB.java and MovingObjectPosition.java
// Methods that return null in Java return std::optional here.

#include "util/BlockPos.h"

#include <optional>
#include <string>

namespace mc {

class Entity;

struct Vec3 {
    double xCoord;
    double yCoord;
    double zCoord;

    // Negative zeros become positive, as in Java's constructor.
    Vec3(double x, double y, double z)
        : xCoord(x == 0.0 ? 0.0 : x)
        , yCoord(y == 0.0 ? 0.0 : y)
        , zCoord(z == 0.0 ? 0.0 : z)
    {
    }
    explicit Vec3(const Vec3i& vec)
        : Vec3(static_cast<double>(vec.x), static_cast<double>(vec.y), static_cast<double>(vec.z))
    {
    }

    Vec3 subtractReverse(const Vec3& vec) const { return Vec3(vec.xCoord - xCoord, vec.yCoord - yCoord, vec.zCoord - zCoord); }
    Vec3 normalize() const;
    double dotProduct(const Vec3& vec) const { return xCoord * vec.xCoord + yCoord * vec.yCoord + zCoord * vec.zCoord; }
    Vec3 crossProduct(const Vec3& vec) const;
    Vec3 subtract(const Vec3& vec) const { return subtract(vec.xCoord, vec.yCoord, vec.zCoord); }
    Vec3 subtract(double x, double y, double z) const { return addVector(-x, -y, -z); }
    Vec3 add(const Vec3& vec) const { return addVector(vec.xCoord, vec.yCoord, vec.zCoord); }
    Vec3 addVector(double x, double y, double z) const { return Vec3(xCoord + x, yCoord + y, zCoord + z); }
    // Float precision square roots (MathHelper.sqrt_double), as in Java.
    double distanceTo(const Vec3& vec) const;
    double squareDistanceTo(const Vec3& vec) const;
    double lengthVector() const;
    std::optional<Vec3> getIntermediateWithXValue(const Vec3& vec, double x) const;
    std::optional<Vec3> getIntermediateWithYValue(const Vec3& vec, double y) const;
    std::optional<Vec3> getIntermediateWithZValue(const Vec3& vec, double z) const;
    // MathHelper's sine table.
    Vec3 rotatePitch(float pitch) const;
    Vec3 rotateYaw(float yaw) const;
    std::string toString() const; // "(1.0, 2.0, 3.0)"
};

struct MovingObjectPosition {
    enum class MovingObjectType { MISS, BLOCK, ENTITY };

    MovingObjectPosition(const Vec3& hitVecIn, EnumFacing facing, const BlockPos& blockPosIn)
        : MovingObjectPosition(MovingObjectType::BLOCK, hitVecIn, facing, blockPosIn)
    {
    }
    MovingObjectPosition(const Vec3& hitVecIn, EnumFacing facing)
        : MovingObjectPosition(MovingObjectType::BLOCK, hitVecIn, facing, BlockPos::ORIGIN)
    {
    }
    MovingObjectPosition(MovingObjectType typeOfHitIn, const Vec3& hitVecIn, EnumFacing sideHitIn, const BlockPos& blockPosIn)
        : blockPos(blockPosIn)
        , typeOfHit(typeOfHitIn)
        , sideHit(sideHitIn)
        , hitVec(hitVecIn)
    {
    }
    MovingObjectPosition(Entity* entityHitIn, const Vec3& hitVecIn)
        : typeOfHit(MovingObjectType::ENTITY)
        , hitVec(hitVecIn)
        , entityHit(entityHitIn)
    {
    }

    // Empty for entity hits (Java's null).
    const std::optional<BlockPos>& getBlockPos() const { return blockPos; }

    std::optional<BlockPos> blockPos;
    MovingObjectType typeOfHit;
    // Empty for entity hits.
    std::optional<EnumFacing> sideHit;
    Vec3 hitVec;
    Entity* entityHit = nullptr;
};

struct AxisAlignedBB {
    double minX;
    double minY;
    double minZ;
    double maxX;
    double maxY;
    double maxZ;

    // Sorts each pair of coordinates.
    AxisAlignedBB(double x1, double y1, double z1, double x2, double y2, double z2);
    // Takes the corners as given, unsorted (Java's BlockPos constructor).
    AxisAlignedBB(const BlockPos& pos1, const BlockPos& pos2);

    static AxisAlignedBB fromBounds(double x1, double y1, double z1, double x2, double y2, double z2)
    {
        return AxisAlignedBB(x1, y1, z1, x2, y2, z2);
    }

    AxisAlignedBB addCoord(double x, double y, double z) const;
    AxisAlignedBB expand(double x, double y, double z) const;
    AxisAlignedBB unionWith(const AxisAlignedBB& other) const; // Java: union
    AxisAlignedBB offset(double x, double y, double z) const;
    double calculateXOffset(const AxisAlignedBB& other, double offsetX) const;
    double calculateYOffset(const AxisAlignedBB& other, double offsetY) const;
    double calculateZOffset(const AxisAlignedBB& other, double offsetZ) const;
    bool intersectsWith(const AxisAlignedBB& other) const;
    bool isVecInside(const Vec3& vec) const;
    double getAverageEdgeLength() const;
    AxisAlignedBB contract(double x, double y, double z) const;
    std::optional<MovingObjectPosition> calculateIntercept(const Vec3& vecA, const Vec3& vecB) const;
    std::string toString() const; // "box[minX, minY, minZ -> maxX, maxY, maxZ]"
    bool hasNaN() const;
};

} // namespace mc
