// Original: net/minecraft/client/main/Main.java

#include "client/Minecraft.h"
#include "client/main/GameConfiguration.h"
#include "client/main/GameInstallation.h"
#include "core/CrashHandler.h"
#include "core/HeapProfiler.h"
#include "core/Log.h"
#include "core/Sha1.h"
#include "core/StringUtil.h"

#include <Windows.h>
#include <shellapi.h>

#include <charconv>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

namespace {

std::vector<std::string> commandLineArguments()
{
    int count = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    std::vector<std::string> result;
    for (int i = 1; i < count; ++i)
        result.push_back(mc::toUtf8(arguments[i]));
    LocalFree(arguments);
    return result;
}

int parseInt(const std::string& text, int fallback)
{
    int value = fallback;
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

// Subset of the jopt-simple options in Main.java; the rest (proxy, server, properties) come with later stages.
mc::GameConfiguration parseArguments(const std::vector<std::string>& arguments, std::vector<std::string>& ignored)
{
    mc::GameConfiguration config;
    std::filesystem::path gameDir = ".";
    std::filesystem::path assetsDir;
    std::filesystem::path resourcePackDir;
    std::string username = "Player" + std::to_string(mc::Minecraft::getSystemTime() % 1000);
    std::optional<std::string> uuid;
    std::string accessToken;
    std::string userType = "legacy";

    for (size_t i = 0; i < arguments.size(); ++i) {
        const std::string& argument = arguments[i];
        const bool hasValue = i + 1 < arguments.size();

        if (argument == "--fullscreen") {
            config.displayInfo.fullscreen = true;
        } else if (argument == "--width" && hasValue) {
            config.displayInfo.width = parseInt(arguments[++i], 854);
        } else if (argument == "--height" && hasValue) {
            config.displayInfo.height = parseInt(arguments[++i], 480);
        } else if (argument == "--gameDir" && hasValue) {
            gameDir = mc::toWide(arguments[++i]);
        } else if (argument == "--assetsDir" && hasValue) {
            assetsDir = mc::toWide(arguments[++i]);
        } else if (argument == "--resourcePackDir" && hasValue) {
            resourcePackDir = mc::toWide(arguments[++i]);
        } else if (argument == "--clientJar" && hasValue) {
            config.folderInfo.clientJar = mc::toWide(arguments[++i]);
        } else if (argument == "--assetIndex" && hasValue) {
            config.folderInfo.assetIndex = arguments[++i];
        } else if (argument == "--version" && hasValue) {
            config.gameInfo.version = arguments[++i];
        } else if (argument == "--username" && hasValue) {
            username = arguments[++i];
        } else if (argument == "--uuid" && hasValue) {
            uuid = arguments[++i];
        } else if (argument == "--accessToken" && hasValue) {
            accessToken = arguments[++i];
        } else if (argument == "--userType" && hasValue) {
            userType = arguments[++i];
        } else {
            ignored.push_back(argument);
        }
    }

    // Java: the player id defaults to the user name.
    config.userInfo.session = mc::Session(username, uuid.value_or(username), accessToken, userType);
    config.folderInfo.mcDataDir = gameDir;
    config.folderInfo.assetsDir = assetsDir;
    config.folderInfo.resourcePacksDir = resourcePackDir.empty() ? gameDir / "resourcepacks" : resourcePackDir;
    return config;
}

void showError(const std::string& message)
{
    MessageBoxW(nullptr, mc::toWide(message).c_str(), L"Minecraft", MB_OK | MB_ICONERROR);
}

// Fills in the client jar and assets directory from an installed Minecraft when not given explicitly.
bool resolveInstallation(mc::GameConfiguration& config)
{
    auto& folders = config.folderInfo;
    const auto installation = mc::findGameInstallation(folders.mcDataDir, folders.clientJar, folders.assetsDir,
        config.gameInfo.version, folders.assetIndex);
    if (!installation) {
        std::string searched;
        for (const auto& root : mc::defaultLauncherRoots(folders.mcDataDir))
            searched += "\n  " + mc::toUtf8(root.wstring());
        const std::string message = "Minecraft " + config.gameInfo.version + " was not found.\n\n"
            "Install it with the official launcher or TLauncher, or pass --clientJar <path to "
            + config.gameInfo.version + ".jar> and --assetsDir <path to assets>.\n\nSearched:" + searched;
        mc::Log::fatal("{}", message);
        showError(message);
        return false;
    }

    folders.clientJar = installation->clientJar;
    folders.assetsDir = installation->assetsDir;
    mc::Log::info("Client jar: {}", mc::toUtf8(folders.clientJar.wstring()));
    mc::Log::info("Assets: {} (index {})", mc::toUtf8(folders.assetsDir.wstring()), folders.assetIndex);

    if (config.gameInfo.version == "1.8.9") {
        const auto hash = mc::sha1File(folders.clientJar);
        if (hash && *hash != mc::kClientJarSha1)
            mc::Log::warn("Client jar SHA-1 is {}, expected {} (modified jar?); textures may differ from vanilla", *hash, mc::kClientJarSha1);
    }
    return true;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    std::vector<std::string> ignored;
    mc::GameConfiguration config = parseArguments(commandLineArguments(), ignored);

    mc::Log::init(config.folderInfo.mcDataDir / "logs");
    mc::Log::setThreadName("Client thread");
    mc::CrashHandler::install(config.folderInfo.mcDataDir / "crash-reports");
    if (const char* trace = std::getenv("MC_HANG_TRACE"))
        mc::CrashHandler::logThreadStackAfter(std::max(1, std::atoi(trace)));
    if (const char* profile = std::getenv("MC_PROFILE"))
        mc::CrashHandler::startSampler(std::max(1, std::atoi(profile)));
    mc::CrashHandler::startHangWatchdog(5);
    if (const char* heap = std::getenv("MC_HEAPPROFILE"))
        mc::HeapProfiler::start(std::max(1, std::atoi(heap)));

    if (!ignored.empty()) {
        std::string list;
        for (const std::string& argument : ignored)
            list += (list.empty() ? "" : ", ") + argument;
        mc::Log::info("Completely ignored arguments: [{}]", list);
    }

    if (!resolveInstallation(config)) {
        mc::Log::shutdown();
        return 1;
    }

    int exitCode = 0;
    try {
        mc::Minecraft minecraft(config);
        minecraft.run();
    } catch (const std::exception& e) {
        mc::Log::fatal("Couldn't start the game: {}", e.what());
        showError(std::string("Couldn't start the game:\n\n") + e.what());
        exitCode = 1;
    }

    mc::Log::shutdown();
    return exitCode;
}
