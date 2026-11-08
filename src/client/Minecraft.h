#pragma once

// Original: net/minecraft/client/Minecraft.java
// Ported so far: startup, resources, languages, settings, sound, fonts, the GUI screen flow, the game loop,
// resizing, fullscreen, screenshots, frame limiting, the integrated server, the client world and player, the
// world renderer and the in-game input. Later ROADMAP stages are noted inline (effects, statistics, streams...).

#include "client/audio/MusicTicker.h"
#include "client/main/GameConfiguration.h"
#include "client/settings/GameSettings.h"
#include "profiler/PlayerUsageSnooper.h"
#include "profiler/Profiler.h"
#include "util/MouseHelper.h"
#include "util/ResourceLocation.h"
#include "util/Session.h"
#include "util/Timer.h"
#include "util/Vec3.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class BlockRendererDispatcher;
class DefaultResourcePack;
class Entity;
class EntityPlayerSP;
class EntityRenderer;
class FontRenderer;
class Framebuffer;
class GuiIngame;
class IResourceManagerReloadListener;
class GuiScreen;
class IntegratedServer;
class IResourceManager;
class IResourcePack;
class ISaveFormat;
class LanguageManager;
class LoadingScreenRenderer;
class ModelManager;
class NetHandlerPlayClient;
class NetworkManager;
class PlayerControllerMP;
class RenderGlobal;
class RenderItem;
class ResourcePackRepository;
class SimpleReloadableResourceManager;
class SoundHandler;
class TextureManager;
class TextureMap;
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
    TextureMap& getTextureMapBlocks() { return *textureMapBlocks; }
    ModelManager& getModelManager() { return *modelManager; }
    RenderItem& getRenderItem() { return *renderItem; }
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
    // Java: loadWorld(world[, message]) -- null unloads the world and stops the integrated server.
    void loadWorld(std::unique_ptr<WorldClient> worldClientIn, const std::string& loadingMessage = "");
    void setDimensionAndSpawnPlayer(int dimension);
    NetHandlerPlayClient* getNetHandler() const;
    bool isSingleplayer() const { return integratedServerIsRunning && theIntegratedServer != nullptr; }
    bool isIntegratedServerRunning() const { return integratedServerIsRunning; }
    IntegratedServer* getIntegratedServer() const { return theIntegratedServer.get(); }
    const Session& getSession() const { return session; }
    const std::filesystem::path& getDataDirectory() const { return mcDataDir; }
    const std::string& getVersion() const { return launchedVersion; }
    bool isGamePaused() const { return gamePaused; }
    bool isRunning() const { return running; }
    Entity* getRenderViewEntity() const { return renderViewEntity; }
    void setRenderViewEntity(Entity* viewingEntity);
    BlockRendererDispatcher& getBlockRendererDispatcher() { return *blockRenderDispatcher; }
    void updateDisplay();
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

    WorldClient* theWorld = nullptr;
    std::shared_ptr<EntityPlayerSP> thePlayer;
    std::unique_ptr<PlayerControllerMP> playerController;
    std::unique_ptr<GuiIngame> ingameGUI;
    std::unique_ptr<EntityRenderer> entityRenderer;
    std::unique_ptr<RenderGlobal> renderGlobal;
    std::unique_ptr<LoadingScreenRenderer> loadingScreen;
    std::optional<MovingObjectPosition> objectMouseOver;
    Entity* pointedEntity = nullptr;
    Profiler mcProfiler;

private:
    void startGame();
    void drawSplashScreen(TextureManager& textureManager);
    void setWindowIcon();
    void createDisplay();
    void setInitialDisplayMode();
    void runGameLoop();
    void runTick();
    void checkWindowResize();
    void clickMouse();
    void rightClickMouse();
    void middleClickMouse() { }
    void sendClickBlockToController(bool leftClick);
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
    std::shared_ptr<TextureMap> textureMapBlocks;
    std::unique_ptr<ModelManager> modelManager;
    std::unique_ptr<RenderItem> renderItem;
    std::unique_ptr<SoundHandler> mcSoundHandler;
    std::unique_ptr<MusicTicker> mcMusicTicker;
    std::unique_ptr<BlockRendererDispatcher> blockRenderDispatcher;
    std::unique_ptr<IResourceManagerReloadListener> grassColorReloadListener;
    std::unique_ptr<IResourceManagerReloadListener> foliageColorReloadListener;
    std::unique_ptr<Framebuffer> framebufferMc;
    std::optional<ResourceLocation> mojangLogo;
    volatile bool running = true;
    bool gamePaused = false;
    Session session;
    std::unique_ptr<WorldClient> worldOwner;
    Entity* renderViewEntity = nullptr;
    // Java: myNetworkManager is the pending connection only (null once the world loads); the connection itself
    // lives on through the net handler. Here it is kept by activeConnection until the world unloads, and a
    // closed one is released at the next frame (the call that closed it may still be running inside it).
    std::shared_ptr<NetworkManager> myNetworkManager;
    std::shared_ptr<NetworkManager> activeConnection;
    std::shared_ptr<NetworkManager> retiredConnection;
    std::unique_ptr<IntegratedServer> theIntegratedServer;
    bool integratedServerIsRunning = false;
    int rightClickDelayTimer = 0;
    int joinPlayerCounter = 0;
    int64_t systemTime = 0;
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
