#include "client/shader/Framebuffer.h"

#include "client/renderer/DxStateManager.h"
#include "client/renderer/OpenGlHelper.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "render/gl/GL.h"

namespace mc {

Framebuffer::Framebuffer(int width, int height, bool depth)
    : useDepth(depth)
{
    createBindFramebuffer(width, height);
}

Framebuffer::~Framebuffer()
{
    deleteFramebuffer();
}

void Framebuffer::createBindFramebuffer(int width, int height)
{
    DxStateManager::enableDepth();
    if (framebufferObject >= 0)
        deleteFramebuffer();
    createFramebuffer(width, height);
    // Java: checkFramebufferComplete() -- emulated framebuffers are always complete.
    OpenGlHelper::glBindFramebuffer(0);
}

void Framebuffer::deleteFramebuffer()
{
    unbindFramebufferTexture();
    unbindFramebuffer();
    if (depthBuffer > -1) {
        OpenGlHelper::glDeleteRenderbuffers(depthBuffer);
        depthBuffer = -1;
    }
    if (framebufferTexture > -1) {
        TextureUtil::deleteTexture(framebufferTexture);
        framebufferTexture = -1;
    }
    if (framebufferObject > -1) {
        OpenGlHelper::glBindFramebuffer(0);
        OpenGlHelper::glDeleteFramebuffers(framebufferObject);
        framebufferObject = -1;
    }
}

void Framebuffer::createFramebuffer(int width, int height)
{
    framebufferWidth = width;
    framebufferHeight = height;
    framebufferTextureWidth = width;
    framebufferTextureHeight = height;

    framebufferObject = OpenGlHelper::glGenFramebuffers();
    framebufferTexture = TextureUtil::glGenTextures();
    if (useDepth)
        depthBuffer = OpenGlHelper::glGenRenderbuffers();

    setFramebufferFilter(9728);
    DxStateManager::bindTexture(framebufferTexture);
    GL::glTexImage2D(GL::GL_TEXTURE_2D, 0, framebufferTextureWidth, framebufferTextureHeight, nullptr);
    OpenGlHelper::glBindFramebuffer(framebufferObject);
    GL::glFramebufferTexture2D(framebufferTexture);
    if (useDepth) {
        GL::glRenderbufferStorage(depthBuffer, framebufferTextureWidth, framebufferTextureHeight);
        GL::glFramebufferRenderbuffer(depthBuffer);
    }

    framebufferClear();
    unbindFramebufferTexture();
}

void Framebuffer::setFramebufferFilter(int filter)
{
    framebufferFilter = filter;
    DxStateManager::bindTexture(framebufferTexture);
    GL::glTexParameterf(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MIN_FILTER, static_cast<float>(filter));
    GL::glTexParameterf(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MAG_FILTER, static_cast<float>(filter));
    GL::glTexParameterf(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_WRAP_S, 10496.0f);
    GL::glTexParameterf(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_WRAP_T, 10496.0f);
    DxStateManager::bindTexture(0);
}

void Framebuffer::bindFramebufferTexture()
{
    DxStateManager::bindTexture(framebufferTexture);
}

void Framebuffer::unbindFramebufferTexture()
{
    DxStateManager::bindTexture(0);
}

void Framebuffer::bindFramebuffer(bool setViewport)
{
    OpenGlHelper::glBindFramebuffer(framebufferObject);
    if (setViewport)
        DxStateManager::viewport(0, 0, framebufferWidth, framebufferHeight);
}

void Framebuffer::unbindFramebuffer()
{
    OpenGlHelper::glBindFramebuffer(0);
}

void Framebuffer::setFramebufferColor(float red, float green, float blue, float alpha)
{
    framebufferColor[0] = red;
    framebufferColor[1] = green;
    framebufferColor[2] = blue;
    framebufferColor[3] = alpha;
}

void Framebuffer::framebufferRender(int width, int height)
{
    framebufferRenderExt(width, height, true);
}

void Framebuffer::framebufferRenderExt(int width, int height, bool disableBlend)
{
    DxStateManager::colorMask(true, true, true, false);
    DxStateManager::disableDepth();
    DxStateManager::depthMask(false);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    DxStateManager::ortho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, 1000.0, 3000.0);
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();
    DxStateManager::translate(0.0f, 0.0f, -2000.0f);
    DxStateManager::viewport(0, 0, width, height);
    DxStateManager::enableTexture2D();
    DxStateManager::disableLighting();
    DxStateManager::disableAlpha();
    if (disableBlend) {
        DxStateManager::disableBlend();
        DxStateManager::enableColorMaterial();
    }
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    bindFramebufferTexture();

    const float f = static_cast<float>(width);
    const float f1 = static_cast<float>(height);
    const float f2 = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferTextureWidth);
    const float f3 = static_cast<float>(framebufferHeight) / static_cast<float>(framebufferTextureHeight);
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(0.0, static_cast<double>(f1), 0.0).tex(0.0, 0.0).color(255, 255, 255, 255).endVertex();
    worldrenderer.pos(static_cast<double>(f), static_cast<double>(f1), 0.0).tex(static_cast<double>(f2), 0.0).color(255, 255, 255, 255).endVertex();
    worldrenderer.pos(static_cast<double>(f), 0.0, 0.0).tex(static_cast<double>(f2), static_cast<double>(f3)).color(255, 255, 255, 255).endVertex();
    worldrenderer.pos(0.0, 0.0, 0.0).tex(0.0, static_cast<double>(f3)).color(255, 255, 255, 255).endVertex();
    tessellator.draw();
    unbindFramebufferTexture();
    DxStateManager::depthMask(true);
    DxStateManager::colorMask(true, true, true, true);
}

void Framebuffer::framebufferClear()
{
    bindFramebuffer(true);
    DxStateManager::clearColor(framebufferColor[0], framebufferColor[1], framebufferColor[2], framebufferColor[3]);
    int mask = 16384;
    if (useDepth) {
        DxStateManager::clearDepth(1.0);
        mask |= 256;
    }
    DxStateManager::clear(mask);
    unbindFramebuffer();
}

} // namespace mc
