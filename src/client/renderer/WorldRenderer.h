#pragma once

// Original: net/minecraft/client/renderer/WorldRenderer.java (the 1.8.9 vertex builder, later BufferBuilder).
// Vertex data is laid out exactly as in Java (native little-endian), so code that pokes raw ints into it
// (block models, putBrightness4, putColor4...) ports unchanged.

#include "client/renderer/vertex/VertexFormat.h"

#include <cstdint>
#include <span>
#include <vector>

namespace mc {

class WorldRenderer {
public:
    class State {
    public:
        State(std::vector<int32_t> buffer, VertexFormat format)
            : stateRawBuffer(std::move(buffer))
            , stateVertexFormat(std::move(format))
        {
        }
        const std::vector<int32_t>& getRawBuffer() const { return stateRawBuffer; }
        int getVertexCount() const { return static_cast<int>(stateRawBuffer.size()) / stateVertexFormat.getIntegerSize(); }
        const VertexFormat& getVertexFormat() const { return stateVertexFormat; }

    private:
        std::vector<int32_t> stateRawBuffer;
        VertexFormat stateVertexFormat;
    };

    // bufferSizeIn is in ints, as in Java.
    explicit WorldRenderer(int bufferSizeIn);

    void sortVertexData(float x, float y, float z);
    State getVertexState() const;
    void setVertexState(const State& state);
    void reset();
    void begin(int glMode, const VertexFormat& format);

    WorldRenderer& tex(double u, double v);
    WorldRenderer& lightmap(int skyLight, int blockLight);
    WorldRenderer& color(float red, float green, float blue, float alpha);
    WorldRenderer& color(int red, int green, int blue, int alpha);
    WorldRenderer& pos(double x, double y, double z);
    WorldRenderer& normal(float x, float y, float z);
    void endVertex();

    void putBrightness4(int b0, int b1, int b2, int b3);
    void putPosition(double x, double y, double z);
    void putColorMultiplier(float red, float green, float blue, int vertexIndex);
    void putColorRGB_F(float red, float green, float blue, int vertexIndex);
    void putColor4(int argb);
    void putColorRGB_F4(float red, float green, float blue);
    void noColor();
    void addVertexData(std::span<const int32_t> vertexData);
    void putNormal(float x, float y, float z);
    void setTranslation(double x, double y, double z);
    void finishDrawing();

    std::span<const uint8_t> getByteBuffer() const { return { byteBuffer.data(), static_cast<size_t>(getBufferSize()) * 4 }; }
    const VertexFormat& getVertexFormat() const { return vertexFormat; }
    int getVertexCount() const { return vertexCount; }
    int getDrawMode() const { return drawMode; }

private:
    void growBuffer(int ints);
    int getBufferSize() const { return vertexCount * vertexFormat.getIntegerSize(); }
    int getColorIndex(int vertexIndex) const;
    void putColor(int argb, int vertexIndex);
    void putColorRGBA(int index, int red, int green, int blue, int alpha);
    void nextVertexFormatIndex();

    int32_t getInt(int intIndex) const;
    void putInt(int intIndex, int32_t value);
    void putFloatAt(int byteIndex, float value);
    void putIntAt(int byteIndex, int32_t value);
    void putShortAt(int byteIndex, int16_t value);
    void putByteAt(int byteIndex, int8_t value);

    std::vector<uint8_t> byteBuffer;
    int vertexCount = 0;
    int vertexFormatIndex = 0;
    bool noColorFlag = false;
    int drawMode = 0;
    double xOffset = 0.0;
    double yOffset = 0.0;
    double zOffset = 0.0;
    VertexFormat vertexFormat;
    bool isDrawing = false;
};

} // namespace mc
