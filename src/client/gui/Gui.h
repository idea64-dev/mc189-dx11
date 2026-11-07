#pragma once

// Original: net/minecraft/client/gui/Gui.java (the sprite overload of drawTexturedModalRect arrives with the
// texture atlas in stage 7).

#include "util/ResourceLocation.h"

#include <string_view>

namespace mc {

class FontRenderer;

class Gui {
public:
    virtual ~Gui() = default;

    static inline const ResourceLocation optionsBackground { "textures/gui/options_background.png" };
    static inline const ResourceLocation statIcons { "textures/gui/container/stats_icons.png" };
    static inline const ResourceLocation icons { "textures/gui/icons.png" };

    static void drawRect(int left, int top, int right, int bottom, int color);
    static void drawModalRectWithCustomSizedTexture(int x, int y, float u, float v, int width, int height, float textureWidth, float textureHeight);
    static void drawScaledCustomSizeModalRect(int x, int y, float u, float v, int uWidth, int vHeight, int width, int height, float tileWidth, float tileHeight);

    void drawCenteredString(FontRenderer& fontRendererIn, std::string_view text, int x, int y, int color);
    void drawString(FontRenderer& fontRendererIn, std::string_view text, int x, int y, int color);
    void drawTexturedModalRect(int x, int y, int textureX, int textureY, int width, int height);
    void drawTexturedModalRect(float xCoord, float yCoord, int minU, int minV, int maxU, int maxV);

protected:
    void drawHorizontalLine(int startX, int endX, int y, int color);
    void drawVerticalLine(int x, int startY, int endY, int color);
    void drawGradientRect(int left, int top, int right, int bottom, int startColor, int endColor);

    float zLevel = 0.0f;
};

} // namespace mc
