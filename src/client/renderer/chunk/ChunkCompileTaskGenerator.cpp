#include "client/renderer/chunk/ChunkCompileTaskGenerator.h"

#include "client/renderer/RegionRenderCache.h"
#include "client/renderer/chunk/CompiledChunk.h"
#include "client/renderer/chunk/RenderChunk.h"

namespace mc {

ChunkCompileTaskGenerator::ChunkCompileTaskGenerator(std::shared_ptr<RenderChunk> renderChunkIn, Type typeIn)
    : renderChunk(std::move(renderChunkIn))
    , type(typeIn)
{
}

ChunkCompileTaskGenerator::~ChunkCompileTaskGenerator() = default;

void ChunkCompileTaskGenerator::setStatus(Status statusIn)
{
    std::lock_guard guard(lock);
    status = statusIn;
}

void ChunkCompileTaskGenerator::finish()
{
    std::lock_guard guard(lock);
    if (type == Type::REBUILD_CHUNK && status != Status::DONE)
        renderChunk->setNeedsUpdate(true);
    finished = true;
    status = Status::DONE;
    for (const std::function<void()>& runnable : listFinishRunnables)
        runnable();
}

void ChunkCompileTaskGenerator::addFinishRunnable(std::function<void()> runnable)
{
    std::lock_guard guard(lock);
    listFinishRunnables.push_back(std::move(runnable));
    if (finished)
        listFinishRunnables.back()();
}

void ChunkCompileTaskGenerator::setRegionCache(std::unique_ptr<RegionRenderCache> cache)
{
    regionCache = std::move(cache);
}

} // namespace mc
