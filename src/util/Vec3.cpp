#include "util/Vec3.h"

#include "core/JavaString.h"
#include "util/MathHelper.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace mc {

// --- Vec3 ---

Vec3 Vec3::normalize() const
{
    const double d0 = static_cast<double>(MathHelper::sqrt_double(xCoord * xCoord + yCoord * yCoord + zCoord * zCoord));
    return d0 < 1.0E-4 ? Vec3(0.0, 0.0, 0.0) : Vec3(xCoord / d0, yCoord / d0, zCoord / d0);
}

Vec3 Vec3::crossProduct(const Vec3& vec) const
{
    return Vec3(yCoord * vec.zCoord - zCoord * vec.yCoord, zCoord * vec.xCoord - xCoord * vec.zCoord, xCoord * vec.yCoord - yCoord * vec.xCoord);
}

double Vec3::distanceTo(const Vec3& vec) const
{
    return static_cast<double>(MathHelper::sqrt_double(squareDistanceTo(vec)));
}

double Vec3::squareDistanceTo(const Vec3& vec) const
{
    const double d0 = vec.xCoord - xCoord;
    const double d1 = vec.yCoord - yCoord;
    const double d2 = vec.zCoord - zCoord;
    return d0 * d0 + d1 * d1 + d2 * d2;
}

double Vec3::lengthVector() const
{
    return static_cast<double>(MathHelper::sqrt_double(xCoord * xCoord + yCoord * yCoord + zCoord * zCoord));
}

std::optional<Vec3> Vec3::getIntermediateWithXValue(const Vec3& vec, double x) const
{
    const double d0 = vec.xCoord - xCoord;
    const double d1 = vec.yCoord - yCoord;
    const double d2 = vec.zCoord - zCoord;
    if (d0 * d0 < 1.0000000116860974E-7)
        return std::nullopt;
    const double d3 = (x - xCoord) / d0;
    if (d3 >= 0.0 && d3 <= 1.0)
        return Vec3(xCoord + d0 * d3, yCoord + d1 * d3, zCoord + d2 * d3);
    return std::nullopt;
}

std::optional<Vec3> Vec3::getIntermediateWithYValue(const Vec3& vec, double y) const
{
    const double d0 = vec.xCoord - xCoord;
    const double d1 = vec.yCoord - yCoord;
    const double d2 = vec.zCoord - zCoord;
    if (d1 * d1 < 1.0000000116860974E-7)
        return std::nullopt;
    const double d3 = (y - yCoord) / d1;
    if (d3 >= 0.0 && d3 <= 1.0)
        return Vec3(xCoord + d0 * d3, yCoord + d1 * d3, zCoord + d2 * d3);
    return std::nullopt;
}

std::optional<Vec3> Vec3::getIntermediateWithZValue(const Vec3& vec, double z) const
{
    const double d0 = vec.xCoord - xCoord;
    const double d1 = vec.yCoord - yCoord;
    const double d2 = vec.zCoord - zCoord;
    if (d2 * d2 < 1.0000000116860974E-7)
        return std::nullopt;
    const double d3 = (z - zCoord) / d2;
    if (d3 >= 0.0 && d3 <= 1.0)
        return Vec3(xCoord + d0 * d3, yCoord + d1 * d3, zCoord + d2 * d3);
    return std::nullopt;
}

Vec3 Vec3::rotatePitch(float pitch) const
{
    const float f = MathHelper::cos(pitch);
    const float f1 = MathHelper::sin(pitch);
    const double d0 = xCoord;
    const double d1 = yCoord * static_cast<double>(f) + zCoord * static_cast<double>(f1);
    const double d2 = zCoord * static_cast<double>(f) - yCoord * static_cast<double>(f1);
    return Vec3(d0, d1, d2);
}

Vec3 Vec3::rotateYaw(float yaw) const
{
    const float f = MathHelper::cos(yaw);
    const float f1 = MathHelper::sin(yaw);
    const double d0 = xCoord * static_cast<double>(f) + zCoord * static_cast<double>(f1);
    const double d1 = yCoord;
    const double d2 = zCoord * static_cast<double>(f) - xCoord * static_cast<double>(f1);
    return Vec3(d0, d1, d2);
}

std::string Vec3::toString() const
{
    return "(" + java::toString(xCoord) + ", " + java::toString(yCoord) + ", " + java::toString(zCoord) + ")";
}

// --- AxisAlignedBB ---

namespace {

// Math.min/max on doubles: NaN wins, and -0.0 is smaller than 0.0.
double javaMin(double a, double b)
{
    if (a != a)
        return a;
    if (a == 0.0 && b == 0.0)
        return std::signbit(a) ? a : b;
    return a <= b ? a : b;
}

double javaMax(double a, double b)
{
    if (a != a)
        return a;
    if (a == 0.0 && b == 0.0)
        return std::signbit(a) ? b : a;
    return a >= b ? a : b;
}

} // namespace

AxisAlignedBB::AxisAlignedBB(double x1, double y1, double z1, double x2, double y2, double z2)
    : minX(javaMin(x1, x2))
    , minY(javaMin(y1, y2))
    , minZ(javaMin(z1, z2))
    , maxX(javaMax(x1, x2))
    , maxY(javaMax(y1, y2))
    , maxZ(javaMax(z1, z2))
{
}

AxisAlignedBB::AxisAlignedBB(const BlockPos& pos1, const BlockPos& pos2)
    : minX(pos1.x)
    , minY(pos1.y)
    , minZ(pos1.z)
    , maxX(pos2.x)
    , maxY(pos2.y)
    , maxZ(pos2.z)
{
}

AxisAlignedBB AxisAlignedBB::addCoord(double x, double y, double z) const
{
    double d0 = minX, d1 = minY, d2 = minZ, d3 = maxX, d4 = maxY, d5 = maxZ;
    if (x < 0.0)
        d0 += x;
    else if (x > 0.0)
        d3 += x;
    if (y < 0.0)
        d1 += y;
    else if (y > 0.0)
        d4 += y;
    if (z < 0.0)
        d2 += z;
    else if (z > 0.0)
        d5 += z;
    return AxisAlignedBB(d0, d1, d2, d3, d4, d5);
}

AxisAlignedBB AxisAlignedBB::expand(double x, double y, double z) const
{
    return AxisAlignedBB(minX - x, minY - y, minZ - z, maxX + x, maxY + y, maxZ + z);
}

AxisAlignedBB AxisAlignedBB::unionWith(const AxisAlignedBB& other) const
{
    return AxisAlignedBB(javaMin(minX, other.minX), javaMin(minY, other.minY), javaMin(minZ, other.minZ), javaMax(maxX, other.maxX),
        javaMax(maxY, other.maxY), javaMax(maxZ, other.maxZ));
}

AxisAlignedBB AxisAlignedBB::offset(double x, double y, double z) const
{
    return AxisAlignedBB(minX + x, minY + y, minZ + z, maxX + x, maxY + y, maxZ + z);
}

double AxisAlignedBB::calculateXOffset(const AxisAlignedBB& other, double offsetX) const
{
    if (other.maxY > minY && other.minY < maxY && other.maxZ > minZ && other.minZ < maxZ) {
        if (offsetX > 0.0 && other.maxX <= minX) {
            const double d1 = minX - other.maxX;
            if (d1 < offsetX)
                offsetX = d1;
        } else if (offsetX < 0.0 && other.minX >= maxX) {
            const double d0 = maxX - other.minX;
            if (d0 > offsetX)
                offsetX = d0;
        }
    }
    return offsetX;
}

double AxisAlignedBB::calculateYOffset(const AxisAlignedBB& other, double offsetY) const
{
    if (other.maxX > minX && other.minX < maxX && other.maxZ > minZ && other.minZ < maxZ) {
        if (offsetY > 0.0 && other.maxY <= minY) {
            const double d1 = minY - other.maxY;
            if (d1 < offsetY)
                offsetY = d1;
        } else if (offsetY < 0.0 && other.minY >= maxY) {
            const double d0 = maxY - other.minY;
            if (d0 > offsetY)
                offsetY = d0;
        }
    }
    return offsetY;
}

double AxisAlignedBB::calculateZOffset(const AxisAlignedBB& other, double offsetZ) const
{
    if (other.maxX > minX && other.minX < maxX && other.maxY > minY && other.minY < maxY) {
        if (offsetZ > 0.0 && other.maxZ <= minZ) {
            const double d1 = minZ - other.maxZ;
            if (d1 < offsetZ)
                offsetZ = d1;
        } else if (offsetZ < 0.0 && other.minZ >= maxZ) {
            const double d0 = maxZ - other.minZ;
            if (d0 > offsetZ)
                offsetZ = d0;
        }
    }
    return offsetZ;
}

bool AxisAlignedBB::intersectsWith(const AxisAlignedBB& other) const
{
    return other.maxX > minX && other.minX < maxX && other.maxY > minY && other.minY < maxY && other.maxZ > minZ && other.minZ < maxZ;
}

bool AxisAlignedBB::isVecInside(const Vec3& vec) const
{
    return vec.xCoord > minX && vec.xCoord < maxX && vec.yCoord > minY && vec.yCoord < maxY && vec.zCoord > minZ && vec.zCoord < maxZ;
}

double AxisAlignedBB::getAverageEdgeLength() const
{
    const double d0 = maxX - minX;
    const double d1 = maxY - minY;
    const double d2 = maxZ - minZ;
    return (d0 + d1 + d2) / 3.0;
}

AxisAlignedBB AxisAlignedBB::contract(double x, double y, double z) const
{
    return AxisAlignedBB(minX + x, minY + y, minZ + z, maxX - x, maxY - y, maxZ - z);
}

std::optional<MovingObjectPosition> AxisAlignedBB::calculateIntercept(const Vec3& vecA, const Vec3& vecB) const
{
    const auto inYZ = [&](const std::optional<Vec3>& v) { return v && v->yCoord >= minY && v->yCoord <= maxY && v->zCoord >= minZ && v->zCoord <= maxZ; };
    const auto inXZ = [&](const std::optional<Vec3>& v) { return v && v->xCoord >= minX && v->xCoord <= maxX && v->zCoord >= minZ && v->zCoord <= maxZ; };
    const auto inXY = [&](const std::optional<Vec3>& v) { return v && v->xCoord >= minX && v->xCoord <= maxX && v->yCoord >= minY && v->yCoord <= maxY; };

    std::array<std::optional<Vec3>, 6> candidates {
        vecA.getIntermediateWithXValue(vecB, minX), vecA.getIntermediateWithXValue(vecB, maxX),
        vecA.getIntermediateWithYValue(vecB, minY), vecA.getIntermediateWithYValue(vecB, maxY),
        vecA.getIntermediateWithZValue(vecB, minZ), vecA.getIntermediateWithZValue(vecB, maxZ),
    };
    if (!inYZ(candidates[0])) candidates[0].reset();
    if (!inYZ(candidates[1])) candidates[1].reset();
    if (!inXZ(candidates[2])) candidates[2].reset();
    if (!inXZ(candidates[3])) candidates[3].reset();
    if (!inXY(candidates[4])) candidates[4].reset();
    if (!inXY(candidates[5])) candidates[5].reset();

    // Java compares the chosen vector by identity, so the earliest candidate wins ties.
    int chosen = -1;
    for (int i = 0; i < 6; ++i) {
        if (candidates[i] && (chosen < 0 || vecA.squareDistanceTo(*candidates[i]) < vecA.squareDistanceTo(*candidates[chosen])))
            chosen = i;
    }
    if (chosen < 0)
        return std::nullopt;

    constexpr EnumFacing sides[6] { EnumFacing::WEST, EnumFacing::EAST, EnumFacing::DOWN, EnumFacing::UP, EnumFacing::NORTH, EnumFacing::SOUTH };
    return MovingObjectPosition(*candidates[chosen], sides[chosen]);
}

std::string AxisAlignedBB::toString() const
{
    return "box[" + java::toString(minX) + ", " + java::toString(minY) + ", " + java::toString(minZ) + " -> " + java::toString(maxX) + ", "
        + java::toString(maxY) + ", " + java::toString(maxZ) + "]";
}

bool AxisAlignedBB::hasNaN() const
{
    return minX != minX || minY != minY || minZ != minZ || maxX != maxX || maxY != maxY || maxZ != maxZ;
}

} // namespace mc
