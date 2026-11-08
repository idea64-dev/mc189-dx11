#include "client/gui/GuiVideoSettings.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/ScaledResolution.h"
#include "client/renderer/OpenGlHelper.h"
#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"

#include <array>
#include <vector>

namespace mc {

namespace {

using Options = GameSettings::Options;

const std::array<const Options*, 16> videoOptions { &Options::GRAPHICS, &Options::RENDER_DISTANCE, &Options::AMBIENT_OCCLUSION,
    &Options::FRAMERATE_LIMIT, &Options::ANAGLYPH, &Options::VIEW_BOBBING, &Options::GUI_SCALE, &Options::GAMMA,
    &Options::RENDER_CLOUDS, &Options::PARTICLES, &Options::USE_FULLSCREEN, &Options::ENABLE_VSYNC, &Options::MIPMAP_LEVELS,
    &Options::BLOCK_ALTERNATIVES, &Options::USE_VBO, &Options::ENTITY_SHADOWS };

} // namespace

GuiVideoSettings::GuiVideoSettings(std::shared_ptr<GuiScreen> parentScreenIn, GameSettings& gameSettingsIn)
    : parentGuiScreen(std::move(parentScreenIn))
    , guiGameSettings(gameSettingsIn)
{
}

void GuiVideoSettings::initGui()
{
    screenTitle = I18n::format("options.videoTitle");
    buttonList.clear();
    addButton<GuiButton>(200, width / 2 - 100, height - 27, I18n::format("gui.done"));

    if (!OpenGlHelper::vboSupported) {
        // Java copies the options up to USE_VBO into an array one shorter, leaving the last slot null.
        std::vector<const Options*> options(videoOptions.size() - 1, nullptr);
        size_t i = 0;
        for (const Options* option : videoOptions) {
            if (option == &Options::USE_VBO)
                break;
            options[i++] = option;
        }
        optionsRowList = std::make_shared<GuiOptionsRowList>(*mc, width, height, 32, height - 32, 25, options);
    } else {
        optionsRowList = std::make_shared<GuiOptionsRowList>(*mc, width, height, 32, height - 32, 25, videoOptions);
    }
}

void GuiVideoSettings::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    const std::shared_ptr<GuiOptionsRowList> list = optionsRowList;
    list->handleMouseInput();
}

void GuiVideoSettings::actionPerformed(GuiButton& button)
{
    if (button.enabled && button.id == 200) {
        mc->gameSettings->saveOptions();
        mc->displayGuiScreen(parentGuiScreen);
    }
}

void GuiVideoSettings::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    const int i = guiGameSettings.guiScale;
    GuiScreen::mouseClicked(mouseX, mouseY, mouseButton);
    const std::shared_ptr<GuiOptionsRowList> list = optionsRowList;
    list->mouseClicked(mouseX, mouseY, mouseButton);

    if (guiGameSettings.guiScale != i) {
        const ScaledResolution scaledresolution(*mc);
        setWorldAndResolution(*mc, scaledresolution.getScaledWidth(), scaledresolution.getScaledHeight());
    }
}

void GuiVideoSettings::mouseReleased(int mouseX, int mouseY, int state)
{
    const int i = guiGameSettings.guiScale;
    GuiScreen::mouseReleased(mouseX, mouseY, state);
    const std::shared_ptr<GuiOptionsRowList> list = optionsRowList;
    list->mouseReleased(mouseX, mouseY, state);

    if (guiGameSettings.guiScale != i) {
        const ScaledResolution scaledresolution(*mc);
        setWorldAndResolution(*mc, scaledresolution.getScaledWidth(), scaledresolution.getScaledHeight());
    }
}

void GuiVideoSettings::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    optionsRowList->drawScreen(mouseX, mouseY, partialTicks);
    drawCenteredString(*fontRendererObj, screenTitle, width / 2, 5, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
