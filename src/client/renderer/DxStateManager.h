#pragma once

// Original: net/minecraft/client/renderer/GlStateManager.java
//
// Same API and the same state cache as the original, on top of the GL emulation (render/gl/GL.h). The cache is
// kept faithfully on purpose: glPushAttrib/glPopAttrib change the real state behind the cache's back, and the
// original's resulting quirks (an enable call skipped because the cache thinks it's already on) must reproduce.

namespace mc {

class DxStateManager {
public:
    enum class TexGen { S, T, R, Q };

    static void pushAttrib();
    static void popAttrib();

    static void disableAlpha();
    static void enableAlpha();
    static void alphaFunc(int func, float ref);

    static void enableLighting();
    static void disableLighting();
    static void enableLight(int light);
    static void disableLight(int light);
    static void enableColorMaterial();
    static void disableColorMaterial();
    static void colorMaterial(int face, int mode);

    static void disableDepth();
    static void enableDepth();
    static void depthFunc(int depthFunc);
    static void depthMask(bool flagIn);

    static void disableBlend();
    static void enableBlend();
    static void blendFunc(int srcFactor, int dstFactor);
    static void tryBlendFuncSeparate(int srcFactor, int dstFactor, int srcFactorAlpha, int dstFactorAlpha);

    static void enableFog();
    static void disableFog();
    static void setFog(int param);
    static void setFogDensity(float param);
    static void setFogStart(float param);
    static void setFogEnd(float param);

    static void enableCull();
    static void disableCull();
    static void cullFace(int mode);

    static void enablePolygonOffset();
    static void disablePolygonOffset();
    static void doPolygonOffset(float factor, float units);

    static void enableColorLogic();
    static void disableColorLogic();
    static void colorLogicOp(int opcode);

    static void enableTexGenCoord(TexGen texGen);
    static void disableTexGenCoord(TexGen texGen);
    static void texGen(TexGen texGen, int param);

    static void setActiveTexture(int texture);
    static void enableTexture2D();
    static void disableTexture2D();
    static int generateTexture();
    static void deleteTexture(int texture);
    static void bindTexture(int texture);

    static void enableNormalize();
    static void disableNormalize();
    static void shadeModel(int mode);
    static void enableRescaleNormal();
    static void disableRescaleNormal();

    static void viewport(int x, int y, int width, int height);
    static void colorMask(bool red, bool green, bool blue, bool alpha);
    static void clearDepth(double depth);
    static void clearColor(float red, float green, float blue, float alpha);
    static void clear(int mask);

    static void matrixMode(int mode);
    static void loadIdentity();
    static void pushMatrix();
    static void popMatrix();
    static void getFloat(int pname, float* params);
    static void ortho(double left, double right, double bottom, double top, double zNear, double zFar);
    static void rotate(float angle, float x, float y, float z);
    static void scale(float x, float y, float z);
    static void scale(double x, double y, double z);
    static void translate(float x, float y, float z);
    static void translate(double x, double y, double z);
    static void multMatrix(const float* matrix);

    static void color(float red, float green, float blue, float alpha);
    static void color(float red, float green, float blue);
    static void resetColor();
};

} // namespace mc
