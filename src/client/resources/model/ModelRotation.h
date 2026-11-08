#pragma once

// Original: net/minecraft/client/resources/model/ModelRotation.java
// The 16 blockstate rotations (x and y in quarter turns) with their LWJGL matrices.

#include "client/renderer/vector/LwjglVector.h"
#include "util/EnumFacing.h"

namespace mc {

class ModelRotation {
public:
    static const ModelRotation X0_Y0, X0_Y90, X0_Y180, X0_Y270, X90_Y0, X90_Y90, X90_Y180, X90_Y270, X180_Y0, X180_Y90, X180_Y180, X180_Y270, X270_Y0,
        X270_Y90, X270_Y180, X270_Y270;

    const lwjgl::Matrix4f& getMatrix4d() const { return matrix4d; }
    EnumFacing rotateFace(EnumFacing facing) const;
    int rotateVertex(EnumFacing facing, int vertexIndex) const;
    int ordinal() const { return ordinalIndex; }
    // Null for angles that are not multiples of 90.
    static const ModelRotation* getModelRotation(int x, int y);

private:
    ModelRotation(int ordinalIn, int x, int y);

    int ordinalIndex;
    int combinedXY;
    lwjgl::Matrix4f matrix4d;
    int quartersX;
    int quartersY;
};

} // namespace mc
