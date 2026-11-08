#pragma once

// Original: net/minecraft/world/storage/ThreadedFileIOBase.java and IThreadedFileIO.java -- the "File IO Thread"
// that writes queued chunks in the background.

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace mc {

class IThreadedFileIO {
public:
    virtual ~IThreadedFileIO() = default;
    // Writes one item; false when there is nothing left.
    virtual bool writeNextIO() = 0;
};

class ThreadedFileIOBase {
public:
    static ThreadedFileIOBase& getThreadedIOInstance();

    void queueIO(IThreadedFileIO* io);
    // Blocks until everything queued so far is written.
    void waitForFinish();
    // Drops an IO object that is going away (Java relies on the GC keeping it alive until written).
    void remove(IThreadedFileIO* io);

    ~ThreadedFileIOBase();

private:
    ThreadedFileIOBase();
    void run();
    void processQueue();

    std::mutex queueLock;
    std::mutex processLock; // held while an IO object writes, so remove() can wait for it
    std::vector<IThreadedFileIO*> threadedIOQueue;
    std::atomic<int64_t> writeQueuedCounter { 0 };
    std::atomic<int64_t> savedIOCounter { 0 };
    std::atomic<bool> isThreadWaiting { false };
    std::atomic<bool> stopping { false };
    std::thread thread;
};

} // namespace mc
