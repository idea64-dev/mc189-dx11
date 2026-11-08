#pragma once

// Original: net/minecraft/client/renderer/chunk/RenderChunk.java (with VboChunkFactory / ListChunkFactory)
// A 16^3 section of the view: its compiled geometry per layer in vertex buffers, rebuilt by the chunk workers.
// Display-list rendering (ListedRenderChunk, "Use VBOs" off) draws the same geometry, so both settings use the
// buffers here.
// Not ported yet: the tile entities with special renderers (chests, signs, ...) -- stage 10.

#include "block/Block.h"
#include "client/renderer/vertex/VertexBuffer.h"
#include "util/Vec3.h"
#include "util/BlockPos.h"

#include <array>
#include <atomic>
#include <memory>
#include <mutex>

namespace mc {

class ChunkCompileTaskGenerator;
class CompiledChunk;
class RenderGlobal;
class World;
class WorldRenderer;

class RenderChunk : public std::enable_shared_from_this<RenderChunk> {
public:
    static std::atomic<int> renderChunksUpdated;

    static std::shared_ptr<RenderChunk> create(World* worldIn, RenderGlobal* renderGlobalIn, const BlockPos& blockPosIn, int indexIn);
    RenderChunk(World* worldIn, RenderGlobal* renderGlobalIn, int indexIn);
    ~RenderChunk();

    bool setFrameIndex(int frameIndexIn);
    VertexBuffer* getVertexBufferByLayer(int layer) { return vertexBuffers[static_cast<size_t>(layer)].get(); }
    void setPosition(const BlockPos& pos);
    void resortTransparency(float x, float y, float z, ChunkCompileTaskGenerator& generator);
    void rebuildChunk(float x, float y, float z, ChunkCompileTaskGenerator& generator);
    std::recursive_mutex& getLockCompileTask() { return lockCompileTask; }
    std::shared_ptr<ChunkCompileTaskGenerator> makeCompileTaskChunk();
    std::shared_ptr<ChunkCompileTaskGenerator> makeCompileTaskTransparency();
    void multModelviewMatrix();
    const std::array<float, 16>& getModelviewMatrix() const { return modelviewMatrix; }
    std::shared_ptr<CompiledChunk> getCompiledChunk();
    // The client thread is the only one that replaces the compiled chunk (setCompiledChunk), so it reads it
    // without the lock and without a reference.
    CompiledChunk* getCompiledChunkOnClientThread() const { return compiledChunk.get(); }
    void setCompiledChunk(std::shared_ptr<CompiledChunk> compiledChunkIn);
    void stopCompileTask();
    void deleteGlResources();
    const BlockPos& getPosition() const { return position; }
    void setNeedsUpdate(bool needsUpdateIn) { needsUpdate = needsUpdateIn; }
    bool isNeedsUpdate() const { return needsUpdate; }
    const BlockPos& getBlockPosOffset16(EnumFacing facing) const { return mapEnumFacing[static_cast<size_t>(ordinal(facing))]; }
    // ViewFrustum::getRenderChunk(getBlockPosOffset16(facing)), remembered until the position changes (the
    // slot of a position in the ring never changes).
    RenderChunk*& cachedNeighbor(EnumFacing facing) const { return neighbors[static_cast<size_t>(ordinal(facing))]; }
    bool hasCachedNeighbor(EnumFacing facing) const { return (neighborsCached >> ordinal(facing)) & 1; }
    void setCachedNeighbor(EnumFacing facing, RenderChunk* neighbor) const
    {
        neighbors[static_cast<size_t>(ordinal(facing))] = neighbor;
        neighborsCached = static_cast<uint8_t>(neighborsCached | (1 << ordinal(facing)));
    }

    AxisAlignedBB boundingBox;

private:
    void finishCompileTask();
    static void preRenderBlocks(WorldRenderer& worldRendererIn, const BlockPos& pos);
    static void postRenderBlocks(EnumWorldBlockLayer layer, float x, float y, float z, WorldRenderer& worldRendererIn, CompiledChunk& compiledChunkIn);
    void initModelviewMatrix();

    World* world;
    RenderGlobal* renderGlobal;
    BlockPos position { -1, -1, -1 };
    std::shared_ptr<CompiledChunk> compiledChunk;
    std::recursive_mutex lockCompileTask;
    std::mutex lockCompiledChunk;
    std::shared_ptr<ChunkCompileTaskGenerator> compileTask;
    int index;
    std::array<float, 16> modelviewMatrix {};
    std::array<std::unique_ptr<VertexBuffer>, 4> vertexBuffers;
    int frameIndex = -1;
    std::atomic<bool> needsUpdate { true };
    std::array<BlockPos, 6> mapEnumFacing;
    mutable std::array<RenderChunk*, 6> neighbors {};
    mutable uint8_t neighborsCached = 0;
};

} // namespace mc
