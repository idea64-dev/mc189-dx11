#pragma once

// Original: net/minecraft/client/renderer/ViewFrustum.java, ChunkRenderContainer.java, VboRenderList.java and
// RenderList.java
// The ring of render chunks around the camera ((2r+1) x 16 x (2r+1) sections, recycled as it moves) and the
// per-layer list of chunks drawn this frame.

#include "block/Block.h"
#include "render/gl/Matrix4f.h"
#include "util/BlockPos.h"

#include <memory>
#include <vector>

namespace mc {

class RenderChunk;
class RenderGlobal;
class World;

class ViewFrustum {
public:
    ViewFrustum(World* worldIn, int renderDistanceChunks, RenderGlobal* renderGlobalIn);

    void deleteGlResources();
    void updateChunkPositions(double viewEntityX, double viewEntityZ);
    void markBlocksForUpdate(int fromX, int fromY, int fromZ, int toX, int toY, int toZ);
    // Null above or below the world.
    RenderChunk* getRenderChunk(const BlockPos& pos);

    std::vector<std::shared_ptr<RenderChunk>> renderChunks;

protected:
    void createRenderChunks();
    void setCountChunksXYZ(int renderDistanceChunks);

    RenderGlobal* renderGlobal;
    World* world;
    int countChunksY = 0;
    int countChunksX = 0;
    int countChunksZ = 0;

private:
    static int func_178157_a(int viewPos, int size, int index);
};

// Java: ChunkRenderContainer / VboRenderList. RenderList (display lists) draws the same geometry the same way.
class ChunkRenderContainer {
public:
    void initialize(double viewEntityXIn, double viewEntityYIn, double viewEntityZIn);
    void preRenderChunk(const RenderChunk& renderChunkIn);
    void addRenderChunk(RenderChunk* renderChunkIn, EnumWorldBlockLayer layer);
    void renderChunkLayer(EnumWorldBlockLayer layer);

private:
    double viewEntityX = 0.0;
    double viewEntityY = 0.0;
    double viewEntityZ = 0.0;
    std::vector<RenderChunk*> renderChunks;
    bool initialized = false;
    // Per draw: the buffer and the modelview VboRenderList would have set for it.
    std::vector<int> drawBuffers;
    std::vector<Matrix4f> drawModelViews;
};

} // namespace mc
