#pragma once

#include <filesystem>
#include <format>
#include <string>
#include <string_view>

namespace mc {

enum class LogLevel { Debug, Info, Warn, Error, Fatal };

// Stand-in for log4j with the vanilla client layout: "[12:34:56] [Client thread/INFO]: message".
// Writes to <gameDir>/logs/latest.log, the debugger output and, when started from a console, stdout.
class Log {
public:
    static void init(const std::filesystem::path& logDirectory);
    static void shutdown();

    // Name shown in the "[thread/LEVEL]" prefix for the calling thread (Java: Thread.setName).
    static void setThreadName(std::string name);

    static void write(LogLevel level, std::string_view message);

    template <class... Args>
    static void debug(std::format_string<Args...> fmt, Args&&... args)
    {
        write(LogLevel::Debug, std::format(fmt, std::forward<Args>(args)...));
    }

    template <class... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args)
    {
        write(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template <class... Args>
    static void warn(std::format_string<Args...> fmt, Args&&... args)
    {
        write(LogLevel::Warn, std::format(fmt, std::forward<Args>(args)...));
    }

    template <class... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args)
    {
        write(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }

    template <class... Args>
    static void fatal(std::format_string<Args...> fmt, Args&&... args)
    {
        write(LogLevel::Fatal, std::format(fmt, std::forward<Args>(args)...));
    }
};

} // namespace mc
