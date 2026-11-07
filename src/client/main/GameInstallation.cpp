#include "client/main/GameInstallation.h"

#include <Windows.h>
#include <ShlObj.h>

#include <string>

namespace mc {

namespace {

std::filesystem::path roamingAppData()
{
    PWSTR folder = nullptr;
    std::filesystem::path result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &folder)))
        result = folder;
    CoTaskMemFree(folder);
    return result;
}

bool isFile(const std::filesystem::path& path)
{
    std::error_code ec;
    return !path.empty() && std::filesystem::is_regular_file(path, ec);
}

} // namespace

std::vector<std::filesystem::path> defaultLauncherRoots(const std::filesystem::path& gameDir)
{
    std::vector<std::filesystem::path> roots { gameDir };
    const std::filesystem::path appData = roamingAppData();
    if (!appData.empty()) {
        roots.push_back(appData / ".minecraft");
        roots.push_back(appData / ".tlauncher" / "legacy" / "Minecraft" / "game");
    }
    return roots;
}

std::optional<GameInstallation> findGameInstallation(const std::filesystem::path& gameDir,
    const std::filesystem::path& explicitJar, const std::filesystem::path& explicitAssetsDir,
    std::string_view version, std::string_view assetIndex)
{
    const std::string versionName(version);
    const std::string indexFile = std::string(assetIndex) + ".json";

    GameInstallation installation;
    installation.clientJar = explicitJar;
    installation.assetsDir = explicitAssetsDir;

    for (const std::filesystem::path& root : defaultLauncherRoots(gameDir)) {
        if (installation.clientJar.empty()) {
            const std::filesystem::path jar = root / "versions" / versionName / (versionName + ".jar");
            if (isFile(jar))
                installation.clientJar = jar;
        }
        if (installation.assetsDir.empty() && isFile(root / "assets" / "indexes" / indexFile))
            installation.assetsDir = root / "assets";
    }

    if (!isFile(installation.clientJar) || installation.assetsDir.empty())
        return std::nullopt;
    return installation;
}

} // namespace mc
