#pragma once

// Original: net/minecraft/client/shader/Framebuffer.java

namespace mc {

class Framebuffer {
public:
    Framebuffer(int width, int height, bool useDepth);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void createBindFramebuffer(int width, int height);
    void deleteFramebuffer();
    void createFramebuffer(int width, int height);
    void setFramebufferFilter(int filter);
    void bindFramebufferTexture();
    void unbindFramebufferTexture();
    void bindFramebuffer(bool setViewport);
    void unbindFramebuffer();
    void setFramebufferColor(float red, float green, float blue, float alpha);
    void framebufferRender(int width, int height);
    void framebufferRenderExt(int width, int height, bool disableBlend);
    void framebufferClear();

    int framebufferTextureWidth = 0;
    int framebufferTextureHeight = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    bool useDepth;
    int framebufferObject = -1;
    int framebufferTexture = -1;
    int depthBuffer = -1;
    float framebufferColor[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
    int framebufferFilter = 0;
};

} // namespace mc
