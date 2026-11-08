#pragma once

// Original: net/minecraft/client/renderer/Tessellator.java and WorldVertexBufferUploader.java

#include "client/renderer/WorldRenderer.h"
#include "render/gl/GL.h"

#include <span>
#include <vector>

namespace mc {

// The vertices of a finished WorldRenderer as the GL emulation takes them; attributes the format lacks come from
// the current GL values, exactly like disabled client arrays.
void convertVertexData(const WorldRenderer& worldRenderer, std::vector<GLVertex>& vertices);
void convertVertexData(std::span<const uint8_t> bytes, const VertexFormat& format, int vertexCount, std::vector<GLVertex>& vertices);

// Java: WorldVertexBufferUploader. Feeds a finished WorldRenderer to the GL emulation; attributes the format
// lacks come from the current GL values, exactly like disabled client arrays.
class WorldVertexBufferUploader {
public:
    void draw(WorldRenderer& worldRenderer);

private:
    std::vector<GLVertex> vertices;
};

class Tessellator {
public:
    explicit Tessellator(int bufferSize);

    static Tessellator& getInstance();

    void draw();
    WorldRenderer& getWorldRenderer() { return worldRenderer; }

private:
    WorldRenderer worldRenderer;
    WorldVertexBufferUploader vboUploader;
};

} // namespace mc
