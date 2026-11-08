#pragma once

// Original: net/minecraft/client/Minecraft.java
// Ported so far: startup, resources, languages, settings, sound, fonts, the GUI screen flow, the game loop,
// resizing, fullscreen, screenshots and frame limiting. World, player, networking and the integrated server
// arrive in later ROADMAP stages; their call sites are noted inline.

#include "client/audio/MusicTicker.h"
#include "client/main/GameConfiguration.h"
#include "client/settings/GameSettings.h"
#include "profiler/PlayerUsageSnooper.h"
#include "util/MouseHelper.h"
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
class EntityRenderer;
class FontRenderer;
class Framebuffer;
class GuiScreen;
class IResourceManager;
class IResourcePack;
class ISaveFormat;
class LanguageManager;
class ResourcePackRepository;
class SimpleReloadableResourceManager;
class SoundHandler;
class TextureManager;
class WorldClient;
class WorldSettings;

class Minecraft : public IPlayerUsage {
public:
    explicit Minecraft(const GameConfiguration& gameConfig);
    ~Minecraft();

    Minecraft(const Minecraft&) = delete;
    Minecraft& operator=(const Minecraft&) = delete;

    void run();
    // Called when the window is closed or the game is quit; the loop exits after the current frame.
    void shutdown();

    // Shows a screen; null shows the main menu when no world is loaded.
    void displayGuiScreen(std::shared_ptr<GuiScreen> guiScreenIn);
    void setIngameFocus();
    void setIngameNotInFocus();
    void displayInGameMenu();
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
    SoundHandler& getSoundHandler() { return *mcSoundHandler; }
    MusicTicker& getMusicTicker() { return *mcMusicTicker; }
    // Java: the player's dimension and game mode pick the music once a world exists (stage 9).
    MusicTicker::MusicType getAmbientMusicType() const { return MusicTicker::MusicType::MENU; }
    ResourcePackRepository& getResourcePackRepository() { return *mcResourcePackRepository; }
    ISaveFormat& getSaveLoader() { return *saveLoader; }
    // Opens (or with settings, creates) a world and starts the integrated server for it. A null settings
    // pointer is Java's null: the world must exist.
    void launchIntegratedServer(const std::string& folderName, const std::string& worldName, const WorldSettings* worldSettingsIn);
    PlayerUsageSnooper& getPlayerUsageSnooper() { return *usageSnooper; }
    // Java: getIntegratedServer().getPlayerUsageSnooper(). Vanilla creates an idle IntegratedServer at startup,
    // so the snooper screen lists its stats even in the main menu.
    PlayerUsageSnooper* getIntegratedServerSnooper() { return integratedServerSnooper.get(); }
    void addServerStatsToSnooper(PlayerUsageSnooper& playerSnooper) override;
    void addServerTypeToSnooper(PlayerUsageSnooper& playerSnooper) override;
    bool isSnooperEnabled() const override { return gameSettings->snooperEnabled; }
    bool hasSoundHandler() const { return mcSoundHandler != nullptr; }
    const IResourceManager& getResourceManager() const;
    bool isUnicode() const;
    bool isFullScreen() const { return fullscreen; }
    // GameSettings FORCE_UNICODE_FONT: fontRendererObj.setUnicodeFlag(isUnicode()).
    void updateFontUnicodeFlag();
    void refreshResources();
    // Draws a textured rectangle from a 256x256 texture with the WorldRenderer.
    void draw(int posX, int posY, int texU, int texV, int width, int height, int red, int green, int blue, int alpha);

    int displayWidth;
    int displayHeight;
    std::unique_ptr<GameSettings> gameSettings;
    // Debug line shown on F3, e.g. "120 fps (0 chunk updates) T: 120 vsync".
    std::string debug;
    std::unique_ptr<FontRenderer> fontRendererObj;
    std::unique_ptr<FontRenderer> standardGalacticFontRenderer;
    std::shared_ptr<GuiScreen> currentScreen;
    MouseHelper mouseHelper;
    bool inGameHasFocus = false;
    bool skipRenderWorld = false;

    // Not ported yet; kept so the ported logic reads like the original.
    WorldClient* theWorld = nullptr;

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

    static Minecraft* theMinecraft;
    static int debugFPS;

    Timer timer { 20.0f };
    bool fullscreen;
    int tempDisplayWidth;
    int tempDisplayHeight;
    std::filesystem::path mcDataDir;
    std::filesystem::path fileResourcepacks;
    std::string launchedVersion;
    std::shared_ptr<DefaultResourcePack> mcDefaultResourcePack;
    std::vector<std::shared_ptr<IResourcePack>> defaultResourcePacks;
    std::unique_ptr<ResourcePackRepository> mcResourcePackRepository;
    std::unique_ptr<SimpleReloadableResourceManager> mcResourceManager;
    std::unique_ptr<LanguageManager> mcLanguageManager;
    std::unique_ptr<TextureManager> renderEngine;
    std::unique_ptr<SoundHandler> mcSoundHandler;
    std::unique_ptr<MusicTicker> mcMusicTicker;
    std::unique_ptr<EntityRenderer> entityRenderer;
    std::unique_ptr<Framebuffer> framebufferMc;
    std::optional<ResourceLocation> mojangLogo;
    volatile bool running = true;
    bool isGamePaused = false;
    int leftClickCounter = 0;
    int fpsCounter = 0;
    int64_t debugUpdateTime;
    int64_t startNanoTime;
    std::unique_ptr<PlayerUsageSnooper> usageSnooper;
    std::unique_ptr<ISaveFormat> saveLoader;
    std::unique_ptr<IPlayerUsage> integratedServer;
    std::unique_ptr<PlayerUsageSnooper> integratedServerSnooper;
};

} // namespace mc
