#include "client/renderer/EntityRenderer.h"

#include "block/BlockBed.h"
#include "block/material/Material.h"
#include "block/state/BlockState.h"
#include "client/Minecraft.h"
#include "client/entity/EntityPlayerSP.h"
#include "client/gui/GuiIngame.h"
#include "client/gui/GuiScreen.h"
#include "client/gui/ScaledResolution.h"
#include "client/multiplayer/PlayerControllerMP.h"
#include "client/multiplayer/WorldClient.h"
#include "client/renderer/ActiveRenderInfo.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/OpenGlHelper.h"
#include "client/renderer/RenderGlobal.h"
#include "client/renderer/RenderHelper.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/WorldRenderer.h"
#include "client/renderer/culling/Frustum.h"
#include "client/renderer/texture/SimpleTexture.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/texture/TextureMap.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/settings/GameSettings.h"
#include "client/shader/Framebuffer.h"
#include "core/Java.h"
#include "enchantment/EnchantmentHelper.h"
#include "entity/Entity.h"
#include "entity/EntityLivingBase.h"
#include "entity/player/EntityPlayer.h"
#include "init/Blocks.h"
#include "item/ItemStack.h"
#include "platform/Display.h"
#include "platform/Mouse.h"
#include "platform/Sys.h"
#include "potion/Potion.h"
#include "tileentity/TileEntity.h"
#include "render/gl/GL.h"
#include "render/gl/Project.h"
#include "util/MathHelper.h"
#include "world/WorldProvider.h"
#include "world/WorldSettings.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/WorldChunkManager.h"

#include <algorithm>
#include <cmath>

namespace mc {

namespace {

constexpr float kPi = 3.1415927f; // (float)Math.PI

const ResourceLocation& locationRainPng()
{
    static const ResourceLocation location("textures/environment/rain.png");
    return location;
}

const ResourceLocation& locationSnowPng()
{
    static const ResourceLocation location("textures/environment/snow.png");
    return location;
}

// Java: Math.random(), one shared generator.
double mathRandom()
{
    static Random random;
    return random.nextDouble();
}

// Java: EnchantmentHelper.getRespiration(entity) -- the best respiration level on the entity's equipment.
int respirationLevel(Entity* entity)
{
    const std::vector<std::shared_ptr<ItemStack>> inventory = entity->getInventory();
    std::vector<const ItemStack*> stacks;
    for (const std::shared_ptr<ItemStack>& stack : inventory)
        stacks.push_back(stack.get());
    return EnchantmentHelper::getMaxEnchantmentLevel(Enchantment::respiration->effectId, stacks);
}

// Java: NV_fog_distance, present on NVIDIA drivers: setupFog then fogs by the distance to the eye instead of by
// depth. Kept on, as on the reference machines.
constexpr bool kNvFogDistance = true;

} // namespace

// --- MouseFilter ---

float MouseFilter::smooth(float value, float smoothing)
{
    field_76336_a += value;
    value = (field_76336_a - field_76334_b) * smoothing;
    field_76335_c += (value - field_76335_c) * 0.5f;
    if ((value > 0.0f && value > field_76335_c) || (value < 0.0f && value < field_76335_c))
        value = field_76335_c;
    field_76334_b += value;
    return value;
}

void MouseFilter::reset()
{
    field_76336_a = 0.0f;
    field_76334_b = 0.0f;
    field_76335_c = 0.0f;
}

// --- EntityRenderer ---

EntityRenderer::EntityRenderer(Minecraft& minecraft)
    : mc(minecraft)
    , prevFrameTime(Minecraft::getSystemTime())
    , lightmapTexture(std::make_shared<DynamicTexture>(16, 16))
    , locationLightMap(minecraft.getTextureManager().getDynamicTextureLocation("lightMap", lightmapTexture))
{
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; ++j) {
            const float f = static_cast<float>(j - 16);
            const float f1 = static_cast<float>(i - 16);
            const float f2 = MathHelper::sqrt_float(f * f + f1 * f1);
            rainXCoords[static_cast<size_t>(i << 5 | j)] = -f1 / f2;
            rainYCoords[static_cast<size_t>(i << 5 | j)] = f / f2;
        }
    }
}

EntityRenderer::~EntityRenderer() = default;

void EntityRenderer::updateRenderer()
{
    updateFovModifierHand();
    updateTorchFlicker();
    fogColor2 = fogColor1;
    thirdPersonDistanceTemp = thirdPersonDistance;

    if (mc.gameSettings->smoothCamera) {
        const float f = mc.gameSettings->mouseSensitivity * 0.6f + 0.2f;
        const float f1 = f * f * f * 8.0f;
        smoothCamFilterX = mouseFilterXAxis.smooth(smoothCamYaw, 0.05f * f1);
        smoothCamFilterY = mouseFilterYAxis.smooth(smoothCamPitch, 0.05f * f1);
        smoothCamPartialTicks = 0.0f;
        smoothCamYaw = 0.0f;
        smoothCamPitch = 0.0f;
    } else {
        smoothCamFilterX = 0.0f;
        smoothCamFilterY = 0.0f;
        mouseFilterXAxis.reset();
        mouseFilterYAxis.reset();
    }

    if (mc.getRenderViewEntity() == nullptr)
        mc.setRenderViewEntity(mc.thePlayer.get());

    Entity* view = mc.getRenderViewEntity();
    const float f3 = mc.theWorld->getLightBrightness(BlockPos(view->posX, view->posY, view->posZ));
    const float f4 = static_cast<float>(mc.gameSettings->renderDistanceChunks) / 32.0f;
    const float f2 = f3 * (1.0f - f4) + f4;
    fogColor1 += (f2 - fogColor1) * 0.1f;
    ++rendererUpdateCount;
    // Java: itemRenderer.updateEquippedItem() (stage 10), addRainParticles() (stages 13 and 15).
    bossColorModifierPrev = bossColorModifier;
    // Java: BossStatus.hasColorModifier (the wither and dragon bars, stage 12).
    if (bossColorModifier > 0.0f)
        bossColorModifier -= 0.0125f;
}

void EntityRenderer::getMouseOver(float partialTicks)
{
    Entity* entity = mc.getRenderViewEntity();
    if (entity == nullptr || mc.theWorld == nullptr)
        return;

    mc.pointedEntity = nullptr;
    double d0 = static_cast<double>(mc.playerController->getBlockReachDistance());
    mc.objectMouseOver = entity->rayTrace(d0, partialTicks);
    double d1 = d0;
    const Vec3 vec3 = entity->getPositionEyes(partialTicks);
    bool flag = false;

    if (mc.playerController->extendedReach()) {
        d0 = 6.0;
        d1 = 6.0;
    } else if (d0 > 3.0) {
        flag = true;
    }

    if (mc.objectMouseOver)
        d1 = mc.objectMouseOver->hitVec.distanceTo(vec3);

    const Vec3 vec31 = entity->getLook(partialTicks);
    const Vec3 vec32 = vec3.addVector(vec31.xCoord * d0, vec31.yCoord * d0, vec31.zCoord * d0);
    pointedEntity = nullptr;
    std::optional<Vec3> vec33;
    const float f = 1.0f;
    const std::vector<Entity*> list = mc.theWorld->getEntitiesInAABBexcluding(entity,
        entity->getEntityBoundingBox().addCoord(vec31.xCoord * d0, vec31.yCoord * d0, vec31.zCoord * d0).expand(f, f, f),
        [](Entity* e) {
            // Java: EntitySelectors.NOT_SPECTATING and canBeCollidedWith.
            auto* player = dynamic_cast<EntityPlayer*>(e);
            return (player == nullptr || !player->isSpectator()) && e->canBeCollidedWith();
        });
    double d2 = d1;

    for (Entity* entity1 : list) {
        const float f1 = entity1->getCollisionBorderSize();
        const AxisAlignedBB axisalignedbb = entity1->getEntityBoundingBox().expand(f1, f1, f1);
        const std::optional<MovingObjectPosition> movingobjectposition = axisalignedbb.calculateIntercept(vec3, vec32);

        if (axisalignedbb.isVecInside(vec3)) {
            if (d2 >= 0.0) {
                pointedEntity = entity1;
                vec33 = movingobjectposition ? movingobjectposition->hitVec : vec3;
                d2 = 0.0;
            }
        } else if (movingobjectposition) {
            const double d3 = vec3.distanceTo(movingobjectposition->hitVec);
            if (d3 < d2 || d2 == 0.0) {
                if (entity1 == entity->ridingEntity.get()) {
                    if (d2 == 0.0) {
                        pointedEntity = entity1;
                        vec33 = movingobjectposition->hitVec;
                    }
                } else {
                    pointedEntity = entity1;
                    vec33 = movingobjectposition->hitVec;
                    d2 = d3;
                }
            }
        }
    }

    if (pointedEntity != nullptr && flag && vec3.distanceTo(*vec33) > 3.0) {
        pointedEntity = nullptr;
        mc.objectMouseOver = MovingObjectPosition(MovingObjectPosition::MovingObjectType::MISS, *vec33, EnumFacing::DOWN, BlockPos(*vec33));
        // Java: a null side for misses.
        mc.objectMouseOver->sideHit.reset();
    }

    if (pointedEntity != nullptr && (d2 < d1 || !mc.objectMouseOver)) {
        mc.objectMouseOver = MovingObjectPosition(pointedEntity, *vec33);
        // Java: EntityLivingBase or EntityItemFrame (item frames come with stage 10).
        if (dynamic_cast<EntityLivingBase*>(pointedEntity) != nullptr)
            mc.pointedEntity = pointedEntity;
    }
}

void EntityRenderer::updateFovModifierHand()
{
    float f = 1.0f;
    if (auto* player = dynamic_cast<AbstractClientPlayer*>(mc.getRenderViewEntity()))
        f = player->getFovModifier();

    fovModifierHandPrev = fovModifierHand;
    fovModifierHand += (f - fovModifierHand) * 0.5f;
    if (fovModifierHand > 1.5f)
        fovModifierHand = 1.5f;
    if (fovModifierHand < 0.1f)
        fovModifierHand = 0.1f;
}

float EntityRenderer::getFOVModifier(float partialTicks, bool useFOVSetting)
{
    if (debugView)
        return 90.0f;

    Entity* entity = mc.getRenderViewEntity();
    float f = 70.0f;
    if (useFOVSetting) {
        f = mc.gameSettings->fovSetting;
        f = f * (fovModifierHandPrev + (fovModifierHand - fovModifierHandPrev) * partialTicks);
    }

    if (auto* living = dynamic_cast<EntityLivingBase*>(entity); living && living->getHealth() <= 0.0f) {
        const float f1 = static_cast<float>(living->deathTime) + partialTicks;
        f /= (1.0f - 500.0f / (f1 + 500.0f)) * 2.0f + 1.0f;
    }

    Block* block = ActiveRenderInfo::getBlockAtEntityViewpoint(mc.theWorld, entity, partialTicks);
    if (&block->getMaterial() == &Material::water)
        f = f * 60.0f / 70.0f;
    return f;
}

void EntityRenderer::hurtCameraEffect(float partialTicks)
{
    auto* entitylivingbase = dynamic_cast<EntityLivingBase*>(mc.getRenderViewEntity());
    if (entitylivingbase == nullptr)
        return;

    float f = static_cast<float>(entitylivingbase->hurtTime) - partialTicks;
    if (entitylivingbase->getHealth() <= 0.0f) {
        const float f1 = static_cast<float>(entitylivingbase->deathTime) + partialTicks;
        DxStateManager::rotate(40.0f - 8000.0f / (f1 + 200.0f), 0.0f, 0.0f, 1.0f);
    }
    if (f < 0.0f)
        return;

    f = f / static_cast<float>(entitylivingbase->maxHurtTime);
    f = MathHelper::sin(f * f * f * f * kPi);
    const float f2 = entitylivingbase->attackedAtYaw;
    DxStateManager::rotate(-f2, 0.0f, 1.0f, 0.0f);
    DxStateManager::rotate(-f * 14.0f, 0.0f, 0.0f, 1.0f);
    DxStateManager::rotate(f2, 0.0f, 1.0f, 0.0f);
}

void EntityRenderer::setupViewBobbing(float partialTicks)
{
    auto* entityplayer = dynamic_cast<EntityPlayer*>(mc.getRenderViewEntity());
    if (entityplayer == nullptr)
        return;

    const float f = entityplayer->distanceWalkedModified - entityplayer->prevDistanceWalkedModified;
    const float f1 = -(entityplayer->distanceWalkedModified + f * partialTicks);
    const float f2 = entityplayer->prevCameraYaw + (entityplayer->cameraYaw - entityplayer->prevCameraYaw) * partialTicks;
    const float f3 = entityplayer->prevCameraPitch + (entityplayer->cameraPitch - entityplayer->prevCameraPitch) * partialTicks;
    DxStateManager::translate(MathHelper::sin(f1 * kPi) * f2 * 0.5f, -std::fabs(MathHelper::cos(f1 * kPi) * f2), 0.0f);
    DxStateManager::rotate(MathHelper::sin(f1 * kPi) * f2 * 3.0f, 0.0f, 0.0f, 1.0f);
    DxStateManager::rotate(std::fabs(MathHelper::cos(f1 * kPi - 0.2f) * f2) * 5.0f, 1.0f, 0.0f, 0.0f);
    DxStateManager::rotate(f3, 1.0f, 0.0f, 0.0f);
}

void EntityRenderer::orientCamera(float partialTicks)
{
    Entity* entity = mc.getRenderViewEntity();
    float f = entity->getEyeHeight();
    double d0 = entity->prevPosX + (entity->posX - entity->prevPosX) * static_cast<double>(partialTicks);
    double d1 = entity->prevPosY + (entity->posY - entity->prevPosY) * static_cast<double>(partialTicks) + static_cast<double>(f);
    double d2 = entity->prevPosZ + (entity->posZ - entity->prevPosZ) * static_cast<double>(partialTicks);
    auto* living = dynamic_cast<EntityLivingBase*>(entity);

    if (living != nullptr && living->isPlayerSleeping()) {
        f = static_cast<float>(static_cast<double>(f) + 1.0);
        DxStateManager::translate(0.0f, 0.3f, 0.0f);

        if (!mc.gameSettings->debugCamEnable) {
            const BlockPos blockpos(entity->posX, entity->posY, entity->posZ);
            const IBlockState* iblockstate = mc.theWorld->getBlockState(blockpos);
            if (iblockstate->getBlock() == Blocks::bed) {
                const int j = getHorizontalIndex(iblockstate->getValue(BlockBed::FACING));
                DxStateManager::rotate(static_cast<float>(j * 90), 0.0f, 1.0f, 0.0f);
            }
            DxStateManager::rotate(entity->prevRotationYaw + (entity->rotationYaw - entity->prevRotationYaw) * partialTicks + 180.0f, 0.0f, -1.0f, 0.0f);
            DxStateManager::rotate(entity->prevRotationPitch + (entity->rotationPitch - entity->prevRotationPitch) * partialTicks, -1.0f, 0.0f, 0.0f);
        }
    } else if (mc.gameSettings->thirdPersonView > 0) {
        double d3 = static_cast<double>(thirdPersonDistanceTemp + (thirdPersonDistance - thirdPersonDistanceTemp) * partialTicks);

        if (mc.gameSettings->debugCamEnable) {
            DxStateManager::translate(0.0f, 0.0f, static_cast<float>(-d3));
        } else {
            const float f1 = entity->rotationYaw;
            float f2 = entity->rotationPitch;
            if (mc.gameSettings->thirdPersonView == 2)
                f2 += 180.0f;

            const double d4 = static_cast<double>(-MathHelper::sin(f1 / 180.0f * kPi) * MathHelper::cos(f2 / 180.0f * kPi)) * d3;
            const double d5 = static_cast<double>(MathHelper::cos(f1 / 180.0f * kPi) * MathHelper::cos(f2 / 180.0f * kPi)) * d3;
            const double d6 = static_cast<double>(-MathHelper::sin(f2 / 180.0f * kPi)) * d3;

            for (int i = 0; i < 8; ++i) {
                float f3 = static_cast<float>((i & 1) * 2 - 1);
                float f4 = static_cast<float>((i >> 1 & 1) * 2 - 1);
                float f5 = static_cast<float>((i >> 2 & 1) * 2 - 1);
                f3 = f3 * 0.1f;
                f4 = f4 * 0.1f;
                f5 = f5 * 0.1f;
                const std::optional<MovingObjectPosition> movingobjectposition = mc.theWorld->rayTraceBlocks(
                    Vec3(d0 + static_cast<double>(f3), d1 + static_cast<double>(f4), d2 + static_cast<double>(f5)),
                    Vec3(d0 - d4 + static_cast<double>(f3) + static_cast<double>(f5), d1 - d6 + static_cast<double>(f4), d2 - d5 + static_cast<double>(f5)));
                if (movingobjectposition) {
                    const double d7 = movingobjectposition->hitVec.distanceTo(Vec3(d0, d1, d2));
                    if (d7 < d3)
                        d3 = d7;
                }
            }

            if (mc.gameSettings->thirdPersonView == 2)
                DxStateManager::rotate(180.0f, 0.0f, 1.0f, 0.0f);

            DxStateManager::rotate(entity->rotationPitch - f2, 1.0f, 0.0f, 0.0f);
            DxStateManager::rotate(entity->rotationYaw - f1, 0.0f, 1.0f, 0.0f);
            DxStateManager::translate(0.0f, 0.0f, static_cast<float>(-d3));
            DxStateManager::rotate(f1 - entity->rotationYaw, 0.0f, 1.0f, 0.0f);
            DxStateManager::rotate(f2 - entity->rotationPitch, 1.0f, 0.0f, 0.0f);
        }
    } else {
        DxStateManager::translate(0.0f, 0.0f, -0.1f);
    }

    if (!mc.gameSettings->debugCamEnable) {
        DxStateManager::rotate(entity->prevRotationPitch + (entity->rotationPitch - entity->prevRotationPitch) * partialTicks, 1.0f, 0.0f, 0.0f);
        // Java: EntityAnimal turns the camera with its head (animals come with stage 10).
        DxStateManager::rotate(entity->prevRotationYaw + (entity->rotationYaw - entity->prevRotationYaw) * partialTicks + 180.0f, 0.0f, 1.0f, 0.0f);
    }

    DxStateManager::translate(0.0f, -f, 0.0f);
    d0 = entity->prevPosX + (entity->posX - entity->prevPosX) * static_cast<double>(partialTicks);
    d1 = entity->prevPosY + (entity->posY - entity->prevPosY) * static_cast<double>(partialTicks) + static_cast<double>(f);
    d2 = entity->prevPosZ + (entity->posZ - entity->prevPosZ) * static_cast<double>(partialTicks);
    cloudFog = mc.renderGlobal->hasCloudFog(d0, d1, d2, partialTicks);
}

void EntityRenderer::setupCameraTransform(float partialTicks, int pass)
{
    farPlaneDistance = static_cast<float>(mc.gameSettings->renderDistanceChunks * 16);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    const float f = 0.07f;

    if (mc.gameSettings->anaglyph)
        DxStateManager::translate(static_cast<float>(-(pass * 2 - 1)) * f, 0.0f, 0.0f);

    if (cameraZoom != 1.0) {
        DxStateManager::translate(static_cast<float>(cameraYaw), static_cast<float>(-cameraPitch), 0.0f);
        DxStateManager::scale(cameraZoom, cameraZoom, 1.0);
    }

    Project::gluPerspective(getFOVModifier(partialTicks, true), static_cast<float>(mc.displayWidth) / static_cast<float>(mc.displayHeight), 0.05f,
        farPlaneDistance * MathHelper::SQRT_2);
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();

    if (mc.gameSettings->anaglyph)
        DxStateManager::translate(static_cast<float>(pass * 2 - 1) * 0.1f, 0.0f, 0.0f);

    hurtCameraEffect(partialTicks);

    if (mc.gameSettings->viewBobbing)
        setupViewBobbing(partialTicks);

    const float f1 = mc.thePlayer->prevTimeInPortal + (mc.thePlayer->timeInPortal - mc.thePlayer->prevTimeInPortal) * partialTicks;
    if (f1 > 0.0f) {
        int i = 20;
        if (mc.thePlayer->isPotionActive(Potion::confusion))
            i = 7;
        float f2 = 5.0f / (f1 * f1 + 5.0f) - f1 * 0.04f;
        f2 = f2 * f2;
        DxStateManager::rotate((static_cast<float>(rendererUpdateCount) + partialTicks) * static_cast<float>(i), 0.0f, 1.0f, 1.0f);
        DxStateManager::scale(1.0f / f2, 1.0f, 1.0f);
        DxStateManager::rotate(-(static_cast<float>(rendererUpdateCount) + partialTicks) * static_cast<float>(i), 0.0f, 1.0f, 1.0f);
    }

    orientCamera(partialTicks);

    if (debugView) {
        switch (debugViewDirection) {
        case 0: DxStateManager::rotate(90.0f, 0.0f, 1.0f, 0.0f); break;
        case 1: DxStateManager::rotate(180.0f, 0.0f, 1.0f, 0.0f); break;
        case 2: DxStateManager::rotate(-90.0f, 0.0f, 1.0f, 0.0f); break;
        case 3: DxStateManager::rotate(90.0f, 1.0f, 0.0f, 0.0f); break;
        case 4: DxStateManager::rotate(-90.0f, 1.0f, 0.0f, 0.0f); break;
        default: break;
        }
    }
}

void EntityRenderer::renderHand(float partialTicks, int xOffset)
{
    if (debugView)
        return;

    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    const float f = 0.07f;
    if (mc.gameSettings->anaglyph)
        DxStateManager::translate(static_cast<float>(-(xOffset * 2 - 1)) * f, 0.0f, 0.0f);

    Project::gluPerspective(getFOVModifier(partialTicks, false), static_cast<float>(mc.displayWidth) / static_cast<float>(mc.displayHeight), 0.05f,
        farPlaneDistance * 2.0f);
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();

    if (mc.gameSettings->anaglyph)
        DxStateManager::translate(static_cast<float>(xOffset * 2 - 1) * 0.1f, 0.0f, 0.0f);

    DxStateManager::pushMatrix();
    hurtCameraEffect(partialTicks);
    if (mc.gameSettings->viewBobbing)
        setupViewBobbing(partialTicks);

    auto* living = dynamic_cast<EntityLivingBase*>(mc.getRenderViewEntity());
    const bool flag = living != nullptr && living->isPlayerSleeping();

    if (mc.gameSettings->thirdPersonView == 0 && !flag && !mc.gameSettings->hideGUI && !mc.playerController->isSpectator()) {
        enableLightmap();
        // Java: itemRenderer.renderItemInFirstPerson(partialTicks) (stage 10).
        disableLightmap();
    }

    DxStateManager::popMatrix();

    if (mc.gameSettings->thirdPersonView == 0 && !flag) {
        // Java: itemRenderer.renderOverlays(partialTicks) -- fire, water and in-wall overlays (stage 10).
        hurtCameraEffect(partialTicks);
    }

    if (mc.gameSettings->viewBobbing)
        setupViewBobbing(partialTicks);
}

void EntityRenderer::disableLightmap()
{
    DxStateManager::setActiveTexture(OpenGlHelper::lightmapTexUnit);
    DxStateManager::disableTexture2D();
    DxStateManager::setActiveTexture(OpenGlHelper::defaultTexUnit);
}

void EntityRenderer::enableLightmap()
{
    DxStateManager::setActiveTexture(OpenGlHelper::lightmapTexUnit);
    DxStateManager::matrixMode(5890);
    DxStateManager::loadIdentity();
    const float f = 0.00390625f;
    DxStateManager::scale(f, f, f);
    DxStateManager::translate(8.0f, 8.0f, 8.0f);
    DxStateManager::matrixMode(5888);
    mc.getTextureManager().bindTexture(locationLightMap);
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MIN_FILTER, GL::GL_LINEAR);
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MAG_FILTER, GL::GL_LINEAR);
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_WRAP_S, GL::GL_CLAMP);
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_WRAP_T, GL::GL_CLAMP);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    DxStateManager::enableTexture2D();
    DxStateManager::setActiveTexture(OpenGlHelper::defaultTexUnit);
}

void EntityRenderer::updateTorchFlicker()
{
    // Java: the four Math.random() calls, left to right.
    const double r0 = mathRandom();
    const double r1 = mathRandom();
    const double r2 = mathRandom();
    const double r3 = mathRandom();
    torchFlickerDX = static_cast<float>(static_cast<double>(torchFlickerDX) + (r0 - r1) * r2 * r3);
    torchFlickerDX = static_cast<float>(static_cast<double>(torchFlickerDX) * 0.9);
    torchFlickerX += (torchFlickerDX - torchFlickerX) * 1.0f;
    lightmapUpdateNeeded = true;
}

void EntityRenderer::updateLightmap(float partialTicks)
{
    if (!lightmapUpdateNeeded)
        return;
    World* world = mc.theWorld;
    if (world == nullptr)
        return;

    const float f = world->getSunBrightness(1.0f);
    const float f1 = f * 0.95f + 0.05f;
    const std::array<float, 16>& table = world->provider->getLightBrightnessTable();
    std::vector<uint32_t>& lightmapColors = lightmapTexture->getTextureData();

    for (int i = 0; i < 256; ++i) {
        float f2 = table[static_cast<size_t>(i / 16)] * f1;
        const float f3 = table[static_cast<size_t>(i % 16)] * (torchFlickerX * 0.1f + 1.5f);
        if (world->getLastLightningBolt() > 0)
            f2 = table[static_cast<size_t>(i / 16)];

        const float f4 = f2 * (f * 0.65f + 0.35f);
        const float f5 = f2 * (f * 0.65f + 0.35f);
        const float f6 = f3 * ((f3 * 0.6f + 0.4f) * 0.6f + 0.4f);
        const float f7 = f3 * (f3 * f3 * 0.6f + 0.4f);
        float f8 = f4 + f3;
        float f9 = f5 + f6;
        float f10 = f2 + f7;
        f8 = f8 * 0.96f + 0.03f;
        f9 = f9 * 0.96f + 0.03f;
        f10 = f10 * 0.96f + 0.03f;

        if (bossColorModifier > 0.0f) {
            const float f11 = bossColorModifierPrev + (bossColorModifier - bossColorModifierPrev) * partialTicks;
            f8 = f8 * (1.0f - f11) + f8 * 0.7f * f11;
            f9 = f9 * (1.0f - f11) + f9 * 0.6f * f11;
            f10 = f10 * (1.0f - f11) + f10 * 0.6f * f11;
        }

        if (world->provider->getDimensionId() == 1) {
            f8 = 0.22f + f3 * 0.75f;
            f9 = 0.28f + f6 * 0.75f;
            f10 = 0.25f + f7 * 0.75f;
        }

        if (mc.thePlayer->isPotionActive(Potion::nightVision)) {
            const float f15 = getNightVisionBrightness(mc.thePlayer.get(), partialTicks);
            float f12 = 1.0f / f8;
            if (f12 > 1.0f / f9)
                f12 = 1.0f / f9;
            if (f12 > 1.0f / f10)
                f12 = 1.0f / f10;
            f8 = f8 * (1.0f - f15) + f8 * f12 * f15;
            f9 = f9 * (1.0f - f15) + f9 * f12 * f15;
            f10 = f10 * (1.0f - f15) + f10 * f12 * f15;
        }

        if (f8 > 1.0f)
            f8 = 1.0f;
        if (f9 > 1.0f)
            f9 = 1.0f;
        if (f10 > 1.0f)
            f10 = 1.0f;

        const float f16 = mc.gameSettings->gammaSetting;
        float f17 = 1.0f - f8;
        float f13 = 1.0f - f9;
        float f14 = 1.0f - f10;
        f17 = 1.0f - f17 * f17 * f17 * f17;
        f13 = 1.0f - f13 * f13 * f13 * f13;
        f14 = 1.0f - f14 * f14 * f14 * f14;
        f8 = f8 * (1.0f - f16) + f17 * f16;
        f9 = f9 * (1.0f - f16) + f13 * f16;
        f10 = f10 * (1.0f - f16) + f14 * f16;
        f8 = f8 * 0.96f + 0.03f;
        f9 = f9 * 0.96f + 0.03f;
        f10 = f10 * 0.96f + 0.03f;

        if (f8 > 1.0f)
            f8 = 1.0f;
        if (f9 > 1.0f)
            f9 = 1.0f;
        if (f10 > 1.0f)
            f10 = 1.0f;
        if (f8 < 0.0f)
            f8 = 0.0f;
        if (f9 < 0.0f)
            f9 = 0.0f;
        if (f10 < 0.0f)
            f10 = 0.0f;

        const int j = 255;
        const int k = java::f2i(f8 * 255.0f);
        const int l = java::f2i(f9 * 255.0f);
        const int i1 = java::f2i(f10 * 255.0f);
        lightmapColors[static_cast<size_t>(i)] = static_cast<uint32_t>(j << 24 | k << 16 | l << 8 | i1);
    }

    lightmapTexture->updateDynamicTexture();
    lightmapUpdateNeeded = false;
}

float EntityRenderer::getNightVisionBrightness(EntityLivingBase* entitylivingbaseIn, float partialTicks)
{
    const int i = entitylivingbaseIn->getActivePotionEffect(Potion::nightVision)->getDuration();
    return i > 200 ? 1.0f : 0.7f + MathHelper::sin((static_cast<float>(i) - partialTicks) * kPi * 0.2f) * 0.3f;
}

void EntityRenderer::updateCameraAndRender(float partialTicks, int64_t nanoTime)
{
    const bool flag = Display::isActive();

    if (!flag && mc.gameSettings->pauseOnLostFocus && (!mc.gameSettings->touchscreen || !Mouse::isButtonDown(1))) {
        if (Minecraft::getSystemTime() - prevFrameTime > 500)
            mc.displayInGameMenu();
    } else {
        prevFrameTime = Minecraft::getSystemTime();
    }

    if (mc.inGameHasFocus && flag && mc.thePlayer) {
        mc.mouseHelper.mouseXYChange();
        const float f = mc.gameSettings->mouseSensitivity * 0.6f + 0.2f;
        const float f1 = f * f * f * 8.0f;
        float f2 = static_cast<float>(mc.mouseHelper.deltaX) * f1;
        float f3 = static_cast<float>(mc.mouseHelper.deltaY) * f1;
        int i = 1;
        if (mc.gameSettings->invertMouse)
            i = -1;

        if (mc.gameSettings->smoothCamera) {
            smoothCamYaw += f2;
            smoothCamPitch += f3;
            const float f4 = partialTicks - smoothCamPartialTicks;
            smoothCamPartialTicks = partialTicks;
            f2 = smoothCamFilterX * f4;
            f3 = smoothCamFilterY * f4;
            mc.thePlayer->setAngles(f2, f3 * static_cast<float>(i));
        } else {
            smoothCamYaw = 0.0f;
            smoothCamPitch = 0.0f;
            mc.thePlayer->setAngles(f2, f3 * static_cast<float>(i));
        }
    }

    if (mc.skipRenderWorld)
        return;

    anaglyphEnable = mc.gameSettings->anaglyph;
    const ScaledResolution scaledresolution(mc);
    const int i1 = scaledresolution.getScaledWidth();
    const int j1 = scaledresolution.getScaledHeight();
    const int k1 = Mouse::getX() * i1 / mc.displayWidth;
    const int l1 = j1 - Mouse::getY() * j1 / mc.displayHeight - 1;
    const int i2 = mc.gameSettings->limitFramerate;

    if (mc.theWorld != nullptr) {
        int j = std::min(Minecraft::getDebugFPS(), i2);
        j = std::max(j, 60);
        const int64_t k = Sys::nanoTime() - nanoTime;
        const int64_t l = std::max(static_cast<int64_t>(1000000000 / j / 4) - k, int64_t { 0 });
        renderWorld(partialTicks, Sys::nanoTime() + l);

        // Java (OpenGlHelper.shadersSupported): the entity outlines and the post shader (stage 15), then the main
        // framebuffer is bound again, which also resets the viewport.
        mc.getFramebuffer().bindFramebuffer(true);

        renderEndNanoTime = Sys::nanoTime();
        if (!mc.gameSettings->hideGUI || mc.currentScreen != nullptr) {
            DxStateManager::alphaFunc(516, 0.1f);
            mc.ingameGUI->renderGameOverlay(partialTicks);
        }
    } else {
        DxStateManager::viewport(0, 0, mc.displayWidth, mc.displayHeight);
        DxStateManager::matrixMode(5889);
        DxStateManager::loadIdentity();
        DxStateManager::matrixMode(5888);
        DxStateManager::loadIdentity();
        setupOverlayRendering();
        renderEndNanoTime = Sys::nanoTime();
    }

    if (const std::shared_ptr<GuiScreen> screen = mc.currentScreen) {
        DxStateManager::clear(256);
        screen->drawScreen(k1, l1, partialTicks);
    }
}

bool EntityRenderer::isDrawBlockOutline()
{
    if (!drawBlockOutline)
        return false;

    Entity* entity = mc.getRenderViewEntity();
    auto* player = dynamic_cast<EntityPlayer*>(entity);
    bool flag = player != nullptr && !mc.gameSettings->hideGUI;

    if (flag && !player->capabilities.allowEdit) {
        const std::shared_ptr<ItemStack> itemstack = player->getCurrentEquippedItem();
        if (mc.objectMouseOver && mc.objectMouseOver->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK) {
            const BlockPos& blockpos = *mc.objectMouseOver->getBlockPos();
            Block* block = mc.theWorld->getBlockState(blockpos)->getBlock();
            if (mc.playerController->getCurrentGameType() == GameType::SPECTATOR) {
                // Java: block.hasTileEntity() && the tile entity is an IInventory.
                flag = block->hasTileEntity() && dynamic_cast<IInventory*>(mc.theWorld->getTileEntity(blockpos)) != nullptr;
            } else {
                flag = itemstack != nullptr && (itemstack->canDestroy(block) || itemstack->canPlaceOn(block));
            }
        }
    }
    return flag;
}

void EntityRenderer::renderWorldDirections(float partialTicks)
{
    if (!mc.gameSettings->showDebugInfo || mc.gameSettings->hideGUI || mc.thePlayer->hasReducedDebug() || mc.gameSettings->reducedDebugInfo)
        return;

    Entity* entity = mc.getRenderViewEntity();
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    GL::glLineWidth(1.0f);
    DxStateManager::disableTexture2D();
    DxStateManager::depthMask(false);
    DxStateManager::pushMatrix();
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();
    orientCamera(partialTicks);
    DxStateManager::translate(0.0f, entity->getEyeHeight(), 0.0f);
    RenderGlobal::drawOutlinedBoundingBox(AxisAlignedBB(0.0, 0.0, 0.0, 0.005, 1.0E-4, 1.0E-4), 255, 0, 0, 255);
    RenderGlobal::drawOutlinedBoundingBox(AxisAlignedBB(0.0, 0.0, 0.0, 1.0E-4, 1.0E-4, 0.005), 0, 0, 255, 255);
    RenderGlobal::drawOutlinedBoundingBox(AxisAlignedBB(0.0, 0.0, 0.0, 1.0E-4, 0.0033, 1.0E-4), 0, 255, 0, 255);
    DxStateManager::popMatrix();
    DxStateManager::depthMask(true);
    DxStateManager::enableTexture2D();
    DxStateManager::disableBlend();
}

void EntityRenderer::renderWorld(float partialTicks, int64_t finishTimeNano)
{
    updateLightmap(partialTicks);

    if (mc.getRenderViewEntity() == nullptr)
        mc.setRenderViewEntity(mc.thePlayer.get());

    getMouseOver(partialTicks);
    DxStateManager::enableDepth();
    DxStateManager::enableAlpha();
    DxStateManager::alphaFunc(516, 0.5f);

    if (mc.gameSettings->anaglyph) {
        anaglyphField = 0;
        DxStateManager::colorMask(false, true, true, false);
        renderWorldPass(0, partialTicks, finishTimeNano);
        anaglyphField = 1;
        DxStateManager::colorMask(true, false, false, false);
        renderWorldPass(1, partialTicks, finishTimeNano);
        DxStateManager::colorMask(true, true, true, false);
    } else {
        renderWorldPass(2, partialTicks, finishTimeNano);
    }
}

void EntityRenderer::renderWorldPass(int pass, float partialTicks, int64_t finishTimeNano)
{
    RenderGlobal& renderglobal = *mc.renderGlobal;
    const bool flag = isDrawBlockOutline();
    DxStateManager::enableCull();
    DxStateManager::viewport(0, 0, mc.displayWidth, mc.displayHeight);
    updateFogColor(partialTicks);
    DxStateManager::clear(16640);
    setupCameraTransform(partialTicks, pass);
    ActiveRenderInfo::updateRenderInfo(mc.thePlayer.get(), mc.gameSettings->thirdPersonView == 2);
    ClippingHelperImpl::getInstance();
    Frustum icamera;
    Entity* entity = mc.getRenderViewEntity();
    const double d0 = entity->lastTickPosX + (entity->posX - entity->lastTickPosX) * static_cast<double>(partialTicks);
    const double d1 = entity->lastTickPosY + (entity->posY - entity->lastTickPosY) * static_cast<double>(partialTicks);
    const double d2 = entity->lastTickPosZ + (entity->posZ - entity->lastTickPosZ) * static_cast<double>(partialTicks);
    icamera.setPosition(d0, d1, d2);

    const float aspect = static_cast<float>(mc.displayWidth) / static_cast<float>(mc.displayHeight);
    if (mc.gameSettings->renderDistanceChunks >= 4) {
        setupFog(-1, partialTicks);
        DxStateManager::matrixMode(5889);
        DxStateManager::loadIdentity();
        Project::gluPerspective(getFOVModifier(partialTicks, true), aspect, 0.05f, farPlaneDistance * 2.0f);
        DxStateManager::matrixMode(5888);
        renderglobal.renderSky(partialTicks, pass);
        DxStateManager::matrixMode(5889);
        DxStateManager::loadIdentity();
        Project::gluPerspective(getFOVModifier(partialTicks, true), aspect, 0.05f, farPlaneDistance * MathHelper::SQRT_2);
        DxStateManager::matrixMode(5888);
    }

    setupFog(0, partialTicks);
    DxStateManager::shadeModel(7425);

    if (entity->posY + static_cast<double>(entity->getEyeHeight()) < 128.0)
        renderCloudsCheck(renderglobal, partialTicks, pass);

    setupFog(0, partialTicks);
    mc.getTextureManager().bindTexture(TextureMap::locationBlocksTexture);
    RenderHelper::disableStandardItemLighting();
    renderglobal.setupTerrain(entity, static_cast<double>(partialTicks), icamera, frameCount++, mc.thePlayer->isSpectator());

    if (pass == 0 || pass == 2)
        renderglobal.updateChunks(finishTimeNano);

    DxStateManager::matrixMode(5888);
    DxStateManager::pushMatrix();
    DxStateManager::disableAlpha();
    renderglobal.renderBlockLayer(EnumWorldBlockLayer::SOLID, static_cast<double>(partialTicks), pass, entity);
    DxStateManager::enableAlpha();
    renderglobal.renderBlockLayer(EnumWorldBlockLayer::CUTOUT_MIPPED, static_cast<double>(partialTicks), pass, entity);
    const std::shared_ptr<ITextureObject> blocksTexture = mc.getTextureManager().getTexture(TextureMap::locationBlocksTexture);
    blocksTexture->setBlurMipmap(false, false);
    renderglobal.renderBlockLayer(EnumWorldBlockLayer::CUTOUT, static_cast<double>(partialTicks), pass, entity);
    blocksTexture->restoreLastBlurMipmap();
    DxStateManager::shadeModel(7424);
    DxStateManager::alphaFunc(516, 0.1f);

    if (!debugView) {
        DxStateManager::matrixMode(5888);
        DxStateManager::popMatrix();
        DxStateManager::pushMatrix();
        RenderHelper::enableStandardItemLighting();
        renderglobal.renderEntities(entity, icamera, partialTicks);
        RenderHelper::disableStandardItemLighting();
        disableLightmap();
        DxStateManager::matrixMode(5888);
        DxStateManager::popMatrix();
        DxStateManager::pushMatrix();

        if (mc.objectMouseOver && entity->isInsideOfMaterial(Material::water) && flag) {
            DxStateManager::disableAlpha();
            renderglobal.drawSelectionBox(static_cast<EntityPlayer*>(entity), *mc.objectMouseOver, 0, partialTicks);
            DxStateManager::enableAlpha();
        }
    }

    DxStateManager::matrixMode(5888);
    DxStateManager::popMatrix();

    if (flag && mc.objectMouseOver && !entity->isInsideOfMaterial(Material::water)) {
        DxStateManager::disableAlpha();
        renderglobal.drawSelectionBox(static_cast<EntityPlayer*>(entity), *mc.objectMouseOver, 0, partialTicks);
        DxStateManager::enableAlpha();
    }

    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 1, 1, 0);
    blocksTexture->setBlurMipmap(false, false);
    renderglobal.drawBlockDamageTexture(Tessellator::getInstance(), Tessellator::getInstance().getWorldRenderer(), entity, partialTicks);
    blocksTexture->restoreLastBlurMipmap();
    DxStateManager::disableBlend();

    if (!debugView) {
        enableLightmap();
        // Java: effectRenderer.renderLitParticles (stage 15).
        RenderHelper::disableStandardItemLighting();
        setupFog(0, partialTicks);
        // Java: effectRenderer.renderParticles (stage 15).
        disableLightmap();
    }

    DxStateManager::depthMask(false);
    DxStateManager::enableCull();
    renderRainSnow(partialTicks);
    DxStateManager::depthMask(true);
    // Java: renderglobal.renderWorldBorder(entity, partialTicks) -- the border wall is not drawn yet.
    DxStateManager::disableBlend();
    DxStateManager::enableCull();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::alphaFunc(516, 0.1f);
    setupFog(0, partialTicks);
    DxStateManager::enableBlend();
    DxStateManager::depthMask(false);
    mc.getTextureManager().bindTexture(TextureMap::locationBlocksTexture);
    DxStateManager::shadeModel(7425);
    renderglobal.renderBlockLayer(EnumWorldBlockLayer::TRANSLUCENT, static_cast<double>(partialTicks), pass, entity);
    DxStateManager::shadeModel(7424);
    DxStateManager::depthMask(true);
    DxStateManager::enableCull();
    DxStateManager::disableBlend();
    DxStateManager::disableFog();

    if (entity->posY + static_cast<double>(entity->getEyeHeight()) >= 128.0)
        renderCloudsCheck(renderglobal, partialTicks, pass);

    if (renderHandEnabled) {
        DxStateManager::clear(256);
        renderHand(partialTicks, pass);
        renderWorldDirections(partialTicks);
    }
}

void EntityRenderer::renderCloudsCheck(RenderGlobal& renderGlobalIn, float partialTicks, int pass)
{
    if (mc.gameSettings->shouldRenderClouds() == 0)
        return;

    const float aspect = static_cast<float>(mc.displayWidth) / static_cast<float>(mc.displayHeight);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    Project::gluPerspective(getFOVModifier(partialTicks, true), aspect, 0.05f, farPlaneDistance * 4.0f);
    DxStateManager::matrixMode(5888);
    DxStateManager::pushMatrix();
    setupFog(0, partialTicks);
    renderGlobalIn.renderClouds(partialTicks, pass);
    DxStateManager::disableFog();
    DxStateManager::popMatrix();
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    Project::gluPerspective(getFOVModifier(partialTicks, true), aspect, 0.05f, farPlaneDistance * MathHelper::SQRT_2);
    DxStateManager::matrixMode(5888);
}

void EntityRenderer::renderRainSnow(float partialTicks)
{
    const float f = mc.theWorld->getRainStrength(partialTicks);
    if (f <= 0.0f)
        return;

    enableLightmap();
    Entity* entity = mc.getRenderViewEntity();
    World* world = mc.theWorld;
    const int i = MathHelper::floor_double(entity->posX);
    const int j = MathHelper::floor_double(entity->posY);
    const int k = MathHelper::floor_double(entity->posZ);
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    DxStateManager::disableCull();
    GL::glNormal3f(0.0f, 1.0f, 0.0f);
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::alphaFunc(516, 0.1f);
    const double d0 = entity->lastTickPosX + (entity->posX - entity->lastTickPosX) * static_cast<double>(partialTicks);
    const double d1 = entity->lastTickPosY + (entity->posY - entity->lastTickPosY) * static_cast<double>(partialTicks);
    const double d2 = entity->lastTickPosZ + (entity->posZ - entity->lastTickPosZ) * static_cast<double>(partialTicks);
    const int l = MathHelper::floor_double(d1);
    int i1 = 5;
    if (mc.gameSettings->fancyGraphics)
        i1 = 10;

    int j1 = -1;
    const float f1 = static_cast<float>(rendererUpdateCount) + partialTicks;
    worldrenderer.setTranslation(-d0, -d1, -d2);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);

    for (int k1 = k - i1; k1 <= k + i1; ++k1) {
        for (int l1 = i - i1; l1 <= i + i1; ++l1) {
            const int i2 = (k1 - k + 16) * 32 + l1 - i + 16;
            const double d3 = static_cast<double>(rainXCoords[static_cast<size_t>(i2)]) * 0.5;
            const double d4 = static_cast<double>(rainYCoords[static_cast<size_t>(i2)]) * 0.5;
            BlockPos mutablePos(l1, 0, k1);
            BiomeGenBase* biomegenbase = world->getBiomeGenForCoords(mutablePos);

            if (!biomegenbase->canRain() && !biomegenbase->getEnableSnow())
                continue;

            const int j2 = world->getPrecipitationHeight(mutablePos).getY();
            int k2 = j - i1;
            int l2 = j + i1;
            if (k2 < j2)
                k2 = j2;
            if (l2 < j2)
                l2 = j2;
            int i3 = j2;
            if (j2 < l)
                i3 = l;

            if (k2 == l2)
                continue;

            // Java: (long)(l1 * l1 * 3121 + l1 * 45238971 ^ k1 * k1 * 418711 + k1 * 13761) in int arithmetic.
            const int seedA = java::add(java::mul(java::mul(l1, l1), 3121), java::mul(l1, 45238971));
            const int seedB = java::add(java::mul(java::mul(k1, k1), 418711), java::mul(k1, 13761));
            random.setSeed(static_cast<int64_t>(seedA ^ seedB));
            mutablePos = BlockPos(l1, k2, k1);
            const float f2 = biomegenbase->getFloatTemperature(mutablePos);

            if (world->getWorldChunkManager()->getTemperatureAtHeight(f2, j2) >= 0.15f) {
                if (j1 != 0) {
                    if (j1 >= 0)
                        tessellator.draw();
                    j1 = 0;
                    mc.getTextureManager().bindTexture(locationRainPng());
                    worldrenderer.begin(7, DefaultVertexFormats::PARTICLE_POSITION_TEX_COLOR_LMAP);
                }

                const int hash = java::add(java::add(java::add(java::add(rendererUpdateCount, java::mul(java::mul(l1, l1), 3121)), java::mul(l1, 45238971)),
                                               java::mul(java::mul(k1, k1), 418711)),
                    java::mul(k1, 13761));
                const double d5 = (static_cast<double>(hash & 31) + static_cast<double>(partialTicks)) / 32.0 * (3.0 + random.nextDouble());
                const double d6 = static_cast<double>(static_cast<float>(l1) + 0.5f) - entity->posX;
                const double d7 = static_cast<double>(static_cast<float>(k1) + 0.5f) - entity->posZ;
                const float f3 = MathHelper::sqrt_double(d6 * d6 + d7 * d7) / static_cast<float>(i1);
                const float f4 = ((1.0f - f3 * f3) * 0.5f + 0.5f) * f;
                mutablePos = BlockPos(l1, i3, k1);
                const int j3 = world->getCombinedLight(mutablePos, 0);
                const int k3 = j3 >> 16 & 65535;
                const int l3 = j3 & 65535;
                worldrenderer.pos(static_cast<double>(l1) - d3 + 0.5, k2, static_cast<double>(k1) - d4 + 0.5)
                    .tex(0.0, static_cast<double>(k2) * 0.25 + d5)
                    .color(1.0f, 1.0f, 1.0f, f4)
                    .lightmap(k3, l3)
                    .endVertex();
                worldrenderer.pos(static_cast<double>(l1) + d3 + 0.5, k2, static_cast<double>(k1) + d4 + 0.5)
                    .tex(1.0, static_cast<double>(k2) * 0.25 + d5)
                    .color(1.0f, 1.0f, 1.0f, f4)
                    .lightmap(k3, l3)
                    .endVertex();
                worldrenderer.pos(static_cast<double>(l1) + d3 + 0.5, l2, static_cast<double>(k1) + d4 + 0.5)
                    .tex(1.0, static_cast<double>(l2) * 0.25 + d5)
                    .color(1.0f, 1.0f, 1.0f, f4)
                    .lightmap(k3, l3)
                    .endVertex();
                worldrenderer.pos(static_cast<double>(l1) - d3 + 0.5, l2, static_cast<double>(k1) - d4 + 0.5)
                    .tex(0.0, static_cast<double>(l2) * 0.25 + d5)
                    .color(1.0f, 1.0f, 1.0f, f4)
                    .lightmap(k3, l3)
                    .endVertex();
            } else {
                if (j1 != 1) {
                    if (j1 >= 0)
                        tessellator.draw();
                    j1 = 1;
                    mc.getTextureManager().bindTexture(locationSnowPng());
                    worldrenderer.begin(7, DefaultVertexFormats::PARTICLE_POSITION_TEX_COLOR_LMAP);
                }

                const double d8 = static_cast<double>((static_cast<float>(rendererUpdateCount & 511) + partialTicks) / 512.0f);
                // Java evaluates left to right: nextDouble, then nextGaussian; then nextDouble, nextGaussian.
                const double r0 = random.nextDouble();
                const double g0 = static_cast<double>(static_cast<float>(random.nextGaussian()));
                const double d9 = r0 + static_cast<double>(f1) * 0.01 * g0;
                const double r1 = random.nextDouble();
                const float g1 = static_cast<float>(random.nextGaussian());
                const double d10 = r1 + static_cast<double>(f1 * g1) * 0.001;
                const double d11 = static_cast<double>(static_cast<float>(l1) + 0.5f) - entity->posX;
                const double d12 = static_cast<double>(static_cast<float>(k1) + 0.5f) - entity->posZ;
                const float f6 = MathHelper::sqrt_double(d11 * d11 + d12 * d12) / static_cast<float>(i1);
                const float f5 = ((1.0f - f6 * f6) * 0.3f + 0.5f) * f;
                mutablePos = BlockPos(l1, i3, k1);
                const int i4 = (world->getCombinedLight(mutablePos, 0) * 3 + 15728880) / 4;
                const int j4 = i4 >> 16 & 65535;
                const int k4 = i4 & 65535;
                worldrenderer.pos(static_cast<double>(l1) - d3 + 0.5, k2, static_cast<double>(k1) - d4 + 0.5)
                    .tex(0.0 + d9, static_cast<double>(k2) * 0.25 + d8 + d10)
                    .color(1.0f, 1.0f, 1.0f, f5)
                    .lightmap(j4, k4)
                    .endVertex();
                worldrenderer.pos(static_cast<double>(l1) + d3 + 0.5, k2, static_cast<double>(k1) + d4 + 0.5)
                    .tex(1.0 + d9, static_cast<double>(k2) * 0.25 + d8 + d10)
                    .color(1.0f, 1.0f, 1.0f, f5)
                    .lightmap(j4, k4)
                    .endVertex();
                worldrenderer.pos(static_cast<double>(l1) + d3 + 0.5, l2, static_cast<double>(k1) + d4 + 0.5)
                    .tex(1.0 + d9, static_cast<double>(l2) * 0.25 + d8 + d10)
                    .color(1.0f, 1.0f, 1.0f, f5)
                    .lightmap(j4, k4)
                    .endVertex();
                worldrenderer.pos(static_cast<double>(l1) - d3 + 0.5, l2, static_cast<double>(k1) - d4 + 0.5)
                    .tex(0.0 + d9, static_cast<double>(l2) * 0.25 + d8 + d10)
                    .color(1.0f, 1.0f, 1.0f, f5)
                    .lightmap(j4, k4)
                    .endVertex();
            }
        }
    }

    if (j1 >= 0)
        tessellator.draw();

    worldrenderer.setTranslation(0.0, 0.0, 0.0);
    DxStateManager::enableCull();
    DxStateManager::disableBlend();
    DxStateManager::alphaFunc(516, 0.1f);
    disableLightmap();
}

void EntityRenderer::setupOverlayRendering()
{
    const ScaledResolution scaledresolution(mc);
    DxStateManager::clear(256);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    DxStateManager::ortho(0.0, scaledresolution.getScaledWidth_double(), scaledresolution.getScaledHeight_double(), 0.0, 1000.0, 3000.0);
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();
    DxStateManager::translate(0.0f, 0.0f, -2000.0f);
}

void EntityRenderer::updateFogColor(float partialTicks)
{
    World* world = mc.theWorld;
    Entity* entity = mc.getRenderViewEntity();
    float f = 0.25f + 0.75f * static_cast<float>(mc.gameSettings->renderDistanceChunks) / 32.0f;
    f = 1.0f - static_cast<float>(std::pow(static_cast<double>(f), 0.25));
    const Vec3 vec3 = world->getSkyColor(entity, partialTicks);
    const float f1 = static_cast<float>(vec3.xCoord);
    const float f2 = static_cast<float>(vec3.yCoord);
    const float f3 = static_cast<float>(vec3.zCoord);
    const Vec3 vec31 = world->getFogColor(partialTicks);
    fogColorRed = static_cast<float>(vec31.xCoord);
    fogColorGreen = static_cast<float>(vec31.yCoord);
    fogColorBlue = static_cast<float>(vec31.zCoord);

    if (mc.gameSettings->renderDistanceChunks >= 4) {
        const double d0 = -1.0;
        const Vec3 vec32 = MathHelper::sin(world->getCelestialAngleRadians(partialTicks)) > 0.0f ? Vec3(d0, 0.0, 0.0) : Vec3(1.0, 0.0, 0.0);
        float f5 = static_cast<float>(entity->getLook(partialTicks).dotProduct(vec32));
        if (f5 < 0.0f)
            f5 = 0.0f;
        if (f5 > 0.0f) {
            const std::optional<std::array<float, 4>> afloat = world->provider->calcSunriseSunsetColors(world->getCelestialAngle(partialTicks), partialTicks);
            if (afloat) {
                f5 = f5 * (*afloat)[3];
                fogColorRed = fogColorRed * (1.0f - f5) + (*afloat)[0] * f5;
                fogColorGreen = fogColorGreen * (1.0f - f5) + (*afloat)[1] * f5;
                fogColorBlue = fogColorBlue * (1.0f - f5) + (*afloat)[2] * f5;
            }
        }
    }

    fogColorRed += (f1 - fogColorRed) * f;
    fogColorGreen += (f2 - fogColorGreen) * f;
    fogColorBlue += (f3 - fogColorBlue) * f;
    const float f8 = world->getRainStrength(partialTicks);
    if (f8 > 0.0f) {
        const float f4 = 1.0f - f8 * 0.5f;
        const float f10 = 1.0f - f8 * 0.4f;
        fogColorRed *= f4;
        fogColorGreen *= f4;
        fogColorBlue *= f10;
    }

    const float f9 = world->getThunderStrength(partialTicks);
    if (f9 > 0.0f) {
        const float f11 = 1.0f - f9 * 0.5f;
        fogColorRed *= f11;
        fogColorGreen *= f11;
        fogColorBlue *= f11;
    }

    Block* block = ActiveRenderInfo::getBlockAtEntityViewpoint(mc.theWorld, entity, partialTicks);
    auto* living = dynamic_cast<EntityLivingBase*>(entity);

    if (cloudFog) {
        const Vec3 vec33 = world->getCloudColour(partialTicks);
        fogColorRed = static_cast<float>(vec33.xCoord);
        fogColorGreen = static_cast<float>(vec33.yCoord);
        fogColorBlue = static_cast<float>(vec33.zCoord);
    } else if (&block->getMaterial() == &Material::water) {
        float f12 = static_cast<float>(respirationLevel(entity)) * 0.2f;
        if (living != nullptr && living->isPotionActive(Potion::waterBreathing))
            f12 = f12 * 0.3f + 0.6f;
        fogColorRed = 0.02f + f12;
        fogColorGreen = 0.02f + f12;
        fogColorBlue = 0.2f + f12;
    } else if (&block->getMaterial() == &Material::lava) {
        fogColorRed = 0.6f;
        fogColorGreen = 0.1f;
        fogColorBlue = 0.0f;
    }

    const float f13 = fogColor2 + (fogColor1 - fogColor2) * partialTicks;
    fogColorRed *= f13;
    fogColorGreen *= f13;
    fogColorBlue *= f13;
    double d1 = (entity->lastTickPosY + (entity->posY - entity->lastTickPosY) * static_cast<double>(partialTicks)) * world->provider->getVoidFogYFactor();

    if (living != nullptr && living->isPotionActive(Potion::blindness)) {
        const int i = living->getActivePotionEffect(Potion::blindness)->getDuration();
        if (i < 20)
            d1 *= static_cast<double>(1.0f - static_cast<float>(i) / 20.0f);
        else
            d1 = 0.0;
    }

    if (d1 < 1.0) {
        if (d1 < 0.0)
            d1 = 0.0;
        d1 = d1 * d1;
        fogColorRed = static_cast<float>(static_cast<double>(fogColorRed) * d1);
        fogColorGreen = static_cast<float>(static_cast<double>(fogColorGreen) * d1);
        fogColorBlue = static_cast<float>(static_cast<double>(fogColorBlue) * d1);
    }

    if (bossColorModifier > 0.0f) {
        const float f14 = bossColorModifierPrev + (bossColorModifier - bossColorModifierPrev) * partialTicks;
        fogColorRed = fogColorRed * (1.0f - f14) + fogColorRed * 0.7f * f14;
        fogColorGreen = fogColorGreen * (1.0f - f14) + fogColorGreen * 0.6f * f14;
        fogColorBlue = fogColorBlue * (1.0f - f14) + fogColorBlue * 0.6f * f14;
    }

    if (living != nullptr && living->isPotionActive(Potion::nightVision)) {
        const float f15 = getNightVisionBrightness(living, partialTicks);
        float f6 = 1.0f / fogColorRed;
        if (f6 > 1.0f / fogColorGreen)
            f6 = 1.0f / fogColorGreen;
        if (f6 > 1.0f / fogColorBlue)
            f6 = 1.0f / fogColorBlue;
        fogColorRed = fogColorRed * (1.0f - f15) + fogColorRed * f6 * f15;
        fogColorGreen = fogColorGreen * (1.0f - f15) + fogColorGreen * f6 * f15;
        fogColorBlue = fogColorBlue * (1.0f - f15) + fogColorBlue * f6 * f15;
    }

    if (mc.gameSettings->anaglyph) {
        const float f16 = (fogColorRed * 30.0f + fogColorGreen * 59.0f + fogColorBlue * 11.0f) / 100.0f;
        const float f17 = (fogColorRed * 30.0f + fogColorGreen * 70.0f) / 100.0f;
        const float f7 = (fogColorRed * 30.0f + fogColorBlue * 70.0f) / 100.0f;
        fogColorRed = f16;
        fogColorGreen = f17;
        fogColorBlue = f7;
    }

    DxStateManager::clearColor(fogColorRed, fogColorGreen, fogColorBlue, 0.0f);
}

void EntityRenderer::setupFog(int startCoords, float partialTicks)
{
    Entity* entity = mc.getRenderViewEntity();
    const float fogColor[4] { fogColorRed, fogColorGreen, fogColorBlue, 1.0f };
    GL::glFogfv(GL::GL_FOG_COLOR, fogColor);
    GL::glNormal3f(0.0f, -1.0f, 0.0f);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    Block* block = ActiveRenderInfo::getBlockAtEntityViewpoint(mc.theWorld, entity, partialTicks);
    auto* living = dynamic_cast<EntityLivingBase*>(entity);

    if (living != nullptr && living->isPotionActive(Potion::blindness)) {
        float f1 = 5.0f;
        const int i = living->getActivePotionEffect(Potion::blindness)->getDuration();
        if (i < 20)
            f1 = 5.0f + (farPlaneDistance - 5.0f) * (1.0f - static_cast<float>(i) / 20.0f);

        DxStateManager::setFog(9729);
        if (startCoords == -1) {
            DxStateManager::setFogStart(0.0f);
            DxStateManager::setFogEnd(f1 * 0.8f);
        } else {
            DxStateManager::setFogStart(f1 * 0.25f);
            DxStateManager::setFogEnd(f1);
        }
        if (kNvFogDistance)
            GL::glFogi(GL::GL_FOG_DISTANCE_MODE_NV, GL::GL_EYE_RADIAL_NV);
    } else if (cloudFog) {
        DxStateManager::setFog(2048);
        DxStateManager::setFogDensity(0.1f);
    } else if (&block->getMaterial() == &Material::water) {
        DxStateManager::setFog(2048);
        if (living != nullptr && living->isPotionActive(Potion::waterBreathing))
            DxStateManager::setFogDensity(0.01f);
        else
            DxStateManager::setFogDensity(0.1f - static_cast<float>(respirationLevel(entity)) * 0.03f);
    } else if (&block->getMaterial() == &Material::lava) {
        DxStateManager::setFog(2048);
        DxStateManager::setFogDensity(2.0f);
    } else {
        const float f = farPlaneDistance;
        DxStateManager::setFog(9729);
        if (startCoords == -1) {
            DxStateManager::setFogStart(0.0f);
            DxStateManager::setFogEnd(f);
        } else {
            DxStateManager::setFogStart(f * 0.75f);
            DxStateManager::setFogEnd(f);
        }
        if (kNvFogDistance)
            GL::glFogi(GL::GL_FOG_DISTANCE_MODE_NV, GL::GL_EYE_RADIAL_NV);
        if (mc.theWorld->provider->doesXZShowFog(java::d2i(entity->posX), java::d2i(entity->posZ))) {
            DxStateManager::setFogStart(f * 0.05f);
            DxStateManager::setFogEnd(std::min(f, 192.0f) * 0.5f);
        }
    }

    DxStateManager::enableColorMaterial();
    DxStateManager::enableFog();
    DxStateManager::colorMaterial(1028, 4608);
}

} // namespace mc
