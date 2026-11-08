#include "core/CrashHandler.h"

#include "core/Log.h"
#include "core/StringUtil.h"
#include <atomic>

#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <TlHelp32.h>

#include <cstdio>
#include <ctime>
#include <chrono>
#include <format>
#include <mutex>
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

// The unwind tables (.pdata) of the loaded modules, read before a thread is suspended: looking functions up
// through RtlLookupFunctionEntry takes a lock the suspended thread may hold (module loads), which would hang
// both threads.
struct ModuleTable {
    struct Module {
        DWORD64 base;
        DWORD64 end;
        const RUNTIME_FUNCTION* functions;
        size_t count;
    };
    std::vector<Module> modules;
};

ModuleTable snapshotModules()
{
    ModuleTable table;
    HMODULE handles[512];
    DWORD needed = 0;
    if (!K32EnumProcessModules(GetCurrentProcess(), handles, sizeof(handles), &needed))
        return table;
    const size_t count = std::min<size_t>(needed / sizeof(HMODULE), std::size(handles));
    for (size_t i = 0; i < count; ++i) {
        const auto base = reinterpret_cast<const uint8_t*>(handles[i]);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            continue;
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            continue;
        const IMAGE_DATA_DIRECTORY& exceptions = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
        table.modules.push_back({ reinterpret_cast<DWORD64>(base), reinterpret_cast<DWORD64>(base) + nt->OptionalHeader.SizeOfImage,
            reinterpret_cast<const RUNTIME_FUNCTION*>(base + exceptions.VirtualAddress), exceptions.Size / sizeof(RUNTIME_FUNCTION) });
    }
    std::sort(table.modules.begin(), table.modules.end(), [](const auto& a, const auto& b) { return a.base < b.base; });
    return table;
}

// RtlLookupFunctionEntry without its lock. Null for leaf code, and for code outside every module.
const RUNTIME_FUNCTION* findFunction(const ModuleTable& table, DWORD64 pc, DWORD64& imageBase, bool& inModule)
{
    inModule = false;
    const auto module = std::upper_bound(table.modules.begin(), table.modules.end(), pc, [](DWORD64 value, const auto& m) { return value < m.base; });
    if (module == table.modules.begin())
        return nullptr;
    const ModuleTable::Module& m = *std::prev(module);
    if (pc >= m.end)
        return nullptr;
    inModule = true;
    imageBase = m.base;
    const auto rva = static_cast<DWORD>(pc - m.base);
    size_t low = 0;
    size_t high = m.count;
    while (low < high) {
        const size_t mid = (low + high) / 2;
        if (rva < m.functions[mid].BeginAddress)
            high = mid;
        else if (rva >= m.functions[mid].EndAddress)
            low = mid + 1;
        else
            return &m.functions[mid];
    }
    return nullptr;
}

// Unwinds the context of a suspended thread: no locks, no allocation.
int unwindSuspended(const ModuleTable& table, CONTEXT context, DWORD64* frames, int maxFrames)
{
    int count = 0;
    while (count < maxFrames && context.Rip != 0) {
        frames[count++] = context.Rip;
        DWORD64 imageBase = 0;
        bool inModule = false;
        const RUNTIME_FUNCTION* function = findFunction(table, context.Rip, imageBase, inModule);
        if (!inModule)
            break;
        if (!function) {
            // A leaf function: the return address is on top of the stack.
            context.Rip = *reinterpret_cast<const DWORD64*>(context.Rsp);
            context.Rsp += 8;
            continue;
        }
        PVOID handlerData = nullptr;
        DWORD64 establisherFrame = 0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.Rip, const_cast<PRUNTIME_FUNCTION>(function), &context, &handlerData, &establisherFrame, nullptr);
    }
    return count;
}

// The function containing `address` (start of its unwind entry), or the address itself for leaf code.
DWORD64 functionStart(DWORD64 address)
{
    DWORD64 imageBase = 0;
    if (const PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry(address, &imageBase, nullptr))
        return imageBase + entry->BeginAddress;
    return address;
}

std::mutex& dbghelpMutex()
{
    static std::mutex mutex;
    return mutex;
}

// DbgHelp is single-threaded: callers hold dbghelpMutex.
void initializeSymbols()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(GetCurrentProcess(), nullptr, TRUE);
}

std::string symbolName(DWORD64 address)
{
    std::lock_guard lock(dbghelpMutex());
    initializeSymbols();
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

std::string CrashHandler::describeAddress(uint64_t address)
{
    std::string name = symbolName(address);
    std::lock_guard lock(dbghelpMutex());
    IMAGEHLP_LINE64 source {};
    source.SizeOfStruct = sizeof(source);
    DWORD displacement = 0;
    if (SymGetLineFromAddr64(GetCurrentProcess(), address, &displacement, &source))
        name += std::format(" ({}:{})", std::filesystem::path(source.FileName).filename().string(), source.LineNumber);
    return name;
}

namespace {

std::atomic<int64_t> lastHeartbeat { 0 };

int64_t steadyMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Every other thread of the process, unwound while suspended (no allocation, no locks), then described.
std::string describeAllThreads()
{
    struct Captured {
        DWORD id;
        std::wstring name;
        DWORD64 frames[48];
        int count = 0;
    };
    std::vector<DWORD> ids;
    if (const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); snapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 entry {};
        entry.dwSize = sizeof(entry);
        for (BOOL ok = Thread32First(snapshot, &entry); ok; ok = Thread32Next(snapshot, &entry)) {
            if (entry.th32OwnerProcessID == GetCurrentProcessId() && entry.th32ThreadID != GetCurrentThreadId())
                ids.push_back(entry.th32ThreadID);
        }
        CloseHandle(snapshot);
    }

    const ModuleTable modules = snapshotModules();
    std::vector<Captured> threads(ids.size());
    for (size_t t = 0; t < ids.size(); ++t) {
        Captured& captured = threads[t];
        captured.id = ids[t];
        const HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_LIMITED_INFORMATION, FALSE, ids[t]);
        if (!thread)
            continue;
        PWSTR description = nullptr;
        if (SUCCEEDED(GetThreadDescription(thread, &description)) && description) {
            captured.name = description;
            LocalFree(description);
        }
        if (SuspendThread(thread) != static_cast<DWORD>(-1)) {
            CONTEXT context {};
            context.ContextFlags = CONTEXT_FULL;
            if (GetThreadContext(thread, &context))
                captured.count = unwindSuspended(modules, context, captured.frames, 48);
            ResumeThread(thread);
        }
        CloseHandle(thread);
    }

    std::string text;
    for (const Captured& captured : threads) {
        text += std::format("\n  thread {} \"{}\":", captured.id, toUtf8(captured.name));
        for (int i = 0; i < captured.count; ++i)
            text += "\n      " + CrashHandler::describeAddress(captured.frames[i]);
    }
    return text;
}

} // namespace

void CrashHandler::heartbeat()
{
    lastHeartbeat.store(steadyMillis(), std::memory_order_relaxed);
}

void CrashHandler::startHangWatchdog(int seconds)
{
    heartbeat();
    std::thread([seconds] {
        Log::setThreadName("Hang watchdog");
        bool reported = false;
        for (;;) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            const int64_t silence = steadyMillis() - lastHeartbeat.load(std::memory_order_relaxed);
            if (silence < seconds * 1000LL) {
                reported = false;
                continue;
            }
            if (reported)
                continue;
            reported = true;
            Log::warn("The client thread has not finished a frame for {} s; all threads:{}", silence / 1000, describeAllThreads());
        }
    }).detach();
}

void CrashHandler::startSampler(int reportSeconds, const char* threadLabel)
{
    HANDLE target = nullptr;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &target, THREAD_ALL_ACCESS, FALSE, 0);
    std::thread([target, reportSeconds, threadLabel] {
        const HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

        std::unordered_map<DWORD64, int> self;
        std::unordered_map<DWORD64, int> total;
        int samples = 0;
        ModuleTable modules = snapshotModules();
        int sinceModules = 0;
        auto reportAt = std::chrono::steady_clock::now() + std::chrono::seconds(reportSeconds);
        for (;;) {
            LARGE_INTEGER due;
            due.QuadPart = -10'000; // 1 ms
            if (timer && SetWaitableTimerEx(timer, &due, 0, nullptr, nullptr, nullptr, 0))
                WaitForSingleObject(timer, INFINITE);
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // Modules come and go (drivers, DbgHelp): read their tables again now and then, not while suspended.
            if (++sinceModules >= 1000) {
                modules = snapshotModules();
                sinceModules = 0;
            }

            // Unwound while the thread is suspended, without anything that could take a lock it holds.
            DWORD64 frames[64];
            int count = 0;
            if (SuspendThread(target) == static_cast<DWORD>(-1)) {
                CloseHandle(target);
                return;
            }
            CONTEXT context {};
            context.ContextFlags = CONTEXT_FULL;
            if (GetThreadContext(target, &context))
                count = unwindSuspended(modules, context, frames, 64);
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
                Log::info("Profile of the {} thread, {} samples\n  self:{}\n  total:{}", threadLabel, samples, topFunctions(self, samples, 40),
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
