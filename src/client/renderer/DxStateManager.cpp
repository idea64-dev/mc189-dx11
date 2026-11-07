#include "client/renderer/DxStateManager.h"

#include "client/renderer/OpenGlHelper.h"
#include "render/gl/GL.h"

namespace mc {

namespace {

class BooleanState {
public:
    explicit BooleanState(int capabilityIn)
        : capability(capabilityIn)
    {
    }

    void setDisabled() { setState(false); }
    void setEnabled() { setState(true); }

    void setState(bool state)
    {
        if (state != currentState) {
            currentState = state;
            if (state)
                GL::glEnable(capability);
            else
                GL::glDisable(capability);
        }
    }

private:
    int capability;
    bool currentState = false;
};

struct AlphaState {
    BooleanState alphaTest { 3008 };
    int func = 519;
    float ref = -1.0f;
};

struct BlendState {
    BooleanState blend { 3042 };
    int srcFactor = 1;
    int dstFactor = 0;
    int srcFactorAlpha = 1;
    int dstFactorAlpha = 0;
};

struct Color {
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
    float alpha = 1.0f;
};

struct ClearState {
    double depth = 1.0;
    Color color { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct ColorLogicState {
    BooleanState colorLogicOp { 3058 };
    int opcode = 5379;
};

struct ColorMask {
    bool red = true;
    bool green = true;
    bool blue = true;
    bool alpha = true;
};

struct ColorMaterialState {
    BooleanState colorMaterial { 2903 };
    int face = 1032;
    int mode = 5634;
};

struct CullState {
    BooleanState cullFace { 2884 };
    int mode = 1029;
};

struct DepthState {
    BooleanState depthTest { 2929 };
    bool maskEnabled = true;
    int depthFunc = 513;
};

struct FogState {
    BooleanState fog { 2912 };
    int mode = 2048;
    float density = 1.0f;
    float start = 0.0f;
    float end = 1.0f;
};

struct PolygonOffsetState {
    BooleanState polygonOffsetFill { 32823 };
    BooleanState polygonOffsetLine { 10754 };
    float factor = 0.0f;
    float units = 0.0f;
};

struct TexGenCoord {
    TexGenCoord(int coordIn, int capability)
        : textureGen(capability)
        , coord(coordIn)
    {
    }
    BooleanState textureGen;
    int coord;
    int param = -1;
};

struct TexGenState {
    TexGenCoord s { 8192, 3168 };
    TexGenCoord t { 8193, 3169 };
    TexGenCoord r { 8194, 3170 };
    TexGenCoord q { 8195, 3171 };
};

struct TextureState {
    BooleanState texture2DState { 3553 };
    int textureName = 0;
};

struct Caches {
    AlphaState alphaState;
    BooleanState lightingState { 2896 };
    BooleanState lightState[8] { BooleanState(16384), BooleanState(16385), BooleanState(16386), BooleanState(16387),
        BooleanState(16388), BooleanState(16389), BooleanState(16390), BooleanState(16391) };
    ColorMaterialState colorMaterialState;
    BlendState blendState;
    DepthState depthState;
    FogState fogState;
    CullState cullState;
    PolygonOffsetState polygonOffsetState;
    ColorLogicState colorLogicState;
    TexGenState texGenState;
    ClearState clearState;
    BooleanState normalizeState { 2977 };
    int activeTextureUnit = 0;
    TextureState textureState[8];
    int activeShadeModel = 7425;
    BooleanState rescaleNormalState { 32826 };
    ColorMask colorMaskState;
    Color colorState;
};

Caches& caches()
{
    static Caches instance;
    return instance;
}

TexGenCoord& texGenCoord(DxStateManager::TexGen texGen)
{
    TexGenState& state = caches().texGenState;
    switch (texGen) {
    case DxStateManager::TexGen::T: return state.t;
    case DxStateManager::TexGen::R: return state.r;
    case DxStateManager::TexGen::Q: return state.q;
    default: return state.s;
    }
}

} // namespace

void DxStateManager::pushAttrib()
{
    GL::glPushAttrib(8256);
}

void DxStateManager::popAttrib()
{
    GL::glPopAttrib();
}

void DxStateManager::disableAlpha()
{
    caches().alphaState.alphaTest.setDisabled();
}

void DxStateManager::enableAlpha()
{
    caches().alphaState.alphaTest.setEnabled();
}

void DxStateManager::alphaFunc(int func, float ref)
{
    AlphaState& state = caches().alphaState;
    if (func != state.func || ref != state.ref) {
        state.func = func;
        state.ref = ref;
        GL::glAlphaFunc(func, ref);
    }
}

void DxStateManager::enableLighting()
{
    caches().lightingState.setEnabled();
}

void DxStateManager::disableLighting()
{
    caches().lightingState.setDisabled();
}

void DxStateManager::enableLight(int light)
{
    caches().lightState[light].setEnabled();
}

void DxStateManager::disableLight(int light)
{
    caches().lightState[light].setDisabled();
}

void DxStateManager::enableColorMaterial()
{
    caches().colorMaterialState.colorMaterial.setEnabled();
}

void DxStateManager::disableColorMaterial()
{
    caches().colorMaterialState.colorMaterial.setDisabled();
}

void DxStateManager::colorMaterial(int face, int mode)
{
    ColorMaterialState& state = caches().colorMaterialState;
    if (face != state.face || mode != state.mode) {
        state.face = face;
        state.mode = mode;
        GL::glColorMaterial(face, mode);
    }
}

void DxStateManager::disableDepth()
{
    caches().depthState.depthTest.setDisabled();
}

void DxStateManager::enableDepth()
{
    caches().depthState.depthTest.setEnabled();
}

void DxStateManager::depthFunc(int depthFunc)
{
    DepthState& state = caches().depthState;
    if (depthFunc != state.depthFunc) {
        state.depthFunc = depthFunc;
        GL::glDepthFunc(depthFunc);
    }
}

void DxStateManager::depthMask(bool flagIn)
{
    DepthState& state = caches().depthState;
    if (flagIn != state.maskEnabled) {
        state.maskEnabled = flagIn;
        GL::glDepthMask(flagIn);
    }
}

void DxStateManager::disableBlend()
{
    caches().blendState.blend.setDisabled();
}

void DxStateManager::enableBlend()
{
    caches().blendState.blend.setEnabled();
}

void DxStateManager::blendFunc(int srcFactor, int dstFactor)
{
    BlendState& state = caches().blendState;
    if (srcFactor != state.srcFactor || dstFactor != state.dstFactor) {
        state.srcFactor = srcFactor;
        state.dstFactor = dstFactor;
        GL::glBlendFunc(srcFactor, dstFactor);
    }
}

void DxStateManager::tryBlendFuncSeparate(int srcFactor, int dstFactor, int srcFactorAlpha, int dstFactorAlpha)
{
    BlendState& state = caches().blendState;
    if (srcFactor != state.srcFactor || dstFactor != state.dstFactor || srcFactorAlpha != state.srcFactorAlpha || dstFactorAlpha != state.dstFactorAlpha) {
        state.srcFactor = srcFactor;
        state.dstFactor = dstFactor;
        state.srcFactorAlpha = srcFactorAlpha;
        state.dstFactorAlpha = dstFactorAlpha;
        OpenGlHelper::glBlendFunc(srcFactor, dstFactor, srcFactorAlpha, dstFactorAlpha);
    }
}

void DxStateManager::enableFog()
{
    caches().fogState.fog.setEnabled();
}

void DxStateManager::disableFog()
{
    caches().fogState.fog.setDisabled();
}

void DxStateManager::setFog(int param)
{
    FogState& state = caches().fogState;
    if (param != state.mode) {
        state.mode = param;
        GL::glFogi(GL::GL_FOG_MODE, param);
    }
}

void DxStateManager::setFogDensity(float param)
{
    FogState& state = caches().fogState;
    if (param != state.density) {
        state.density = param;
        GL::glFogf(GL::GL_FOG_DENSITY, param);
    }
}

void DxStateManager::setFogStart(float param)
{
    FogState& state = caches().fogState;
    if (param != state.start) {
        state.start = param;
        GL::glFogf(GL::GL_FOG_START, param);
    }
}

void DxStateManager::setFogEnd(float param)
{
    FogState& state = caches().fogState;
    if (param != state.end) {
        state.end = param;
        GL::glFogf(GL::GL_FOG_END, param);
    }
}

void DxStateManager::enableCull()
{
    caches().cullState.cullFace.setEnabled();
}

void DxStateManager::disableCull()
{
    caches().cullState.cullFace.setDisabled();
}

void DxStateManager::cullFace(int mode)
{
    CullState& state = caches().cullState;
    if (mode != state.mode) {
        state.mode = mode;
        GL::glCullFace(mode);
    }
}

void DxStateManager::enablePolygonOffset()
{
    caches().polygonOffsetState.polygonOffsetFill.setEnabled();
}

void DxStateManager::disablePolygonOffset()
{
    caches().polygonOffsetState.polygonOffsetFill.setDisabled();
}

void DxStateManager::doPolygonOffset(float factor, float units)
{
    PolygonOffsetState& state = caches().polygonOffsetState;
    if (factor != state.factor || units != state.units) {
        state.factor = factor;
        state.units = units;
        GL::glPolygonOffset(factor, units);
    }
}

void DxStateManager::enableColorLogic()
{
    caches().colorLogicState.colorLogicOp.setEnabled();
}

void DxStateManager::disableColorLogic()
{
    caches().colorLogicState.colorLogicOp.setDisabled();
}

void DxStateManager::colorLogicOp(int opcode)
{
    ColorLogicState& state = caches().colorLogicState;
    if (opcode != state.opcode) {
        state.opcode = opcode;
        GL::glLogicOp(opcode);
    }
}

void DxStateManager::enableTexGenCoord(TexGen texGen)
{
    texGenCoord(texGen).textureGen.setEnabled();
}

void DxStateManager::disableTexGenCoord(TexGen texGen)
{
    texGenCoord(texGen).textureGen.setDisabled();
}

void DxStateManager::texGen(TexGen texGen, int param)
{
    TexGenCoord& coord = texGenCoord(texGen);
    if (param != coord.param)
        coord.param = param; // texture coordinate generation arrives with the end portal (stage 10)
}

void DxStateManager::setActiveTexture(int texture)
{
    Caches& c = caches();
    if (c.activeTextureUnit != texture - OpenGlHelper::defaultTexUnit) {
        c.activeTextureUnit = texture - OpenGlHelper::defaultTexUnit;
        OpenGlHelper::setActiveTexture(texture);
    }
}

void DxStateManager::enableTexture2D()
{
    Caches& c = caches();
    c.textureState[c.activeTextureUnit].texture2DState.setEnabled();
}

void DxStateManager::disableTexture2D()
{
    Caches& c = caches();
    c.textureState[c.activeTextureUnit].texture2DState.setDisabled();
}

int DxStateManager::generateTexture()
{
    return GL::glGenTextures();
}

void DxStateManager::deleteTexture(int texture)
{
    GL::glDeleteTextures(texture);
    for (TextureState& state : caches().textureState) {
        if (state.textureName == texture)
            state.textureName = -1;
    }
}

void DxStateManager::bindTexture(int texture)
{
    Caches& c = caches();
    if (texture != c.textureState[c.activeTextureUnit].textureName) {
        c.textureState[c.activeTextureUnit].textureName = texture;
        GL::glBindTexture(GL::GL_TEXTURE_2D, texture);
    }
}

void DxStateManager::enableNormalize()
{
    caches().normalizeState.setEnabled();
}

void DxStateManager::disableNormalize()
{
    caches().normalizeState.setDisabled();
}

void DxStateManager::shadeModel(int mode)
{
    Caches& c = caches();
    if (mode != c.activeShadeModel) {
        c.activeShadeModel = mode;
        GL::glShadeModel(mode);
    }
}

void DxStateManager::enableRescaleNormal()
{
    caches().rescaleNormalState.setEnabled();
}

void DxStateManager::disableRescaleNormal()
{
    caches().rescaleNormalState.setDisabled();
}

void DxStateManager::viewport(int x, int y, int width, int height)
{
    GL::glViewport(x, y, width, height);
}

void DxStateManager::colorMask(bool red, bool green, bool blue, bool alpha)
{
    ColorMask& state = caches().colorMaskState;
    if (red != state.red || green != state.green || blue != state.blue || alpha != state.alpha) {
        state = ColorMask { red, green, blue, alpha };
        GL::glColorMask(red, green, blue, alpha);
    }
}

void DxStateManager::clearDepth(double depth)
{
    ClearState& state = caches().clearState;
    if (depth != state.depth) {
        state.depth = depth;
        GL::glClearDepth(depth);
    }
}

void DxStateManager::clearColor(float red, float green, float blue, float alpha)
{
    Color& state = caches().clearState.color;
    if (red != state.red || green != state.green || blue != state.blue || alpha != state.alpha) {
        state = Color { red, green, blue, alpha };
        GL::glClearColor(red, green, blue, alpha);
    }
}

void DxStateManager::clear(int mask)
{
    GL::glClear(mask);
}

void DxStateManager::matrixMode(int mode)
{
    GL::glMatrixMode(mode);
}

void DxStateManager::loadIdentity()
{
    GL::glLoadIdentity();
}

void DxStateManager::pushMatrix()
{
    GL::glPushMatrix();
}

void DxStateManager::popMatrix()
{
    GL::glPopMatrix();
}

void DxStateManager::getFloat(int pname, float* params)
{
    GL::glGetFloatv(pname, params);
}

void DxStateManager::ortho(double left, double right, double bottom, double top, double zNear, double zFar)
{
    GL::glOrtho(left, right, bottom, top, zNear, zFar);
}

void DxStateManager::rotate(float angle, float x, float y, float z)
{
    GL::glRotatef(angle, x, y, z);
}

void DxStateManager::scale(float x, float y, float z)
{
    GL::glScalef(x, y, z);
}

void DxStateManager::scale(double x, double y, double z)
{
    GL::glScaled(x, y, z);
}

void DxStateManager::translate(float x, float y, float z)
{
    GL::glTranslatef(x, y, z);
}

void DxStateManager::translate(double x, double y, double z)
{
    GL::glTranslated(x, y, z);
}

void DxStateManager::multMatrix(const float* matrix)
{
    GL::glMultMatrixf(matrix);
}

void DxStateManager::color(float red, float green, float blue, float alpha)
{
    Color& state = caches().colorState;
    if (red != state.red || green != state.green || blue != state.blue || alpha != state.alpha) {
        state = Color { red, green, blue, alpha };
        GL::glColor4f(red, green, blue, alpha);
    }
}

void DxStateManager::color(float red, float green, float blue)
{
    color(red, green, blue, 1.0f);
}

void DxStateManager::resetColor()
{
    Color& state = caches().colorState;
    state.red = state.green = state.blue = state.alpha = -1.0f;
}

} // namespace mc
