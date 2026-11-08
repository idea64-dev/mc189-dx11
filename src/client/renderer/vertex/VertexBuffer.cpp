#include "client/renderer/vertex/VertexBuffer.h"

#include "client/renderer/Tessellator.h"
#include "client/renderer/WorldRenderer.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "render/gl/GL.h"

namespace mc {

VertexBuffer::VertexBuffer(VertexFormat vertexFormatIn)
    : glBufferId(GL::glGenBuffers())
    , vertexFormat(std::move(vertexFormatIn))
    , onGpu(vertexFormat.hasColor() && vertexFormat.hasUvOffset(0) && vertexFormat.hasUvOffset(1))
{
}

VertexBuffer::~VertexBuffer()
{
    deleteGlBuffers();
}

void VertexBuffer::bufferData(const WorldRenderer& worldRenderer)
{
    count = worldRenderer.getVertexCount();
    // The draw mode is only known at drawArrays; the GPU copy is built for the mode the data was made in.
    bufferedMode = worldRenderer.getDrawMode();
    blockBuffer = bufferedMode == 7 && vertexFormat == DefaultVertexFormats::BLOCK && worldRenderer.getVertexFormat() == DefaultVertexFormats::BLOCK;
    if (blockBuffer) {
        if (glBufferId >= 0)
            GL::bufferBlockVertices(glBufferId, worldRenderer.getByteBuffer(), static_cast<uint32_t>(count));
    } else if (onGpu) {
        std::vector<GLVertex> vertices;
        convertVertexData(worldRenderer, vertices);
        if (glBufferId >= 0)
            GL::bufferVertices(glBufferId, bufferedMode, vertices);
    } else {
        const std::span<const uint8_t> data = worldRenderer.getByteBuffer();
        bytes.assign(data.begin(), data.end());
    }
}

void VertexBuffer::drawArrays(int mode)
{
    if (count == 0)
        return;
    if (blockBuffer || onGpu) {
        if (glBufferId >= 0)
            GL::drawBuffer(glBufferId);
        return;
    }
    std::vector<GLVertex> vertices;
    convertVertexData(bytes, vertexFormat, count, vertices);
    GL::drawVertices(mode, vertices);
}

void VertexBuffer::deleteGlBuffers()
{
    if (glBufferId >= 0) {
        GL::glDeleteBuffers(glBufferId);
        glBufferId = -1;
    }
}

void VertexBufferUploader::draw(WorldRenderer& worldRenderer)
{
    // Java resets first: its byte buffer keeps the limit finishDrawing set, the count here goes with reset.
    vertexBuffer->bufferData(worldRenderer);
    worldRenderer.reset();
}

} // namespace mc
