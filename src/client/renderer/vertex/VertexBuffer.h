#pragma once

// Original: net/minecraft/client/renderer/vertex/VertexBuffer.java and VertexBufferUploader.java
// A vertex buffer object drawn with glDrawArrays. Attributes its format lacks come from the GL state current at
// draw time, as for disabled client arrays. Render chunk quads (the BLOCK format) go to the GPU as they are
// (GL::bufferBlockVertices, drawn by ChunkRenderContainer in one batch per layer); other buffers whose format
// carries colour and both texture coordinates live on the GPU expanded; the rest (sky, stars: positions only)
// keep their bytes and are expanded with the current colour per draw.

#include "client/renderer/vertex/VertexFormat.h"

#include <cstdint>
#include <span>
#include <vector>

namespace mc {

class WorldRenderer;

class VertexBuffer {
public:
    explicit VertexBuffer(VertexFormat vertexFormatIn);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    // Java: bufferData(worldRenderer.getByteBuffer()) -- the vertices drawn as `mode` later.
    void bufferData(const WorldRenderer& worldRenderer);
    void drawArrays(int mode);
    void deleteGlBuffers();
    int getCount() const { return count; }
    // Filled through GL::bufferBlockVertices: drawable with GL::drawBlockBuffers.
    bool isBlockBuffer() const { return blockBuffer && count > 0 && glBufferId >= 0; }
    int getGlBufferId() const { return glBufferId; }

private:
    int glBufferId;
    VertexFormat vertexFormat;
    int count = 0;
    bool onGpu;
    bool blockBuffer = false;
    int bufferedMode = -1;
    std::vector<uint8_t> bytes;
};

// Java: VertexBufferUploader -- a WorldVertexBufferUploader that fills a VertexBuffer instead of drawing.
class VertexBufferUploader {
public:
    void setVertexBuffer(VertexBuffer* vertexBufferIn) { vertexBuffer = vertexBufferIn; }
    void draw(WorldRenderer& worldRenderer);

private:
    VertexBuffer* vertexBuffer = nullptr;
};

} // namespace mc
