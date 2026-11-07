#include "client/gui/Gui.h"

#include "client/gui/FontRenderer.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"

#include <utility>

namespace mc {

void Gui::drawHorizontalLine(int startX, int endX, int y, int color)
{
    if (endX < startX)
        std::swap(startX, endX);
    drawRect(startX, y, endX + 1, y + 1, color);
}

void Gui::drawVerticalLine(int x, int startY, int endY, int color)
{
    if (endY < startY)
        std::swap(startY, endY);
    drawRect(x, startY + 1, x + 1, endY, color);
}

void Gui::drawRect(int left, int top, int right, int bottom, int color)
{
    if (left < right)
        std::swap(left, right);
    if (top < bottom)
        std::swap(top, bottom);

    const float f3 = static_cast<float>((color >> 24) & 255) / 255.0f;
    const float f = static_cast<float>((color >> 16) & 255) / 255.0f;
    const float f1 = static_cast<float>((color >> 8) & 255) / 255.0f;
    const float f2 = static_cast<float>(color & 255) / 255.0f;
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    DxStateManager::enableBlend();
    DxStateManager::disableTexture2D();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::color(f, f1, f2, f3);
    worldrenderer.begin(7, DefaultVertexFormats::POSITION);
    worldrenderer.pos(left, bottom, 0.0).endVertex();
    worldrenderer.pos(right, bottom, 0.0).endVertex();
    worldrenderer.pos(right, top, 0.0).endVertex();
    worldrenderer.pos(left, top, 0.0).endVertex();
    tessellator.draw();
    DxStateManager::enableTexture2D();
    DxStateManager::disableBlend();
}

void Gui::drawGradientRect(int left, int top, int right, int bottom, int startColor, int endColor)
{
    const float f = static_cast<float>((startColor >> 24) & 255) / 255.0f;
    const float f1 = static_cast<float>((startColor >> 16) & 255) / 255.0f;
    const float f2 = static_cast<float>((startColor >> 8) & 255) / 255.0f;
    const float f3 = static_cast<float>(startColor & 255) / 255.0f;
    const float f4 = static_cast<float>((endColor >> 24) & 255) / 255.0f;
    const float f5 = static_cast<float>((endColor >> 16) & 255) / 255.0f;
    const float f6 = static_cast<float>((endColor >> 8) & 255) / 255.0f;
    const float f7 = static_cast<float>(endColor & 255) / 255.0f;
    DxStateManager::disableTexture2D();
    DxStateManager::enableBlend();
    DxStateManager::disableAlpha();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::shadeModel(7425);
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_COLOR);
    worldrenderer.pos(right, top, zLevel).color(f1, f2, f3, f).endVertex();
    worldrenderer.pos(left, top, zLevel).color(f1, f2, f3, f).endVertex();
    worldrenderer.pos(left, bottom, zLevel).color(f5, f6, f7, f4).endVertex();
    worldrenderer.pos(right, bottom, zLevel).color(f5, f6, f7, f4).endVertex();
    tessellator.draw();
    DxStateManager::shadeModel(7424);
    DxStateManager::disableBlend();
    DxStateManager::enableAlpha();
    DxStateManager::enableTexture2D();
}

void Gui::drawCenteredString(FontRenderer& fontRendererIn, std::string_view text, int x, int y, int color)
{
    fontRendererIn.drawStringWithShadow(text, static_cast<float>(x - fontRendererIn.getStringWidth(text) / 2), static_cast<float>(y), color);
}

void Gui::drawString(FontRenderer& fontRendererIn, std::string_view text, int x, int y, int color)
{
    fontRendererIn.drawStringWithShadow(text, static_cast<float>(x), static_cast<float>(y), color);
}

void Gui::drawTexturedModalRect(int x, int y, int textureX, int textureY, int width, int height)
{
    const float f = 0.00390625f;
    const float f1 = 0.00390625f;
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(x + 0, y + height, zLevel).tex(static_cast<float>(textureX + 0) * f, static_cast<float>(textureY + height) * f1).endVertex();
    worldrenderer.pos(x + width, y + height, zLevel).tex(static_cast<float>(textureX + width) * f, static_cast<float>(textureY + height) * f1).endVertex();
    worldrenderer.pos(x + width, y + 0, zLevel).tex(static_cast<float>(textureX + width) * f, static_cast<float>(textureY + 0) * f1).endVertex();
    worldrenderer.pos(x + 0, y + 0, zLevel).tex(static_cast<float>(textureX + 0) * f, static_cast<float>(textureY + 0) * f1).endVertex();
    tessellator.draw();
}

void Gui::drawTexturedModalRect(float xCoord, float yCoord, int minU, int minV, int maxU, int maxV)
{
    const float f = 0.00390625f;
    const float f1 = 0.00390625f;
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(xCoord + 0.0f, yCoord + static_cast<float>(maxV), zLevel).tex(static_cast<float>(minU + 0) * f, static_cast<float>(minV + maxV) * f1).endVertex();
    worldrenderer.pos(xCoord + static_cast<float>(maxU), yCoord + static_cast<float>(maxV), zLevel).tex(static_cast<float>(minU + maxU) * f, static_cast<float>(minV + maxV) * f1).endVertex();
    worldrenderer.pos(xCoord + static_cast<float>(maxU), yCoord + 0.0f, zLevel).tex(static_cast<float>(minU + maxU) * f, static_cast<float>(minV + 0) * f1).endVertex();
    worldrenderer.pos(xCoord + 0.0f, yCoord + 0.0f, zLevel).tex(static_cast<float>(minU + 0) * f, static_cast<float>(minV + 0) * f1).endVertex();
    tessellator.draw();
}

void Gui::drawModalRectWithCustomSizedTexture(int x, int y, float u, float v, int width, int height, float textureWidth, float textureHeight)
{
    const float f = 1.0f / textureWidth;
    const float f1 = 1.0f / textureHeight;
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(x, y + height, 0.0).tex(u * f, (v + static_cast<float>(height)) * f1).endVertex();
    worldrenderer.pos(x + width, y + height, 0.0).tex((u + static_cast<float>(width)) * f, (v + static_cast<float>(height)) * f1).endVertex();
    worldrenderer.pos(x + width, y, 0.0).tex((u + static_cast<float>(width)) * f, v * f1).endVertex();
    worldrenderer.pos(x, y, 0.0).tex(u * f, v * f1).endVertex();
    tessellator.draw();
}

void Gui::drawScaledCustomSizeModalRect(int x, int y, float u, float v, int uWidth, int vHeight, int width, int height, float tileWidth, float tileHeight)
{
    const float f = 1.0f / tileWidth;
    const float f1 = 1.0f / tileHeight;
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(x, y + height, 0.0).tex(u * f, (v + static_cast<float>(vHeight)) * f1).endVertex();
    worldrenderer.pos(x + width, y + height, 0.0).tex((u + static_cast<float>(uWidth)) * f, (v + static_cast<float>(vHeight)) * f1).endVertex();
    worldrenderer.pos(x + width, y, 0.0).tex((u + static_cast<float>(uWidth)) * f, v * f1).endVertex();
    worldrenderer.pos(x, y, 0.0).tex(u * f, v * f1).endVertex();
    tessellator.draw();
}

} // namespace mc
