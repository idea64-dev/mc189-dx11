#include "client/gui/GuiMainMenu.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiErrorScreen.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/texture/SimpleTexture.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/resources/Locale.h"
#include "client/shader/Framebuffer.h"
#include "core/Java.h"
#include "render/gl/GL.h"
#include "render/gl/Project.h"
#include "util/MathHelper.h"
#include "util/Random.h"

#include <array>
#include <ctime>
#include <numbers>

namespace mc {

namespace {

// Java: a static Random shared by all main menus.
Random& menuRandom()
{
    static Random random;
    return random;
}

const ResourceLocation splashTexts("texts/splashes.txt");
const ResourceLocation minecraftTitleTextures("textures/gui/title/minecraft.png");
const std::array<ResourceLocation, 6> titlePanoramaPaths {
    ResourceLocation("textures/gui/title/background/panorama_0.png"), ResourceLocation("textures/gui/title/background/panorama_1.png"),
    ResourceLocation("textures/gui/title/background/panorama_2.png"), ResourceLocation("textures/gui/title/background/panorama_3.png"),
    ResourceLocation("textures/gui/title/background/panorama_4.png"), ResourceLocation("textures/gui/title/background/panorama_5.png")
};

// String.trim(): strips characters <= ' ' from both ends.
std::string_view trimJava(std::string_view s)
{
    while (!s.empty() && static_cast<unsigned char>(s.front()) <= ' ')
        s.remove_prefix(1);
    while (!s.empty() && static_cast<unsigned char>(s.back()) <= ' ')
        s.remove_suffix(1);
    return s;
}

} // namespace

GuiMainMenu::GuiMainMenu()
    : splashText("missingno")
{
    try {
        const std::shared_ptr<IResource> resource = Minecraft::getMinecraft()->getResourceManager().getResource(splashTexts);
        const auto& data = resource->getInputStream();
        const std::string_view contents(reinterpret_cast<const char*>(data.data()), data.size());

        std::vector<std::string> list;
        size_t start = 0;
        while (start < contents.size()) {
            size_t end = contents.find_first_of("\r\n", start);
            const std::string_view line = trimJava(contents.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
            if (!line.empty())
                list.emplace_back(line);
            if (end == std::string_view::npos)
                break;
            start = end + ((contents[end] == '\r' && end + 1 < contents.size() && contents[end + 1] == '\n') ? 2 : 1);
        }

        if (!list.empty()) {
            // 125780783 is "This message will never appear on the splash screen, isn't that weird?".hashCode()
            do {
                splashText = list[static_cast<size_t>(menuRandom().nextInt(static_cast<int32_t>(list.size())))];
            } while (java::hashCode(splashText) == 125780783);
        }
    } catch (const std::exception&) {
        // Java: IOException ignored, "missingno" stays
    }

    updateCounter = menuRandom().nextFloat();
}

void GuiMainMenu::updateScreen()
{
    ++panoramaTimer;
}

void GuiMainMenu::keyTyped(char16_t /*typedChar*/, int /*keyCode*/)
{
}

void GuiMainMenu::initGui()
{
    viewportTexture = std::make_shared<DynamicTexture>(256, 256);
    backgroundTexture = mc->getTextureManager().getDynamicTextureLocation("background", viewportTexture);

    const std::time_t now = std::time(nullptr);
    std::tm local {};
    localtime_s(&local, &now);
    const int month = local.tm_mon + 1;
    const int day = local.tm_mday;
    if (month == 12 && day == 24)
        splashText = "Merry X-mas!";
    else if (month == 1 && day == 1)
        splashText = "Happy new year!";
    else if (month == 10 && day == 31)
        splashText = "OOoooOOOoooo! Spooky!";

    const int j = height / 4 + 48;
    addSingleplayerMultiplayerButtons(j, 24);
    addButton<GuiButton>(0, width / 2 - 100, j + 72 + 12, 98, 20, I18n::format("menu.options"));
    addButton<GuiButton>(4, width / 2 + 2, j + 72 + 12, 98, 20, I18n::format("menu.quit"));
    addButton<GuiButtonLanguage>(5, width / 2 - 124, j + 72 + 12);

    // Java: mc.setConnectedToRealms(false) and the Realms notification screen.
}

void GuiMainMenu::addSingleplayerMultiplayerButtons(int y, int spacing)
{
    addButton<GuiButton>(1, width / 2 - 100, y, I18n::format("menu.singleplayer"));
    addButton<GuiButton>(2, width / 2 - 100, y + spacing * 1, I18n::format("menu.multiplayer"));
    realmsButton = &addButton<GuiButton>(14, width / 2 - 100, y + spacing * 2, I18n::format("menu.online"));
}

void GuiMainMenu::actionPerformed(GuiButton& button)
{
    if (button.id == 0)
        mc->displayGuiScreen(GuiErrorScreen::notPortedYet(I18n::format("options.title"), "stage 3"));
    if (button.id == 5)
        mc->displayGuiScreen(GuiErrorScreen::notPortedYet(I18n::format("options.language"), "stage 3"));
    if (button.id == 1)
        mc->displayGuiScreen(GuiErrorScreen::notPortedYet(I18n::format("menu.singleplayer"), "stage 5"));
    if (button.id == 2)
        mc->displayGuiScreen(GuiErrorScreen::notPortedYet(I18n::format("menu.multiplayer"), "stage 14"));
    if (button.id == 14 && realmsButton->visible)
        mc->displayGuiScreen(GuiErrorScreen::notPortedYet(I18n::format("menu.online"), "out of scope"));
    if (button.id == 4)
        mc->shutdown();
}

void GuiMainMenu::confirmClicked(bool /*result*/, int /*id*/)
{
    // Java: deleting the demo world (12) and the OpenGL help link (13); neither exists in this port.
}

void GuiMainMenu::drawPanorama(int /*mouseX*/, int /*mouseY*/, float partialTicks)
{
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    DxStateManager::matrixMode(5889);
    DxStateManager::pushMatrix();
    DxStateManager::loadIdentity();
    Project::gluPerspective(120.0f, 1.0f, 0.05f, 10.0f);
    DxStateManager::matrixMode(5888);
    DxStateManager::pushMatrix();
    DxStateManager::loadIdentity();
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    DxStateManager::rotate(180.0f, 1.0f, 0.0f, 0.0f);
    DxStateManager::rotate(90.0f, 0.0f, 0.0f, 1.0f);
    DxStateManager::enableBlend();
    DxStateManager::disableAlpha();
    DxStateManager::disableCull();
    DxStateManager::depthMask(false);
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    const int i = 8;

    for (int j = 0; j < i * i; ++j) {
        DxStateManager::pushMatrix();
        const float f = (static_cast<float>(j % i) / static_cast<float>(i) - 0.5f) / 64.0f;
        const float f1 = (static_cast<float>(j / i) / static_cast<float>(i) - 0.5f) / 64.0f;
        const float f2 = 0.0f;
        DxStateManager::translate(f, f1, f2);
        DxStateManager::rotate(MathHelper::sin((static_cast<float>(panoramaTimer) + partialTicks) / 400.0f) * 25.0f + 20.0f, 1.0f, 0.0f, 0.0f);
        DxStateManager::rotate(-(static_cast<float>(panoramaTimer) + partialTicks) * 0.1f, 0.0f, 1.0f, 0.0f);

        for (int k = 0; k < 6; ++k) {
            DxStateManager::pushMatrix();
            if (k == 1)
                DxStateManager::rotate(90.0f, 0.0f, 1.0f, 0.0f);
            if (k == 2)
                DxStateManager::rotate(180.0f, 0.0f, 1.0f, 0.0f);
            if (k == 3)
                DxStateManager::rotate(-90.0f, 0.0f, 1.0f, 0.0f);
            if (k == 4)
                DxStateManager::rotate(90.0f, 1.0f, 0.0f, 0.0f);
            if (k == 5)
                DxStateManager::rotate(-90.0f, 1.0f, 0.0f, 0.0f);

            mc->getTextureManager().bindTexture(titlePanoramaPaths[static_cast<size_t>(k)]);
            worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
            const int l = 255 / (j + 1);
            worldrenderer.pos(-1.0, -1.0, 1.0).tex(0.0, 0.0).color(255, 255, 255, l).endVertex();
            worldrenderer.pos(1.0, -1.0, 1.0).tex(1.0, 0.0).color(255, 255, 255, l).endVertex();
            worldrenderer.pos(1.0, 1.0, 1.0).tex(1.0, 1.0).color(255, 255, 255, l).endVertex();
            worldrenderer.pos(-1.0, 1.0, 1.0).tex(0.0, 1.0).color(255, 255, 255, l).endVertex();
            tessellator.draw();
            DxStateManager::popMatrix();
        }

        DxStateManager::popMatrix();
        DxStateManager::colorMask(true, true, true, false);
    }

    worldrenderer.setTranslation(0.0, 0.0, 0.0);
    DxStateManager::colorMask(true, true, true, true);
    DxStateManager::matrixMode(5889);
    DxStateManager::popMatrix();
    DxStateManager::matrixMode(5888);
    DxStateManager::popMatrix();
    DxStateManager::depthMask(true);
    DxStateManager::enableCull();
    DxStateManager::enableDepth();
}

void GuiMainMenu::rotateAndBlurSkybox(float /*partialTicks*/)
{
    mc->getTextureManager().bindTexture(*backgroundTexture);
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MIN_FILTER, GL::GL_LINEAR);
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MAG_FILTER, GL::GL_LINEAR);
    GL::glCopyTexSubImage2D(GL::GL_TEXTURE_2D, 0, 0, 0, 0, 0, 256, 256);
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::colorMask(true, true, true, false);
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    DxStateManager::disableAlpha();
    const int i = 3;

    for (int j = 0; j < i; ++j) {
        const float f = 1.0f / static_cast<float>(j + 1);
        const int k = width;
        const int l = height;
        const float f1 = static_cast<float>(j - i / 2) / 256.0f;
        worldrenderer.pos(k, l, zLevel).tex(0.0f + f1, 1.0).color(1.0f, 1.0f, 1.0f, f).endVertex();
        worldrenderer.pos(k, 0.0, zLevel).tex(1.0f + f1, 1.0).color(1.0f, 1.0f, 1.0f, f).endVertex();
        worldrenderer.pos(0.0, 0.0, zLevel).tex(1.0f + f1, 0.0).color(1.0f, 1.0f, 1.0f, f).endVertex();
        worldrenderer.pos(0.0, l, zLevel).tex(0.0f + f1, 0.0).color(1.0f, 1.0f, 1.0f, f).endVertex();
    }

    tessellator.draw();
    DxStateManager::enableAlpha();
    DxStateManager::colorMask(true, true, true, true);
}

void GuiMainMenu::renderSkybox(int mouseX, int mouseY, float partialTicks)
{
    mc->getFramebuffer().unbindFramebuffer();
    DxStateManager::viewport(0, 0, 256, 256);
    drawPanorama(mouseX, mouseY, partialTicks);
    for (int pass = 0; pass < 7; ++pass)
        rotateAndBlurSkybox(partialTicks);
    mc->getFramebuffer().bindFramebuffer(true);
    DxStateManager::viewport(0, 0, mc->displayWidth, mc->displayHeight);

    const float f = width > height ? 120.0f / static_cast<float>(width) : 120.0f / static_cast<float>(height);
    const float f1 = static_cast<float>(height) * f / 256.0f;
    const float f2 = static_cast<float>(width) * f / 256.0f;
    const int i = width;
    const int j = height;
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(0.0, j, zLevel).tex(0.5f - f1, 0.5f + f2).color(1.0f, 1.0f, 1.0f, 1.0f).endVertex();
    worldrenderer.pos(i, j, zLevel).tex(0.5f - f1, 0.5f - f2).color(1.0f, 1.0f, 1.0f, 1.0f).endVertex();
    worldrenderer.pos(i, 0.0, zLevel).tex(0.5f + f1, 0.5f - f2).color(1.0f, 1.0f, 1.0f, 1.0f).endVertex();
    worldrenderer.pos(0.0, 0.0, zLevel).tex(0.5f + f1, 0.5f + f2).color(1.0f, 1.0f, 1.0f, 1.0f).endVertex();
    tessellator.draw();
}

void GuiMainMenu::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    DxStateManager::disableAlpha();
    renderSkybox(mouseX, mouseY, partialTicks);
    DxStateManager::enableAlpha();
    const int i = 274;
    const int j = width / 2 - i / 2;
    const int k = 30;
    drawGradientRect(0, 0, width, height, -2130706433, 16777215);
    drawGradientRect(0, 0, width, height, 0, INT32_MIN);
    mc->getTextureManager().bindTexture(minecraftTitleTextures);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);

    if (static_cast<double>(updateCounter) < 1.0E-4) {
        // "Minceraft"
        drawTexturedModalRect(j + 0, k + 0, 0, 0, 99, 44);
        drawTexturedModalRect(j + 99, k + 0, 129, 0, 27, 44);
        drawTexturedModalRect(j + 99 + 26, k + 0, 126, 0, 3, 44);
        drawTexturedModalRect(j + 99 + 26 + 3, k + 0, 99, 0, 26, 44);
        drawTexturedModalRect(j + 155, k + 0, 0, 45, 155, 44);
    } else {
        drawTexturedModalRect(j + 0, k + 0, 0, 0, 155, 44);
        drawTexturedModalRect(j + 155, k + 0, 0, 45, 155, 44);
    }

    DxStateManager::pushMatrix();
    DxStateManager::translate(static_cast<float>(width / 2 + 90), 70.0f, 0.0f);
    DxStateManager::rotate(-20.0f, 0.0f, 0.0f, 1.0f);
    float f = 1.8f - MathHelper::abs(MathHelper::sin(static_cast<float>(Minecraft::getSystemTime() % 1000) / 1000.0f * static_cast<float>(std::numbers::pi) * 2.0f) * 0.1f);
    f = f * 100.0f / static_cast<float>(fontRendererObj->getStringWidth(splashText) + 32);
    DxStateManager::scale(f, f, f);
    drawCenteredString(*fontRendererObj, splashText, 0, -8, -256);
    DxStateManager::popMatrix();

    const std::string s = "Minecraft 1.8.9";
    drawString(*fontRendererObj, s, 2, height - 10, -1);
    const std::string s1 = "Copyright Mojang AB. Do not distribute!";
    drawString(*fontRendererObj, s1, width - fontRendererObj->getStringWidth(s1) - 2, height - 10, -1);

    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
