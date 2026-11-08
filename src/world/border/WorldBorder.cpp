#include "world/border/WorldBorder.h"

#include "core/JavaTime.h"
#include "util/BlockPos.h"
#include "util/Vec3.h"
#include "world/ChunkCoordIntPair.h"

#include <algorithm>
#include <cmath>

namespace mc {

bool WorldBorder::contains(const BlockPos& pos) const
{
    return static_cast<double>(pos.getX() + 1) > minX() && static_cast<double>(pos.getX()) < maxX() && static_cast<double>(pos.getZ() + 1) > minZ()
        && static_cast<double>(pos.getZ()) < maxZ();
}

bool WorldBorder::contains(const ChunkCoordIntPair& range) const
{
    return static_cast<double>(range.getXEnd()) > minX() && static_cast<double>(range.getXStart()) < maxX() && static_cast<double>(range.getZEnd()) > minZ()
        && static_cast<double>(range.getZStart()) < maxZ();
}

bool WorldBorder::contains(const AxisAlignedBB& bb) const
{
    return bb.maxX > minX() && bb.minX < maxX() && bb.maxZ > minZ() && bb.minZ < maxZ();
}

double WorldBorder::getClosestDistance(double x, double z) const
{
    const double d0 = z - minZ();
    const double d1 = maxZ() - z;
    const double d2 = x - minX();
    const double d3 = maxX() - x;
    double d4 = std::min(d2, d3);
    d4 = std::min(d4, d0);
    return std::min(d4, d1);
}

EnumBorderStatus WorldBorder::getStatus() const
{
    return endDiameter < startDiameter ? EnumBorderStatus::SHRINKING : (endDiameter > startDiameter ? EnumBorderStatus::GROWING : EnumBorderStatus::STATIONARY);
}

double WorldBorder::minX() const
{
    double d0 = getCenterX() - diameter() / 2.0;
    if (d0 < static_cast<double>(-worldSize))
        d0 = static_cast<double>(-worldSize);
    return d0;
}

double WorldBorder::minZ() const
{
    double d0 = getCenterZ() - diameter() / 2.0;
    if (d0 < static_cast<double>(-worldSize))
        d0 = static_cast<double>(-worldSize);
    return d0;
}

double WorldBorder::maxX() const
{
    double d0 = getCenterX() + diameter() / 2.0;
    if (d0 > static_cast<double>(worldSize))
        d0 = static_cast<double>(worldSize);
    return d0;
}

double WorldBorder::maxZ() const
{
    double d0 = getCenterZ() + diameter() / 2.0;
    if (d0 > static_cast<double>(worldSize))
        d0 = static_cast<double>(worldSize);
    return d0;
}

void WorldBorder::setCenter(double x, double z)
{
    centerX = x;
    centerZ = z;
    for (IBorderListener* listener : std::vector<IBorderListener*>(listeners))
        listener->onCenterChanged(*this, x, z);
}

double WorldBorder::getDiameter()
{
    if (getStatus() != EnumBorderStatus::STATIONARY) {
        const double d0 = static_cast<double>(static_cast<float>(java::currentTimeMillis() - startTime) / static_cast<float>(endTime - startTime));
        if (d0 < 1.0)
            return startDiameter + (endDiameter - startDiameter) * d0;
        setTransition(endDiameter);
    }
    return startDiameter;
}

int64_t WorldBorder::getTimeUntilTarget() const
{
    return getStatus() != EnumBorderStatus::STATIONARY ? endTime - java::currentTimeMillis() : 0;
}

void WorldBorder::setTransition(double newSize)
{
    startDiameter = newSize;
    endDiameter = newSize;
    endTime = java::currentTimeMillis();
    startTime = endTime;
    for (IBorderListener* listener : std::vector<IBorderListener*>(listeners))
        listener->onSizeChanged(*this, newSize);
}

void WorldBorder::setTransition(double oldSize, double newSize, int64_t time)
{
    startDiameter = oldSize;
    endDiameter = newSize;
    startTime = java::currentTimeMillis();
    endTime = startTime + time;
    for (IBorderListener* listener : std::vector<IBorderListener*>(listeners))
        listener->onTransitionStarted(*this, oldSize, newSize, time);
}

void WorldBorder::setDamageBuffer(double bufferSize)
{
    damageBuffer = bufferSize;
    for (IBorderListener* listener : std::vector<IBorderListener*>(listeners))
        listener->onDamageBufferChanged(*this, bufferSize);
}

void WorldBorder::setDamageAmount(double newAmount)
{
    damageAmount = newAmount;
    for (IBorderListener* listener : std::vector<IBorderListener*>(listeners))
        listener->onDamageAmountChanged(*this, newAmount);
}

double WorldBorder::getResizeSpeed() const
{
    return endTime == startTime ? 0.0 : std::abs(startDiameter - endDiameter) / static_cast<double>(endTime - startTime);
}

void WorldBorder::setWarningTime(int time)
{
    warningTime = time;
    for (IBorderListener* listener : std::vector<IBorderListener*>(listeners))
        listener->onWarningTimeChanged(*this, time);
}

void WorldBorder::setWarningDistance(int distance)
{
    warningDistance = distance;
    for (IBorderListener* listener : std::vector<IBorderListener*>(listeners))
        listener->onWarningDistanceChanged(*this, distance);
}

} // namespace mc
