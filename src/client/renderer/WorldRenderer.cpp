#include "client/renderer/WorldRenderer.h"

#include "core/Java.h"
#include "core/Log.h"
#include "util/MathHelper.h"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <stdexcept>

namespace mc {

namespace {

constexpr int kGrowStep = 2097152; // bytes, as in Java

// Floats.compare / Float.compare: total order with -0.0 < 0.0 and NaN above everything.
int floatCompare(float a, float b)
{
    if (a < b)
        return -1;
    if (a > b)
        return 1;
    const int32_t bitsA = java::floatToRawIntBits(a != a ? std::numeric_limits<float>::quiet_NaN() : a);
    const int32_t bitsB = java::floatToRawIntBits(b != b ? std::numeric_limits<float>::quiet_NaN() : b);
    return bitsA == bitsB ? 0 : (bitsA < bitsB ? -1 : 1);
}

} // namespace

WorldRenderer::WorldRenderer(int bufferSizeIn)
    : byteBuffer(static_cast<size_t>(bufferSizeIn) * 4)
{
}

int32_t WorldRenderer::getInt(int intIndex) const
{
    int32_t value;
    std::memcpy(&value, byteBuffer.data() + static_cast<size_t>(intIndex) * 4, 4);
    return value;
}

void WorldRenderer::putInt(int intIndex, int32_t value)
{
    std::memcpy(byteBuffer.data() + static_cast<size_t>(intIndex) * 4, &value, 4);
}

void WorldRenderer::putFloatAt(int byteIndex, float value)
{
    std::memcpy(byteBuffer.data() + byteIndex, &value, 4);
}

void WorldRenderer::putIntAt(int byteIndex, int32_t value)
{
    std::memcpy(byteBuffer.data() + byteIndex, &value, 4);
}

void WorldRenderer::putShortAt(int byteIndex, int16_t value)
{
    std::memcpy(byteBuffer.data() + byteIndex, &value, 2);
}

void WorldRenderer::putByteAt(int byteIndex, int8_t value)
{
    byteBuffer[static_cast<size_t>(byteIndex)] = static_cast<uint8_t>(value);
}

void WorldRenderer::growBuffer(int ints)
{
    // Room for the data so far, `ints` more, and the vertex being written next.
    const size_t needed = (static_cast<size_t>(getBufferSize()) + static_cast<size_t>(ints) + static_cast<size_t>(vertexFormat.getIntegerSize())) * 4;
    if (needed <= byteBuffer.size())
        return;

    const size_t oldSize = byteBuffer.size();
    const size_t newSize = (needed / kGrowStep + 1) * kGrowStep;
    Log::warn("Needed to grow BufferBuilder buffer: Old size {} bytes, new size {} bytes.", oldSize, newSize);
    byteBuffer.resize(newSize);
}

void WorldRenderer::sortVertexData(float x, float y, float z)
{
    const int quadCount = vertexCount / 4;
    const int stride = vertexFormat.getIntegerSize();
    std::vector<float> distances(static_cast<size_t>(quadCount));

    for (int j = 0; j < quadCount; ++j) {
        const int base = j * vertexFormat.getNextOffset();
        const auto read = [&](int vertex, int component) {
            return java::intBitsToFloat(getInt(base + stride * vertex + component));
        };
        const float cx = (read(0, 0) + read(1, 0) + read(2, 0) + read(3, 0)) * 0.25f - static_cast<float>(static_cast<double>(x) + xOffset);
        const float cy = (read(0, 1) + read(1, 1) + read(2, 1) + read(3, 1)) * 0.25f - static_cast<float>(static_cast<double>(y) + yOffset);
        const float cz = (read(0, 2) + read(1, 2) + read(2, 2) + read(3, 2)) * 0.25f - static_cast<float>(static_cast<double>(z) + zOffset);
        distances[static_cast<size_t>(j)] = cx * cx + cy * cy + cz * cz;
    }

    // Java sorts Integer[] with TimSort (stable), farthest quad first.
    std::vector<int> order(static_cast<size_t>(quadCount));
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return floatCompare(distances[static_cast<size_t>(b)], distances[static_cast<size_t>(a)]) < 0;
    });

    // Java permutes in place with a cycle walk; copying into a fresh buffer gives the same layout.
    const size_t quadBytes = static_cast<size_t>(vertexFormat.getNextOffset()) * 4;
    std::vector<uint8_t> sorted(quadBytes * static_cast<size_t>(quadCount));
    for (int i = 0; i < quadCount; ++i)
        std::memcpy(sorted.data() + quadBytes * static_cast<size_t>(i), byteBuffer.data() + quadBytes * static_cast<size_t>(order[static_cast<size_t>(i)]), quadBytes);
    std::memcpy(byteBuffer.data(), sorted.data(), sorted.size());
}

WorldRenderer::State WorldRenderer::getVertexState() const
{
    std::vector<int32_t> data(static_cast<size_t>(getBufferSize()));
    std::memcpy(data.data(), byteBuffer.data(), data.size() * 4);
    return State(std::move(data), vertexFormat);
}

void WorldRenderer::setVertexState(const State& state)
{
    vertexCount = 0;
    vertexFormat = state.getVertexFormat();
    growBuffer(static_cast<int>(state.getRawBuffer().size()));
    std::memcpy(byteBuffer.data(), state.getRawBuffer().data(), state.getRawBuffer().size() * 4);
    vertexCount = state.getVertexCount();
}

void WorldRenderer::reset()
{
    vertexCount = 0;
    vertexFormatIndex = 0;
}

void WorldRenderer::begin(int glMode, const VertexFormat& format)
{
    if (isDrawing)
        throw std::logic_error("Already building!");

    isDrawing = true;
    reset();
    drawMode = glMode;
    vertexFormat = format;
    noColorFlag = false;
    growBuffer(0);
}

WorldRenderer& WorldRenderer::tex(double u, double v)
{
    const int i = vertexCount * vertexFormat.getNextOffset() + vertexFormat.getOffset(vertexFormatIndex);
    switch (vertexFormat.getElement(vertexFormatIndex).getType()) {
    case VertexFormatElement::EnumType::FLOAT:
        putFloatAt(i, static_cast<float>(u));
        putFloatAt(i + 4, static_cast<float>(v));
        break;
    case VertexFormatElement::EnumType::UINT:
    case VertexFormatElement::EnumType::INT:
        putIntAt(i, java::d2i(u));
        putIntAt(i + 4, java::d2i(v));
        break;
    case VertexFormatElement::EnumType::USHORT:
    case VertexFormatElement::EnumType::SHORT:
        putShortAt(i, static_cast<int16_t>(java::d2i(v)));
        putShortAt(i + 2, static_cast<int16_t>(java::d2i(u)));
        break;
    case VertexFormatElement::EnumType::UBYTE:
    case VertexFormatElement::EnumType::BYTE:
        putByteAt(i, static_cast<int8_t>(java::d2i(v)));
        putByteAt(i + 1, static_cast<int8_t>(java::d2i(u)));
        break;
    }
    nextVertexFormatIndex();
    return *this;
}

WorldRenderer& WorldRenderer::lightmap(int skyLight, int blockLight)
{
    const int i = vertexCount * vertexFormat.getNextOffset() + vertexFormat.getOffset(vertexFormatIndex);
    switch (vertexFormat.getElement(vertexFormatIndex).getType()) {
    case VertexFormatElement::EnumType::FLOAT:
        putFloatAt(i, static_cast<float>(skyLight));
        putFloatAt(i + 4, static_cast<float>(blockLight));
        break;
    case VertexFormatElement::EnumType::UINT:
    case VertexFormatElement::EnumType::INT:
        putIntAt(i, skyLight);
        putIntAt(i + 4, blockLight);
        break;
    case VertexFormatElement::EnumType::USHORT:
    case VertexFormatElement::EnumType::SHORT:
        putShortAt(i, static_cast<int16_t>(blockLight));
        putShortAt(i + 2, static_cast<int16_t>(skyLight));
        break;
    case VertexFormatElement::EnumType::UBYTE:
    case VertexFormatElement::EnumType::BYTE:
        putByteAt(i, static_cast<int8_t>(blockLight));
        putByteAt(i + 1, static_cast<int8_t>(skyLight));
        break;
    }
    nextVertexFormatIndex();
    return *this;
}

void WorldRenderer::putBrightness4(int b0, int b1, int b2, int b3)
{
    const int i = (vertexCount - 4) * vertexFormat.getIntegerSize() + vertexFormat.getUvOffsetById(1) / 4;
    const int j = vertexFormat.getNextOffset() >> 2;
    putInt(i, b0);
    putInt(i + j, b1);
    putInt(i + j * 2, b2);
    putInt(i + j * 3, b3);
}

void WorldRenderer::putPosition(double x, double y, double z)
{
    const int stride = vertexFormat.getIntegerSize();
    const int base = (vertexCount - 4) * stride;
    for (int k = 0; k < 4; ++k) {
        const int l = base + k * stride;
        putInt(l, java::floatToRawIntBits(static_cast<float>(x + xOffset) + java::intBitsToFloat(getInt(l))));
        putInt(l + 1, java::floatToRawIntBits(static_cast<float>(y + yOffset) + java::intBitsToFloat(getInt(l + 1))));
        putInt(l + 2, java::floatToRawIntBits(static_cast<float>(z + zOffset) + java::intBitsToFloat(getInt(l + 2))));
    }
}

int WorldRenderer::getColorIndex(int vertexIndex) const
{
    return ((vertexCount - vertexIndex) * vertexFormat.getNextOffset() + vertexFormat.getColorOffset()) / 4;
}

void WorldRenderer::putColorMultiplier(float red, float green, float blue, int vertexIndex)
{
    const int i = getColorIndex(vertexIndex);
    int j = -1;
    if (!noColorFlag) {
        j = getInt(i);
        const int k = java::f2i(static_cast<float>(j & 255) * red);
        const int l = java::f2i(static_cast<float>((j >> 8) & 255) * green);
        const int i1 = java::f2i(static_cast<float>((j >> 16) & 255) * blue);
        j = j & -16777216;
        j = j | java::shl(i1, 16) | java::shl(l, 8) | k;
    }
    putInt(i, j);
}

void WorldRenderer::putColor(int argb, int vertexIndex)
{
    const int i = getColorIndex(vertexIndex);
    putColorRGBA(i, (argb >> 16) & 255, (argb >> 8) & 255, argb & 255, (argb >> 24) & 255);
}

void WorldRenderer::putColorRGB_F(float red, float green, float blue, int vertexIndex)
{
    const int i = getColorIndex(vertexIndex);
    const int j = MathHelper::clamp_int(java::f2i(red * 255.0f), 0, 255);
    const int k = MathHelper::clamp_int(java::f2i(green * 255.0f), 0, 255);
    const int l = MathHelper::clamp_int(java::f2i(blue * 255.0f), 0, 255);
    putColorRGBA(i, j, k, l, 255);
}

void WorldRenderer::putColorRGBA(int index, int red, int green, int blue, int alpha)
{
    putInt(index, java::shl(alpha, 24) | java::shl(blue, 16) | java::shl(green, 8) | red);
}

void WorldRenderer::noColor()
{
    noColorFlag = true;
}

WorldRenderer& WorldRenderer::color(float red, float green, float blue, float alpha)
{
    return color(java::f2i(red * 255.0f), java::f2i(green * 255.0f), java::f2i(blue * 255.0f), java::f2i(alpha * 255.0f));
}

WorldRenderer& WorldRenderer::color(int red, int green, int blue, int alpha)
{
    if (noColorFlag)
        return *this;

    const int i = vertexCount * vertexFormat.getNextOffset() + vertexFormat.getOffset(vertexFormatIndex);
    switch (vertexFormat.getElement(vertexFormatIndex).getType()) {
    case VertexFormatElement::EnumType::FLOAT:
        putFloatAt(i, static_cast<float>(red) / 255.0f);
        putFloatAt(i + 4, static_cast<float>(green) / 255.0f);
        putFloatAt(i + 8, static_cast<float>(blue) / 255.0f);
        putFloatAt(i + 12, static_cast<float>(alpha) / 255.0f);
        break;
    case VertexFormatElement::EnumType::UINT:
    case VertexFormatElement::EnumType::INT:
        putFloatAt(i, static_cast<float>(red));
        putFloatAt(i + 4, static_cast<float>(green));
        putFloatAt(i + 8, static_cast<float>(blue));
        putFloatAt(i + 12, static_cast<float>(alpha));
        break;
    case VertexFormatElement::EnumType::USHORT:
    case VertexFormatElement::EnumType::SHORT:
        putShortAt(i, static_cast<int16_t>(red));
        putShortAt(i + 2, static_cast<int16_t>(green));
        putShortAt(i + 4, static_cast<int16_t>(blue));
        putShortAt(i + 6, static_cast<int16_t>(alpha));
        break;
    case VertexFormatElement::EnumType::UBYTE:
    case VertexFormatElement::EnumType::BYTE:
        putByteAt(i, static_cast<int8_t>(red));
        putByteAt(i + 1, static_cast<int8_t>(green));
        putByteAt(i + 2, static_cast<int8_t>(blue));
        putByteAt(i + 3, static_cast<int8_t>(alpha));
        break;
    }
    nextVertexFormatIndex();
    return *this;
}

void WorldRenderer::addVertexData(std::span<const int32_t> vertexData)
{
    growBuffer(static_cast<int>(vertexData.size()));
    std::memcpy(byteBuffer.data() + static_cast<size_t>(getBufferSize()) * 4, vertexData.data(), vertexData.size_bytes());
    vertexCount += static_cast<int>(vertexData.size()) / vertexFormat.getIntegerSize();
}

void WorldRenderer::endVertex()
{
    ++vertexCount;
    growBuffer(vertexFormat.getIntegerSize());
}

WorldRenderer& WorldRenderer::pos(double x, double y, double z)
{
    const int i = vertexCount * vertexFormat.getNextOffset() + vertexFormat.getOffset(vertexFormatIndex);
    switch (vertexFormat.getElement(vertexFormatIndex).getType()) {
    case VertexFormatElement::EnumType::FLOAT:
        putFloatAt(i, static_cast<float>(x + xOffset));
        putFloatAt(i + 4, static_cast<float>(y + yOffset));
        putFloatAt(i + 8, static_cast<float>(z + zOffset));
        break;
    case VertexFormatElement::EnumType::UINT:
    case VertexFormatElement::EnumType::INT:
        putIntAt(i, java::floatToRawIntBits(static_cast<float>(x + xOffset)));
        putIntAt(i + 4, java::floatToRawIntBits(static_cast<float>(y + yOffset)));
        putIntAt(i + 8, java::floatToRawIntBits(static_cast<float>(z + zOffset)));
        break;
    case VertexFormatElement::EnumType::USHORT:
    case VertexFormatElement::EnumType::SHORT:
        putShortAt(i, static_cast<int16_t>(java::d2i(x + xOffset)));
        putShortAt(i + 2, static_cast<int16_t>(java::d2i(y + yOffset)));
        putShortAt(i + 4, static_cast<int16_t>(java::d2i(z + zOffset)));
        break;
    case VertexFormatElement::EnumType::UBYTE:
    case VertexFormatElement::EnumType::BYTE:
        putByteAt(i, static_cast<int8_t>(java::d2i(x + xOffset)));
        putByteAt(i + 1, static_cast<int8_t>(java::d2i(y + yOffset)));
        putByteAt(i + 2, static_cast<int8_t>(java::d2i(z + zOffset)));
        break;
    }
    nextVertexFormatIndex();
    return *this;
}

void WorldRenderer::putNormal(float x, float y, float z)
{
    const int i = static_cast<int8_t>(java::f2i(x * 127.0f)) & 255;
    const int j = static_cast<int8_t>(java::f2i(y * 127.0f)) & 255;
    const int k = static_cast<int8_t>(java::f2i(z * 127.0f)) & 255;
    const int l = i | (j << 8) | (k << 16);
    const int stride = vertexFormat.getNextOffset() >> 2;
    const int base = (vertexCount - 4) * stride + vertexFormat.getNormalOffset() / 4;
    putInt(base, l);
    putInt(base + stride, l);
    putInt(base + stride * 2, l);
    putInt(base + stride * 3, l);
}

void WorldRenderer::nextVertexFormatIndex()
{
    ++vertexFormatIndex;
    vertexFormatIndex %= vertexFormat.getElementCount();
    if (vertexFormat.getElement(vertexFormatIndex).getUsage() == VertexFormatElement::EnumUsage::PADDING)
        nextVertexFormatIndex();
}

WorldRenderer& WorldRenderer::normal(float x, float y, float z)
{
    const int i = vertexCount * vertexFormat.getNextOffset() + vertexFormat.getOffset(vertexFormatIndex);
    switch (vertexFormat.getElement(vertexFormatIndex).getType()) {
    case VertexFormatElement::EnumType::FLOAT:
        putFloatAt(i, x);
        putFloatAt(i + 4, y);
        putFloatAt(i + 8, z);
        break;
    case VertexFormatElement::EnumType::UINT:
    case VertexFormatElement::EnumType::INT:
        putIntAt(i, java::f2i(x));
        putIntAt(i + 4, java::f2i(y));
        putIntAt(i + 8, java::f2i(z));
        break;
    case VertexFormatElement::EnumType::USHORT:
    case VertexFormatElement::EnumType::SHORT:
        // Java: (short)((int)x * 32767 & 65535) -- the int cast binds before the multiply.
        putShortAt(i, static_cast<int16_t>(java::mul(java::f2i(x), 32767) & 65535));
        putShortAt(i + 2, static_cast<int16_t>(java::mul(java::f2i(y), 32767) & 65535));
        putShortAt(i + 4, static_cast<int16_t>(java::mul(java::f2i(z), 32767) & 65535));
        break;
    case VertexFormatElement::EnumType::UBYTE:
    case VertexFormatElement::EnumType::BYTE:
        // Java: (byte)((int)x * 127 & 255) -- only exact -1/0/1 normals survive, a quirk of the original.
        putByteAt(i, static_cast<int8_t>(java::mul(java::f2i(x), 127) & 255));
        putByteAt(i + 1, static_cast<int8_t>(java::mul(java::f2i(y), 127) & 255));
        putByteAt(i + 2, static_cast<int8_t>(java::mul(java::f2i(z), 127) & 255));
        break;
    }
    nextVertexFormatIndex();
    return *this;
}

void WorldRenderer::setTranslation(double x, double y, double z)
{
    xOffset = x;
    yOffset = y;
    zOffset = z;
}

void WorldRenderer::finishDrawing()
{
    if (!isDrawing)
        throw std::logic_error("Not building!");
    isDrawing = false;
}

void WorldRenderer::putColor4(int argb)
{
    for (int i = 0; i < 4; ++i)
        putColor(argb, i + 1);
}

void WorldRenderer::putColorRGB_F4(float red, float green, float blue)
{
    for (int i = 0; i < 4; ++i)
        putColorRGB_F(red, green, blue, i + 1);
}

} // namespace mc
