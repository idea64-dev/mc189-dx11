#pragma once

// Original: net/minecraft/world/border/WorldBorder.java, EnumBorderStatus.java and IBorderListener.java
// getClosestDistance(Entity) comes with entities (stage 10).

#include <cstdint>
#include <vector>

namespace mc {

struct BlockPos;
struct ChunkCoordIntPair;
struct AxisAlignedBB;
class WorldBorder;

enum class EnumBorderStatus { GROWING, SHRINKING, STATIONARY };

// Java: EnumBorderStatus.getID() -- the color the border is drawn in.
constexpr int getBorderStatusColor(EnumBorderStatus status)
{
    constexpr int colors[3] { 4259712, 16724016, 2138367 };
    return colors[static_cast<int>(status)];
}

class IBorderListener {
public:
    virtual ~IBorderListener() = default;
    virtual void onSizeChanged(WorldBorder& border, double newSize) = 0;
    virtual void onTransitionStarted(WorldBorder& border, double oldSize, double newSize, int64_t time) = 0;
    virtual void onCenterChanged(WorldBorder& border, double x, double z) = 0;
    virtual void onWarningTimeChanged(WorldBorder& border, int newTime) = 0;
    virtual void onWarningDistanceChanged(WorldBorder& border, int newDistance) = 0;
    virtual void onDamageAmountChanged(WorldBorder& border, double newAmount) = 0;
    virtual void onDamageBufferChanged(WorldBorder& border, double newSize) = 0;
};

class WorldBorder {
public:
    virtual ~WorldBorder() = default;

    bool contains(const BlockPos& pos) const;
    bool contains(const ChunkCoordIntPair& range) const;
    bool contains(const AxisAlignedBB& bb) const;
    double getClosestDistance(double x, double z) const;
    EnumBorderStatus getStatus() const;
    double minX() const;
    double minZ() const;
    double maxX() const;
    double maxZ() const;
    // Virtual for the Nether, whose border center is scaled by 1/8.
    virtual double getCenterX() const { return centerX; }
    virtual double getCenterZ() const { return centerZ; }
    void setCenter(double x, double z);
    // Interpolates a running transition by the wall clock; once it has run out, the target becomes the size.
    double getDiameter();
    int64_t getTimeUntilTarget() const;
    double getTargetSize() const { return endDiameter; }
    void setTransition(double newSize);
    void setTransition(double oldSize, double newSize, int64_t time);
    void addListener(IBorderListener* listener) { listeners.push_back(listener); }
    void setSize(int size) { worldSize = size; }
    int getSize() const { return worldSize; }
    double getDamageBuffer() const { return damageBuffer; }
    void setDamageBuffer(double bufferSize);
    double getDamageAmount() const { return damageAmount; }
    void setDamageAmount(double newAmount);
    double getResizeSpeed() const;
    int getWarningTime() const { return warningTime; }
    void setWarningTime(int time);
    int getWarningDistance() const { return warningDistance; }
    void setWarningDistance(int distance);

private:
    // The diameter as getDiameter() reports it, for the const queries (Java calls getDiameter() there too).
    double diameter() const { return const_cast<WorldBorder*>(this)->getDiameter(); }

    std::vector<IBorderListener*> listeners;
    double centerX = 0.0;
    double centerZ = 0.0;
    double startDiameter = 6.0E7;
    double endDiameter = 6.0E7;
    int64_t endTime = 0;
    int64_t startTime = 0;
    int worldSize = 29999984;
    double damageAmount = 0.2;
    double damageBuffer = 5.0;
    int warningTime = 15;
    int warningDistance = 5;
};

} // namespace mc
