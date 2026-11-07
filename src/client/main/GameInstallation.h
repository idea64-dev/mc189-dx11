#pragma once

// Locates the vanilla client jar and asset store. Java got them from the launcher (classpath and --assetsDir);
// this port reads Mojang's files from an existing Minecraft 1.8.9 installation instead of shipping them.

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace mc {

struct GameInstallation {
    std::filesystem::path clientJar;
    std::filesystem::path assetsDir;
};

// SHA-1 of the official 1.8.9 client jar (launchermeta "downloads.client.sha1").
inline constexpr std::string_view kClientJarSha1 = "3870888a6c3d349d3771a3e9d16c9bf5e076b908";

// Launcher folders searched when no explicit paths are given, in order:
// the game directory, %APPDATA%\.minecraft, %APPDATA%\.tlauncher\legacy\Minecraft\game.
std::vector<std::filesystem::path> defaultLauncherRoots(const std::filesystem::path& gameDir);

// An explicit jar/assets path wins; whatever is missing is looked up under the launcher roots.
std::optional<GameInstallation> findGameInstallation(const std::filesystem::path& gameDir,
    const std::filesystem::path& explicitJar, const std::filesystem::path& explicitAssetsDir,
    std::string_view version, std::string_view assetIndex);

} // namespace mc
