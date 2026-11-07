#pragma once

// Original: net/minecraft/client/renderer/texture/ITextureObject.java and AbstractTexture.java

namespace mc {

class IResourceManager;

class ITextureObject {
public:
    virtual ~ITextureObject() = default;

    virtual void setBlurMipmap(bool blur, bool mipmap) = 0;
    virtual void restoreLastBlurMipmap() = 0;
    // Throws on missing or undecodable resources (Java: IOException).
    virtual void loadTexture(const IResourceManager& resourceManager) = 0;
    virtual int getGlTextureId() = 0;
};

class AbstractTexture : public ITextureObject {
public:
    ~AbstractTexture() override;

    void setBlurMipmapDirect(bool blur, bool mipmap);
    void setBlurMipmap(bool blur, bool mipmap) override;
    void restoreLastBlurMipmap() override;
    int getGlTextureId() override;
    void deleteGlTexture();

protected:
    int glTextureId = -1;
    bool blur = false;
    bool mipmap = false;
    bool blurLast = false;
    bool mipmapLast = false;
};

} // namespace mc
