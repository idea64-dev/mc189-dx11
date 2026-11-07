#pragma once

// Original: net/minecraft/client/Minecraft.java
// Ported so far: startup, the game loop, window resize, fullscreen toggle, frame limiting and the debug FPS
// counter. Everything else (resources, GUI, world, sound, network) arrives in later ROADMAP stages.

#include "client/main/GameConfiguration.h"
#include "client/settings/GameSettings.h"
#include "util/ResourceLocation.h"
#include "util/Timer.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class DefaultResourcePack;
class Framebuffer;
class GuiScreen;
class IResourcePack;
class LanguageManager;
class SimpleReloadableResourceManager;
class TextureManager;
class WorldClient;

class Minecraft {
public:
    explicit Minecraft(const GameConfiguration& gameConfig);
    ~Minecraft();

    Minecraft(const Minecraft&) = delete;
    Minecraft& operator=(const Minecraft&) = delete;

    void run();
    // Called when the window is closed or the game is quit; the loop exits after the current frame.
    void shutdown();

    void toggleFullscreen();
    void dispatchKeypresses();

    int getLimitFramerate() const;
    bool isFramerateLimitBelowMax() const;

    static Minecraft* getMinecraft() { return theMinecraft; }
    // Milliseconds of the LWJGL system clock.
    static int64_t getSystemTime();
    static int getDebugFPS() { return debugFPS; }

    const DefaultResourcePack& getDefaultResourcePack() const { return *mcDefaultResourcePack; }
    TextureManager& getTextureManager() { return *renderEngine; }
    Framebuffer& getFramebuffer() { return *framebufferMc; }
    LanguageManager& getLanguageManager() { return *mcLanguageManager; }
    bool isUnicode() const;
    bool isFullScreen() const { return fullscreen; }
    // GameSettings FORCE_UNICODE_FONT: fontRendererObj.setUnicodeFlag(isUnicode()) once the font exists (stage 2.3).
    void updateFontUnicodeFlag();
    void refreshResources();
    // Draws a textured rectangle from a 256x256 texture with the WorldRenderer.
    void draw(int posX, int posY, int texU, int texV, int width, int height, int red, int green, int blue, int alpha);

    int displayWidth;
    int displayHeight;
    std::unique_ptr<GameSettings> gameSettings;
    // Debug line shown on F3, e.g. "120 fps (0 chunk updates) T: 120 vsync".
    std::string debug;

    // Not ported yet; kept so the ported logic reads like the original.
    WorldClient* theWorld = nullptr;
    GuiScreen* currentScreen = nullptr;

private:
    void startGame();
    void drawSplashScreen(TextureManager& textureManager);
    void setWindowIcon();
    void createDisplay();
    void setInitialDisplayMode();
    void runGameLoop();
    void runTick();
    void updateDisplay();
    void checkWindowResize();
    void resize(int width, int height);
    void updateFramebufferSize();
    void shutdownMinecraftApplet();
    void displayCrashReport(const std::string& description, const std::exception& cause);

    // Temporary input check for ROADMAP stage 1, removed once GuiScreen handles input.
    void debugInputTick();
    // Temporary stand-in for EntityRenderer.updateCameraAndRender until GuiMainMenu is ported (stage 2.5).
    void renderStage2Preview();

    static Minecraft* theMinecraft;
    static int debugFPS;

    Timer timer { 20.0f };
    bool fullscreen;
    int tempDisplayWidth;
    int tempDisplayHeight;
    std::filesystem::path mcDataDir;
    std::string launchedVersion;
    std::shared_ptr<DefaultResourcePack> mcDefaultResourcePack;
    std::vector<std::shared_ptr<IResourcePack>> defaultResourcePacks;
    std::unique_ptr<SimpleReloadableResourceManager> mcResourceManager;
    std::unique_ptr<LanguageManager> mcLanguageManager;
    std::unique_ptr<TextureManager> renderEngine;
    std::unique_ptr<Framebuffer> framebufferMc;
    std::optional<ResourceLocation> mojangLogo;
    volatile bool running = true;
    bool isGamePaused = false;
    int fpsCounter = 0;
    int64_t debugUpdateTime;
    int64_t startNanoTime;

    int debugMouseDX = 0;
    int debugMouseDY = 0;
};

} // namespace mc
