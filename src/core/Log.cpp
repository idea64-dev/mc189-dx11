#include "core/Log.h"

#include "core/StringUtil.h"

#include <Windows.h>

#include <cstdio>
#include <ctime>
#include <fstream>
#include <mutex>

namespace mc {

namespace {

std::mutex logMutex;
std::ofstream logFile;
bool hasConsole = false;
thread_local std::string threadName = "main";

const char* levelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    }
    return "INFO";
}

} // namespace

void Log::init(const std::filesystem::path& logDirectory)
{
    std::lock_guard lock(logMutex);

    // A WIN32-subsystem executable has no console; reuse the parent's one when launched from cmd/PowerShell.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* stream = nullptr;
        freopen_s(&stream, "CONOUT$", "w", stdout);
        SetConsoleOutputCP(CP_UTF8);
        hasConsole = true;
    }

    std::error_code ec;
    std::filesystem::create_directories(logDirectory, ec);
    logFile.open(logDirectory / "latest.log", std::ios::out | std::ios::trunc | std::ios::binary);
}

void Log::shutdown()
{
    std::lock_guard lock(logMutex);
    logFile.close();
    if (hasConsole)
        std::fflush(stdout);
}

void Log::setThreadName(std::string name)
{
    threadName = std::move(name);
}

void Log::write(LogLevel level, std::string_view message)
{
#ifndef MC_DEBUG
    if (level == LogLevel::Debug)
        return;
#endif

    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);

    const std::string line = std::format("[{:02}:{:02}:{:02}] [{}/{}]: {}\n",
        local.tm_hour, local.tm_min, local.tm_sec, threadName, levelName(level), message);

    std::lock_guard lock(logMutex);
    if (logFile.is_open()) {
        logFile << line;
        logFile.flush();
    }
    if (hasConsole) {
        std::fputs(line.c_str(), stdout);
        std::fflush(stdout);
    }
    OutputDebugStringW(toWide(line).c_str());
}

} // namespace mc
