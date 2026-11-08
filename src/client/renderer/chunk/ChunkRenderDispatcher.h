#pragma once

// Original: net/minecraft/client/renderer/chunk/ChunkRenderDispatcher.java and ChunkRenderWorker.java
// Two "Chunk Batcher" threads build render chunks from a queue of at most 100 tasks with five vertex builders;
// the uploads into the vertex buffers run on the client thread (runChunkUploads), the chunks next to the camera
// are built there at once (updateChunkNow).

#include "block/Block.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace mc {

class ChunkCompileTaskGenerator;
class ChunkRenderDispatcher;
class CompiledChunk;
class RegionRenderCacheBuilder;
class RenderChunk;
class WorldRenderer;

// Java: ArrayBlockingQueue -- bounded, blocking take, removal of a given element.
template <class T>
class BlockingQueue {
public:
    explicit BlockingQueue(size_t capacityIn)
        : capacity(capacityIn)
    {
    }

    bool offer(T value)
    {
        std::lock_guard guard(mutex);
        if (items.size() >= capacity)
            return false;
        items.push_back(std::move(value));
        available.notify_one();
        return true;
    }
    void add(T value)
    {
        std::lock_guard guard(mutex);
        items.push_back(std::move(value));
        available.notify_one();
    }
    // Waits for an element; empty when the queue was shut down.
    std::optional<T> take()
    {
        std::unique_lock guard(mutex);
        available.wait(guard, [this] { return closed || !items.empty(); });
        if (items.empty())
            return std::nullopt;
        T value = std::move(items.front());
        items.pop_front();
        return value;
    }
    // Waits at most `timeout`; empty when nothing came.
    std::optional<T> take(std::chrono::milliseconds timeout)
    {
        std::unique_lock guard(mutex);
        available.wait_for(guard, timeout, [this] { return closed || !items.empty(); });
        if (items.empty())
            return std::nullopt;
        T value = std::move(items.front());
        items.pop_front();
        return value;
    }
    std::optional<T> poll()
    {
        std::lock_guard guard(mutex);
        if (items.empty())
            return std::nullopt;
        T value = std::move(items.front());
        items.pop_front();
        return value;
    }
    void remove(const T& value)
    {
        std::lock_guard guard(mutex);
        for (auto it = items.begin(); it != items.end(); ++it) {
            if (*it == value) {
                items.erase(it);
                return;
            }
        }
    }
    bool isEmpty()
    {
        std::lock_guard guard(mutex);
        return items.empty();
    }
    size_t size()
    {
        std::lock_guard guard(mutex);
        return items.size();
    }
    void close()
    {
        std::lock_guard guard(mutex);
        closed = true;
        available.notify_all();
    }

private:
    std::mutex mutex;
    std::condition_variable available;
    std::deque<T> items;
    size_t capacity;
    bool closed = false;
};

class ChunkRenderWorker {
public:
    // A worker without its own builder takes them from the dispatcher's pool (the batcher threads).
    ChunkRenderWorker(ChunkRenderDispatcher& dispatcher, std::unique_ptr<RegionRenderCacheBuilder> builder = nullptr);
    ~ChunkRenderWorker();

    void run();
    void processTask(const std::shared_ptr<ChunkCompileTaskGenerator>& generator);

private:
    RegionRenderCacheBuilder* getRegionRenderCacheBuilder();
    void freeRenderBuilder(ChunkCompileTaskGenerator& taskGenerator);

    ChunkRenderDispatcher& chunkRenderDispatcher;
    std::unique_ptr<RegionRenderCacheBuilder> regionRenderCacheBuilder;
};

class ChunkRenderDispatcher {
public:
    ChunkRenderDispatcher();
    ~ChunkRenderDispatcher();

    std::string getDebugInfo();
    // Runs queued uploads until there are none or finishTimeNano (System.nanoTime) has passed; 0 runs one.
    bool runChunkUploads(int64_t finishTimeNano);
    bool updateChunkLater(RenderChunk* chunkRenderer);
    bool updateChunkNow(RenderChunk* chunkRenderer);
    void stopChunkUpdates();
    void freeRenderBuilder(RegionRenderCacheBuilder* builder);
    // Empty only when shutting down.
    RegionRenderCacheBuilder* allocateRenderBuilder();
    std::shared_ptr<ChunkCompileTaskGenerator> getNextChunkUpdate();
    bool updateTransparencyLater(RenderChunk* chunkRenderer);
    void clearChunkUpdates();
    bool isCallingFromClientThread() const { return std::this_thread::get_id() == clientThread; }

    // Java: uploadChunk -- at once on the client thread, else queued; `done` runs after the upload (or the skip).
    // Runs `task` now on the client thread, else queues it with the uploads.
    void runOnClientThread(std::function<void()> task);
    void uploadChunk(EnumWorldBlockLayer layer, WorldRenderer& worldRenderer, RenderChunk* chunkRenderer, std::function<bool()> cancelled,
        std::function<void()> done);

private:
    void uploadVertexBuffer(WorldRenderer& worldRenderer, RenderChunk* chunkRenderer, EnumWorldBlockLayer layer);

    std::thread::id clientThread;
    BlockingQueue<std::shared_ptr<ChunkCompileTaskGenerator>> queueChunkUpdates { 100 };
    BlockingQueue<RegionRenderCacheBuilder*> queueFreeRenderBuilders { 5 };
    std::vector<std::unique_ptr<RegionRenderCacheBuilder>> renderBuilders;
    std::mutex uploadsMutex;
    std::deque<std::function<void()>> queueChunkUploads;
    std::vector<std::unique_ptr<ChunkRenderWorker>> listThreadedWorkers;
    std::vector<std::thread> threads;
    std::unique_ptr<ChunkRenderWorker> renderWorker;
};

} // namespace mc
