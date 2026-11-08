#pragma once

// Original: net/minecraft/client/renderer/EntityRenderer.java and util/MouseFilter.java
// The frame: camera (FOV, view bobbing, hurt shake, third person), fog, the lightmap texture, sky, terrain
// layers, selection box, block cracks, rain and snow, then the HUD and the open screen.
// Not ported yet: the post-processing shaders (stage 15), rain particles and sounds (addRainParticles, stages 13
// and 15), and the first-person hand and screen overlays (ItemRenderer, with the entity models of stage 10).

#include "util/Random.h"
#include "util/ResourceLocation.h"

#include <array>
#include <cstdint>
#include <memory>

namespace mc {

class Block;
class DynamicTexture;
class Entity;
class EntityLivingBase;
class Minecraft;
class RenderGlobal;

// Java: net/minecraft/util/MouseFilter -- smooths the camera in cinematic mode.
class MouseFilter {
public:
    float smooth(float value, float smoothing);
    void reset();

private:
    float field_76336_a = 0.0f;
    float field_76334_b = 0.0f;
    float field_76335_c = 0.0f;
};

class EntityRenderer {
public:
    static inline bool anaglyphEnable = false;
    static inline int anaglyphField = 0;

    explicit EntityRenderer(Minecraft& mc);
    ~EntityRenderer();

    void updateRenderer();
    void getMouseOver(float partialTicks);
    void disableLightmap();
    void enableLightmap();
    void updateCameraAndRender(float partialTicks, int64_t nanoTime);
    void renderWorld(float partialTicks, int64_t finishTimeNano);
    void setupOverlayRendering();

private:
    void updateFovModifierHand();
    float getFOVModifier(float partialTicks, bool useFOVSetting);
    void hurtCameraEffect(float partialTicks);
    void setupViewBobbing(float partialTicks);
    void orientCamera(float partialTicks);
    void setupCameraTransform(float partialTicks, int pass);
    void renderHand(float partialTicks, int xOffset);
    void updateTorchFlicker();
    void updateLightmap(float partialTicks);
    float getNightVisionBrightness(EntityLivingBase* entitylivingbaseIn, float partialTicks);
    bool isDrawBlockOutline();
    void renderWorldDirections(float partialTicks);
    void renderWorldPass(int pass, float partialTicks, int64_t finishTimeNano);
    void renderCloudsCheck(RenderGlobal& renderGlobalIn, float partialTicks, int pass);
    void renderRainSnow(float partialTicks);
    void updateFogColor(float partialTicks);
    void setupFog(int startCoords, float partialTicks);

    Minecraft& mc;
    Random random;
    float farPlaneDistance = 0.0f;
    int rendererUpdateCount = 0;
    Entity* pointedEntity = nullptr;
    MouseFilter mouseFilterXAxis;
    MouseFilter mouseFilterYAxis;
    float thirdPersonDistance = 4.0f;
    float thirdPersonDistanceTemp = 4.0f;
    float smoothCamYaw = 0.0f;
    float smoothCamPitch = 0.0f;
    float smoothCamFilterX = 0.0f;
    float smoothCamFilterY = 0.0f;
    float smoothCamPartialTicks = 0.0f;
    float fovModifierHand = 0.0f;
    float fovModifierHandPrev = 0.0f;
    float bossColorModifier = 0.0f;
    float bossColorModifierPrev = 0.0f;
    bool cloudFog = false;
    bool renderHandEnabled = true;
    bool drawBlockOutline = true;
    int64_t prevFrameTime;
    int64_t renderEndNanoTime = 0;
    std::shared_ptr<DynamicTexture> lightmapTexture;
    ResourceLocation locationLightMap;
    bool lightmapUpdateNeeded = false;
    float torchFlickerX = 0.0f;
    float torchFlickerDX = 0.0f;
    std::array<float, 1024> rainXCoords {};
    std::array<float, 1024> rainYCoords {};
    float fogColorRed = 0.0f;
    float fogColorGreen = 0.0f;
    float fogColorBlue = 0.0f;
    float fogColor2 = 0.0f;
    float fogColor1 = 0.0f;
    int debugViewDirection = 0;
    bool debugView = false;
    double cameraZoom = 1.0;
    double cameraYaw = 0.0;
    double cameraPitch = 0.0;
    int frameCount = 0;
};

} // namespace mc
