#pragma once

namespace mc {

// Sampling heap profiler (MC_HEAPPROFILE=<seconds>): logs every `reportSeconds` the allocation stacks that
// hold the most live memory and how much each grew since the previous report. The global operator new and
// delete (HeapProfiler.cpp) are plain malloc and free until it starts.
class HeapProfiler {
public:
    static void start(int reportSeconds);
};

} // namespace mc
