#include "core/HeapProfiler.h"

#include "core/CrashHandler.h"
#include "core/Log.h"

#include <Windows.h>
#include <Psapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <format>
#include <mutex>
#include <new>
#include <thread>
#include <unordered_map>
#include <vector>

// Replaces the global operator new/delete: plain malloc/free, plus sampling while the profiler runs. About
// one allocation per kSampleInterval bytes gets its stack recorded until it is freed, so the stacks holding
// the most live memory, and those that keep growing, show where memory goes. Sampled allocations come from
// a private heap behind a marked header, so delete recognizes them without a lock; the others cost nothing.

namespace mc {

namespace {

constexpr int64_t kSampleInterval = 256 * 1024;
constexpr int kMaxFrames = 24;

std::atomic<bool> enabled { false };
thread_local int64_t untilSample = kSampleInterval;
thread_local bool busy = false;

// The profiler's own containers live on a private heap, outside what they measure.
HANDLE privateHeap()
{
    static const HANDLE heap = HeapCreate(0, 0, 0);
    return heap;
}

template <class T>
struct PrivateAllocator {
    using value_type = T;
    PrivateAllocator() = default;
    template <class U>
    PrivateAllocator(const PrivateAllocator<U>&) noexcept
    {
    }
    T* allocate(size_t n)
    {
        void* p = HeapAlloc(privateHeap(), 0, n * sizeof(T));
        if (!p)
            throw std::bad_alloc();
        return static_cast<T*>(p);
    }
    void deallocate(T* p, size_t) noexcept { HeapFree(privateHeap(), 0, p); }
    template <class U>
    bool operator==(const PrivateAllocator<U>&) const noexcept
    {
        return true;
    }
};

struct Stack {
    void* frames[kMaxFrames];
    uint16_t count;
    bool operator==(const Stack& other) const { return count == other.count && std::memcmp(frames, other.frames, count * sizeof(void*)) == 0; }
};

struct StackHash {
    size_t operator()(const Stack& stack) const
    {
        size_t h = stack.count;
        for (int i = 0; i < stack.count; ++i)
            h = h * 1000003u ^ reinterpret_cast<size_t>(stack.frames[i]);
        return h;
    }
};

struct Sample {
    uint32_t stack;
    size_t weight;
};

// In front of a sampled allocation.
struct alignas(16) SampleHeader {
    uint64_t magic;
    uint64_t size;
};
constexpr uint64_t kMagic = 0x5EA9C0DEF00DB16Bull;

struct Profile {
    std::mutex mutex;
    std::unordered_map<void*, Sample, std::hash<void*>, std::equal_to<void*>, PrivateAllocator<std::pair<void* const, Sample>>> live;
    std::unordered_map<Stack, uint32_t, StackHash, std::equal_to<Stack>, PrivateAllocator<std::pair<const Stack, uint32_t>>> stackIds;
    std::vector<Stack, PrivateAllocator<Stack>> stacks;
};

Profile& profile()
{
    static Profile* instance = new (HeapAlloc(privateHeap(), 0, sizeof(Profile))) Profile();
    return *instance;
}

uint64_t nextRandom()
{
    thread_local uint64_t state = 0x9E3779B97F4A7C15ull ^ GetCurrentThreadId();
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

bool shouldSample(size_t size)
{
    untilSample -= static_cast<int64_t>(size);
    if (untilSample > 0 || busy)
        return false;
    untilSample = kSampleInterval / 2 + static_cast<int64_t>(nextRandom() % kSampleInterval);
    return true;
}

void recordAllocation(void* p, size_t size)
{
    busy = true;
    Stack stack {};
    stack.count = RtlCaptureStackBackTrace(2, kMaxFrames, stack.frames, nullptr);
    {
        Profile& pr = profile();
        std::lock_guard lock(pr.mutex);
        auto [it, inserted] = pr.stackIds.try_emplace(stack, static_cast<uint32_t>(pr.stacks.size()));
        if (inserted)
            pr.stacks.push_back(stack);
        pr.live[p] = Sample { it->second, std::max(static_cast<size_t>(kSampleInterval), size) };
    }
    busy = false;
}

void* allocateSampled(size_t size)
{
    auto* header = static_cast<SampleHeader*>(HeapAlloc(privateHeap(), 0, sizeof(SampleHeader) + size));
    if (!header)
        return nullptr;
    header->magic = kMagic;
    header->size = size;
    void* p = header + 1;
    recordAllocation(p, size);
    return p;
}

// The header of a malloc block is readable memory as well; the magic tells them apart.
bool isSampled(void* p)
{
    return static_cast<SampleHeader*>(p)[-1].magic == kMagic;
}

void freeSampled(void* p)
{
    {
        Profile& pr = profile();
        std::lock_guard lock(pr.mutex);
        pr.live.erase(p);
    }
    SampleHeader* header = static_cast<SampleHeader*>(p) - 1;
    header->magic = 0;
    HeapFree(privateHeap(), 0, header);
}

// The frames worth showing: past the allocator and standard library plumbing.
std::string describeStack(const Stack& stack)
{
    std::string text;
    int shown = 0;
    for (int i = 0; i < stack.count && shown < 7; ++i) {
        const std::string name = CrashHandler::describeAddress(reinterpret_cast<uint64_t>(stack.frames[i]));
        if (shown == 0 && (name.starts_with("operator new") || name.starts_with("std::") || name.starts_with("malloc")))
            continue;
        text += "\n      " + name;
        ++shown;
    }
    return text;
}

void reportLoop(int reportSeconds)
{
    busy = true;
    std::unordered_map<uint32_t, size_t> previous;
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(reportSeconds));
        std::unordered_map<uint32_t, std::pair<size_t, size_t>> byStack; // bytes, samples
        std::vector<Stack> stacks;
        {
            Profile& pr = profile();
            std::lock_guard lock(pr.mutex);
            for (const auto& [pointer, sample] : pr.live) {
                auto& entry = byStack[sample.stack];
                entry.first += sample.weight;
                ++entry.second;
            }
            stacks.assign(pr.stacks.begin(), pr.stacks.end());
        }
        std::vector<std::pair<uint32_t, size_t>> sorted;
        size_t total = 0;
        for (const auto& [id, entry] : byStack) {
            sorted.emplace_back(id, entry.first);
            total += entry.first;
        }
        std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

        PROCESS_MEMORY_COUNTERS_EX counters {};
        counters.cb = sizeof(counters);
        K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters));
        std::string text = std::format("Heap profile: ~{} MB live in sampled C++ allocations, {} MB private bytes", total >> 20, counters.PrivateUsage >> 20);
        for (size_t i = 0; i < sorted.size() && i < 15; ++i) {
            const auto [id, bytes] = sorted[i];
            const auto it = previous.find(id);
            const int64_t delta = static_cast<int64_t>(bytes) - static_cast<int64_t>(it == previous.end() ? 0 : it->second);
            text += std::format("\n  {:7.1f} MB ({:+.1f} MB){}", bytes / 1048576.0, delta / 1048576.0, describeStack(stacks[id]));
        }
        Log::info("{}", text);
        previous.clear();
        for (const auto& [id, bytes] : sorted)
            previous[id] = bytes;
    }
}

} // namespace

void HeapProfiler::start(int reportSeconds)
{
    profile();
    enabled = true;
    std::thread(reportLoop, reportSeconds).detach();
}

} // namespace mc

// --- Global allocation functions ---

void* operator new(size_t size)
{
    void* p = mc::enabled.load(std::memory_order_relaxed) && mc::shouldSample(size) ? mc::allocateSampled(size) : std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}

void* operator new[](size_t size)
{
    return operator new(size);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return mc::enabled.load(std::memory_order_relaxed) && mc::shouldSample(size) ? mc::allocateSampled(size) : std::malloc(size ? size : 1);
}

void* operator new[](size_t size, const std::nothrow_t& tag) noexcept
{
    return operator new(size, tag);
}

void operator delete(void* p) noexcept
{
    if (p && mc::enabled.load(std::memory_order_relaxed) && mc::isSampled(p)) {
        mc::freeSampled(p);
        return;
    }
    std::free(p);
}

void operator delete[](void* p) noexcept
{
    operator delete(p);
}

void operator delete(void* p, size_t) noexcept
{
    operator delete(p);
}

void operator delete[](void* p, size_t) noexcept
{
    operator delete(p);
}

void operator delete(void* p, const std::nothrow_t&) noexcept
{
    operator delete(p);
}

void operator delete[](void* p, const std::nothrow_t&) noexcept
{
    operator delete(p);
}
