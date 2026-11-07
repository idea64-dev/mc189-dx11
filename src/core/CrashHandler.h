#pragma once

#include <filesystem>

namespace mc {

// Writes <crashDir>/crash-<date>-client.dmp and logs a fatal line when the process dies from an unhandled
// SEH exception (access violation, stack overflow...). C++ exceptions are reported by Minecraft::run.
class CrashHandler {
public:
    static void install(const std::filesystem::path& crashDir);
};

} // namespace mc
