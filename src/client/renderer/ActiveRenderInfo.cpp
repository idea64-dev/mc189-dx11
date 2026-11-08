#include "client/renderer/ActiveRenderInfo.h"

#include "block/BlockLiquid.h"
#include "block/material/Material.h"
#include "block/state/BlockState.h"
#include "client/renderer/DxStateManager.h"
#include "entity/Entity.h"
#include "entity/player/EntityPlayer.h"
#include "render/gl/GL.h"
#include "render/gl/Project.h"
#include "util/MathHelper.h"
#include "world/World.h"

namespace mc {

void ActiveRenderInfo::updateRenderInfo(EntityPlayer* entityplayerIn, bool thirdPersonFront)
{
    float modelview[16];
    float projection[16];
    int viewport[4];
    DxStateManager::getFloat(2982, modelview);
    DxStateManager::getFloat(2983, projection);
    GL::glGetIntegerv(GL::GL_VIEWPORT, viewport);
    const float f = static_cast<float>((viewport[0] + viewport[2]) / 2);
    const float f1 = static_cast<float>((viewport[1] + viewport[3]) / 2);
    // Java leaves the previous coordinates in the buffer when the matrix cannot be inverted.
    static float objectCoords[3] {};
    Project::gluUnProject(f, f1, 0.0f, modelview, projection, viewport, objectCoords);
    position = Vec3(static_cast<double>(objectCoords[0]), static_cast<double>(objectCoords[1]), static_cast<double>(objectCoords[2]));

    const int i = thirdPersonFront ? 1 : 0;
    const float f2 = entityplayerIn->rotationPitch;
    const float f3 = entityplayerIn->rotationYaw;
    const auto pi = static_cast<float>(3.141592653589793);
    rotationX = MathHelper::cos(f3 * pi / 180.0f) * static_cast<float>(1 - i * 2);
    rotationZ = MathHelper::sin(f3 * pi / 180.0f) * static_cast<float>(1 - i * 2);
    rotationYZ = -rotationZ * MathHelper::sin(f2 * pi / 180.0f) * static_cast<float>(1 - i * 2);
    rotationXY = rotationX * MathHelper::sin(f2 * pi / 180.0f) * static_cast<float>(1 - i * 2);
    rotationXZ = MathHelper::cos(f2 * pi / 180.0f);
}

Vec3 ActiveRenderInfo::projectViewFromEntity(const Entity* entity, double partialTicks)
{
    const double d0 = entity->prevPosX + (entity->posX - entity->prevPosX) * partialTicks;
    const double d1 = entity->prevPosY + (entity->posY - entity->prevPosY) * partialTicks;
    const double d2 = entity->prevPosZ + (entity->posZ - entity->prevPosZ) * partialTicks;
    return Vec3(d0 + position.xCoord, d1 + position.yCoord, d2 + position.zCoord);
}

Block* ActiveRenderInfo::getBlockAtEntityViewpoint(World* worldIn, const Entity* entity, float partialTicks)
{
    const Vec3 vec3 = projectViewFromEntity(entity, static_cast<double>(partialTicks));
    const BlockPos blockpos(vec3.xCoord, vec3.yCoord, vec3.zCoord);
    const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
    Block* block = iblockstate->getBlock();

    if (block->getMaterial().isLiquid()) {
        float f = 0.0f;
        if (dynamic_cast<BlockLiquid*>(iblockstate->getBlock()) != nullptr)
            f = BlockLiquid::getLiquidHeightPercent(iblockstate->getValue(BlockLiquid::LEVEL)) - 0.11111111f;
        const float f1 = static_cast<float>(blockpos.getY() + 1) - f;
        if (vec3.yCoord >= static_cast<double>(f1))
            block = worldIn->getBlockState(blockpos.up())->getBlock();
    }
    return block;
}

} // namespace mc
