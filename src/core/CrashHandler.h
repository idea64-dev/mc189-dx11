#pragma once

#include <filesystem>

namespace mc {

// Writes <crashDir>/crash-<date>-client.dmp and logs a fatal line when the process dies from an unhandled
// SEH exception (access violation, stack overflow...). C++ exceptions are reported by Minecraft::run.
class CrashHandler {
public:
    static void install(const std::filesystem::path& crashDir);
    // Prints the stack of access violations to stderr when they happen (the test runner).
    static void installStackPrinter();
    // Diagnostics for hangs: logs the calling thread's stack every `seconds` (MC_HANG_TRACE=<seconds>).
    static void logThreadStackAfter(int seconds);
    // Sampling profiler of the calling thread (MC_PROFILE=<seconds>): about every millisecond its stack is
    // captured; every `reportSeconds` the functions with the most samples, on top of the stack and anywhere in
    // it, are logged.
    static void startSampler(int reportSeconds);
};

} // namespace mc
