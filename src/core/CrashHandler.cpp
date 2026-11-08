#include "core/CrashHandler.h"

#include "core/Log.h"
#include "core/StringUtil.h"

#include <Windows.h>
#include <DbgHelp.h>

#include <cstdio>
#include <ctime>
#include <chrono>
#include <format>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace mc {

namespace {

std::filesystem::path crashDirectory;

// The faulting thread's stack with symbols and lines from the PDB, for the log.
std::string describeStack(const CONTEXT& faultContext, HANDLE thread = GetCurrentThread())
{
    const HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymInitialize(process, nullptr, TRUE);

    CONTEXT context = faultContext;
    STACKFRAME64 frame {};
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    std::string text;
    for (int i = 0; i < 48; ++i) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
        const DWORD64 address = frame.AddrPC.Offset;
        if (address == 0)
            break;
        alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 512] {};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 511;
        DWORD64 displacement = 0;
        std::string line = std::format("\n    at 0x{:X}", address);
        if (SymFromAddr(process, address, &displacement, symbol))
            line = std::format("\n    at {}", symbol->Name);
        IMAGEHLP_LINE64 source {};
        source.SizeOfStruct = sizeof(source);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, address, &lineDisplacement, &source))
            line += std::format(" ({}:{})", source.FileName, source.LineNumber);
        text += line;
    }
    return text;
}

LONG WINAPI onUnhandledException(EXCEPTION_POINTERS* exception)
{
    const std::time_t now = std::time(nullptr);
    std::tm local {};
    localtime_s(&local, &now);
    // Same naming scheme as the Java crash reports: crash-2026-11-08_12.34.56-client.txt
    const std::wstring name = toWide(std::format("crash-{:04}-{:02}-{:02}_{:02}.{:02}.{:02}-client.dmp",
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec));

    std::error_code ec;
    std::filesystem::create_directories(crashDirectory, ec);
    const std::filesystem::path dumpPath = crashDirectory / name;

    const HANDLE file = CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION info {};
        info.ThreadId = GetCurrentThreadId();
        info.ExceptionPointers = exception;
        info.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
            static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo),
            &info, nullptr, nullptr);
        CloseHandle(file);
    }

    Log::fatal("Unhandled exception 0x{:08X} at {}, minidump: {}{}",
        static_cast<unsigned long>(exception->ExceptionRecord->ExceptionCode),
        exception->ExceptionRecord->ExceptionAddress, toUtf8(dumpPath.wstring()), describeStack(*exception->ContextRecord));
    Log::shutdown();
    return EXCEPTION_EXECUTE_HANDLER;
}

// First chance: prints the stack of an access violation to stderr at once (before any handler or a broken
// logger can lose it). Only for the test runner.
LONG WINAPI onFirstChanceException(EXCEPTION_POINTERS* exception)
{
    if (exception->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || exception->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
        const std::string stack = describeStack(*exception->ContextRecord);
        std::fprintf(stderr, "Exception 0x%08lX at %p%s\n", static_cast<unsigned long>(exception->ExceptionRecord->ExceptionCode),
            exception->ExceptionRecord->ExceptionAddress, stack.c_str());
        std::fflush(stderr);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

void CrashHandler::install(const std::filesystem::path& crashDir)
{
    crashDirectory = std::filesystem::absolute(crashDir);
    SetUnhandledExceptionFilter(onUnhandledException);
}

void CrashHandler::installStackPrinter()
{
    AddVectoredExceptionHandler(1, onFirstChanceException);
}

void CrashHandler::logThreadStackAfter(int seconds)
{
    HANDLE target = nullptr;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &target, THREAD_ALL_ACCESS, FALSE, 0);
    std::thread([target, seconds] {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            if (SuspendThread(target) == static_cast<DWORD>(-1))
                return;
            CONTEXT context {};
            context.ContextFlags = CONTEXT_FULL;
            const bool ok = GetThreadContext(target, &context) != FALSE;
            const std::string stack = ok ? describeStack(context, target) : std::string(" (no context)");
            ResumeThread(target);
            Log::warn("Main thread stack after {} s:{}", seconds, stack);
        }
    }).detach();
}

namespace {

// The function containing `address` (start of its unwind entry), or the address itself for leaf code.
DWORD64 functionStart(DWORD64 address)
{
    DWORD64 imageBase = 0;
    if (const PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry(address, &imageBase, nullptr))
        return imageBase + entry->BeginAddress;
    return address;
}

std::string symbolName(DWORD64 address)
{
    const HANDLE process = GetCurrentProcess();
    alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 512] {};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 511;
    DWORD64 displacement = 0;
    if (SymFromAddr(process, address, &displacement, symbol))
        return symbol->Name;
    return std::format("0x{:X}", address);
}

std::string topFunctions(const std::unordered_map<DWORD64, int>& counts, int samples, size_t limit)
{
    std::vector<std::pair<DWORD64, int>> sorted(counts.begin(), counts.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    std::string text;
    for (size_t i = 0; i < sorted.size() && i < limit; ++i)
        text += std::format("\n    {:5.1f}%  {}", 100.0 * sorted[i].second / samples, symbolName(sorted[i].first));
    return text;
}

} // namespace

void CrashHandler::startSampler(int reportSeconds)
{
    HANDLE target = nullptr;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &target, THREAD_ALL_ACCESS, FALSE, 0);
    std::thread([target, reportSeconds] {
        const HANDLE process = GetCurrentProcess();
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        SymInitialize(process, nullptr, TRUE);
        const HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

        std::unordered_map<DWORD64, int> self;
        std::unordered_map<DWORD64, int> total;
        int samples = 0;
        auto reportAt = std::chrono::steady_clock::now() + std::chrono::seconds(reportSeconds);
        for (;;) {
            LARGE_INTEGER due;
            due.QuadPart = -10'000; // 1 ms
            if (timer && SetWaitableTimerEx(timer, &due, 0, nullptr, nullptr, nullptr, 0))
                WaitForSingleObject(timer, INFINITE);
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // Unwound while the thread is suspended, without anything that could take a lock it holds.
            DWORD64 frames[64];
            int count = 0;
            if (SuspendThread(target) == static_cast<DWORD>(-1))
                return;
            CONTEXT context {};
            context.ContextFlags = CONTEXT_FULL;
            if (GetThreadContext(target, &context)) {
                while (count < 64 && context.Rip != 0) {
                    frames[count++] = context.Rip;
                    DWORD64 imageBase = 0;
                    const PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry(context.Rip, &imageBase, nullptr);
                    if (!entry) {
                        context.Rip = *reinterpret_cast<const DWORD64*>(context.Rsp);
                        context.Rsp += 8;
                        continue;
                    }
                    PVOID handlerData = nullptr;
                    DWORD64 establisherFrame = 0;
                    RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.Rip, entry, &context, &handlerData, &establisherFrame, nullptr);
                }
            }
            ResumeThread(target);

            if (count > 0) {
                ++samples;
                ++self[functionStart(frames[0])];
                std::unordered_set<DWORD64> seen;
                for (int i = 0; i < count; ++i) {
                    const DWORD64 function = functionStart(frames[i]);
                    if (seen.insert(function).second)
                        ++total[function];
                }
            }

            if (std::chrono::steady_clock::now() >= reportAt && samples > 0) {
                Log::info("Profile of the client thread, {} samples\n  self:{}\n  total:{}", samples, topFunctions(self, samples, 40),
                    topFunctions(total, samples, 60));
                self.clear();
                total.clear();
                samples = 0;
                reportAt = std::chrono::steady_clock::now() + std::chrono::seconds(reportSeconds);
            }
        }
    }).detach();
}

} // namespace mc
