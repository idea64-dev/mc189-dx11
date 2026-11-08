#include "world/storage/ThreadedFileIOBase.h"

#include "core/Log.h"

#include <algorithm>
#include <chrono>

namespace mc {

ThreadedFileIOBase& ThreadedFileIOBase::getThreadedIOInstance()
{
    static ThreadedFileIOBase instance;
    return instance;
}

ThreadedFileIOBase::ThreadedFileIOBase()
    : thread([this] { run(); })
{
}

ThreadedFileIOBase::~ThreadedFileIOBase()
{
    stopping = true;
    if (thread.joinable())
        thread.join();
}

void ThreadedFileIOBase::run()
{
    Log::setThreadName("File IO Thread");
    while (!stopping)
        processQueue();
}

void ThreadedFileIOBase::processQueue()
{
    for (size_t i = 0;; ++i) {
        std::unique_lock processing(processLock);
        IThreadedFileIO* io;
        {
            std::lock_guard guard(queueLock);
            if (i >= threadedIOQueue.size())
                break;
            io = threadedIOQueue[i];
        }
        const bool flag = io->writeNextIO();
        if (!flag) {
            std::lock_guard guard(queueLock);
            if (i < threadedIOQueue.size() && threadedIOQueue[i] == io)
                threadedIOQueue.erase(threadedIOQueue.begin() + static_cast<std::ptrdiff_t>(i--));
            ++savedIOCounter;
        }
        processing.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(isThreadWaiting ? 0 : 10));
    }
    bool empty;
    {
        std::lock_guard guard(queueLock);
        empty = threadedIOQueue.empty();
    }
    if (empty)
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
}

void ThreadedFileIOBase::queueIO(IThreadedFileIO* io)
{
    std::lock_guard guard(queueLock);
    if (std::find(threadedIOQueue.begin(), threadedIOQueue.end(), io) == threadedIOQueue.end()) {
        ++writeQueuedCounter;
        threadedIOQueue.push_back(io);
    }
}

void ThreadedFileIOBase::waitForFinish()
{
    isThreadWaiting = true;
    while (writeQueuedCounter != savedIOCounter)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    isThreadWaiting = false;
}

void ThreadedFileIOBase::remove(IThreadedFileIO* io)
{
    std::lock_guard processing(processLock);
    std::lock_guard guard(queueLock);
    const auto it = std::find(threadedIOQueue.begin(), threadedIOQueue.end(), io);
    if (it != threadedIOQueue.end()) {
        threadedIOQueue.erase(it);
        ++savedIOCounter;
    }
}

} // namespace mc
