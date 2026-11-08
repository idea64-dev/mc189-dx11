#include "client/renderer/chunk/ChunkRenderDispatcher.h"

#include "client/Minecraft.h"
#include "client/renderer/RegionRenderCache.h"
#include "client/renderer/chunk/ChunkCompileTaskGenerator.h"
#include "client/renderer/chunk/CompiledChunk.h"
#include "client/renderer/chunk/RenderChunk.h"
#include "client/renderer/vertex/VertexBuffer.h"
#include "core/Log.h"
#include "entity/Entity.h"
#include "platform/Sys.h"

#include <atomic>
#include <format>

namespace mc {

namespace {

const char* statusName(ChunkCompileTaskGenerator::Status status)
{
    switch (status) {
    case ChunkCompileTaskGenerator::Status::PENDING: return "PENDING";
    case ChunkCompileTaskGenerator::Status::COMPILING: return "COMPILING";
    case ChunkCompileTaskGenerator::Status::UPLOADING: return "UPLOADING";
    case ChunkCompileTaskGenerator::Status::DONE: return "DONE";
    }
    return "?";
}

} // namespace

// --- ChunkRenderDispatcher ---

ChunkRenderDispatcher::ChunkRenderDispatcher()
    : clientThread(std::this_thread::get_id())
{
    for (int j = 0; j < 5; ++j) {
        renderBuilders.push_back(std::make_unique<RegionRenderCacheBuilder>());
        queueFreeRenderBuilders.add(renderBuilders.back().get());
    }
    renderWorker = std::make_unique<ChunkRenderWorker>(*this, std::make_unique<RegionRenderCacheBuilder>());

    for (int i = 0; i < 2; ++i) {
        listThreadedWorkers.push_back(std::make_unique<ChunkRenderWorker>(*this));
        ChunkRenderWorker* worker = listThreadedWorkers.back().get();
        threads.emplace_back([worker] { worker->run(); });
    }
}

ChunkRenderDispatcher::~ChunkRenderDispatcher()
{
    // Java's batcher threads are daemons that die with the JVM; these are told to stop.
    clearChunkUpdates();
    queueChunkUpdates.close();
    queueFreeRenderBuilders.close();
    for (std::thread& thread : threads)
        thread.join();
}

std::string ChunkRenderDispatcher::getDebugInfo()
{
    size_t uploads;
    {
        std::lock_guard guard(uploadsMutex);
        uploads = queueChunkUploads.size();
    }
    return std::format("pC: {:03}, pU: {}, aB: {}", queueChunkUpdates.size(), uploads, queueFreeRenderBuilders.size());
}

bool ChunkRenderDispatcher::runChunkUploads(int64_t finishTimeNano)
{
    bool flag = false;
    for (;;) {
        std::function<void()> task;
        {
            std::lock_guard guard(uploadsMutex);
            if (!queueChunkUploads.empty()) {
                task = std::move(queueChunkUploads.front());
                queueChunkUploads.pop_front();
            }
        }
        const bool flag1 = static_cast<bool>(task);
        if (flag1) {
            task();
            flag = true;
        }
        if (finishTimeNano == 0 || !flag1)
            break;
        if (finishTimeNano - Sys::nanoTime() < 0)
            break;
    }
    return flag;
}

bool ChunkRenderDispatcher::updateChunkLater(RenderChunk* chunkRenderer)
{
    std::lock_guard guard(chunkRenderer->getLockCompileTask());
    const std::shared_ptr<ChunkCompileTaskGenerator> generator = chunkRenderer->makeCompileTaskChunk();
    std::weak_ptr<ChunkCompileTaskGenerator> weak = generator;
    generator->addFinishRunnable([this, weak] {
        if (std::shared_ptr<ChunkCompileTaskGenerator> task = weak.lock())
            queueChunkUpdates.remove(task);
    });
    const bool flag = queueChunkUpdates.offer(generator);
    if (!flag)
        generator->finish();
    return flag;
}

bool ChunkRenderDispatcher::updateChunkNow(RenderChunk* chunkRenderer)
{
    std::lock_guard guard(chunkRenderer->getLockCompileTask());
    const std::shared_ptr<ChunkCompileTaskGenerator> generator = chunkRenderer->makeCompileTaskChunk();
    renderWorker->processTask(generator);
    return true;
}

void ChunkRenderDispatcher::stopChunkUpdates()
{
    clearChunkUpdates();
    while (runChunkUploads(0)) {
    }

    std::vector<RegionRenderCacheBuilder*> list;
    while (list.size() != 5) {
        // A worker still finishing a task frees its builder only after its uploads ran on this thread, so keep
        // running them while waiting (Java blocks in take() and can hang here).
        if (std::optional<RegionRenderCacheBuilder*> builder = queueFreeRenderBuilders.take(std::chrono::milliseconds(1)); builder && *builder)
            list.push_back(*builder);
        while (runChunkUploads(0)) {
        }
    }
    for (RegionRenderCacheBuilder* builder : list)
        queueFreeRenderBuilders.add(builder);
}

void ChunkRenderDispatcher::freeRenderBuilder(RegionRenderCacheBuilder* builder)
{
    queueFreeRenderBuilders.add(builder);
}

RegionRenderCacheBuilder* ChunkRenderDispatcher::allocateRenderBuilder()
{
    return queueFreeRenderBuilders.take().value_or(nullptr);
}

std::shared_ptr<ChunkCompileTaskGenerator> ChunkRenderDispatcher::getNextChunkUpdate()
{
    return queueChunkUpdates.take().value_or(nullptr);
}

bool ChunkRenderDispatcher::updateTransparencyLater(RenderChunk* chunkRenderer)
{
    std::lock_guard guard(chunkRenderer->getLockCompileTask());
    const std::shared_ptr<ChunkCompileTaskGenerator> generator = chunkRenderer->makeCompileTaskTransparency();
    if (!generator)
        return true;
    std::weak_ptr<ChunkCompileTaskGenerator> weak = generator;
    generator->addFinishRunnable([this, weak] {
        if (std::shared_ptr<ChunkCompileTaskGenerator> task = weak.lock())
            queueChunkUpdates.remove(task);
    });
    return queueChunkUpdates.offer(generator);
}

void ChunkRenderDispatcher::uploadChunk(EnumWorldBlockLayer layer, WorldRenderer& worldRenderer, RenderChunk* chunkRenderer, std::function<bool()> cancelled,
    std::function<void()> done)
{
    if (isCallingFromClientThread()) {
        if (!cancelled())
            uploadVertexBuffer(worldRenderer, chunkRenderer, layer);
        done();
        return;
    }
    std::lock_guard guard(uploadsMutex);
    queueChunkUploads.push_back([this, layer, &worldRenderer, chunkRenderer, cancelled = std::move(cancelled), done = std::move(done)] {
        if (!cancelled())
            uploadVertexBuffer(worldRenderer, chunkRenderer, layer);
        done();
    });
}

void ChunkRenderDispatcher::runOnClientThread(std::function<void()> task)
{
    if (isCallingFromClientThread()) {
        task();
        return;
    }
    std::lock_guard guard(uploadsMutex);
    queueChunkUploads.push_back(std::move(task));
}

void ChunkRenderDispatcher::uploadVertexBuffer(WorldRenderer& worldRenderer, RenderChunk* chunkRenderer, EnumWorldBlockLayer layer)
{
    // Java: VertexBufferUploader (or the display list, which draws the same), then setTranslation(0, 0, 0).
    VertexBufferUploader vertexUploader;
    vertexUploader.setVertexBuffer(chunkRenderer->getVertexBufferByLayer(static_cast<int>(layer)));
    vertexUploader.draw(worldRenderer);
    worldRenderer.setTranslation(0.0, 0.0, 0.0);
}

void ChunkRenderDispatcher::clearChunkUpdates()
{
    while (!queueChunkUpdates.isEmpty()) {
        if (std::optional<std::shared_ptr<ChunkCompileTaskGenerator>> generator = queueChunkUpdates.poll(); generator && *generator)
            (*generator)->finish();
    }
}

// --- ChunkRenderWorker ---

ChunkRenderWorker::ChunkRenderWorker(ChunkRenderDispatcher& dispatcher, std::unique_ptr<RegionRenderCacheBuilder> builder)
    : chunkRenderDispatcher(dispatcher)
    , regionRenderCacheBuilder(std::move(builder))
{
}

ChunkRenderWorker::~ChunkRenderWorker() = default;

void ChunkRenderWorker::run()
{
    for (;;) {
        const std::shared_ptr<ChunkCompileTaskGenerator> generator = chunkRenderDispatcher.getNextChunkUpdate();
        if (!generator)
            return; // Java: interrupted
        try {
            processTask(generator);
        } catch (const std::exception& e) {
            // Java: crashes the game with "Batching chunks".
            Log::error("Batching chunks: {}", e.what());
            return;
        }
    }
}

void ChunkRenderWorker::processTask(const std::shared_ptr<ChunkCompileTaskGenerator>& generator)
{
    {
        std::lock_guard guard(generator->getLock());
        if (generator->getStatus() != ChunkCompileTaskGenerator::Status::PENDING) {
            if (!generator->isFinished())
                Log::warn("Chunk render task was {} when I expected it to be pending; ignoring task", statusName(generator->getStatus()));
            return;
        }
        generator->setStatus(ChunkCompileTaskGenerator::Status::COMPILING);
    }

    Entity* entity = Minecraft::getMinecraft()->getRenderViewEntity();
    if (entity == nullptr) {
        generator->finish();
        return;
    }

    RegionRenderCacheBuilder* builder = getRegionRenderCacheBuilder();
    if (builder == nullptr)
        return;
    generator->setRegionRenderCacheBuilder(builder);
    const float f = static_cast<float>(entity->posX);
    const float f1 = static_cast<float>(entity->posY) + entity->getEyeHeight();
    const float f2 = static_cast<float>(entity->posZ);
    const ChunkCompileTaskGenerator::Type type = generator->getType();

    if (type == ChunkCompileTaskGenerator::Type::REBUILD_CHUNK)
        generator->getRenderChunk()->rebuildChunk(f, f1, f2, *generator);
    else
        generator->getRenderChunk()->resortTransparency(f, f1, f2, *generator);

    {
        std::lock_guard guard(generator->getLock());
        if (generator->getStatus() != ChunkCompileTaskGenerator::Status::COMPILING) {
            if (!generator->isFinished())
                Log::warn("Chunk render task was {} when I expected it to be compiling; aborting task", statusName(generator->getStatus()));
            freeRenderBuilder(*generator);
            return;
        }
        generator->setStatus(ChunkCompileTaskGenerator::Status::UPLOADING);
    }

    const std::shared_ptr<CompiledChunk> compiledchunk = generator->getCompiledChunk();
    std::vector<EnumWorldBlockLayer> layers;
    if (type == ChunkCompileTaskGenerator::Type::REBUILD_CHUNK) {
        for (int l = 0; l < 4; ++l) {
            if (compiledchunk->isLayerStarted(static_cast<EnumWorldBlockLayer>(l)))
                layers.push_back(static_cast<EnumWorldBlockLayer>(l));
        }
    } else {
        layers.push_back(EnumWorldBlockLayer::TRANSLUCENT);
    }

    // Java: Futures.allAsList over the uploads, cancelled by the task's finish; the callback frees the builder and,
    // when the task is still uploading, publishes the compiled chunk. The builder is freed only once every queued
    // upload has run or been skipped, as nothing may write into it while an upload still reads it.
    struct Job {
        std::shared_ptr<ChunkCompileTaskGenerator> generator;
        std::shared_ptr<CompiledChunk> compiledChunk;
        size_t pending;
        std::atomic<bool> cancelled { false };
        std::mutex mutex;
    };
    auto job = std::make_shared<Job>();
    job->generator = generator;
    job->compiledChunk = compiledchunk;
    job->pending = layers.size();

    ChunkRenderWorker* self = this;
    const auto onAllDone = [self, job] {
        self->freeRenderBuilder(*job->generator);
        if (job->cancelled)
            return;
        {
            std::lock_guard guard(job->generator->getLock());
            if (job->generator->getStatus() != ChunkCompileTaskGenerator::Status::UPLOADING) {
                if (!job->generator->isFinished())
                    Log::warn("Chunk render task was {} when I expected it to be uploading; aborting task", statusName(job->generator->getStatus()));
                return;
            }
            job->generator->setStatus(ChunkCompileTaskGenerator::Status::DONE);
        }
        job->generator->getRenderChunk()->setCompiledChunk(job->compiledChunk);
    };

    if (layers.empty()) {
        // Like the uploads, on the client thread: it is the only one that replaces compiled chunks.
        chunkRenderDispatcher.runOnClientThread(onAllDone);
        return;
    }

    for (EnumWorldBlockLayer layer : layers) {
        chunkRenderDispatcher.uploadChunk(
            layer, builder->getWorldRendererByLayer(layer), generator->getRenderChunk(), [job] { return job->cancelled.load(); },
            [job, onAllDone] {
                bool last;
                {
                    std::lock_guard guard(job->mutex);
                    last = --job->pending == 0;
                }
                if (last)
                    onAllDone();
            });
    }

    std::weak_ptr<Job> weakJob = job;
    generator->addFinishRunnable([weakJob] {
        if (std::shared_ptr<Job> j = weakJob.lock())
            j->cancelled = true;
    });
}

RegionRenderCacheBuilder* ChunkRenderWorker::getRegionRenderCacheBuilder()
{
    return regionRenderCacheBuilder ? regionRenderCacheBuilder.get() : chunkRenderDispatcher.allocateRenderBuilder();
}

void ChunkRenderWorker::freeRenderBuilder(ChunkCompileTaskGenerator& taskGenerator)
{
    if (!regionRenderCacheBuilder)
        chunkRenderDispatcher.freeRenderBuilder(taskGenerator.getRegionRenderCacheBuilder());
}

} // namespace mc
