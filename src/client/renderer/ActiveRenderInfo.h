#pragma once

// Original: net/minecraft/client/renderer/ActiveRenderInfo.java -- the camera of the frame being drawn: its
// offset from the view entity (unprojected from the screen centre) and the rotations particles face.

#include "util/Vec3.h"

namespace mc {

class Block;
class Entity;
class EntityPlayer;
class World;

class ActiveRenderInfo {
public:
    static void updateRenderInfo(EntityPlayer* entityplayerIn, bool thirdPersonFront);
    static Vec3 projectViewFromEntity(const Entity* entity, double partialTicks);
    static Block* getBlockAtEntityViewpoint(World* worldIn, const Entity* entity, float partialTicks);
    static const Vec3& getPosition() { return position; }
    static float getRotationX() { return rotationX; }
    static float getRotationXZ() { return rotationXZ; }
    static float getRotationZ() { return rotationZ; }
    static float getRotationYZ() { return rotationYZ; }
    static float getRotationXY() { return rotationXY; }

private:
    static inline Vec3 position { 0.0, 0.0, 0.0 };
    static inline float rotationX = 0.0f;
    static inline float rotationXZ = 0.0f;
    static inline float rotationZ = 0.0f;
    static inline float rotationYZ = 0.0f;
    static inline float rotationXY = 0.0f;
};

} // namespace mc
