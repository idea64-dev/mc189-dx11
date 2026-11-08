#pragma once

// Original: net/minecraft/client/gui/GuiOverlayDebug.java
// The F3 screen. The right column's Java runtime lines describe the native process instead (there is no JVM:
// "Java" becomes the C++ runtime, the heap figures the process's committed memory against the physical memory);
// the GL_VENDOR / GL_RENDERER / GL_VERSION lines come from the GL emulation, which reports the Direct3D 11
// adapter. Not ported yet: the lagometer (FrameTimer), the particle count (EffectRenderer, stage 15) and the
// shader line (post shaders, stage 15).

#include "client/gui/Gui.h"

#include <string>
#include <vector>

namespace mc {

class FontRenderer;
class Minecraft;
class ScaledResolution;

class GuiOverlayDebug : public Gui {
public:
    explicit GuiOverlayDebug(Minecraft* mcIn);

    void renderDebugInfo(const ScaledResolution& scaledResolutionIn);

protected:
    void renderDebugInfoLeft();
    void renderDebugInfoRight(const ScaledResolution& scaledRes);
    std::vector<std::string> call();
    std::vector<std::string> getDebugInfoRight();

private:
    bool isReducedDebug() const;

    Minecraft* mc;
    FontRenderer* fontRenderer;
};

} // namespace mc
