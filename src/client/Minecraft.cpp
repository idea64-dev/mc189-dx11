#include "client/Minecraft.h"

#include "client/gui/ScaledResolution.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/texture/SimpleTexture.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/resources/DefaultResourcePack.h"
#include "client/resources/LanguageManager.h"
#include "client/resources/ResourceIndex.h"
#include "client/resources/SimpleReloadableResourceManager.h"
#include "client/shader/Framebuffer.h"
#include "render/gl/GL.h"
#include "util/ScreenShotHelper.h"
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

namespace {

const ResourceLocation locationMojangPng("textures/gui/title/mojang.png");

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
    , launchedVersion(gameConfig.gameInfo.version)
    , debugUpdateTime(getSystemTime())
    , startNanoTime(Sys::nanoTime())
{
    theMinecraft = this;
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
    Display::setVSyncEnabled(gameSettings->enableVsync);

    framebufferMc = std::make_unique<Framebuffer>(displayWidth, displayHeight, true);
    framebufferMc->setFramebufferColor(0.0f, 0.0f, 0.0f, 0.0f);
    defaultResourcePacks.push_back(mcDefaultResourcePack);
    mcResourceManager = std::make_unique<SimpleReloadableResourceManager>();
    mcLanguageManager = std::make_unique<LanguageManager>(gameSettings->language);
    mcResourceManager->registerReloadListener(*mcLanguageManager);
    refreshResources();
    renderEngine = std::make_unique<TextureManager>(*mcResourceManager);
    mcResourceManager->registerReloadListener(*renderEngine);
    drawSplashScreen(*renderEngine);

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
    DxStateManager::viewport(0, 0, displayWidth, displayHeight);

    if (gameSettings->fullScreen && !fullscreen)
        toggleFullscreen();
}

void Minecraft::refreshResources()
{
    // Java also adds the resource packs selected in the repository (stage 3).
    std::vector<std::shared_ptr<IResourcePack>> list = defaultResourcePacks;
    try {
        mcResourceManager->reloadResources(list);
    } catch (const std::exception& e) {
        Log::info("Caught error stitching, removing all assigned resourcepacks ({})", e.what());
        list = defaultResourcePacks;
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

    DxStateManager::pushMatrix();
    DxStateManager::clear(16640);
    framebufferMc->bindFramebuffer(true);
    DxStateManager::enableTexture2D();
    renderStage2Preview(); // Java: entityRenderer.updateCameraAndRender(timer.renderPartialTicks, i)
    framebufferMc->unbindFramebuffer();
    DxStateManager::popMatrix();
    DxStateManager::pushMatrix();
    framebufferMc->framebufferRender(displayWidth, displayHeight);
    DxStateManager::popMatrix();

    updateDisplay();
    std::this_thread::yield();

    ++fpsCounter;
    const int64_t k = Sys::nanoTime();
    startNanoTime = k;

    while (getSystemTime() >= debugUpdateTime + 1000LL) {
        debugFPS = fpsCounter;
        const bool unlimited = static_cast<float>(gameSettings->limitFramerate) == GameSettings::Options::FRAMERATE_LIMIT.getValueMax();
        debug = std::format("{} fps ({} chunk update{}) T: {}{}{}{}",
            debugFPS, 0, "s",
            unlimited ? std::string("inf") : std::to_string(gameSettings->limitFramerate),
            gameSettings->enableVsync ? " vsync" : "",
            gameSettings->fancyGraphics ? "" : " fast",
            gameSettings->clouds == 0 ? "" : (gameSettings->clouds == 1 ? " fast-clouds" : " fancy-clouds"));
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
        const int k = Keyboard::getEventKey() == 0 ? Keyboard::getEventCharacter() + 256 : Keyboard::getEventKey();
        KeyBinding::setKeyBindState(k, Keyboard::getEventKeyState());
        if (Keyboard::getEventKeyState())
            KeyBinding::onTick(k);
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
    framebufferMc->createBindFramebuffer(displayWidth, displayHeight);
    // Java: entityRenderer.updateShaderGroupSize(displayWidth, displayHeight) (stage 15).
}

void Minecraft::renderStage2Preview()
{
    // Temporary: exercises the render core (textures, blending, alpha test, matrices, gradients) the way the
    // GUI will. Replaced by the real screen rendering in stage 2.4/2.5.
    const ScaledResolution scaledresolution(*this);
    const int width = scaledresolution.getScaledWidth();
    const int height = scaledresolution.getScaledHeight();

    // EntityRenderer.setupOverlayRendering
    DxStateManager::clear(256);
    DxStateManager::matrixMode(5889);
    DxStateManager::loadIdentity();
    DxStateManager::ortho(0.0, scaledresolution.getScaledWidth_double(), scaledresolution.getScaledHeight_double(), 0.0, 1000.0, 3000.0);
    DxStateManager::matrixMode(5888);
    DxStateManager::loadIdentity();
    DxStateManager::translate(0.0f, 0.0f, -2000.0f);

    // GuiScreen.drawBackground(0)
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    DxStateManager::disableLighting();
    DxStateManager::disableFog();
    renderEngine->bindTexture(ResourceLocation("textures/gui/options_background.png"));
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    const float f = 32.0f;
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(0.0, height, 0.0).tex(0.0, static_cast<float>(height) / f).color(64, 64, 64, 255).endVertex();
    worldrenderer.pos(width, height, 0.0).tex(static_cast<float>(width) / f, static_cast<float>(height) / f).color(64, 64, 64, 255).endVertex();
    worldrenderer.pos(width, 0.0, 0.0).tex(static_cast<float>(width) / f, 0.0).color(64, 64, 64, 255).endVertex();
    worldrenderer.pos(0.0, 0.0, 0.0).tex(0.0, 0.0).color(64, 64, 64, 255).endVertex();
    tessellator.draw();

    // Gui.drawGradientRect: translucent top-to-bottom gradient band
    DxStateManager::disableTexture2D();
    DxStateManager::enableBlend();
    DxStateManager::disableAlpha();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::shadeModel(7425);
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_COLOR);
    worldrenderer.pos(width, 10.0, 0.0).color(0x10, 0x10, 0x10, 0xC0).endVertex();
    worldrenderer.pos(0.0, 10.0, 0.0).color(0x10, 0x10, 0x10, 0xC0).endVertex();
    worldrenderer.pos(0.0, 40.0, 0.0).color(0x60, 0x20, 0xA0, 0x50).endVertex();
    worldrenderer.pos(width, 40.0, 0.0).color(0x60, 0x20, 0xA0, 0x50).endVertex();
    tessellator.draw();
    DxStateManager::shadeModel(7424);
    DxStateManager::enableAlpha();
    DxStateManager::enableTexture2D();

    // The Mojang logo, scaled down and spinning, blended over the background
    renderEngine->bindTexture(mojangLogo ? *mojangLogo : ResourceLocation("textures/gui/title/mojang.png"));
    DxStateManager::pushMatrix();
    DxStateManager::translate(static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f + 20.0f, 0.0f);
    DxStateManager::rotate(static_cast<float>(getSystemTime() % 7200) / 20.0f, 0.0f, 0.0f, 1.0f);
    DxStateManager::scale(0.5f, 0.5f, 0.5f);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 0.75f);
    draw(-128, -128, 0, 0, 256, 256, 255, 255, 255, 191);
    DxStateManager::popMatrix();
    DxStateManager::disableBlend();

    // A button from widgets.png at its native 200x20 size: must be pixel-exact against the original
    renderEngine->bindTexture(ResourceLocation("textures/gui/widgets.png"));
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    draw(width / 2 - 100, height - 30, 0, 66, 200, 20, 255, 255, 255, 255);
}

void Minecraft::shutdown()
{
    running = false;
}

void Minecraft::shutdownMinecraftApplet()
{
    Log::info("Stopping!");
    // GL objects go before the context, like the JVM releasing them before Display.destroy().
    renderEngine.reset();
    framebufferMc.reset();
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

} // namespace mc
