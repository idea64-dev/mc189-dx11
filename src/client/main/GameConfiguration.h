#pragma once

// Original: net/minecraft/client/main/GameConfiguration.java (fields used so far)

#include "util/Session.h"

#include <filesystem>
#include <string>

namespace mc {

struct GameConfiguration {
    struct UserInformation {
        Session session;
    };

    struct DisplayInformation {
        int width = 854;
        int height = 480;
        bool fullscreen = false;
    };

    struct FolderInformation {
        std::filesystem::path mcDataDir;
        std::filesystem::path resourcePacksDir;
        std::filesystem::path assetsDir;
        std::string assetIndex = "1.8";
        // Port addition: Java had the client jar on its classpath.
        std::filesystem::path clientJar;
    };

    struct GameInformation {
        std::string version = "1.8.9";
    };

    UserInformation userInfo;
    DisplayInformation displayInfo;
    FolderInformation folderInfo;
    GameInformation gameInfo;
};

} // namespace mc
