#pragma once

// OpenGL enum values used by the original code (GL11/GL12/GL13/GL14/GL30 in LWJGL). The Java code passes many of
// them as bare numbers (GlStateManager.blendFunc(770, 771)); keeping the values lets those calls port unchanged.

namespace mc::GL {

// Primitive modes
inline constexpr int GL_POINTS = 0x0000;
inline constexpr int GL_LINES = 0x0001;
inline constexpr int GL_LINE_LOOP = 0x0002;
inline constexpr int GL_LINE_STRIP = 0x0003;
inline constexpr int GL_TRIANGLES = 0x0004;
inline constexpr int GL_TRIANGLE_STRIP = 0x0005;
inline constexpr int GL_TRIANGLE_FAN = 0x0006;
inline constexpr int GL_QUADS = 0x0007;
inline constexpr int GL_QUAD_STRIP = 0x0008;
inline constexpr int GL_POLYGON = 0x0009;

// Clear bits
inline constexpr int GL_DEPTH_BUFFER_BIT = 0x00000100;
inline constexpr int GL_STENCIL_BUFFER_BIT = 0x00000400;
inline constexpr int GL_COLOR_BUFFER_BIT = 0x00004000;

// Comparison functions (depth, alpha)
inline constexpr int GL_NEVER = 0x0200;
inline constexpr int GL_LESS = 0x0201;
inline constexpr int GL_EQUAL = 0x0202;
inline constexpr int GL_LEQUAL = 0x0203;
inline constexpr int GL_GREATER = 0x0204;
inline constexpr int GL_NOTEQUAL = 0x0205;
inline constexpr int GL_GEQUAL = 0x0206;
inline constexpr int GL_ALWAYS = 0x0207;

// Blend factors
inline constexpr int GL_ZERO = 0;
inline constexpr int GL_ONE = 1;
inline constexpr int GL_SRC_COLOR = 0x0300;
inline constexpr int GL_ONE_MINUS_SRC_COLOR = 0x0301;
inline constexpr int GL_SRC_ALPHA = 0x0302;
inline constexpr int GL_ONE_MINUS_SRC_ALPHA = 0x0303;
inline constexpr int GL_DST_ALPHA = 0x0304;
inline constexpr int GL_ONE_MINUS_DST_ALPHA = 0x0305;
inline constexpr int GL_DST_COLOR = 0x0306;
inline constexpr int GL_ONE_MINUS_DST_COLOR = 0x0307;
inline constexpr int GL_SRC_ALPHA_SATURATE = 0x0308;
inline constexpr int GL_CONSTANT_COLOR = 0x8001;
inline constexpr int GL_ONE_MINUS_CONSTANT_COLOR = 0x8002;
inline constexpr int GL_CONSTANT_ALPHA = 0x8003;
inline constexpr int GL_ONE_MINUS_CONSTANT_ALPHA = 0x8004;

// Faces
inline constexpr int GL_FRONT = 0x0404;
inline constexpr int GL_BACK = 0x0405;
inline constexpr int GL_FRONT_AND_BACK = 0x0408;
inline constexpr int GL_CW = 0x0900;
inline constexpr int GL_CCW = 0x0901;

// Capabilities (glEnable/glDisable)
inline constexpr int GL_CULL_FACE = 0x0B44;
inline constexpr int GL_LIGHTING = 0x0B50;
inline constexpr int GL_COLOR_MATERIAL = 0x0B57;
inline constexpr int GL_FOG = 0x0B60;
inline constexpr int GL_DEPTH_TEST = 0x0B71;
inline constexpr int GL_NORMALIZE = 0x0BA1;
inline constexpr int GL_ALPHA_TEST = 0x0BC0;
inline constexpr int GL_BLEND = 0x0BE2;
inline constexpr int GL_COLOR_LOGIC_OP = 0x0BF2;
inline constexpr int GL_SCISSOR_TEST = 0x0C11;
inline constexpr int GL_TEXTURE_2D = 0x0DE1;
inline constexpr int GL_POLYGON_OFFSET_LINE = 0x2A02;
inline constexpr int GL_POLYGON_OFFSET_FILL = 0x8037;
inline constexpr int GL_RESCALE_NORMAL = 0x803A;
inline constexpr int GL_LIGHT0 = 0x4000;
inline constexpr int GL_LIGHT1 = 0x4001;

// glPushAttrib bits
inline constexpr int GL_LIGHTING_BIT = 0x00000040;
inline constexpr int GL_ENABLE_BIT = 0x00002000;

// Shading
inline constexpr int GL_FLAT = 0x1D00;
inline constexpr int GL_SMOOTH = 0x1D01;

// Matrix modes
inline constexpr int GL_MODELVIEW = 0x1700;
inline constexpr int GL_PROJECTION = 0x1701;
inline constexpr int GL_TEXTURE = 0x1702;
inline constexpr int GL_MODELVIEW_MATRIX = 0x0BA6;
inline constexpr int GL_PROJECTION_MATRIX = 0x0BA7;

// Fog
inline constexpr int GL_FOG_DENSITY = 0x0B62;
inline constexpr int GL_FOG_START = 0x0B63;
inline constexpr int GL_FOG_END = 0x0B64;
inline constexpr int GL_FOG_MODE = 0x0B65;
inline constexpr int GL_FOG_COLOR = 0x0B66;
inline constexpr int GL_EXP = 0x0800;
inline constexpr int GL_EXP2 = 0x0801;
inline constexpr int GL_LINEAR = 0x2601;

// Lighting
inline constexpr int GL_AMBIENT = 0x1200;
inline constexpr int GL_DIFFUSE = 0x1201;
inline constexpr int GL_SPECULAR = 0x1202;
inline constexpr int GL_POSITION = 0x1203;
inline constexpr int GL_LIGHT_MODEL_AMBIENT = 0x0B53;
inline constexpr int GL_AMBIENT_AND_DIFFUSE = 0x1602;

// Textures
inline constexpr int GL_TEXTURE0 = 0x84C0;
inline constexpr int GL_TEXTURE1 = 0x84C1;
inline constexpr int GL_TEXTURE_MAG_FILTER = 0x2800;
inline constexpr int GL_TEXTURE_MIN_FILTER = 0x2801;
inline constexpr int GL_TEXTURE_WRAP_S = 0x2802;
inline constexpr int GL_TEXTURE_WRAP_T = 0x2803;
inline constexpr int GL_TEXTURE_MIN_LOD = 0x813A;
inline constexpr int GL_TEXTURE_MAX_LOD = 0x813B;
inline constexpr int GL_TEXTURE_BASE_LEVEL = 0x813C;
inline constexpr int GL_TEXTURE_MAX_LEVEL = 0x813D;
inline constexpr int GL_TEXTURE_LOD_BIAS = 0x8501;
inline constexpr int GL_NEAREST = 0x2600;
inline constexpr int GL_NEAREST_MIPMAP_NEAREST = 0x2700;
inline constexpr int GL_LINEAR_MIPMAP_NEAREST = 0x2701;
inline constexpr int GL_NEAREST_MIPMAP_LINEAR = 0x2702;
inline constexpr int GL_LINEAR_MIPMAP_LINEAR = 0x2703;
inline constexpr int GL_CLAMP = 0x2900;
inline constexpr int GL_REPEAT = 0x2901;
inline constexpr int GL_CLAMP_TO_EDGE = 0x812F;
inline constexpr int GL_TEXTURE_ENV = 0x2300;
inline constexpr int GL_TEXTURE_ENV_MODE = 0x2200;
inline constexpr int GL_MODULATE = 0x2100;
inline constexpr int GL_REPLACE = 0x1E01;

// Pixel formats
inline constexpr int GL_RGBA = 0x1908;
inline constexpr int GL_RGBA8 = 0x8058;
inline constexpr int GL_BGRA = 0x80E1;
inline constexpr int GL_UNSIGNED_BYTE = 0x1401;
inline constexpr int GL_UNSIGNED_INT_8_8_8_8_REV = 0x8367;
inline constexpr int GL_DEPTH_COMPONENT24 = 0x81A6;

} // namespace mc::GL
