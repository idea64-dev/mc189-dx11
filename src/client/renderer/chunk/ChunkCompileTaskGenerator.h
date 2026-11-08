#pragma once

// Original: net/minecraft/client/renderer/chunk/ChunkCompileTaskGenerator.java
// One rebuild or translucency re-sort of a render chunk, shared by the render chunk, the dispatcher's queue and
// the worker compiling it. Its region cache is taken when the task is made, on the client thread: Java reads the
// world's chunk map from the worker, which a C++ hash map does not survive (the chunks it lists are read from the
// worker as in Java).

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace mc {

class CompiledChunk;
class RegionRenderCache;
class RegionRenderCacheBuilder;
class RenderChunk;

class ChunkCompileTaskGenerator {
public:
    enum class Status { PENDING, COMPILING, UPLOADING, DONE };
    enum class Type { REBUILD_CHUNK, RESORT_TRANSPARENCY };

    // The render chunk is kept alive while a worker may still hold the task (Java: garbage collection).
    ChunkCompileTaskGenerator(std::shared_ptr<RenderChunk> renderChunkIn, Type typeIn);
    ~ChunkCompileTaskGenerator();

    Status getStatus() const { return status; }
    RenderChunk* getRenderChunk() const { return renderChunk.get(); }
    const std::shared_ptr<CompiledChunk>& getCompiledChunk() const { return compiledChunk; }
    void setCompiledChunk(std::shared_ptr<CompiledChunk> compiledChunkIn) { compiledChunk = std::move(compiledChunkIn); }
    RegionRenderCacheBuilder* getRegionRenderCacheBuilder() const { return regionRenderCacheBuilder; }
    void setRegionRenderCacheBuilder(RegionRenderCacheBuilder* builder) { regionRenderCacheBuilder = builder; }
    void setStatus(Status statusIn);
    void finish();
    void addFinishRunnable(std::function<void()> runnable);
    std::recursive_mutex& getLock() { return lock; }
    Type getType() const { return type; }
    bool isFinished() const { return finished; }
    RegionRenderCache* getRegionCache() const { return regionCache.get(); }
    void setRegionCache(std::unique_ptr<RegionRenderCache> cache);

private:
    std::shared_ptr<RenderChunk> renderChunk;
    std::recursive_mutex lock;
    std::vector<std::function<void()>> listFinishRunnables;
    Type type;
    RegionRenderCacheBuilder* regionRenderCacheBuilder = nullptr;
    std::shared_ptr<CompiledChunk> compiledChunk;
    std::atomic<Status> status { Status::PENDING };
    std::atomic<bool> finished { false };
    std::unique_ptr<RegionRenderCache> regionCache;
};

} // namespace mc
