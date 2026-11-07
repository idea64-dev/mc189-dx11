#include "client/Minecraft.h"

#include "client/renderer/texture/TextureUtil.h"
#include "client/resources/DefaultResourcePack.h"
#include "client/resources/ResourceIndex.h"
#include "core/Log.h"
#include "core/ZipArchive.h"
#include "core/StringUtil.h"
#include "platform/Display.h"
#include "platform/Keyboard.h"
#include "platform/Mouse.h"
#include "platform/Sys.h"
#include "render/RenderDevice.h"

#include <Windows.h>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <thread>

namespace mc {

Minecraft* Minecraft::theMinecraft = nullptr;
int Minecraft::debugFPS = 0;

Minecraft::Minecraft(const GameConfiguration& gameConfig)
    : displayWidth(gameConfig.displayInfo.width > 0 ? gameConfig.displayInfo.width : 1)
    , displayHeight(gameConfig.displayInfo.height > 0 ? gameConfig.displayInfo.height : 1)
    , fullscreen(gameConfig.displayInfo.fullscreen)
    , tempDisplayWidth(gameConfig.displayInfo.width)
    , tempDisplayHeight(gameConfig.displayInfo.height)
    , mcDataDir(gameConfig.folderInfo.mcDataDir)
    , launchedVersion(gameConfig.gameInfo.version)
    , debugUpdateTime(getSystemTime())
    , startNanoTime(Sys::nanoTime())
{
    theMinecraft = this;
    mcDefaultResourcePack = std::make_unique<DefaultResourcePack>(
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
    // Java: new GameSettings(this, mcDataDir) loads options.txt here (stage 2.2).
    // Java: startTimerHackThread() raised the Windows timer resolution; Display::sync uses a high-resolution timer instead.

    if (gameSettings.overrideHeight > 0 && gameSettings.overrideWidth > 0) {
        displayWidth = gameSettings.overrideWidth;
        displayHeight = gameSettings.overrideHeight;
    }

    Log::info("Minecraft {} (C++ / Direct3D 11 port)", launchedVersion);
    setWindowIcon();
    setInitialDisplayMode();
    createDisplay();

    // The window size is DPI-scaled by Display, so take the real client size.
    displayWidth = Display::getWidth();
    displayHeight = Display::getHeight();
    Display::setVSyncEnabled(gameSettings.enableVsync);

    if (gameSettings.fullScreen && !fullscreen)
        toggleFullscreen();
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
}

void Minecraft::runGameLoop()
{
    if (Display::isCreated() && Display::isCloseRequested())
        shutdown();

    if (isGamePaused && theWorld != nullptr) {
        const float f = timer.renderPartialTicks;
        timer.updateTimer();
        timer.renderPartialTicks = f;
    } else {
        timer.updateTimer();
    }

    for (int j = 0; j < timer.elapsedTicks; ++j)
        runTick();

    // Java: GlStateManager.clear(16640) + framebufferMc + entityRenderer.updateCameraAndRender (stage 2+).
    // Placeholder sky colour until there is something to draw.
    RenderDevice& device = Display::getDevice();
    device.bindBackBuffer();
    device.clear(0.47f, 0.65f, 1.0f, 1.0f);

    updateDisplay();
    std::this_thread::yield();

    ++fpsCounter;
    const int64_t k = Sys::nanoTime();
    startNanoTime = k;

    while (getSystemTime() >= debugUpdateTime + 1000LL) {
        debugFPS = fpsCounter;
        const bool unlimited = static_cast<float>(gameSettings.limitFramerate) == GameSettings::FRAMERATE_LIMIT_MAX;
        debug = std::format("{} fps ({} chunk update{}) T: {}{}{}{}",
            debugFPS, 0, "s",
            unlimited ? std::string("inf") : std::to_string(gameSettings.limitFramerate),
            gameSettings.enableVsync ? " vsync" : "",
            gameSettings.fancyGraphics ? "" : " fast",
            gameSettings.clouds == 0 ? "" : (gameSettings.clouds == 1 ? " fast-clouds" : " fancy-clouds"));
        debugUpdateTime += 1000LL;
        fpsCounter = 0;

        // Stage 1 only: no F3 screen yet, so show the debug line in the title.
        Display::setTitle(std::format("Minecraft 1.8.9 - {}", debug));
        if (debugMouseDX != 0 || debugMouseDY != 0) {
            Log::info("[input test] grabbed mouse moved dx={} dy={}", debugMouseDX, debugMouseDY);
            debugMouseDX = 0;
            debugMouseDY = 0;
        }
    }

    if (isFramerateLimitBelowMax())
        Display::sync(getLimitFramerate());
}

void Minecraft::runTick()
{
    // Java: with no screen open, mouse and keyboard events drive the player and key bindings.
    debugInputTick();

    while (Keyboard::next()) {
        // KeyBinding::setKeyBindState / onTick arrive with stage 2.2.
        dispatchKeypresses();
    }
}

void Minecraft::debugInputTick()
{
    while (Mouse::next()) {
        const int button = Mouse::getEventButton();
        if (button >= 0) {
            Log::info("[input test] mouse button {} {} at ({}, {})", button,
                Mouse::getEventButtonState() ? "pressed" : "released", Mouse::getEventX(), Mouse::getEventY());
            if (button == 0 && Mouse::getEventButtonState() && !Mouse::isGrabbed()) {
                Mouse::setGrabbed(true);
                Log::info("[input test] mouse grabbed, Escape releases it");
            }
        } else if (Mouse::getEventDWheel() != 0) {
            Log::info("[input test] mouse wheel {}", Mouse::getEventDWheel());
        }
    }

    if (Mouse::isGrabbed()) {
        debugMouseDX += Mouse::getDX();
        debugMouseDY += Mouse::getDY();
    }
    // Key events are logged from dispatchKeypresses(), which sees every one of them.
}

void Minecraft::dispatchKeypresses()
{
    const int i = Keyboard::getEventKey() == 0 ? Keyboard::getEventCharacter() : Keyboard::getEventKey();

    // Stage 1 input check: log every key event in LWJGL codes.
    if (Keyboard::getEventKeyState()) {
        const char16_t character = Keyboard::getEventCharacter();
        Log::info("[input test] key {} ({}){}{}", Keyboard::getKeyName(Keyboard::getEventKey()), Keyboard::getEventKey(),
            character >= 32 ? std::format(" char '{}'", toUtf8(std::wstring(1, static_cast<wchar_t>(character)))) : std::string(),
            Keyboard::isRepeatEvent() ? " (repeat)" : "");
        if (Keyboard::getEventKey() == Keyboard::KEY_ESCAPE && Mouse::isGrabbed()) {
            Mouse::setGrabbed(false);
            Log::info("[input test] mouse released");
        }
    }

    if (i != 0 && !Keyboard::isRepeatEvent()) {
        if (Keyboard::getEventKeyState()) {
            if (i == gameSettings.keyBindFullscreen) {
                toggleFullscreen();
            }
            // keyBindScreenshot: stage 3. Twitch stream keys are not ported.
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
    return theWorld == nullptr && currentScreen != nullptr ? 30 : gameSettings.limitFramerate;
}

bool Minecraft::isFramerateLimitBelowMax() const
{
    return static_cast<float>(getLimitFramerate()) < GameSettings::FRAMERATE_LIMIT_MAX;
}

void Minecraft::toggleFullscreen()
{
    // Java switched LWJGL display modes and restored tempDisplayWidth x tempDisplayHeight on exit.
    // Borderless fullscreen restores the previous window placement instead.
    fullscreen = !fullscreen;
    gameSettings.fullScreen = fullscreen;

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

    Display::setVSyncEnabled(gameSettings.enableVsync);
    Log::info("Fullscreen {} ({}x{})", fullscreen ? "on" : "off", displayWidth, displayHeight);
}

void Minecraft::resize(int width, int height)
{
    displayWidth = std::max(1, width);
    displayHeight = std::max(1, height);
    // Java: currentScreen->onResize(ScaledResolution), new LoadingScreenRenderer (stage 2).
    updateFramebufferSize();
}

void Minecraft::updateFramebufferSize()
{
    // Java: framebufferMc.createBindFramebuffer(displayWidth, displayHeight) and the shader group (stage 2+).
    // The swapchain itself is already resized by Display.
}

void Minecraft::shutdown()
{
    running = false;
}

void Minecraft::shutdownMinecraftApplet()
{
    Log::info("Stopping!");
    Display::destroy();
}

void Minecraft::displayCrashReport(const std::string& description, const std::exception& cause)
{
    // Java: CrashReport with system details saved to crash-reports/ (stage 15).
    Log::fatal("{}: {}", description, cause.what());
    const std::wstring text = toWide(std::format("{}\n\n{}", description, cause.what()));
    MessageBoxW(nullptr, text.c_str(), L"Minecraft has crashed!", MB_OK | MB_ICONERROR);
}

} // namespace mc
