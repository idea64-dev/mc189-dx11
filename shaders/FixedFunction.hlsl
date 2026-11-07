// OpenGL 1.x fixed-function pipeline as used by Minecraft 1.8.9, in one shader driven by constants.
// Compiled at build time by fxc (see CMakeLists.txt). Must match DrawConstants in src/render/gl/GL.cpp.

#define FLAG_TEXTURE0       0x01
#define FLAG_TEXTURE1       0x02
#define FLAG_LIGHTING       0x04
#define FLAG_LIGHT0         0x08
#define FLAG_LIGHT1         0x10
#define FLAG_COLOR_MATERIAL 0x20
#define FLAG_NORMALIZE      0x40

#define FOG_NONE   0
#define FOG_LINEAR 1
#define FOG_EXP    2
#define FOG_EXP2   3

cbuffer DrawConstants : register(b0)
{
    float4x4 modelView;
    float4x4 projection;
    float4x4 textureMatrix0;
    float4x4 textureMatrix1;
    float4 normalMatrix[3];     // rows of (modelView^-1)^T, upper 3x3
    float4 fogColor;
    float4 fogParams;           // start, end, density, 1 / (end - start)
    int fogMode;
    int alphaFunc;              // GL_NEVER (0x200) .. GL_ALWAYS (0x207); 0 = alpha test disabled
    float alphaRef;
    int flags;
    float4 lightModelAmbient;
    float4 lightPosition[2];    // eye space, as glLight(GL_POSITION) stores it
    float4 lightDiffuse[2];
    float4 lightAmbient[2];
};

Texture2D texture0 : register(t0);
Texture2D texture1 : register(t1);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float3 normal : NORMAL;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float eyeDepth : TEXCOORD2;
};

float2 transformTexCoord(float4x4 m, float2 uv)
{
    float4 t = mul(m, float4(uv, 0.0, 1.0));
    return t.xy / t.w;
}

float4 applyLighting(float4 vertexColor, float3 objectNormal)
{
    float3 n = float3(dot(normalMatrix[0].xyz, objectNormal), dot(normalMatrix[1].xyz, objectNormal), dot(normalMatrix[2].xyz, objectNormal));
    if (flags & FLAG_NORMALIZE)
        n = normalize(n);

    // GL material defaults, replaced by the vertex colour under glColorMaterial(GL_AMBIENT_AND_DIFFUSE).
    float4 ambient = (flags & FLAG_COLOR_MATERIAL) ? vertexColor : float4(0.2, 0.2, 0.2, 1.0);
    float4 diffuse = (flags & FLAG_COLOR_MATERIAL) ? vertexColor : float4(0.8, 0.8, 0.8, 1.0);

    float3 color = lightModelAmbient.rgb * ambient.rgb;
    [unroll]
    for (int i = 0; i < 2; ++i)
    {
        if (flags & (FLAG_LIGHT0 << i))
        {
            float3 toLight = normalize(lightPosition[i].xyz); // Minecraft only uses directional lights (w = 0)
            color += lightAmbient[i].rgb * ambient.rgb + max(dot(n, toLight), 0.0) * lightDiffuse[i].rgb * diffuse.rgb;
        }
    }
    return float4(saturate(color), diffuse.a);
}

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    float4 eye = mul(modelView, float4(input.position, 1.0));
    float4 clip = mul(projection, eye);
    clip.z = (clip.z + clip.w) * 0.5; // GL clip depth [-w, w] -> D3D [0, w]
    clip.y = -clip.y;                 // render targets are stored bottom-up, like GL framebuffers
    output.position = clip;

    float4 color = saturate(input.color);
    if (flags & FLAG_LIGHTING)
        color = applyLighting(color, input.normal);
    output.color = color;

    output.uv0 = transformTexCoord(textureMatrix0, input.uv0);
    output.uv1 = transformTexCoord(textureMatrix1, input.uv1);
    output.eyeDepth = abs(eye.z / eye.w);
    return output;
}

bool alphaTestPasses(float alpha)
{
    switch (alphaFunc)
    {
    case 0x200: return false;
    case 0x201: return alpha < alphaRef;
    case 0x202: return alpha == alphaRef;
    case 0x203: return alpha <= alphaRef;
    case 0x204: return alpha > alphaRef;
    case 0x205: return alpha != alphaRef;
    case 0x206: return alpha >= alphaRef;
    default: return true;
    }
}

float4 shadeFragment(float4 color, float2 uv0, float2 uv1, float eyeDepth)
{
    // Sample unconditionally (implicit derivatives must not sit in flow control), then select.
    float4 t0 = texture0.Sample(sampler0, uv0);
    float4 t1 = texture1.Sample(sampler1, uv1);
    if (flags & FLAG_TEXTURE0)
        color *= t0; // GL_MODULATE
    if (flags & FLAG_TEXTURE1)
        color *= t1;

    if (fogMode != FOG_NONE)
    {
        float f;
        if (fogMode == FOG_LINEAR)
            f = (fogParams.y - eyeDepth) * fogParams.w;
        else if (fogMode == FOG_EXP)
            f = exp(-fogParams.z * eyeDepth);
        else
            f = exp(-(fogParams.z * eyeDepth) * (fogParams.z * eyeDepth));
        color.rgb = lerp(fogColor.rgb, color.rgb, saturate(f));
    }

    if (!alphaTestPasses(color.a))
        discard;
    return color;
}

float4 PSMain(VertexOutput input) : SV_Target
{
    return shadeFragment(input.color, input.uv0, input.uv1, input.eyeDepth);
}

// glShadeModel(GL_FLAT): the colour comes from the provoking vertex, which the index order puts first.
struct FlatInput
{
    float4 position : SV_Position;
    nointerpolation float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float eyeDepth : TEXCOORD2;
};

float4 PSMainFlat(FlatInput input) : SV_Target
{
    return shadeFragment(input.color, input.uv0, input.uv1, input.eyeDepth);
}

// --- Present: copies the GL-style (bottom-up) window framebuffer to the swapchain, flipping it upright ---

Texture2D presentSource : register(t0);

float4 PresentVS(uint id : SV_VertexID, out float2 uv : TEXCOORD0) : SV_Position
{
    float2 p = float2((id << 1) & 2, id & 2); // fullscreen triangle
    uv = float2(p.x, p.y);
    return float4(p.x * 2.0 - 1.0, 1.0 - p.y * 2.0, 0.0, 1.0);
}

float4 PresentPS(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
    uint width, height;
    presentSource.GetDimensions(width, height);
    int2 texel = int2(position.xy);
    return float4(presentSource.Load(int3(texel.x, int(height) - 1 - texel.y, 0)).rgb, 1.0);
}
