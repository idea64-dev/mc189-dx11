#pragma once

// Original: net/minecraft/world/gen/structure/StructureBoundingBox.java -- an inclusive integer box.

#include "util/BlockPos.h"
#include "util/EnumFacing.h"

#include <algorithm>
#include <climits>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class NBTTagIntArray;

struct StructureBoundingBox {
    int minX = 0;
    int minY = 0;
    int minZ = 0;
    int maxX = 0;
    int maxY = 0;
    int maxZ = 0;

    StructureBoundingBox() = default;
    // Java: StructureBoundingBox(int[]) -- leaves zeros unless there are exactly six values.
    explicit StructureBoundingBox(const std::vector<int32_t>& coords)
    {
        if (coords.size() == 6) {
            minX = coords[0];
            minY = coords[1];
            minZ = coords[2];
            maxX = coords[3];
            maxY = coords[4];
            maxZ = coords[5];
        }
    }
    StructureBoundingBox(int xMin, int yMin, int zMin, int xMax, int yMax, int zMax)
        : minX(xMin)
        , minY(yMin)
        , minZ(zMin)
        , maxX(xMax)
        , maxY(yMax)
        , maxZ(zMax)
    {
    }
    StructureBoundingBox(const Vec3i& vec1, const Vec3i& vec2)
        : minX(std::min(vec1.getX(), vec2.getX()))
        , minY(std::min(vec1.getY(), vec2.getY()))
        , minZ(std::min(vec1.getZ(), vec2.getZ()))
        , maxX(std::max(vec1.getX(), vec2.getX()))
        , maxY(std::max(vec1.getY(), vec2.getY()))
        , maxZ(std::max(vec1.getZ(), vec2.getZ()))
    {
    }
    // Java: StructureBoundingBox(xMin, zMin, xMax, zMax) -- y from 1 to 512.
    static StructureBoundingBox ofXZ(int xMin, int zMin, int xMax, int zMax) { return StructureBoundingBox(xMin, 1, zMin, xMax, 512, zMax); }

    static StructureBoundingBox getNewBoundingBox() { return StructureBoundingBox(INT_MAX, INT_MAX, INT_MAX, INT_MIN, INT_MIN, INT_MIN); }
    static StructureBoundingBox getComponentToAddBoundingBox(int structureMinX, int structureMinY, int structureMinZ, int xMin, int yMin, int zMin, int xMax,
        int yMax, int zMax, EnumFacing facing);
    static StructureBoundingBox func_175899_a(int x1, int y1, int z1, int x2, int y2, int z2)
    {
        return StructureBoundingBox(std::min(x1, x2), std::min(y1, y2), std::min(z1, z2), std::max(x1, x2), std::max(y1, y2), std::max(z1, z2));
    }

    bool intersectsWith(const StructureBoundingBox& structurebb) const
    {
        return maxX >= structurebb.minX && minX <= structurebb.maxX && maxZ >= structurebb.minZ && minZ <= structurebb.maxZ && maxY >= structurebb.minY
            && minY <= structurebb.maxY;
    }
    bool intersectsWith(int minXIn, int minZIn, int maxXIn, int maxZIn) const { return maxX >= minXIn && minX <= maxXIn && maxZ >= minZIn && minZ <= maxZIn; }
    void expandTo(const StructureBoundingBox& sbb)
    {
        minX = std::min(minX, sbb.minX);
        minY = std::min(minY, sbb.minY);
        minZ = std::min(minZ, sbb.minZ);
        maxX = std::max(maxX, sbb.maxX);
        maxY = std::max(maxY, sbb.maxY);
        maxZ = std::max(maxZ, sbb.maxZ);
    }
    void offset(int x, int y, int z)
    {
        minX += x;
        minY += y;
        minZ += z;
        maxX += x;
        maxY += y;
        maxZ += z;
    }
    bool isVecInside(const Vec3i& vec) const
    {
        return vec.getX() >= minX && vec.getX() <= maxX && vec.getZ() >= minZ && vec.getZ() <= maxZ && vec.getY() >= minY && vec.getY() <= maxY;
    }
    Vec3i func_175896_b() const { return Vec3i(maxX - minX, maxY - minY, maxZ - minZ); }
    int getXSize() const { return maxX - minX + 1; }
    int getYSize() const { return maxY - minY + 1; }
    int getZSize() const { return maxZ - minZ + 1; }
    Vec3i getCenter() const { return BlockPos(minX + (maxX - minX + 1) / 2, minY + (maxY - minY + 1) / 2, minZ + (maxZ - minZ + 1) / 2); }
    std::string toString() const;
    std::shared_ptr<NBTTagIntArray> toNBTTagIntArray() const;
};

} // namespace mc
