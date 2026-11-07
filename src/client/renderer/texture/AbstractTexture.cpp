#include "client/renderer/texture/ITextureObject.h"

#include "client/renderer/texture/TextureUtil.h"
#include "render/gl/GL.h"

namespace mc {

AbstractTexture::~AbstractTexture()
{
    // Java leaks GL names to the garbage collector; release the D3D11 texture with its owner instead.
    deleteGlTexture();
}

void AbstractTexture::setBlurMipmapDirect(bool blurIn, bool mipmapIn)
{
    blur = blurIn;
    mipmap = mipmapIn;
    int minFilter;
    int magFilter;
    if (blurIn) {
        minFilter = mipmapIn ? 9987 : 9729;
        magFilter = 9729;
    } else {
        minFilter = mipmapIn ? 9986 : 9728;
        magFilter = 9728;
    }
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MIN_FILTER, minFilter);
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MAG_FILTER, magFilter);
}

void AbstractTexture::setBlurMipmap(bool blurIn, bool mipmapIn)
{
    blurLast = blur;
    mipmapLast = mipmap;
    setBlurMipmapDirect(blurIn, mipmapIn);
}

void AbstractTexture::restoreLastBlurMipmap()
{
    setBlurMipmapDirect(blurLast, mipmapLast);
}

int AbstractTexture::getGlTextureId()
{
    if (glTextureId == -1)
        glTextureId = TextureUtil::glGenTextures();
    return glTextureId;
}

void AbstractTexture::deleteGlTexture()
{
    if (glTextureId != -1) {
        TextureUtil::deleteTexture(glTextureId);
        glTextureId = -1;
    }
}

} // namespace mc
