#pragma once

// Original: net/minecraft/client/renderer/EntityRenderer.java
// Only the screen path of updateCameraAndRender (no world loaded) and setupOverlayRendering so far; world
// rendering, the camera, lightmap, fog and post-processing arrive with stage 7.

#include <cstdint>

namespace mc {

class Minecraft;

class EntityRenderer {
public:
    explicit EntityRenderer(Minecraft& mc);

    void updateCameraAndRender(float partialTicks, int64_t nanoTime);
    void setupOverlayRendering();

private:
    Minecraft& mc;
    int64_t prevFrameTime;
    int64_t renderEndNanoTime = 0;
};

} // namespace mc
