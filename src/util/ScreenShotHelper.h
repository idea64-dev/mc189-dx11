#pragma once

// Original: net/minecraft/util/ScreenShotHelper.java
// Returns the chat message text for now; IChatComponent arrives with the chat (stage 8).

#include <filesystem>
#include <string>

namespace mc {

class Framebuffer;

class ScreenShotHelper {
public:
    static std::string saveScreenshot(const std::filesystem::path& gameDirectory, int width, int height, Framebuffer& buffer);
    static std::string saveScreenshot(const std::filesystem::path& gameDirectory, const std::string& screenshotName, int width, int height, Framebuffer& buffer);

private:
    static std::filesystem::path getTimestampedPNGFileForDirectory(const std::filesystem::path& directory);
};

} // namespace mc
