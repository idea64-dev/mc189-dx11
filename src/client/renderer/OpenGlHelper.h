#pragma once

// Original: net/minecraft/client/renderer/OpenGlHelper.java
// The capability probing (ARB/EXT fallbacks, GL version checks) disappears: the D3D11 emulation always has
// multitexture, separate blend functions and framebuffer objects. Shader programs and VBO helpers arrive with
// the post-processing shaders and world rendering.

#include "render/gl/GL.h"

namespace mc {

class OpenGlHelper {
public:
    static constexpr int defaultTexUnit = GL::GL_TEXTURE0;
    static constexpr int lightmapTexUnit = GL::GL_TEXTURE1;
    // Vertex buffers always exist on D3D11, so the video settings offer "Use VBOs" like on any modern GL driver.
    static constexpr bool vboSupported = true;

    static void setActiveTexture(int texture) { GL::glActiveTexture(texture); }
    // Client-side texture unit selection only matters for vertex arrays, which the emulation fills directly.
    static void setClientActiveTexture(int /*texture*/) { }
    static void setLightmapTextureCoords(int target, float x, float y) { GL::glMultiTexCoord2f(target, x, y); }
    static void glBlendFunc(int sFactorRGB, int dFactorRGB, int sFactorAlpha, int dFactorAlpha)
    {
        GL::glBlendFuncSeparate(sFactorRGB, dFactorRGB, sFactorAlpha, dFactorAlpha);
    }

    // Java: framebufferSupported && gameSettings.fboEnable. The emulation always renders through FBOs.
    static bool isFramebufferEnabled() { return true; }

    static int glGenFramebuffers() { return GL::glGenFramebuffers(); }
    static void glBindFramebuffer(int framebuffer) { GL::glBindFramebuffer(framebuffer); }
    static void glDeleteFramebuffers(int framebuffer) { GL::glDeleteFramebuffers(framebuffer); }
    static int glGenRenderbuffers() { return GL::glGenRenderbuffers(); }
    static void glDeleteRenderbuffers(int renderbuffer) { GL::glDeleteRenderbuffers(renderbuffer); }
};

} // namespace mc
