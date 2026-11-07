#include "core/CrashHandler.h"

#include "core/Log.h"
#include "core/StringUtil.h"

#include <Windows.h>
#include <DbgHelp.h>

#include <ctime>
#include <format>

namespace mc {

namespace {

std::filesystem::path crashDirectory;

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

    Log::fatal("Unhandled exception 0x{:08X} at {}, minidump: {}",
        static_cast<unsigned long>(exception->ExceptionRecord->ExceptionCode),
        exception->ExceptionRecord->ExceptionAddress, toUtf8(dumpPath.wstring()));
    Log::shutdown();
    return EXCEPTION_EXECUTE_HANDLER;
}

} // namespace

void CrashHandler::install(const std::filesystem::path& crashDir)
{
    crashDirectory = std::filesystem::absolute(crashDir);
    SetUnhandledExceptionFilter(onUnhandledException);
}

} // namespace mc
