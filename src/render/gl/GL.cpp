#include "render/gl/GL.h"

#include "core/Log.h"
#include "core/WinError.h"
#include "platform/Display.h"
#include "render/RenderDevice.h"

#include <FixedFunctionPS.h>
#include <FixedFunctionPSFlat.h>
#include <FixedFunctionVS.h>
#include <PresentPS.h>
#include <PresentVS.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

using Microsoft::WRL::ComPtr;

namespace mc::GL {

namespace {

constexpr int kTextureUnits = 2;
constexpr DXGI_FORMAT kColorFormat = DXGI_FORMAT_B8G8R8A8_UNORM; // ints of 0xAARRGGBB, as Minecraft uploads them
constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

// Mirrors cbuffer DrawConstants in shaders/FixedFunction.hlsl (16-byte aligned, column-major matrices).
struct alignas(16) DrawConstants {
    float modelView[16];
    float projection[16];
    float textureMatrix0[16];
    float textureMatrix1[16];
    float normalMatrix[3][4];
    float fogColor[4];
    float fogParams[4];
    int32_t fogMode;
    int32_t alphaFunc;
    float alphaRef;
    int32_t flags;
    float lightModelAmbient[4];
    float lightPosition[2][4];
    float lightDiffuse[2][4];
    float lightAmbient[2][4];
};

enum DrawFlags : int32_t {
    FLAG_TEXTURE0 = 0x01,
    FLAG_TEXTURE1 = 0x02,
    FLAG_LIGHTING = 0x04,
    FLAG_LIGHT0 = 0x08,
    FLAG_LIGHT1 = 0x10,
    FLAG_COLOR_MATERIAL = 0x20,
    FLAG_NORMALIZE = 0x40,
};

struct Texture {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11RenderTargetView> rtv;
    int width = 0;
    int height = 0;
    int levels = 0;
    // GL texture object parameters (GL defaults)
    int minFilter = GL_NEAREST_MIPMAP_LINEAR;
    int magFilter = GL_LINEAR;
    int wrapS = GL_REPEAT;
    int wrapT = GL_REPEAT;
    int maxLevel = 1000;
    float minLod = -1000.0f;
    float maxLod = 1000.0f;
    float lodBias = 0.0f;
};

struct Renderbuffer {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11DepthStencilView> dsv;
    int width = 0;
    int height = 0;
};

struct Framebuffer {
    int colorTexture = 0;
    int depthRenderbuffer = 0;
};

struct TextureUnit {
    bool enabled = false;
    int bound = 0;
    Matrix4f matrix = Matrix4f::identity();
    std::vector<Matrix4f> stack;
    float s = 0.0f;
    float t = 0.0f;
    int envMode = GL_MODULATE;
};

struct Light {
    bool enabled = false;
    float position[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    float diffuse[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
};

// What glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT) saves.
struct AttribSnapshot {
    int mask = 0;
    bool alphaTest, blend, depthTest, cullFace, fog, lighting, colorMaterial, normalize, rescaleNormal, scissorTest,
        polygonOffsetFill, polygonOffsetLine, colorLogicOp;
    bool textureEnabled[kTextureUnits];
    bool lightEnabled[2];
    Light lights[2];
    float lightModelAmbient[4];
    int shadeModel;
};

struct RenderTarget {
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ID3D11Texture2D* color = nullptr;
    int width = 0;
    int height = 0;
};

struct State {
    RenderDevice* device = nullptr;
    ID3D11Device* d3d = nullptr;
    ID3D11DeviceContext* context = nullptr;

    // Objects
    std::unordered_map<int, Texture> textures;
    std::unordered_map<int, Framebuffer> framebuffers;
    std::unordered_map<int, Renderbuffer> renderbuffers;
    int nextName = 1;
    Texture windowColor;
    Renderbuffer windowDepth;
    int boundFramebuffer = 0;

    // Capabilities
    bool alphaTest = false;
    bool blend = false;
    bool depthTest = false;
    bool cullFace = false;
    bool fog = false;
    bool lighting = false;
    bool colorMaterial = false;
    bool normalize = false;
    bool rescaleNormal = false;
    bool scissorTest = false;
    bool polygonOffsetFill = false;
    bool polygonOffsetLine = false;
    bool colorLogicOp = false;

    // Fragment state
    int alphaFunc = GL_ALWAYS;
    float alphaRef = 0.0f;
    int blendSrcRgb = GL_ONE;
    int blendDstRgb = GL_ZERO;
    int blendSrcAlpha = GL_ONE;
    int blendDstAlpha = GL_ZERO;
    int depthFunc = GL_LESS;
    bool depthMask = true;
    std::array<bool, 4> colorMask { true, true, true, true };
    int cullMode = GL_BACK;
    int frontFace = GL_CCW;
    float polygonOffsetFactor = 0.0f;
    float polygonOffsetUnits = 0.0f;
    int shadeModel = GL_SMOOTH;
    float lineWidth = 1.0f;
    int logicOp = 0x1503; // GL_COPY

    // Fog and lighting
    int fogMode = GL_EXP;
    float fogDensity = 1.0f;
    float fogStart = 0.0f;
    float fogEnd = 1.0f;
    float fogColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    Light lights[2];
    float lightModelAmbient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };

    // Current attributes
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float normal[3] = { 0.0f, 0.0f, 1.0f };

    // Matrices
    int matrixMode = GL_MODELVIEW;
    Matrix4f modelView = Matrix4f::identity();
    Matrix4f projection = Matrix4f::identity();
    std::vector<Matrix4f> modelViewStack;
    std::vector<Matrix4f> projectionStack;
    int activeTexture = 0;
    TextureUnit units[kTextureUnits];

    int viewport[4] = { 0, 0, 0, 0 };
    int scissor[4] = { 0, 0, 0, 0 };
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float clearDepth = 1.0f;

    std::vector<AttribSnapshot> attribStack;

    // Immediate mode
    bool inBeginEnd = false;
    int beginMode = GL_TRIANGLES;
    std::vector<GLVertex> immediate;

    // Direct3D objects
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11PixelShader> pixelShaderFlat;
    ComPtr<ID3D11VertexShader> presentVertexShader;
    ComPtr<ID3D11PixelShader> presentPixelShader;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> constantBuffer;
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    UINT vertexBufferSize = 0;
    UINT vertexBufferOffset = 0;
    UINT indexBufferSize = 0;
    UINT indexBufferOffset = 0;
    std::unordered_map<uint64_t, ComPtr<ID3D11BlendState>> blendStates;
    std::unordered_map<uint64_t, ComPtr<ID3D11DepthStencilState>> depthStates;
    std::unordered_map<uint64_t, ComPtr<ID3D11RasterizerState>> rasterizerStates;
    std::unordered_map<uint64_t, ComPtr<ID3D11SamplerState>> samplerStates;
    ComPtr<ID3D11RasterizerState> presentRasterizer;
    ComPtr<ID3D11DepthStencilState> presentDepth;
    ComPtr<ID3D11BlendState> presentBlend;
    std::vector<uint32_t> indices;

    int drawCalls = 0;
    int drawCallsLastFrame = 0;
    std::unordered_set<std::string> warnedOnce;
};

State& state()
{
    static State instance;
    return instance;
}

void warnOnce(const std::string& message)
{
    if (state().warnedOnce.insert(message).second)
        Log::warn("[GL emulation] {}", message);
}

// --- Object helpers ---

Texture* findTexture(int name)
{
    auto& textures = state().textures;
    const auto it = textures.find(name);
    return it == textures.end() ? nullptr : &it->second;
}

Texture* boundTexture()
{
    State& s = state();
    return findTexture(s.units[s.activeTexture].bound);
}

int fullMipChain(int width, int height)
{
    int levels = 1;
    for (int size = std::max(width, height); size > 1; size >>= 1)
        ++levels;
    return levels;
}

void createTextureStorage(Texture& texture, int width, int height, int levels)
{
    State& s = state();
    D3D11_TEXTURE2D_DESC desc {};
    desc.Width = static_cast<UINT>(std::max(1, width));
    desc.Height = static_cast<UINT>(std::max(1, height));
    desc.MipLevels = static_cast<UINT>(levels);
    desc.ArraySize = 1;
    desc.Format = kColorFormat;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    throwIfFailed(s.d3d->CreateTexture2D(&desc, nullptr, texture.texture.ReleaseAndGetAddressOf()), "CreateTexture2D");
    throwIfFailed(s.d3d->CreateShaderResourceView(texture.texture.Get(), nullptr, texture.srv.ReleaseAndGetAddressOf()), "CreateShaderResourceView");

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.Format = kColorFormat;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    throwIfFailed(s.d3d->CreateRenderTargetView(texture.texture.Get(), &rtvDesc, texture.rtv.ReleaseAndGetAddressOf()), "CreateRenderTargetView");

    texture.width = width;
    texture.height = height;
    texture.levels = levels;
}

void createRenderbufferStorage(Renderbuffer& renderbuffer, int width, int height)
{
    State& s = state();
    D3D11_TEXTURE2D_DESC desc {};
    desc.Width = static_cast<UINT>(std::max(1, width));
    desc.Height = static_cast<UINT>(std::max(1, height));
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = kDepthFormat;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    throwIfFailed(s.d3d->CreateTexture2D(&desc, nullptr, renderbuffer.texture.ReleaseAndGetAddressOf()), "CreateTexture2D(depth)");
    throwIfFailed(s.d3d->CreateDepthStencilView(renderbuffer.texture.Get(), nullptr, renderbuffer.dsv.ReleaseAndGetAddressOf()), "CreateDepthStencilView");
    renderbuffer.width = width;
    renderbuffer.height = height;
}

// The default framebuffer follows the window size, like a GL context's.
void ensureWindowFramebuffer()
{
    State& s = state();
    const int width = std::max(1, Display::getWidth());
    const int height = std::max(1, Display::getHeight());
    if (s.windowColor.texture && s.windowColor.width == width && s.windowColor.height == height)
        return;

    createTextureStorage(s.windowColor, width, height, 1);
    createRenderbufferStorage(s.windowDepth, width, height);
    const float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    s.context->ClearRenderTargetView(s.windowColor.rtv.Get(), black);
    s.context->ClearDepthStencilView(s.windowDepth.dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

RenderTarget currentTarget()
{
    State& s = state();
    RenderTarget target;
    if (s.boundFramebuffer == 0) {
        ensureWindowFramebuffer();
        target.rtv = s.windowColor.rtv.Get();
        target.dsv = s.windowDepth.dsv.Get();
        target.color = s.windowColor.texture.Get();
        target.width = s.windowColor.width;
        target.height = s.windowColor.height;
        return target;
    }

    const auto fbo = s.framebuffers.find(s.boundFramebuffer);
    if (fbo == s.framebuffers.end())
        return target;
    if (Texture* color = findTexture(fbo->second.colorTexture); color && color->texture) {
        target.rtv = color->rtv.Get();
        target.color = color->texture.Get();
        target.width = color->width;
        target.height = color->height;
    }
    if (const auto depth = s.renderbuffers.find(fbo->second.depthRenderbuffer); depth != s.renderbuffers.end())
        target.dsv = depth->second.dsv.Get();
    return target;
}

// --- Pipeline state objects ---

D3D11_BLEND toD3DBlend(int factor, bool alphaChannel)
{
    switch (factor) {
    case GL_ZERO: return D3D11_BLEND_ZERO;
    case GL_ONE: return D3D11_BLEND_ONE;
    // D3D forbids colour factors on the alpha channel; their alpha component is the alpha factor anyway.
    case GL_SRC_COLOR: return alphaChannel ? D3D11_BLEND_SRC_ALPHA : D3D11_BLEND_SRC_COLOR;
    case GL_ONE_MINUS_SRC_COLOR: return alphaChannel ? D3D11_BLEND_INV_SRC_ALPHA : D3D11_BLEND_INV_SRC_COLOR;
    case GL_SRC_ALPHA: return D3D11_BLEND_SRC_ALPHA;
    case GL_ONE_MINUS_SRC_ALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
    case GL_DST_ALPHA: return D3D11_BLEND_DEST_ALPHA;
    case GL_ONE_MINUS_DST_ALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
    case GL_DST_COLOR: return alphaChannel ? D3D11_BLEND_DEST_ALPHA : D3D11_BLEND_DEST_COLOR;
    case GL_ONE_MINUS_DST_COLOR: return alphaChannel ? D3D11_BLEND_INV_DEST_ALPHA : D3D11_BLEND_INV_DEST_COLOR;
    case GL_SRC_ALPHA_SATURATE: return alphaChannel ? D3D11_BLEND_ONE : D3D11_BLEND_SRC_ALPHA_SAT;
    default:
        warnOnce("unsupported blend factor " + std::to_string(factor));
        return D3D11_BLEND_ONE;
    }
}

D3D11_COMPARISON_FUNC toD3DComparison(int func)
{
    switch (func) {
    case GL_NEVER: return D3D11_COMPARISON_NEVER;
    case GL_LESS: return D3D11_COMPARISON_LESS;
    case GL_EQUAL: return D3D11_COMPARISON_EQUAL;
    case GL_LEQUAL: return D3D11_COMPARISON_LESS_EQUAL;
    case GL_GREATER: return D3D11_COMPARISON_GREATER;
    case GL_NOTEQUAL: return D3D11_COMPARISON_NOT_EQUAL;
    case GL_GEQUAL: return D3D11_COMPARISON_GREATER_EQUAL;
    default: return D3D11_COMPARISON_ALWAYS;
    }
}

ID3D11BlendState* blendState()
{
    State& s = state();
    const uint8_t mask = static_cast<uint8_t>((s.colorMask[0] ? 1 : 0) | (s.colorMask[1] ? 2 : 0) | (s.colorMask[2] ? 4 : 0) | (s.colorMask[3] ? 8 : 0));
    uint64_t key = mask;
    if (s.blend) {
        key |= 1ull << 4;
        key |= static_cast<uint64_t>(s.blendSrcRgb & 0xFFFF) << 8;
        key |= static_cast<uint64_t>(s.blendDstRgb & 0xFFFF) << 24;
        key |= static_cast<uint64_t>(s.blendSrcAlpha & 0xFFF) << 40;
        key |= static_cast<uint64_t>(s.blendDstAlpha & 0xFFF) << 52;
    }

    auto& cached = s.blendStates[key];
    if (!cached) {
        D3D11_BLEND_DESC desc {};
        D3D11_RENDER_TARGET_BLEND_DESC& rt = desc.RenderTarget[0];
        rt.BlendEnable = s.blend;
        rt.SrcBlend = toD3DBlend(s.blendSrcRgb, false);
        rt.DestBlend = toD3DBlend(s.blendDstRgb, false);
        rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = toD3DBlend(s.blendSrcAlpha, true);
        rt.DestBlendAlpha = toD3DBlend(s.blendDstAlpha, true);
        rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        // D3D11_COLOR_WRITE_ENABLE is RGBA = bits 0..3, matching the mask above.
        rt.RenderTargetWriteMask = mask;
        throwIfFailed(s.d3d->CreateBlendState(&desc, cached.GetAddressOf()), "CreateBlendState");
    }
    return cached.Get();
}

ID3D11DepthStencilState* depthState()
{
    State& s = state();
    const uint64_t key = (s.depthTest ? 1u : 0u) | (s.depthMask ? 2u : 0u) | (static_cast<uint64_t>(s.depthFunc) << 4);
    auto& cached = s.depthStates[key];
    if (!cached) {
        D3D11_DEPTH_STENCIL_DESC desc {};
        // GL: with the depth test disabled the depth buffer is not written either.
        desc.DepthEnable = s.depthTest;
        desc.DepthWriteMask = s.depthMask ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = toD3DComparison(s.depthFunc);
        throwIfFailed(s.d3d->CreateDepthStencilState(&desc, cached.GetAddressOf()), "CreateDepthStencilState");
    }
    return cached.Get();
}

ID3D11RasterizerState* rasterizerState(bool polygons)
{
    State& s = state();
    const bool offset = polygons ? s.polygonOffsetFill : s.polygonOffsetLine;
    const int depthBias = offset ? static_cast<int>(std::lround(s.polygonOffsetUnits)) : 0;
    const float slopeBias = offset ? s.polygonOffsetFactor : 0.0f;

    uint64_t key = (s.cullFace && polygons ? 1u : 0u) | (s.cullMode == GL_FRONT ? 2u : 0u) | (s.frontFace == GL_CW ? 4u : 0u)
        | (s.scissorTest ? 8u : 0u);
    key |= static_cast<uint64_t>(static_cast<uint32_t>(depthBias) & 0xFFFFFF) << 8;
    uint32_t slopeBits;
    std::memcpy(&slopeBits, &slopeBias, sizeof(slopeBits));
    key |= static_cast<uint64_t>(slopeBits) << 32;

    auto& cached = s.rasterizerStates[key];
    if (!cached) {
        D3D11_RASTERIZER_DESC desc {};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = (s.cullFace && polygons) ? (s.cullMode == GL_FRONT ? D3D11_CULL_FRONT : D3D11_CULL_BACK) : D3D11_CULL_NONE;
        // Targets are stored bottom-up, which mirrors winding: GL's counter-clockwise front is clockwise here.
        desc.FrontCounterClockwise = s.frontFace == GL_CW;
        desc.DepthBias = depthBias;
        desc.SlopeScaledDepthBias = slopeBias;
        desc.DepthClipEnable = TRUE;
        desc.ScissorEnable = s.scissorTest;
        throwIfFailed(s.d3d->CreateRasterizerState(&desc, cached.GetAddressOf()), "CreateRasterizerState");
    }
    return cached.Get();
}

ID3D11SamplerState* samplerState(const Texture& texture)
{
    State& s = state();
    const bool minLinear = texture.minFilter == GL_LINEAR || texture.minFilter == GL_LINEAR_MIPMAP_NEAREST || texture.minFilter == GL_LINEAR_MIPMAP_LINEAR;
    const bool magLinear = texture.magFilter == GL_LINEAR;
    const bool mipmapped = texture.minFilter != GL_NEAREST && texture.minFilter != GL_LINEAR;
    const bool mipLinear = texture.minFilter == GL_NEAREST_MIPMAP_LINEAR || texture.minFilter == GL_LINEAR_MIPMAP_LINEAR;
    const bool clampS = texture.wrapS != GL_REPEAT;
    const bool clampT = texture.wrapT != GL_REPEAT;
    const float maxLod = mipmapped ? std::min(texture.maxLod, static_cast<float>(std::min(texture.maxLevel, texture.levels - 1))) : 0.0f;
    const float minLod = mipmapped ? std::max(texture.minLod, 0.0f) : 0.0f;

    uint64_t key = (minLinear ? 1u : 0u) | (magLinear ? 2u : 0u) | (mipLinear ? 4u : 0u) | (clampS ? 8u : 0u) | (clampT ? 16u : 0u);
    key |= static_cast<uint64_t>(std::clamp(static_cast<int>(maxLod * 16.0f), 0, 0xFFFF)) << 8;
    key |= static_cast<uint64_t>(std::clamp(static_cast<int>(minLod * 16.0f), 0, 0xFFFF)) << 24;
    key |= static_cast<uint64_t>(std::clamp(static_cast<int>((texture.lodBias + 16.0f) * 64.0f), 0, 0xFFFF)) << 40;

    auto& cached = s.samplerStates[key];
    if (!cached) {
        D3D11_SAMPLER_DESC desc {};
        int filter = 0;
        if (mipLinear)
            filter |= 0x1;
        if (magLinear)
            filter |= 0x4;
        if (minLinear)
            filter |= 0x10;
        desc.Filter = static_cast<D3D11_FILTER>(filter); // D3D11_FILTER_MIN_x_MAG_y_MIP_z bit layout
        desc.AddressU = clampS ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = clampT ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias = texture.lodBias;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = minLod;
        desc.MaxLOD = maxLod;
        throwIfFailed(s.d3d->CreateSamplerState(&desc, cached.GetAddressOf()), "CreateSamplerState");
    }
    return cached.Get();
}

// GL disables texturing for a unit whose texture is missing or incomplete (mipmap filter without the mips).
const Texture* usableTexture(int unit)
{
    State& s = state();
    if (!s.units[unit].enabled)
        return nullptr;
    const Texture* texture = findTexture(s.units[unit].bound);
    if (!texture || !texture->texture)
        return nullptr;
    const bool mipmapped = texture->minFilter != GL_NEAREST && texture->minFilter != GL_LINEAR;
    if (mipmapped) {
        const int needed = std::min(texture->maxLevel, fullMipChain(texture->width, texture->height) - 1) + 1;
        if (texture->levels < needed)
            return nullptr;
    }
    return texture;
}

// --- Index generation: GL primitive modes to D3D lists, with GL's provoking vertex first for GL_FLAT ---

D3D11_PRIMITIVE_TOPOLOGY buildIndices(int mode, uint32_t count, bool flat, std::vector<uint32_t>& out)
{
    out.clear();
    switch (mode) {
    case GL_POINTS:
        for (uint32_t i = 0; i < count; ++i)
            out.push_back(i);
        return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

    case GL_LINES:
        for (uint32_t i = 0; i + 1 < count; i += 2) {
            out.push_back(flat ? i + 1 : i);
            out.push_back(flat ? i : i + 1);
        }
        return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
        for (uint32_t i = 0; i + 1 < count; ++i) {
            out.push_back(flat ? i + 1 : i);
            out.push_back(flat ? i : i + 1);
        }
        if (mode == GL_LINE_LOOP && count > 2) {
            out.push_back(flat ? 0 : count - 1);
            out.push_back(flat ? count - 1 : 0);
        }
        return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

    case GL_TRIANGLES:
        for (uint32_t i = 0; i + 2 < count; i += 3) {
            if (flat)
                out.insert(out.end(), { i + 2, i, i + 1 });
            else
                out.insert(out.end(), { i, i + 1, i + 2 });
        }
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    case GL_TRIANGLE_STRIP:
        for (uint32_t i = 0; i + 2 < count; ++i) {
            const bool even = (i % 2) == 0;
            const uint32_t a = even ? i : i + 1;
            const uint32_t b = even ? i + 1 : i;
            if (flat)
                out.insert(out.end(), { i + 2, a, b });
            else
                out.insert(out.end(), { a, b, i + 2 });
        }
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    case GL_TRIANGLE_FAN:
        for (uint32_t i = 1; i + 1 < count; ++i) {
            if (flat)
                out.insert(out.end(), { i + 1, 0, i });
            else
                out.insert(out.end(), { 0, i, i + 1 });
        }
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    case GL_POLYGON: // provoking vertex is the first one
        for (uint32_t i = 1; i + 1 < count; ++i)
            out.insert(out.end(), { 0, i, i + 1 });
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    case GL_QUADS:
        for (uint32_t i = 0; i + 3 < count; i += 4) {
            if (flat) // provoking vertex is the 4th
                out.insert(out.end(), { i + 3, i, i + 1, i + 3, i + 1, i + 2 });
            else
                out.insert(out.end(), { i, i + 1, i + 2, i, i + 2, i + 3 });
        }
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    case GL_QUAD_STRIP:
        for (uint32_t i = 0; i + 3 < count; i += 2) {
            const uint32_t v0 = i, v1 = i + 1, v2 = i + 3, v3 = i + 2;
            if (flat)
                out.insert(out.end(), { v2, v3, v0, v2, v0, v1 });
            else
                out.insert(out.end(), { v0, v1, v2, v0, v2, v3 });
        }
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    default:
        warnOnce("unsupported primitive mode " + std::to_string(mode));
        return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

// Appends data to a dynamic ring buffer, discarding it when full. Returns the byte offset.
UINT appendToRingBuffer(ComPtr<ID3D11Buffer>& buffer, UINT& size, UINT& offset, UINT bindFlags, const void* data, UINT bytes)
{
    State& s = state();
    if (!buffer || bytes > size) {
        size = std::max<UINT>(bytes * 2, bindFlags == D3D11_BIND_VERTEX_BUFFER ? 8u << 20 : 2u << 20);
        D3D11_BUFFER_DESC desc {};
        desc.ByteWidth = size;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        throwIfFailed(s.d3d->CreateBuffer(&desc, nullptr, buffer.ReleaseAndGetAddressOf()), "CreateBuffer(dynamic)");
        offset = size; // force a discard on first use
    }

    D3D11_MAP mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
    if (offset + bytes > size) {
        mapType = D3D11_MAP_WRITE_DISCARD;
        offset = 0;
    }
    D3D11_MAPPED_SUBRESOURCE mapped;
    throwIfFailed(s.context->Map(buffer.Get(), 0, mapType, 0, &mapped), "Map(ring buffer)");
    std::memcpy(static_cast<uint8_t*>(mapped.pData) + offset, data, bytes);
    s.context->Unmap(buffer.Get(), 0);

    const UINT start = offset;
    offset += bytes;
    return start;
}

Matrix4f& activeMatrix()
{
    State& s = state();
    switch (s.matrixMode) {
    case GL_PROJECTION: return s.projection;
    case GL_TEXTURE: return s.units[s.activeTexture].matrix;
    default: return s.modelView;
    }
}

std::vector<Matrix4f>& activeStack()
{
    State& s = state();
    switch (s.matrixMode) {
    case GL_PROJECTION: return s.projectionStack;
    case GL_TEXTURE: return s.units[s.activeTexture].stack;
    default: return s.modelViewStack;
    }
}

void fillConstants(DrawConstants& c)
{
    State& s = state();
    std::memcpy(c.modelView, s.modelView.m.data(), sizeof(c.modelView));
    std::memcpy(c.projection, s.projection.m.data(), sizeof(c.projection));
    std::memcpy(c.textureMatrix0, s.units[0].matrix.m.data(), sizeof(c.textureMatrix0));
    std::memcpy(c.textureMatrix1, s.units[1].matrix.m.data(), sizeof(c.textureMatrix1));

    const auto normalMatrix = s.modelView.normalMatrix();
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column)
            c.normalMatrix[row][column] = normalMatrix[static_cast<size_t>(row * 3 + column)];
        c.normalMatrix[row][3] = 0.0f;
    }

    std::memcpy(c.fogColor, s.fogColor, sizeof(c.fogColor));
    c.fogParams[0] = s.fogStart;
    c.fogParams[1] = s.fogEnd;
    c.fogParams[2] = s.fogDensity;
    c.fogParams[3] = s.fogEnd != s.fogStart ? 1.0f / (s.fogEnd - s.fogStart) : 0.0f;
    c.fogMode = !s.fog ? 0 : (s.fogMode == GL_LINEAR ? 1 : (s.fogMode == GL_EXP ? 2 : 3));
    c.alphaFunc = s.alphaTest ? s.alphaFunc : 0;
    c.alphaRef = std::clamp(s.alphaRef, 0.0f, 1.0f);

    int flags = 0;
    if (usableTexture(0))
        flags |= FLAG_TEXTURE0;
    if (usableTexture(1))
        flags |= FLAG_TEXTURE1;
    if (s.lighting)
        flags |= FLAG_LIGHTING;
    if (s.lights[0].enabled)
        flags |= FLAG_LIGHT0;
    if (s.lights[1].enabled)
        flags |= FLAG_LIGHT1;
    if (s.colorMaterial)
        flags |= FLAG_COLOR_MATERIAL;
    if (s.normalize || s.rescaleNormal)
        flags |= FLAG_NORMALIZE;
    c.flags = flags;

    std::memcpy(c.lightModelAmbient, s.lightModelAmbient, sizeof(c.lightModelAmbient));
    for (int i = 0; i < 2; ++i) {
        std::memcpy(c.lightPosition[i], s.lights[i].position, sizeof(c.lightPosition[i]));
        std::memcpy(c.lightDiffuse[i], s.lights[i].diffuse, sizeof(c.lightDiffuse[i]));
        std::memcpy(c.lightAmbient[i], s.lights[i].ambient, sizeof(c.lightAmbient[i]));
    }
}

void setCapability(int capability, bool enabled)
{
    State& s = state();
    switch (capability) {
    case GL_ALPHA_TEST: s.alphaTest = enabled; break;
    case GL_BLEND: s.blend = enabled; break;
    case GL_DEPTH_TEST: s.depthTest = enabled; break;
    case GL_CULL_FACE: s.cullFace = enabled; break;
    case GL_FOG: s.fog = enabled; break;
    case GL_LIGHTING: s.lighting = enabled; break;
    case GL_COLOR_MATERIAL: s.colorMaterial = enabled; break;
    case GL_NORMALIZE: s.normalize = enabled; break;
    case GL_RESCALE_NORMAL: s.rescaleNormal = enabled; break;
    case GL_SCISSOR_TEST: s.scissorTest = enabled; break;
    case GL_POLYGON_OFFSET_FILL: s.polygonOffsetFill = enabled; break;
    case GL_POLYGON_OFFSET_LINE: s.polygonOffsetLine = enabled; break;
    case GL_COLOR_LOGIC_OP:
        s.colorLogicOp = enabled;
        if (enabled)
            warnOnce("GL_COLOR_LOGIC_OP is not emulated yet");
        break;
    case GL_TEXTURE_2D: s.units[s.activeTexture].enabled = enabled; break;
    case GL_LIGHT0: s.lights[0].enabled = enabled; break;
    case GL_LIGHT1: s.lights[1].enabled = enabled; break;
    default:
        // Lights 2..7, texgen, line smooth etc. are never used by the game in a way that changes the output.
        break;
    }
}

} // namespace

// ============================================================================================================

void initialize(RenderDevice& device)
{
    State& s = state();
    s.device = &device;
    s.d3d = device.getDevice();
    s.context = device.getContext();

    throwIfFailed(s.d3d->CreateVertexShader(g_FixedFunctionVS, sizeof(g_FixedFunctionVS), nullptr, s.vertexShader.GetAddressOf()), "CreateVertexShader");
    throwIfFailed(s.d3d->CreatePixelShader(g_FixedFunctionPS, sizeof(g_FixedFunctionPS), nullptr, s.pixelShader.GetAddressOf()), "CreatePixelShader");
    throwIfFailed(s.d3d->CreatePixelShader(g_FixedFunctionPSFlat, sizeof(g_FixedFunctionPSFlat), nullptr, s.pixelShaderFlat.GetAddressOf()), "CreatePixelShader(flat)");
    throwIfFailed(s.d3d->CreateVertexShader(g_PresentVS, sizeof(g_PresentVS), nullptr, s.presentVertexShader.GetAddressOf()), "CreateVertexShader(present)");
    throwIfFailed(s.d3d->CreatePixelShader(g_PresentPS, sizeof(g_PresentPS), nullptr, s.presentPixelShader.GetAddressOf()), "CreatePixelShader(present)");

    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(GLVertex, x), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(GLVertex, r), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(GLVertex, u0), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(GLVertex, u1), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(GLVertex, nx), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    throwIfFailed(s.d3d->CreateInputLayout(layout, static_cast<UINT>(std::size(layout)), g_FixedFunctionVS, sizeof(g_FixedFunctionVS), s.inputLayout.GetAddressOf()), "CreateInputLayout");

    D3D11_BUFFER_DESC cbDesc {};
    cbDesc.ByteWidth = sizeof(DrawConstants);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    throwIfFailed(s.d3d->CreateBuffer(&cbDesc, nullptr, s.constantBuffer.GetAddressOf()), "CreateBuffer(constants)");

    D3D11_RASTERIZER_DESC rasterizer {};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    throwIfFailed(s.d3d->CreateRasterizerState(&rasterizer, s.presentRasterizer.GetAddressOf()), "CreateRasterizerState(present)");
    D3D11_DEPTH_STENCIL_DESC depth {};
    throwIfFailed(s.d3d->CreateDepthStencilState(&depth, s.presentDepth.GetAddressOf()), "CreateDepthStencilState(present)");
    D3D11_BLEND_DESC blend {};
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    throwIfFailed(s.d3d->CreateBlendState(&blend, s.presentBlend.GetAddressOf()), "CreateBlendState(present)");

    // GL defaults: light 0 is white, the viewport and scissor box cover the window.
    for (int i = 0; i < 4; ++i)
        s.lights[0].diffuse[i] = 1.0f;
    ensureWindowFramebuffer();
    s.viewport[2] = s.scissor[2] = s.windowColor.width;
    s.viewport[3] = s.scissor[3] = s.windowColor.height;

    device.setPrePresentCallback([] {
        State& st = state();
        ensureWindowFramebuffer();
        ID3D11RenderTargetView* backBuffer = st.device->getBackBufferView();
        st.context->OMSetRenderTargets(1, &backBuffer, nullptr);
        D3D11_VIEWPORT viewport {};
        viewport.Width = static_cast<float>(st.device->getWidth());
        viewport.Height = static_cast<float>(st.device->getHeight());
        viewport.MaxDepth = 1.0f;
        st.context->RSSetViewports(1, &viewport);
        st.context->RSSetState(st.presentRasterizer.Get());
        st.context->OMSetDepthStencilState(st.presentDepth.Get(), 0);
        st.context->OMSetBlendState(st.presentBlend.Get(), nullptr, 0xFFFFFFFF);
        st.context->IASetInputLayout(nullptr);
        st.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        st.context->VSSetShader(st.presentVertexShader.Get(), nullptr, 0);
        st.context->PSSetShader(st.presentPixelShader.Get(), nullptr, 0);
        ID3D11ShaderResourceView* source = st.windowColor.srv.Get();
        st.context->PSSetShaderResources(0, 1, &source);
        st.context->Draw(3, 0);
        ID3D11ShaderResourceView* none[2] = {};
        st.context->PSSetShaderResources(0, 2, none);
        st.drawCallsLastFrame = st.drawCalls;
        st.drawCalls = 0;
    });

    Log::info("OpenGL 1.x emulation on Direct3D 11 initialized ({}x{})", s.windowColor.width, s.windowColor.height);
}

void shutdown()
{
    State& s = state();
    if (s.device)
        s.device->setPrePresentCallback(nullptr);
    s = State {};
}

// --- Capabilities ---

void glEnable(int capability)
{
    setCapability(capability, true);
}

void glDisable(int capability)
{
    setCapability(capability, false);
}

bool glIsEnabled(int capability)
{
    State& s = state();
    switch (capability) {
    case GL_ALPHA_TEST: return s.alphaTest;
    case GL_BLEND: return s.blend;
    case GL_DEPTH_TEST: return s.depthTest;
    case GL_CULL_FACE: return s.cullFace;
    case GL_FOG: return s.fog;
    case GL_LIGHTING: return s.lighting;
    case GL_TEXTURE_2D: return s.units[s.activeTexture].enabled;
    case GL_SCISSOR_TEST: return s.scissorTest;
    default: return false;
    }
}

void glPushAttrib(int mask)
{
    State& s = state();
    AttribSnapshot snapshot;
    snapshot.mask = mask;
    snapshot.alphaTest = s.alphaTest;
    snapshot.blend = s.blend;
    snapshot.depthTest = s.depthTest;
    snapshot.cullFace = s.cullFace;
    snapshot.fog = s.fog;
    snapshot.lighting = s.lighting;
    snapshot.colorMaterial = s.colorMaterial;
    snapshot.normalize = s.normalize;
    snapshot.rescaleNormal = s.rescaleNormal;
    snapshot.scissorTest = s.scissorTest;
    snapshot.polygonOffsetFill = s.polygonOffsetFill;
    snapshot.polygonOffsetLine = s.polygonOffsetLine;
    snapshot.colorLogicOp = s.colorLogicOp;
    for (int i = 0; i < kTextureUnits; ++i)
        snapshot.textureEnabled[i] = s.units[i].enabled;
    for (int i = 0; i < 2; ++i) {
        snapshot.lightEnabled[i] = s.lights[i].enabled;
        snapshot.lights[i] = s.lights[i];
    }
    std::memcpy(snapshot.lightModelAmbient, s.lightModelAmbient, sizeof(snapshot.lightModelAmbient));
    snapshot.shadeModel = s.shadeModel;
    s.attribStack.push_back(snapshot);
}

void glPopAttrib()
{
    State& s = state();
    if (s.attribStack.empty()) {
        warnOnce("attribute stack underflow");
        return;
    }
    const AttribSnapshot snapshot = s.attribStack.back();
    s.attribStack.pop_back();

    if (snapshot.mask & GL_ENABLE_BIT) {
        s.alphaTest = snapshot.alphaTest;
        s.blend = snapshot.blend;
        s.depthTest = snapshot.depthTest;
        s.cullFace = snapshot.cullFace;
        s.fog = snapshot.fog;
        s.lighting = snapshot.lighting;
        s.colorMaterial = snapshot.colorMaterial;
        s.normalize = snapshot.normalize;
        s.rescaleNormal = snapshot.rescaleNormal;
        s.scissorTest = snapshot.scissorTest;
        s.polygonOffsetFill = snapshot.polygonOffsetFill;
        s.polygonOffsetLine = snapshot.polygonOffsetLine;
        s.colorLogicOp = snapshot.colorLogicOp;
        for (int i = 0; i < kTextureUnits; ++i)
            s.units[i].enabled = snapshot.textureEnabled[i];
        for (int i = 0; i < 2; ++i)
            s.lights[i].enabled = snapshot.lightEnabled[i];
    }
    if (snapshot.mask & GL_LIGHTING_BIT) {
        s.lighting = snapshot.lighting;
        s.colorMaterial = snapshot.colorMaterial;
        for (int i = 0; i < 2; ++i)
            s.lights[i] = snapshot.lights[i];
        std::memcpy(s.lightModelAmbient, snapshot.lightModelAmbient, sizeof(s.lightModelAmbient));
        s.shadeModel = snapshot.shadeModel;
    }
}

// --- Fragment operations ---

void glAlphaFunc(int func, float ref)
{
    state().alphaFunc = func;
    state().alphaRef = ref;
}

void glBlendFunc(int srcFactor, int dstFactor)
{
    glBlendFuncSeparate(srcFactor, dstFactor, srcFactor, dstFactor);
}

void glBlendFuncSeparate(int srcRgb, int dstRgb, int srcAlpha, int dstAlpha)
{
    State& s = state();
    s.blendSrcRgb = srcRgb;
    s.blendDstRgb = dstRgb;
    s.blendSrcAlpha = srcAlpha;
    s.blendDstAlpha = dstAlpha;
}

void glDepthFunc(int func)
{
    state().depthFunc = func;
}

void glDepthMask(bool flag)
{
    state().depthMask = flag;
}

void glColorMask(bool red, bool green, bool blue, bool alpha)
{
    state().colorMask = { red, green, blue, alpha };
}

void glCullFace(int mode)
{
    state().cullMode = mode;
}

void glFrontFace(int mode)
{
    state().frontFace = mode;
}

void glPolygonOffset(float factor, float units)
{
    state().polygonOffsetFactor = factor;
    state().polygonOffsetUnits = units;
}

void glShadeModel(int mode)
{
    state().shadeModel = mode;
}

void glLineWidth(float width)
{
    state().lineWidth = width;
    if (width != 1.0f)
        warnOnce("line widths other than 1 are drawn 1 pixel wide");
}

void glLogicOp(int opcode)
{
    state().logicOp = opcode;
}

// --- Fog and lighting ---

void glFogi(int pname, int param)
{
    if (pname == GL_FOG_MODE)
        state().fogMode = param;
    else
        glFogf(pname, static_cast<float>(param));
}

void glFogf(int pname, float param)
{
    State& s = state();
    switch (pname) {
    case GL_FOG_DENSITY: s.fogDensity = param; break;
    case GL_FOG_START: s.fogStart = param; break;
    case GL_FOG_END: s.fogEnd = param; break;
    case GL_FOG_MODE: s.fogMode = static_cast<int>(param); break;
    default: break;
    }
}

void glFogfv(int pname, const float* params)
{
    if (pname == GL_FOG_COLOR)
        std::memcpy(state().fogColor, params, sizeof(float) * 4);
    else
        glFogf(pname, params[0]);
}

void glLightfv(int light, int pname, const float* params)
{
    State& s = state();
    const int index = light - GL_LIGHT0;
    if (index < 0 || index > 1)
        return;
    Light& l = s.lights[index];
    switch (pname) {
    case GL_POSITION: {
        // GL stores light positions in eye space, transformed by the modelview matrix current at this call.
        const Matrix4f& m = s.modelView;
        for (int row = 0; row < 4; ++row)
            l.position[row] = m.at(row, 0) * params[0] + m.at(row, 1) * params[1] + m.at(row, 2) * params[2] + m.at(row, 3) * params[3];
        break;
    }
    case GL_DIFFUSE: std::memcpy(l.diffuse, params, sizeof(float) * 4); break;
    case GL_AMBIENT: std::memcpy(l.ambient, params, sizeof(float) * 4); break;
    default: break; // specular is always black in Minecraft
    }
}

void glLightModelfv(int pname, const float* params)
{
    if (pname == GL_LIGHT_MODEL_AMBIENT)
        std::memcpy(state().lightModelAmbient, params, sizeof(float) * 4);
}

void glColorMaterial(int /*face*/, int mode)
{
    if (mode != GL_AMBIENT_AND_DIFFUSE)
        warnOnce("glColorMaterial modes other than GL_AMBIENT_AND_DIFFUSE are not emulated");
}

// --- Current attributes and immediate mode ---

void glColor4f(float red, float green, float blue, float alpha)
{
    State& s = state();
    s.color[0] = red;
    s.color[1] = green;
    s.color[2] = blue;
    s.color[3] = alpha;
}

void glNormal3f(float x, float y, float z)
{
    State& s = state();
    s.normal[0] = x;
    s.normal[1] = y;
    s.normal[2] = z;
}

void glTexCoord2f(float u, float v)
{
    State& s = state();
    s.units[0].s = u;
    s.units[0].t = v;
}

void glMultiTexCoord2f(int textureUnit, float u, float v)
{
    State& s = state();
    const int unit = textureUnit - GL_TEXTURE0;
    if (unit < 0 || unit >= kTextureUnits)
        return;
    s.units[unit].s = u;
    s.units[unit].t = v;
}

const float* currentColor()
{
    return state().color;
}

const float* currentNormal()
{
    return state().normal;
}

const float* currentTexCoord(int unit)
{
    return &state().units[std::clamp(unit, 0, kTextureUnits - 1)].s;
}

void glBegin(int mode)
{
    State& s = state();
    s.inBeginEnd = true;
    s.beginMode = mode;
    s.immediate.clear();
}

void glVertex3f(float x, float y, float z)
{
    State& s = state();
    GLVertex v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.r = s.color[0];
    v.g = s.color[1];
    v.b = s.color[2];
    v.a = s.color[3];
    v.u0 = s.units[0].s;
    v.v0 = s.units[0].t;
    v.u1 = s.units[1].s;
    v.v1 = s.units[1].t;
    v.nx = s.normal[0];
    v.ny = s.normal[1];
    v.nz = s.normal[2];
    s.immediate.push_back(v);
}

void glVertex2f(float x, float y)
{
    glVertex3f(x, y, 0.0f);
}

void glEnd()
{
    State& s = state();
    s.inBeginEnd = false;
    drawVertices(s.beginMode, s.immediate);
    s.immediate.clear();
}

void drawVertices(int mode, std::span<const GLVertex> vertices)
{
    State& s = state();
    if (vertices.empty())
        return;

    const bool polygons = mode >= GL_TRIANGLES;
    if (polygons && s.cullFace && s.cullMode == GL_FRONT_AND_BACK)
        return;

    const RenderTarget target = currentTarget();
    if (!target.rtv)
        return;

    const bool flat = s.shadeModel == GL_FLAT;
    const D3D11_PRIMITIVE_TOPOLOGY topology = buildIndices(mode, static_cast<uint32_t>(vertices.size()), flat, s.indices);
    if (topology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED || s.indices.empty())
        return;

    const UINT vertexBytes = static_cast<UINT>(vertices.size_bytes());
    const UINT vertexOffset = appendToRingBuffer(s.vertexBuffer, s.vertexBufferSize, s.vertexBufferOffset, D3D11_BIND_VERTEX_BUFFER, vertices.data(), vertexBytes);
    const UINT indexBytes = static_cast<UINT>(s.indices.size() * sizeof(uint32_t));
    const UINT indexOffset = appendToRingBuffer(s.indexBuffer, s.indexBufferSize, s.indexBufferOffset, D3D11_BIND_INDEX_BUFFER, s.indices.data(), indexBytes);

    DrawConstants constants {};
    fillConstants(constants);
    D3D11_MAPPED_SUBRESOURCE mapped;
    throwIfFailed(s.context->Map(s.constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map(constants)");
    std::memcpy(mapped.pData, &constants, sizeof(constants));
    s.context->Unmap(s.constantBuffer.Get(), 0);

    ID3D11DeviceContext* ctx = s.context;
    ctx->OMSetRenderTargets(1, &target.rtv, target.dsv);

    // Rows of every target are GL window rows (bottom-up storage), so GL coordinates apply directly.
    D3D11_VIEWPORT viewport {};
    viewport.TopLeftX = static_cast<float>(s.viewport[0]);
    viewport.TopLeftY = static_cast<float>(s.viewport[1]);
    viewport.Width = static_cast<float>(s.viewport[2]);
    viewport.Height = static_cast<float>(s.viewport[3]);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &viewport);
    if (s.scissorTest) {
        const D3D11_RECT rect { s.scissor[0], s.scissor[1], s.scissor[0] + s.scissor[2], s.scissor[1] + s.scissor[3] };
        ctx->RSSetScissorRects(1, &rect);
    }

    ctx->RSSetState(rasterizerState(polygons));
    ctx->OMSetDepthStencilState(depthState(), 0);
    ctx->OMSetBlendState(blendState(), nullptr, 0xFFFFFFFF);

    ctx->IASetInputLayout(s.inputLayout.Get());
    ctx->IASetPrimitiveTopology(topology);
    ID3D11Buffer* vertexBuffer = s.vertexBuffer.Get();
    const UINT stride = sizeof(GLVertex);
    ctx->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &vertexOffset);
    ctx->IASetIndexBuffer(s.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, indexOffset);

    ctx->VSSetShader(s.vertexShader.Get(), nullptr, 0);
    ctx->PSSetShader(flat ? s.pixelShaderFlat.Get() : s.pixelShader.Get(), nullptr, 0);
    ID3D11Buffer* cb = s.constantBuffer.Get();
    ctx->VSSetConstantBuffers(0, 1, &cb);
    ctx->PSSetConstantBuffers(0, 1, &cb);

    ID3D11ShaderResourceView* views[kTextureUnits] = {};
    ID3D11SamplerState* samplers[kTextureUnits] = {};
    for (int unit = 0; unit < kTextureUnits; ++unit) {
        if (const Texture* texture = usableTexture(unit)) {
            views[unit] = texture->srv.Get();
            samplers[unit] = samplerState(*texture);
        }
    }
    ctx->PSSetShaderResources(0, kTextureUnits, views);
    ctx->PSSetSamplers(0, kTextureUnits, samplers);

    ctx->DrawIndexed(static_cast<UINT>(s.indices.size()), 0, 0);
    ++s.drawCalls;
}

// --- Matrices ---

void glMatrixMode(int mode)
{
    state().matrixMode = mode;
}

void glLoadIdentity()
{
    activeMatrix() = Matrix4f::identity();
}

void glPushMatrix()
{
    activeStack().push_back(activeMatrix());
}

void glPopMatrix()
{
    auto& stack = activeStack();
    if (stack.empty()) {
        warnOnce("matrix stack underflow");
        return;
    }
    activeMatrix() = stack.back();
    stack.pop_back();
}

void glLoadMatrixf(const float* matrix)
{
    std::memcpy(activeMatrix().m.data(), matrix, sizeof(float) * 16);
}

void glMultMatrixf(const float* matrix)
{
    Matrix4f other;
    std::memcpy(other.m.data(), matrix, sizeof(float) * 16);
    activeMatrix() = activeMatrix() * other;
}

void glTranslatef(float x, float y, float z)
{
    activeMatrix().translate(x, y, z);
}

void glTranslated(double x, double y, double z)
{
    activeMatrix().translate(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

void glScalef(float x, float y, float z)
{
    activeMatrix().scale(x, y, z);
}

void glScaled(double x, double y, double z)
{
    activeMatrix().scale(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

void glRotatef(float angle, float x, float y, float z)
{
    activeMatrix().rotate(angle, x, y, z);
}

void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar)
{
    activeMatrix().ortho(left, right, bottom, top, zNear, zFar);
}

void glGetFloatv(int pname, float* params)
{
    State& s = state();
    if (pname == GL_MODELVIEW_MATRIX)
        std::memcpy(params, s.modelView.m.data(), sizeof(float) * 16);
    else if (pname == GL_PROJECTION_MATRIX)
        std::memcpy(params, s.projection.m.data(), sizeof(float) * 16);
    else if (pname == GL_FOG_COLOR)
        std::memcpy(params, s.fogColor, sizeof(float) * 4);
    else
        warnOnce("glGetFloatv(" + std::to_string(pname) + ") is not emulated");
}

const Matrix4f& currentMatrix(int mode)
{
    State& s = state();
    return mode == GL_PROJECTION ? s.projection : (mode == GL_TEXTURE ? s.units[s.activeTexture].matrix : s.modelView);
}

// --- Viewport, scissor, clear ---

void glViewport(int x, int y, int width, int height)
{
    State& s = state();
    s.viewport[0] = x;
    s.viewport[1] = y;
    s.viewport[2] = width;
    s.viewport[3] = height;
}

void glScissor(int x, int y, int width, int height)
{
    State& s = state();
    s.scissor[0] = x;
    s.scissor[1] = y;
    s.scissor[2] = width;
    s.scissor[3] = height;
}

void glClearColor(float red, float green, float blue, float alpha)
{
    State& s = state();
    s.clearColor[0] = red;
    s.clearColor[1] = green;
    s.clearColor[2] = blue;
    s.clearColor[3] = alpha;
}

void glClearDepth(double depth)
{
    state().clearDepth = static_cast<float>(depth);
}

void glClear(int mask)
{
    State& s = state();
    const RenderTarget target = currentTarget();
    if (s.scissorTest)
        warnOnce("glClear with the scissor test enabled clears the whole target");

    if ((mask & GL_COLOR_BUFFER_BIT) && target.rtv) {
        const bool anyChannel = s.colorMask[0] || s.colorMask[1] || s.colorMask[2] || s.colorMask[3];
        const bool allChannels = s.colorMask[0] && s.colorMask[1] && s.colorMask[2] && s.colorMask[3];
        if (anyChannel && !allChannels)
            warnOnce("glClear with a partial colour mask clears all channels");
        if (anyChannel) {
            float color[4];
            for (int i = 0; i < 4; ++i)
                color[i] = std::clamp(s.clearColor[i], 0.0f, 1.0f);
            s.context->ClearRenderTargetView(target.rtv, color);
        }
    }
    // GL honours glDepthMask when clearing.
    if ((mask & GL_DEPTH_BUFFER_BIT) && target.dsv && s.depthMask)
        s.context->ClearDepthStencilView(target.dsv, D3D11_CLEAR_DEPTH, std::clamp(s.clearDepth, 0.0f, 1.0f), 0);
}

// --- Textures ---

void glActiveTexture(int texture)
{
    const int unit = texture - GL_TEXTURE0;
    if (unit < 0 || unit >= kTextureUnits) {
        warnOnce("texture unit " + std::to_string(unit) + " is not emulated");
        return;
    }
    state().activeTexture = unit;
}

int glGenTextures()
{
    State& s = state();
    const int name = s.nextName++;
    s.textures.emplace(name, Texture {});
    return name;
}

void glDeleteTextures(int texture)
{
    State& s = state();
    s.textures.erase(texture);
    for (TextureUnit& unit : s.units) {
        if (unit.bound == texture)
            unit.bound = 0;
    }
}

void glBindTexture(int /*target*/, int texture)
{
    State& s = state();
    // GL creates the texture object on first bind of an unused name.
    if (texture != 0 && !s.textures.contains(texture))
        s.textures.emplace(texture, Texture {});
    s.units[s.activeTexture].bound = texture;
}

void glTexParameteri(int target, int pname, int param)
{
    glTexParameterf(target, pname, static_cast<float>(param));
}

void glTexParameterf(int /*target*/, int pname, float param)
{
    Texture* texture = boundTexture();
    if (!texture)
        return;
    switch (pname) {
    case GL_TEXTURE_MIN_FILTER: texture->minFilter = static_cast<int>(param); break;
    case GL_TEXTURE_MAG_FILTER: texture->magFilter = static_cast<int>(param); break;
    case GL_TEXTURE_WRAP_S: texture->wrapS = static_cast<int>(param); break;
    case GL_TEXTURE_WRAP_T: texture->wrapT = static_cast<int>(param); break;
    case GL_TEXTURE_MAX_LEVEL: texture->maxLevel = static_cast<int>(param); break;
    case GL_TEXTURE_MIN_LOD: texture->minLod = param; break;
    case GL_TEXTURE_MAX_LOD: texture->maxLod = param; break;
    case GL_TEXTURE_LOD_BIAS: texture->lodBias = param; break;
    default: break;
    }
}

void glTexEnvi(int /*target*/, int pname, int param)
{
    State& s = state();
    if (pname == GL_TEXTURE_ENV_MODE) {
        s.units[s.activeTexture].envMode = param;
        if (param != GL_MODULATE)
            warnOnce("texture environment modes other than GL_MODULATE are not emulated yet");
    }
}

void glTexImage2D(int /*target*/, int level, int width, int height, const uint32_t* pixels)
{
    State& s = state();
    Texture* texture = boundTexture();
    if (!texture) {
        warnOnce("glTexImage2D without a bound texture");
        return;
    }

    if (level == 0) {
        const int levels = texture->maxLevel >= 1000 ? 1 : std::clamp(texture->maxLevel + 1, 1, fullMipChain(width, height));
        if (!texture->texture || texture->width != width || texture->height != height || texture->levels != levels)
            createTextureStorage(*texture, width, height, levels);
    } else if (!texture->texture || level >= texture->levels) {
        // A mip level beyond the allocated chain: grow the chain, keeping existing levels.
        const int baseWidth = texture->texture ? texture->width : width << level;
        const int baseHeight = texture->texture ? texture->height : height << level;
        Texture grown = *texture;
        createTextureStorage(grown, baseWidth, baseHeight, std::min(level + 1, fullMipChain(baseWidth, baseHeight)));
        for (int i = 0; texture->texture && i < texture->levels; ++i)
            s.context->CopySubresourceRegion(grown.texture.Get(), static_cast<UINT>(i), 0, 0, 0, texture->texture.Get(), static_cast<UINT>(i), nullptr);
        *texture = grown;
    }

    if (pixels)
        glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, pixels);
}

void glTexSubImage2D(int /*target*/, int level, int x, int y, int width, int height, const uint32_t* pixels)
{
    State& s = state();
    Texture* texture = boundTexture();
    if (!texture || !texture->texture || level >= texture->levels || width <= 0 || height <= 0)
        return;

    const D3D11_BOX box { static_cast<UINT>(x), static_cast<UINT>(y), 0, static_cast<UINT>(x + width), static_cast<UINT>(y + height), 1 };
    s.context->UpdateSubresource(texture->texture.Get(), static_cast<UINT>(level), &box, pixels, static_cast<UINT>(width) * 4u, 0);
}

void glCopyTexSubImage2D(int /*target*/, int level, int xOffset, int yOffset, int x, int y, int width, int height)
{
    State& s = state();
    Texture* destination = boundTexture();
    const RenderTarget source = currentTarget();
    if (!destination || !destination->texture || !source.color || level >= destination->levels)
        return;

    // Clip the source rectangle to the framebuffer, shifting the destination with it.
    if (x < 0) { xOffset -= x; width += x; x = 0; }
    if (y < 0) { yOffset -= y; height += y; y = 0; }
    width = std::min({ width, source.width - x, (destination->width >> level) - xOffset });
    height = std::min({ height, source.height - y, (destination->height >> level) - yOffset });
    if (width <= 0 || height <= 0)
        return;

    const D3D11_BOX box { static_cast<UINT>(x), static_cast<UINT>(y), 0, static_cast<UINT>(x + width), static_cast<UINT>(y + height), 1 };
    s.context->CopySubresourceRegion(destination->texture.Get(), static_cast<UINT>(level), static_cast<UINT>(xOffset), static_cast<UINT>(yOffset), 0,
        source.color, 0, &box);
}

namespace {

std::vector<uint32_t> readTexture(ID3D11Texture2D* source, UINT subresource, int x, int y, int width, int height)
{
    State& s = state();
    std::vector<uint32_t> pixels;
    if (!source || width <= 0 || height <= 0)
        return pixels;

    D3D11_TEXTURE2D_DESC desc {};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = kColorFormat;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    throwIfFailed(s.d3d->CreateTexture2D(&desc, nullptr, staging.GetAddressOf()), "CreateTexture2D(staging)");

    const D3D11_BOX box { static_cast<UINT>(x), static_cast<UINT>(y), 0, static_cast<UINT>(x + width), static_cast<UINT>(y + height), 1 };
    s.context->CopySubresourceRegion(staging.Get(), 0, 0, 0, 0, source, subresource, &box);

    D3D11_MAPPED_SUBRESOURCE mapped;
    throwIfFailed(s.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Map(staging)");
    pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
    for (int row = 0; row < height; ++row)
        std::memcpy(&pixels[static_cast<size_t>(row) * static_cast<size_t>(width)], static_cast<const uint8_t*>(mapped.pData) + static_cast<size_t>(row) * mapped.RowPitch, static_cast<size_t>(width) * 4);
    s.context->Unmap(staging.Get(), 0);
    return pixels;
}

} // namespace

std::vector<uint32_t> glGetTexImage(int /*target*/, int level)
{
    const Texture* texture = boundTexture();
    if (!texture || !texture->texture || level >= texture->levels)
        return {};
    return readTexture(texture->texture.Get(), static_cast<UINT>(level), 0, 0, std::max(1, texture->width >> level), std::max(1, texture->height >> level));
}

int textureWidth(int texture)
{
    const Texture* t = findTexture(texture);
    return t ? t->width : 0;
}

int textureHeight(int texture)
{
    const Texture* t = findTexture(texture);
    return t ? t->height : 0;
}

// --- Framebuffer objects ---

int glGenFramebuffers()
{
    State& s = state();
    const int name = s.nextName++;
    s.framebuffers.emplace(name, Framebuffer {});
    return name;
}

void glDeleteFramebuffers(int framebuffer)
{
    State& s = state();
    s.framebuffers.erase(framebuffer);
    if (s.boundFramebuffer == framebuffer)
        s.boundFramebuffer = 0;
}

void glBindFramebuffer(int framebuffer)
{
    state().boundFramebuffer = framebuffer;
}

void glFramebufferTexture2D(int texture)
{
    State& s = state();
    if (const auto it = s.framebuffers.find(s.boundFramebuffer); it != s.framebuffers.end())
        it->second.colorTexture = texture;
}

int glGenRenderbuffers()
{
    State& s = state();
    const int name = s.nextName++;
    s.renderbuffers.emplace(name, Renderbuffer {});
    return name;
}

void glDeleteRenderbuffers(int renderbuffer)
{
    state().renderbuffers.erase(renderbuffer);
}

void glRenderbufferStorage(int renderbuffer, int width, int height)
{
    State& s = state();
    if (const auto it = s.renderbuffers.find(renderbuffer); it != s.renderbuffers.end())
        createRenderbufferStorage(it->second, width, height);
}

void glFramebufferRenderbuffer(int renderbuffer)
{
    State& s = state();
    if (const auto it = s.framebuffers.find(s.boundFramebuffer); it != s.framebuffers.end())
        it->second.depthRenderbuffer = renderbuffer;
}

std::vector<uint32_t> glReadPixels(int x, int y, int width, int height)
{
    const RenderTarget source = currentTarget();
    if (!source.color || x < 0 || y < 0 || x + width > source.width || y + height > source.height)
        return {};
    return readTexture(source.color, 0, x, y, width, height);
}

int drawCallsLastFrame()
{
    return state().drawCallsLastFrame;
}

} // namespace mc::GL
