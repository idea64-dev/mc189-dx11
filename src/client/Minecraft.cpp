#include "client/Minecraft.h"

#include "client/audio/SoundHandler.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiControls.h"
#include "client/gui/GuiMainMenu.h"
#include "client/gui/GuiScreen.h"
#include "client/gui/ScaledResolution.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/EntityRenderer.h"
#include "client/renderer/OpenGlHelper.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/texture/SimpleTexture.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/resources/DefaultResourcePack.h"
#include "client/resources/LanguageManager.h"
#include "client/resources/Locale.h"
#include "client/resources/ResourceIndex.h"
#include "client/resources/ResourcePackRepository.h"
#include "client/resources/SimpleReloadableResourceManager.h"
#include "client/shader/Framebuffer.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/ZipArchive.h"
#include "platform/Display.h"
#include "platform/Keyboard.h"
#include "platform/Mouse.h"
#include "platform/Sys.h"
#include "render/gl/GL.h"
#include "util/ScreenShotHelper.h"

#include <Windows.h>
#include <d3d11.h>

#include <algorithm>
#include <chrono>
#include <format>
#include <stdexcept>
#include <thread>

namespace mc {

namespace {

const ResourceLocation locationMojangPng("textures/gui/title/mojang.png");

// Java: MinecraftServer.getCurrentTimeMillis()
int64_t currentTimeMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

// Java: the IntegratedServer that Minecraft's constructor creates before any world exists. Until a world is
// loaded (stage 9) only its snooper is visible, on the Snooper Settings screen.
class IdleIntegratedServer : public IPlayerUsage {
public:
    explicit IdleIntegratedServer(Minecraft& mcIn)
        : mc(mcIn)
    {
    }

    void addServerStatsToSnooper(PlayerUsageSnooper& playerSnooper) override
    {
        playerSnooper.addClientStat("whitelist_enabled", "false");
        playerSnooper.addClientStat("whitelist_count", "0");
        playerSnooper.addClientStat("uses_auth", "false");
        playerSnooper.addClientStat("gui_state", "disabled");
        // Java's "/ 60L * 1000L" (meant to be minutes) kept as is.
        playerSnooper.addClientStat("run_time", std::to_string((currentTimeMillis() - playerSnooper.getMinecraftStartTimeMillis()) / 60LL * 1000LL));
        playerSnooper.addClientStat("avg_tick_ms", "0");
        playerSnooper.addClientStat("worlds", "0");
        playerSnooper.addClientStat("snooper_partner", mc.getPlayerUsageSnooper().getUniqueID());
    }

    void addServerTypeToSnooper(PlayerUsageSnooper& playerSnooper) override
    {
        playerSnooper.addStatToSnooper("singleplayer", "true");
        playerSnooper.addStatToSnooper("server_brand", "vanilla");
        playerSnooper.addStatToSnooper("gui_supported", "supported");
        playerSnooper.addStatToSnooper("dedicated", "false");
    }

    bool isSnooperEnabled() const override { return mc.isSnooperEnabled(); }

private:
    Minecraft& mc;
};

} // namespace

Minecraft* Minecraft::theMinecraft = nullptr;
int Minecraft::debugFPS = 0;

Minecraft::Minecraft(const GameConfiguration& gameConfig)
    : displayWidth(gameConfig.displayInfo.width > 0 ? gameConfig.displayInfo.width : 1)
    , displayHeight(gameConfig.displayInfo.height > 0 ? gameConfig.displayInfo.height : 1)
    , fullscreen(gameConfig.displayInfo.fullscreen)
    , tempDisplayWidth(gameConfig.displayInfo.width)
    , tempDisplayHeight(gameConfig.displayInfo.height)
    , mcDataDir(gameConfig.folderInfo.mcDataDir)
    , fileResourcepacks(gameConfig.folderInfo.resourcePacksDir)
    , launchedVersion(gameConfig.gameInfo.version)
    , debugUpdateTime(getSystemTime())
    , startNanoTime(Sys::nanoTime())
{
    theMinecraft = this;
    usageSnooper = std::make_unique<PlayerUsageSnooper>("client", *this, currentTimeMillis());
    integratedServer = std::make_unique<IdleIntegratedServer>(*this);
    integratedServerSnooper = std::make_unique<PlayerUsageSnooper>("server", *integratedServer, currentTimeMillis());
    mcDefaultResourcePack = std::make_shared<DefaultResourcePack>(
        std::make_shared<const ZipArchive>(gameConfig.folderInfo.clientJar),
        ResourceIndex(gameConfig.folderInfo.assetsDir, gameConfig.folderInfo.assetIndex).getResourceMap());
}

Minecraft::~Minecraft()
{
    if (theMinecraft == this)
        theMinecraft = nullptr;
}

int64_t Minecraft::getSystemTime()
{
    return Sys::getTime() * 1000LL / Sys::getTimerResolution();
}

void Minecraft::run()
{
    running = true;

    try {
        startGame();
    } catch (const std::exception& e) {
        displayCrashReport("Initializing game", e);
        shutdownMinecraftApplet();
        return;
    }

    try {
        while (running)
            runGameLoop();
    } catch (const std::exception& e) {
        Log::fatal("Unreported exception thrown! {}", e.what());
        displayCrashReport("Unexpected error", e);
    }

    shutdownMinecraftApplet();
}

void Minecraft::startGame()
{
    gameSettings = std::make_unique<GameSettings>(this, mcDataDir);
    // Java: startTimerHackThread() raised the Windows timer resolution; Display::sync uses a high-resolution timer instead.

    if (gameSettings->overrideHeight > 0 && gameSettings->overrideWidth > 0) {
        displayWidth = gameSettings->overrideWidth;
        displayHeight = gameSettings->overrideHeight;
    }

    Log::info("Minecraft {} (C++ / Direct3D 11 port)", launchedVersion);
    setWindowIcon();
    setInitialDisplayMode();
    createDisplay();

    // The window size is DPI-scaled by Display, so take the real client size.
    displayWidth = Display::getWidth();
    displayHeight = Display::getHeight();

    framebufferMc = std::make_unique<Framebuffer>(displayWidth, displayHeight, true);
    framebufferMc->setFramebufferColor(0.0f, 0.0f, 0.0f, 0.0f);
    defaultResourcePacks.push_back(mcDefaultResourcePack);
    mcResourcePackRepository = std::make_unique<ResourcePackRepository>(fileResourcepacks, mcDataDir / "server-resource-packs", mcDefaultResourcePack, *gameSettings);
    mcResourceManager = std::make_unique<SimpleReloadableResourceManager>();
    mcLanguageManager = std::make_unique<LanguageManager>(gameSettings->language);
    mcResourceManager->registerReloadListener(*mcLanguageManager);
    refreshResources();
    renderEngine = std::make_unique<TextureManager>(*mcResourceManager);
    mcResourceManager->registerReloadListener(*renderEngine);
    drawSplashScreen(*renderEngine);

    // Java: skinManager, saveLoader (stage 5)
    mcSoundHandler = std::make_unique<SoundHandler>(*mcResourceManager, *gameSettings);
    mcResourceManager->registerReloadListener(*mcSoundHandler);
    mcMusicTicker = std::make_unique<MusicTicker>(*this);

    fontRendererObj = std::make_unique<FontRenderer>(*gameSettings, ResourceLocation("textures/font/ascii.png"), *renderEngine, false);
    if (!gameSettings->language.empty()) {
        fontRendererObj->setUnicodeFlag(isUnicode());
        fontRendererObj->setBidiFlag(mcLanguageManager->isCurrentLanguageBidirectional());
    }
    standardGalacticFontRenderer = std::make_unique<FontRenderer>(*gameSettings, ResourceLocation("textures/font/ascii_sga.png"), *renderEngine, false);
    mcResourceManager->registerReloadListener(*fontRendererObj);
    mcResourceManager->registerReloadListener(*standardGalacticFontRenderer);
    // Java: GrassColorReloadListener, FoliageColorReloadListener, AchievementList, mouseHelper (stage 4+)

    DxStateManager::enableTexture2D();
    DxStateManager::shadeModel(7425);
    DxStateManager::clearDepth(1.0);
    DxStateManager::enableDepth();
    DxStateManager::depthFunc(515);
    DxStateManager::enableAlpha();
    DxStateManager::alphaFunc(516, 0.1f);
    DxStateManager::cullFace(1029);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    DxStateManager::matrixMode(5888);

    // Java: texture map, model manager, render item/manager, block renderer (stage 7)
    entityRenderer = std::make_unique<EntityRenderer>(*this);
    // Java: renderGlobal, guiAchievement (stage 7/12)
    DxStateManager::viewport(0, 0, displayWidth, displayHeight);
    // Java: effectRenderer (stage 15), ingameGUI (stage 8)

    // Java: connects to --server here (stage 14).
    displayGuiScreen(std::make_shared<GuiMainMenu>());

    if (mojangLogo)
        renderEngine->deleteTexture(*mojangLogo);
    mojangLogo.reset();
    // Java: loadingScreen = new LoadingScreenRenderer(this) (stage 5)

    if (gameSettings->fullScreen && !fullscreen)
        toggleFullscreen();

    Display::setVSyncEnabled(gameSettings->enableVsync);
}

void Minecraft::refreshResources()
{
    std::vector<std::shared_ptr<IResourcePack>> list = defaultResourcePacks;
    for (const auto& entry : mcResourcePackRepository->getRepositoryEntries())
        list.push_back(entry->getResourcePack());
    if (auto serverPack = mcResourcePackRepository->getResourcePackInstance())
        list.push_back(std::move(serverPack));

    try {
        mcResourceManager->reloadResources(list);
    } catch (const std::exception& e) {
        Log::info("Caught error stitching, removing all assigned resourcepacks ({})", e.what());
        list = defaultResourcePacks;
        mcResourcePackRepository->setRepositories({});
        mcResourceManager->reloadResources(list);
        gameSettings->resourcePacks.clear();
        gameSettings->incompatibleResourcePacks.clear();
        gameSettings->saveOptions();
    }
    mcLanguageManager->parseLanguageMetadata(list);
    // Java: renderGlobal.loadRenderers() (stage 7)
}

bool Minecraft::isUnicode() const
{
    return (mcLanguageManager && mcLanguageManager->isCurrentLocaleUnicode()) || (gameSettings && gameSettings->forceUnicodeFont);
}

void Minecraft::updateFontUnicodeFlag()
{
    if (fontRendererObj)
        fontRendererObj->setUnicodeFlag(isUnicode());
}

const IResourceManager& Minecraft::getResourceManager() const
{
    return *mcResourceManager;
}

void Minecraft::drawSplashScreen(TextureManager& textureManager)
{
    const ScaledResolution scaledresolution(*this);
    const int i = scaledresolution.getScaleFactor();
    Framebuffer framebuffer(scaledresolution.getScaledWidth() * i, scaledresolution.getScaledHeight() * i, true);
    framebuffer.bindFramebuffer(false);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    DxStateManager::ortho(0.0, static_cast<double>(scaledresolution.getScaledWidth()), static_cast<double>(scaledresolution.getScaledHeight()), 0.0, 1000.0, 3000.0);
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();
    DxStateManager::translate(0.0f, 0.0f, -2000.0f);
    DxStateManager::disableLighting();
    DxStateManager::disableFog();
    DxStateManager::disableDepth();
    DxStateManager::enableTexture2D();

    try {
        const std::vector<uint8_t> data = mcDefaultResourcePack->getInputStream(locationMojangPng);
        mojangLogo = textureManager.getDynamicTextureLocation("logo", std::make_shared<DynamicTexture>(TextureUtil::readBufferedImage(data)));
        textureManager.bindTexture(*mojangLogo);
    } catch (const std::exception& e) {
        Log::error("Unable to load logo: {} ({})", locationMojangPng.toString(), e.what());
    }

    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(0.0, static_cast<double>(displayHeight), 0.0).tex(0.0, 0.0).color(255, 255, 255, 255).endVertex();
    worldrenderer.pos(static_cast<double>(displayWidth), static_cast<double>(displayHeight), 0.0).tex(0.0, 0.0).color(255, 255, 255, 255).endVertex();
    worldrenderer.pos(static_cast<double>(displayWidth), 0.0, 0.0).tex(0.0, 0.0).color(255, 255, 255, 255).endVertex();
    worldrenderer.pos(0.0, 0.0, 0.0).tex(0.0, 0.0).color(255, 255, 255, 255).endVertex();
    tessellator.draw();
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    const int j = 256;
    const int k = 256;
    draw((scaledresolution.getScaledWidth() - j) / 2, (scaledresolution.getScaledHeight() - k) / 2, 0, 0, j, k, 255, 255, 255, 255);
    DxStateManager::disableLighting();
    DxStateManager::disableFog();
    framebuffer.unbindFramebuffer();
    framebuffer.framebufferRender(scaledresolution.getScaledWidth() * i, scaledresolution.getScaledHeight() * i);
    DxStateManager::enableAlpha();
    DxStateManager::alphaFunc(516, 0.1f);
    updateDisplay();
}

void Minecraft::draw(int posX, int posY, int texU, int texV, int width, int height, int red, int green, int blue, int alpha)
{
    const float f = 0.00390625f;
    const float f1 = 0.00390625f;
    WorldRenderer& worldrenderer = Tessellator::getInstance().getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(posX, posY + height, 0.0).tex(static_cast<float>(texU) * f, static_cast<float>(texV + height) * f1).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(posX + width, posY + height, 0.0).tex(static_cast<float>(texU + width) * f, static_cast<float>(texV + height) * f1).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(posX + width, posY, 0.0).tex(static_cast<float>(texU + width) * f, static_cast<float>(texV) * f1).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(posX, posY, 0.0).tex(static_cast<float>(texU) * f, static_cast<float>(texV) * f1).color(red, green, blue, alpha).endVertex();
    Tessellator::getInstance().draw();
}

void Minecraft::setWindowIcon()
{
    try {
        std::vector<DisplayIcon> icons;
        for (const char* name : { "icons/icon_16x16.png", "icons/icon_32x32.png" }) {
            const auto data = mcDefaultResourcePack->getInputStreamAssets(ResourceLocation(name));
            if (!data)
                return; // Java: both or nothing
            BufferedImage image = TextureUtil::readBufferedImage(*data);
            icons.push_back(DisplayIcon { image.width, image.height, std::move(image.pixels) });
        }
        Display::setIcon(std::move(icons));
    } catch (const std::exception& e) {
        Log::error("Couldn't set icon: {}", e.what());
    }
}

void Minecraft::setInitialDisplayMode()
{
    if (fullscreen) {
        Display::setFullscreen(true);
    } else {
        Display::setDisplayMode(DisplayMode { displayWidth, displayHeight });
    }
}

void Minecraft::createDisplay()
{
    Display::setResizable(true);
    Display::setTitle("Minecraft 1.8.9");
    Display::create();
    GL::initialize(Display::getDevice());
}

void Minecraft::runGameLoop()
{
    const int64_t i = Sys::nanoTime();

    if (Display::isCreated() && Display::isCloseRequested())
        shutdown();

    if (isGamePaused && theWorld != nullptr) {
        const float f = timer.renderPartialTicks;
        timer.updateTimer();
        timer.renderPartialTicks = f;
    } else {
        timer.updateTimer();
    }

    // Java: scheduledExecutables (stage 9)

    for (int j = 0; j < timer.elapsedTicks; ++j)
        runTick();

    // Java: sound listener (stage 13)

    DxStateManager::pushMatrix();
    DxStateManager::clear(16640);
    framebufferMc->bindFramebuffer(true);
    DxStateManager::enableTexture2D();
    if (!skipRenderWorld)
        entityRenderer->updateCameraAndRender(timer.renderPartialTicks, i);
    // Java: profiler chart (stage 8), guiAchievement.updateAchievementWindow() (stage 12)
    framebufferMc->unbindFramebuffer();
    DxStateManager::popMatrix();
    DxStateManager::pushMatrix();
    framebufferMc->framebufferRender(displayWidth, displayHeight);
    DxStateManager::popMatrix();

    updateDisplay();
    std::this_thread::yield();

    ++fpsCounter;
    // Java: isGamePaused = isSingleplayer() && currentScreen.doesGuiPauseGame() && !integratedServer.getPublic()
    isGamePaused = false;
    const int64_t k = Sys::nanoTime();
    startNanoTime = k;

    while (getSystemTime() >= debugUpdateTime + 1000LL) {
        debugFPS = fpsCounter;
        const bool unlimited = static_cast<float>(gameSettings->limitFramerate) == GameSettings::Options::FRAMERATE_LIMIT.getValueMax();
        debug = std::format("{} fps ({} chunk update{}) T: {}{}{}{}{}",
            debugFPS, 0, "s",
            unlimited ? std::string("inf") : std::to_string(gameSettings->limitFramerate),
            gameSettings->enableVsync ? " vsync" : "",
            gameSettings->fancyGraphics ? "" : " fast",
            gameSettings->clouds == 0 ? "" : (gameSettings->clouds == 1 ? " fast-clouds" : " fancy-clouds"),
            OpenGlHelper::vboSupported && gameSettings->useVbo ? " vbo" : "");
        debugUpdateTime += 1000LL;
        fpsCounter = 0;
        usageSnooper->addMemoryStatsToSnooper();
        if (!usageSnooper->isSnooperRunning())
            usageSnooper->startSnooper();
    }

    if (isFramerateLimitBelowMax())
        Display::sync(getLimitFramerate());
}

void Minecraft::runTick()
{
    // Java: rightClickDelayTimer, ingameGUI.updateTick(), entityRenderer.getMouseOver(), playerController (stage 8)
    if (!isGamePaused)
        renderEngine->tick();

    if (currentScreen != nullptr)
        leftClickCounter = 10000;

    if (const std::shared_ptr<GuiScreen> screen = currentScreen) {
        screen->handleInput();
        if (const std::shared_ptr<GuiScreen> after = currentScreen)
            after->updateScreen();
    }

    if (currentScreen == nullptr || currentScreen->allowUserInput) {
        while (Mouse::next()) {
            const int i = Mouse::getEventButton();
            KeyBinding::setKeyBindState(i - 100, Mouse::getEventButtonState());
            if (Mouse::getEventButtonState())
                KeyBinding::onTick(i - 100);
            // Java: hotbar scrolling and spectator menu (stage 8)
            if (currentScreen == nullptr) {
                if (!inGameHasFocus && Mouse::getEventButtonState())
                    setIngameFocus();
            } else {
                currentScreen->handleMouseInput();
            }
        }

        if (leftClickCounter > 0)
            --leftClickCounter;

        while (Keyboard::next()) {
            const int k = Keyboard::getEventKey() == 0 ? Keyboard::getEventCharacter() + 256 : Keyboard::getEventKey();
            KeyBinding::setKeyBindState(k, Keyboard::getEventKeyState());
            if (Keyboard::getEventKeyState())
                KeyBinding::onTick(k);
            // Java: F3 debug combinations and the crash key (stage 8)
            dispatchKeypresses();
            if (Keyboard::getEventKeyState()) {
                if (currentScreen != nullptr)
                    currentScreen->handleKeyboardInput();
                // Java: in-game keys (pause, F1, F3, F5...) (stage 8)
            }
        }
        // Java: key bindings for the player (stage 8)
    }

    if (!isGamePaused) {
        mcMusicTicker->update();
        mcSoundHandler->update();
    }
    // Java: world ticking (stage 9)
}

void Minecraft::displayGuiScreen(std::shared_ptr<GuiScreen> guiScreenIn)
{
    // Keep the old screen alive while its onGuiClosed runs (it may be the caller).
    const std::shared_ptr<GuiScreen> previous = currentScreen;
    if (previous)
        previous->onGuiClosed();

    if (guiScreenIn == nullptr && theWorld == nullptr)
        guiScreenIn = std::make_shared<GuiMainMenu>();
    // Java: GuiGameOver when the player is dead (stage 8)

    if (dynamic_cast<GuiMainMenu*>(guiScreenIn.get())) {
        gameSettings->showDebugInfo = false;
        // Java: ingameGUI.getChatGUI().clearChatMessages() (stage 8)
    }

    currentScreen = guiScreenIn;

    if (guiScreenIn != nullptr) {
        setIngameNotInFocus();
        const ScaledResolution scaledresolution(*this);
        guiScreenIn->setWorldAndResolution(*this, scaledresolution.getScaledWidth(), scaledresolution.getScaledHeight());
        skipRenderWorld = false;
    } else {
        mcSoundHandler->resumeSounds();
        setIngameFocus();
    }
}

void Minecraft::setIngameFocus()
{
    if (Display::isActive() && !inGameHasFocus) {
        inGameHasFocus = true;
        mouseHelper.grabMouseCursor();
        displayGuiScreen(nullptr);
        leftClickCounter = 10000;
    }
}

void Minecraft::setIngameNotInFocus()
{
    if (inGameHasFocus) {
        KeyBinding::unPressAllKeys();
        inGameHasFocus = false;
        mouseHelper.ungrabMouseCursor();
    }
}

void Minecraft::displayInGameMenu()
{
    if (currentScreen == nullptr) {
        // Java: displayGuiScreen(new GuiIngameMenu()) and pauses the integrated server (stage 8)
    }
}

void Minecraft::dispatchKeypresses()
{
    const int i = Keyboard::getEventKey() == 0 ? Keyboard::getEventCharacter() : Keyboard::getEventKey();

    if (i != 0 && !Keyboard::isRepeatEvent()) {
        // A key just assigned on the controls screen doesn't also trigger its old action.
        const auto* controls = dynamic_cast<const GuiControls*>(currentScreen.get());
        if ((controls == nullptr || controls->time <= getSystemTime() - 20LL) && Keyboard::getEventKeyState()) {
            if (i == gameSettings->keyBindFullscreen.getKeyCode()) {
                toggleFullscreen();
            } else if (i == gameSettings->keyBindScreenshot.getKeyCode()) {
                // Java prints the result to the chat (stage 8).
                ScreenShotHelper::saveScreenshot(mcDataDir, displayWidth, displayHeight, *framebufferMc);
            }
            // Twitch stream keys are not ported.
        }
    }
}

void Minecraft::updateDisplay()
{
    Display::update();
    checkWindowResize();
}

void Minecraft::checkWindowResize()
{
    if (!fullscreen && Display::wasResized()) {
        const int i = displayWidth;
        const int j = displayHeight;
        displayWidth = Display::getWidth();
        displayHeight = Display::getHeight();

        if (displayWidth != i || displayHeight != j) {
            if (displayWidth <= 0)
                displayWidth = 1;
            if (displayHeight <= 0)
                displayHeight = 1;
            resize(displayWidth, displayHeight);
        }
    }
}

int Minecraft::getLimitFramerate() const
{
    return theWorld == nullptr && currentScreen != nullptr ? 30 : gameSettings->limitFramerate;
}

bool Minecraft::isFramerateLimitBelowMax() const
{
    return static_cast<float>(getLimitFramerate()) < GameSettings::Options::FRAMERATE_LIMIT.getValueMax();
}

void Minecraft::toggleFullscreen()
{
    // Java switched LWJGL display modes and restored tempDisplayWidth x tempDisplayHeight on exit.
    // Borderless fullscreen restores the previous window placement instead.
    fullscreen = !fullscreen;
    gameSettings->fullScreen = fullscreen;

    Display::setFullscreen(fullscreen);
    displayWidth = Display::getWidth();
    displayHeight = Display::getHeight();
    if (displayWidth <= 0)
        displayWidth = 1;
    if (displayHeight <= 0)
        displayHeight = 1;

    if (currentScreen != nullptr) {
        resize(displayWidth, displayHeight);
    } else {
        updateFramebufferSize();
    }

    Display::setVSyncEnabled(gameSettings->enableVsync);
}

void Minecraft::resize(int width, int height)
{
    displayWidth = std::max(1, width);
    displayHeight = std::max(1, height);

    if (const std::shared_ptr<GuiScreen> screen = currentScreen) {
        const ScaledResolution scaledresolution(*this);
        screen->onResize(*this, scaledresolution.getScaledWidth(), scaledresolution.getScaledHeight());
    }
    // Java: loadingScreen = new LoadingScreenRenderer(this) (stage 5)
    updateFramebufferSize();
}

void Minecraft::updateFramebufferSize()
{
    framebufferMc->createBindFramebuffer(displayWidth, displayHeight);
    // Java: entityRenderer.updateShaderGroupSize(displayWidth, displayHeight) (stage 15).
}

void Minecraft::shutdown()
{
    running = false;
}

void Minecraft::shutdownMinecraftApplet()
{
    Log::info("Stopping!");
    if (mcSoundHandler)
        mcSoundHandler->unloadSounds();

    // GL objects go before the context, like the JVM releasing them before Display.destroy().
    currentScreen.reset();
    mcResourcePackRepository.reset();
    entityRenderer.reset();
    fontRendererObj.reset();
    standardGalacticFontRenderer.reset();
    renderEngine.reset();
    framebufferMc.reset();
    mcMusicTicker.reset();
    mcSoundHandler.reset();
    mcResourceManager.reset();
    if (Display::isCreated())
        GL::shutdown();
    Display::destroy();
}

void Minecraft::displayCrashReport(const std::string& description, const std::exception& cause)
{
    // Java: CrashReport with system details saved to crash-reports/ (stage 15).
    Log::fatal("{}: {}", description, cause.what());
    const std::wstring text = toWide(std::format("{}\n\n{}", description, cause.what()));
    MessageBoxW(nullptr, text.c_str(), L"Minecraft has crashed!", MB_OK | MB_ICONERROR);
}

void Minecraft::addServerStatsToSnooper(PlayerUsageSnooper& playerSnooper)
{
    playerSnooper.addClientStat("fps", std::to_string(debugFPS));
    playerSnooper.addClientStat("vsync_enabled", gameSettings->enableVsync ? "true" : "false");
    playerSnooper.addClientStat("display_frequency", std::to_string(Display::getDisplayMode().frequency));
    playerSnooper.addClientStat("display_type", fullscreen ? "fullscreen" : "windowed");
    playerSnooper.addClientStat("run_time", std::to_string((currentTimeMillis() - playerSnooper.getMinecraftStartTimeMillis()) / 60LL * 1000LL));
    // Java: getCurrentAction(); the idle integrated server always exists, so the menu reports "singleplayer".
    playerSnooper.addClientStat("current_action", "singleplayer");
    playerSnooper.addClientStat("endianness", "little");
    const auto entries = mcResourcePackRepository ? mcResourcePackRepository->getRepositoryEntries() : std::vector<std::shared_ptr<ResourcePackRepository::Entry>> {};
    playerSnooper.addClientStat("resource_packs", std::to_string(entries.size()));
    int i = 0;
    for (const auto& entry : entries)
        playerSnooper.addClientStat("resource_pack[" + std::to_string(i++) + "]", entry->getResourcePackName());
    playerSnooper.addClientStat("snooper_partner", integratedServerSnooper->getUniqueID());
}

void Minecraft::addServerTypeToSnooper(PlayerUsageSnooper& playerSnooper)
{
    playerSnooper.addStatToSnooper("opengl_version", GL::glGetString(GL::GL_VERSION));
    playerSnooper.addStatToSnooper("opengl_vendor", GL::glGetString(GL::GL_VENDOR));
    playerSnooper.addStatToSnooper("client_brand", "vanilla");
    playerSnooper.addStatToSnooper("launched_version", launchedVersion);
    // Java also lists ~150 gl_caps[...] OpenGL extension flags, which have no Direct3D meaning.
    playerSnooper.addStatToSnooper("gl_max_texture_size", std::to_string(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
}

} // namespace mc
