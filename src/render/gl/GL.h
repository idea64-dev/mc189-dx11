#pragma once

// OpenGL 1.x emulation on Direct3D 11: the subset of GL11/GL12/GL13/GL14/GL30 that Minecraft 1.8.9 calls,
// with GL semantics (state machine, matrix stacks, current colour/normal/texcoords, integer texture and
// framebuffer names, bottom-left window origin). Code ported from Java calls these as GL::glXxx(...), and
// DxStateManager (the GlStateManager replacement) is built on top.
//
// Render targets, including the window's default framebuffer, are stored bottom-up like GL framebuffers, so
// viewport/scissor/copy/read coordinates and texture coordinates into rendered images match the original.
// The window framebuffer is flipped upright once per frame when the swapchain is presented.
//
// Single-threaded: call only from the client thread.

#include "render/gl/GLConstants.h"
#include "render/gl/Matrix4f.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mc {

class RenderDevice;

// One vertex as the fixed-function pipeline sees it after attribute fetch.
struct GLVertex {
    float x, y, z;
    float r, g, b, a;
    float u0, v0;
    float u1, v1;
    float nx, ny, nz;
};

namespace GL {

// Context lifetime (Display::create / Display::destroy).
void initialize(RenderDevice& device);
void shutdown();

// GL_VENDOR, GL_RENDERER and GL_VERSION describe the Direct3D adapter (the snooper reports them).
std::string glGetString(int name);

// --- Capabilities ---
void glEnable(int capability);
void glDisable(int capability);
bool glIsEnabled(int capability);

// GL_ENABLE_BIT and GL_LIGHTING_BIT are honoured; GlStateManager.pushAttrib uses exactly those (8256).
void glPushAttrib(int mask);
void glPopAttrib();

// --- Fragment operations ---
void glAlphaFunc(int func, float ref);
void glBlendFunc(int srcFactor, int dstFactor);
void glBlendFuncSeparate(int srcRgb, int dstRgb, int srcAlpha, int dstAlpha);
void glDepthFunc(int func);
void glDepthMask(bool flag);
void glColorMask(bool red, bool green, bool blue, bool alpha);
void glCullFace(int mode);
void glFrontFace(int mode);
void glPolygonOffset(float factor, float units);
void glShadeModel(int mode);
void glLineWidth(float width);
void glLogicOp(int opcode);

// --- Fog and lighting ---
void glFogi(int pname, int param);
void glFogf(int pname, float param);
void glFogfv(int pname, const float* params);
void glLightfv(int light, int pname, const float* params);
void glLightModelfv(int pname, const float* params);
void glColorMaterial(int face, int mode);

// --- Current vertex attributes and immediate mode ---
void glColor4f(float red, float green, float blue, float alpha);
void glNormal3f(float x, float y, float z);
void glTexCoord2f(float s, float t);
void glMultiTexCoord2f(int textureUnit, float s, float t);
void glBegin(int mode);
void glVertex3f(float x, float y, float z);
void glVertex2f(float x, float y);
void glEnd();
const float* currentColor();
const float* currentNormal();
const float* currentTexCoord(int unit);

// Draws vertices that already carry every attribute (WorldVertexBufferUploader fills missing ones from the
// current values, as GL does for disabled client arrays).
void drawVertices(int mode, std::span<const GLVertex> vertices);

// --- Matrices ---
void glMatrixMode(int mode);
void glLoadIdentity();
void glPushMatrix();
void glPopMatrix();
void glLoadMatrixf(const float* matrix);
void glMultMatrixf(const float* matrix);
void glTranslatef(float x, float y, float z);
void glTranslated(double x, double y, double z);
void glScalef(float x, float y, float z);
void glScaled(double x, double y, double z);
void glRotatef(float angle, float x, float y, float z);
void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar);
// GL_MODELVIEW_MATRIX / GL_PROJECTION_MATRIX, 16 floats column-major.
void glGetFloatv(int pname, float* params);
const Matrix4f& currentMatrix(int mode);

// --- Viewport, scissor, clear ---
void glViewport(int x, int y, int width, int height);
void glScissor(int x, int y, int width, int height);
void glClearColor(float red, float green, float blue, float alpha);
void glClearDepth(double depth);
void glClear(int mask);

// --- Textures (names are ints; 0 is "no texture") ---
void glActiveTexture(int texture);
int glGenTextures();
void glDeleteTextures(int texture);
void glBindTexture(int target, int texture);
void glTexParameteri(int target, int pname, int param);
void glTexParameterf(int target, int pname, float param);
void glTexEnvi(int target, int pname, int param);
// pixels: width * height ints of 0xAARRGGBB (GL_BGRA + GL_UNSIGNED_INT_8_8_8_8_REV), or null to allocate.
void glTexImage2D(int target, int level, int width, int height, const uint32_t* pixels);
void glTexSubImage2D(int target, int level, int x, int y, int width, int height, const uint32_t* pixels);
// Copies a rectangle of the bound framebuffer (GL window coordinates) into the bound texture.
void glCopyTexSubImage2D(int target, int level, int xOffset, int yOffset, int x, int y, int width, int height);
// Rows of the bound texture's level as stored (bottom-up for rendered images), like glGetTexImage(GL_BGRA).
std::vector<uint32_t> glGetTexImage(int target, int level);
int textureWidth(int texture);
int textureHeight(int texture);

// --- Framebuffer objects ---
int glGenFramebuffers();
void glDeleteFramebuffers(int framebuffer);
void glBindFramebuffer(int framebuffer);
void glFramebufferTexture2D(int texture);
int glGenRenderbuffers();
void glDeleteRenderbuffers(int renderbuffer);
void glRenderbufferStorage(int renderbuffer, int width, int height);
void glFramebufferRenderbuffer(int renderbuffer);
// Bottom-up rows of 0xAARRGGBB, like glReadPixels(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV).
std::vector<uint32_t> glReadPixels(int x, int y, int width, int height);

// Number of draw calls issued to Direct3D since the last present (for the debug screen).
int drawCallsLastFrame();

} // namespace GL

} // namespace mc
