#include "client/renderer/Tessellator.h"

#include "client/renderer/DxStateManager.h"
#include "render/gl/GL.h"

#include <cstring>

namespace mc {

namespace {

template <class T>
T readAt(const uint8_t* data)
{
    T value;
    std::memcpy(&value, data, sizeof(T));
    return value;
}

// Reads component `i` of an element as GL's vertex array fetch would: integer colours and normals are
// normalized, integer texture coordinates and positions are not.
float component(const uint8_t* data, VertexFormatElement::EnumType type, int i, bool normalized)
{
    using T = VertexFormatElement::EnumType;
    switch (type) {
    case T::FLOAT: return readAt<float>(data + i * 4);
    case T::UBYTE: return normalized ? static_cast<float>(data[i]) / 255.0f : static_cast<float>(data[i]);
    case T::BYTE: {
        const auto v = static_cast<int8_t>(data[i]);
        // GL 2.x signed normalization: (2c + 1) / (2^8 - 1)
        return normalized ? (2.0f * static_cast<float>(v) + 1.0f) / 255.0f : static_cast<float>(v);
    }
    case T::USHORT: {
        const auto v = readAt<uint16_t>(data + i * 2);
        return normalized ? static_cast<float>(v) / 65535.0f : static_cast<float>(v);
    }
    case T::SHORT: {
        const auto v = readAt<int16_t>(data + i * 2);
        return normalized ? (2.0f * static_cast<float>(v) + 1.0f) / 65535.0f : static_cast<float>(v);
    }
    case T::UINT: return static_cast<float>(readAt<uint32_t>(data + i * 4));
    case T::INT: return static_cast<float>(readAt<int32_t>(data + i * 4));
    }
    return 0.0f;
}

} // namespace

void convertVertexData(const WorldRenderer& worldRenderer, std::vector<GLVertex>& vertices)
{
    convertVertexData(worldRenderer.getByteBuffer(), worldRenderer.getVertexFormat(), worldRenderer.getVertexCount(), vertices);
}

void convertVertexData(std::span<const uint8_t> bytes, const VertexFormat& format, int vertexCount, std::vector<GLVertex>& vertices)
{
    const int stride = format.getNextOffset();

    GLVertex defaults {};
    const float* color = GL::currentColor();
    const float* normal = GL::currentNormal();
    const float* uv0 = GL::currentTexCoord(0);
    const float* uv1 = GL::currentTexCoord(1);
    defaults.r = color[0];
    defaults.g = color[1];
    defaults.b = color[2];
    defaults.a = color[3];
    defaults.u0 = uv0[0];
    defaults.v0 = uv0[1];
    defaults.u1 = uv1[0];
    defaults.v1 = uv1[1];
    defaults.nx = normal[0];
    defaults.ny = normal[1];
    defaults.nz = normal[2];

    vertices.assign(static_cast<size_t>(vertexCount), defaults);
    for (int e = 0; e < format.getElementCount(); ++e) {
        const VertexFormatElement& element = format.getElement(e);
        const int offset = format.getOffset(e);
        const auto type = element.getType();
        const int count = element.getElementCount();

        for (size_t v = 0; v < vertices.size(); ++v) {
            const uint8_t* data = bytes.data() + v * static_cast<size_t>(stride) + static_cast<size_t>(offset);
            GLVertex& out = vertices[v];
            switch (element.getUsage()) {
            case VertexFormatElement::EnumUsage::POSITION:
                out.x = component(data, type, 0, false);
                out.y = count > 1 ? component(data, type, 1, false) : 0.0f;
                out.z = count > 2 ? component(data, type, 2, false) : 0.0f;
                break;
            case VertexFormatElement::EnumUsage::COLOR:
                out.r = component(data, type, 0, true);
                out.g = component(data, type, 1, true);
                out.b = component(data, type, 2, true);
                out.a = count > 3 ? component(data, type, 3, true) : 1.0f;
                break;
            case VertexFormatElement::EnumUsage::UV:
                if (element.getIndex() == 0) {
                    out.u0 = component(data, type, 0, false);
                    out.v0 = count > 1 ? component(data, type, 1, false) : 0.0f;
                } else if (element.getIndex() == 1) {
                    out.u1 = component(data, type, 0, false);
                    out.v1 = count > 1 ? component(data, type, 1, false) : 0.0f;
                }
                break;
            case VertexFormatElement::EnumUsage::NORMAL:
                out.nx = component(data, type, 0, true);
                out.ny = component(data, type, 1, true);
                out.nz = component(data, type, 2, true);
                break;
            default:
                break;
            }
        }
    }
}

void WorldVertexBufferUploader::draw(WorldRenderer& worldRenderer)
{
    if (worldRenderer.getVertexCount() > 0) {
        convertVertexData(worldRenderer, vertices);
        GL::drawVertices(worldRenderer.getDrawMode(), vertices);

        if (worldRenderer.getVertexFormat().hasColor())
            DxStateManager::resetColor();
    }

    worldRenderer.reset();
}

Tessellator::Tessellator(int bufferSize)
    : worldRenderer(bufferSize)
{
}

Tessellator& Tessellator::getInstance()
{
    static Tessellator instance(2097152);
    return instance;
}

void Tessellator::draw()
{
    worldRenderer.finishDrawing();
    vboUploader.draw(worldRenderer);
}

} // namespace mc
