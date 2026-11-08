#include "client/gui/GuiCreateWorld.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiCustomizeWorldScreen.h"
#include "client/gui/GuiErrorScreen.h"
#include "client/gui/GuiTextField.h"
#include "client/resources/Locale.h"
#include "core/Java.h"
#include "core/JavaString.h"
#include "core/StringUtil.h"
#include "platform/Keyboard.h"
#include "util/ChatAllowedCharacters.h"
#include "util/Random.h"
#include "world/WorldSettings.h"
#include "world/storage/SaveFormat.h"
#include "world/storage/WorldInfo.h"

#include <cctype>
#include <stdexcept>

namespace mc {

namespace {

constexpr const char* disallowedFilenames[] { "CON", "COM", "PRN", "AUX", "CLOCK$", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8",
    "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };

// String.equalsIgnoreCase against an ASCII name.
bool equalsIgnoreCase(std::u16string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] >= 128 || std::toupper(static_cast<unsigned char>(a[i])) != std::toupper(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

} // namespace

GuiCreateWorld::GuiCreateWorld(std::shared_ptr<GuiScreen> parentScreenIn)
    : parentScreen(std::move(parentScreenIn))
    , worldName(I18n::format("selectWorld.newWorld"))
{
}

GuiCreateWorld::~GuiCreateWorld() = default;

void GuiCreateWorld::updateScreen()
{
    worldNameField->updateCursorCounter();
    worldSeedField->updateCursorCounter();
}

void GuiCreateWorld::initGui()
{
    Keyboard::enableRepeatEvents(true);
    buttonList.clear();
    addButton<GuiButton>(0, width / 2 - 155, height - 28, 150, 20, I18n::format("selectWorld.create"));
    addButton<GuiButton>(1, width / 2 + 5, height - 28, 150, 20, I18n::format("gui.cancel"));
    btnGameMode = &addButton<GuiButton>(2, width / 2 - 75, 115, 150, 20, I18n::format("selectWorld.gameMode"));
    btnMoreOptions = &addButton<GuiButton>(3, width / 2 - 75, 187, 150, 20, I18n::format("selectWorld.moreWorldOptions"));
    btnMapFeatures = &addButton<GuiButton>(4, width / 2 - 155, 100, 150, 20, I18n::format("selectWorld.mapFeatures"));
    btnMapFeatures->visible = false;
    btnBonusItems = &addButton<GuiButton>(7, width / 2 + 5, 151, 150, 20, I18n::format("selectWorld.bonusItems"));
    btnBonusItems->visible = false;
    btnMapType = &addButton<GuiButton>(5, width / 2 + 5, 100, 150, 20, I18n::format("selectWorld.mapType"));
    btnMapType->visible = false;
    btnAllowCommands = &addButton<GuiButton>(6, width / 2 - 155, 151, 150, 20, I18n::format("selectWorld.allowCommands"));
    btnAllowCommands->visible = false;
    btnCustomizeType = &addButton<GuiButton>(8, width / 2 + 5, 120, 150, 20, I18n::format("selectWorld.customizeType"));
    btnCustomizeType->visible = false;
    worldNameField = std::make_unique<GuiTextField>(9, *fontRendererObj, width / 2 - 100, 60, 200, 20);
    worldNameField->setFocused(true);
    worldNameField->setText(worldName);
    worldSeedField = std::make_unique<GuiTextField>(10, *fontRendererObj, width / 2 - 100, 60, 200, 20);
    worldSeedField->setText(worldSeed);
    showMoreWorldOptions(inMoreWorldOptionsDisplay);
    calcSaveDirName();
    updateDisplayState();
}

void GuiCreateWorld::calcSaveDirName()
{
    std::u16string name = toUtf16(java::trim(worldNameField->getText()));
    for (const char16_t c0 : ChatAllowedCharacters::allowedCharactersArray)
        std::replace(name.begin(), name.end(), c0, u'_');
    saveDirName = fromUtf16(name);
    if (saveDirName.empty())
        saveDirName = "World";
    saveDirName = getUncollidingSaveDirName(mc->getSaveLoader(), saveDirName);
}

void GuiCreateWorld::updateDisplayState()
{
    btnGameMode->displayString = I18n::format("selectWorld.gameMode") + ": " + I18n::format("selectWorld.gameMode." + gameMode);
    gameModeDesc1 = I18n::format("selectWorld.gameMode." + gameMode + ".line1");
    gameModeDesc2 = I18n::format("selectWorld.gameMode." + gameMode + ".line2");
    btnMapFeatures->displayString = I18n::format("selectWorld.mapFeatures") + " " + (generateStructuresEnabled ? I18n::format("options.on") : I18n::format("options.off"));
    btnBonusItems->displayString = I18n::format("selectWorld.bonusItems") + " " + (bonusChestEnabled && !hardCoreMode ? I18n::format("options.on") : I18n::format("options.off"));
    btnMapType->displayString = I18n::format("selectWorld.mapType") + " " + I18n::format(WorldType::worldTypes[static_cast<size_t>(selectedIndex)]->getTranslateName());
    btnAllowCommands->displayString = I18n::format("selectWorld.allowCommands") + " " + (allowCheats && !hardCoreMode ? I18n::format("options.on") : I18n::format("options.off"));
}

std::string GuiCreateWorld::getUncollidingSaveDirName(ISaveFormat& saveLoader, std::string name)
{
    // replaceAll("[\\./\"]", "_")
    std::replace_if(name.begin(), name.end(), [](char c) { return c == '.' || c == '/' || c == '"'; }, '_');
    for (const char* s : disallowedFilenames) {
        if (equalsIgnoreCase(toUtf16(name), s))
            name = "_" + name + "_";
    }
    while (saveLoader.getWorldInfo(name))
        name += "-";
    return name;
}

void GuiCreateWorld::onGuiClosed()
{
    Keyboard::enableRepeatEvents(false);
}

void GuiCreateWorld::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;
    const auto self = shared_from_this();
    if (button.id == 1) {
        mc->displayGuiScreen(parentScreen);
    } else if (button.id == 0) {
        mc->displayGuiScreen(nullptr);
        if (alreadyGenerated)
            return;
        alreadyGenerated = true;
        int64_t i = Random().nextLong();
        const std::string s = worldSeedField->getText();
        if (!s.empty()) {
            try {
                const int64_t j = java::parseLong(s);
                if (j != 0)
                    i = j;
            } catch (const std::invalid_argument&) {
                i = static_cast<int64_t>(java::hashCode(s));
            }
        }
        const GameType gametype = getGameTypeByName(gameMode);
        WorldSettings worldsettings(i, gametype, generateStructuresEnabled, hardCoreMode, WorldType::worldTypes[static_cast<size_t>(selectedIndex)]);
        worldsettings.setWorldName(chunkProviderSettingsJson);
        if (bonusChestEnabled && !hardCoreMode)
            worldsettings.enableBonusChest();
        if (allowCheats && !hardCoreMode)
            worldsettings.enableCommands();
        mc->launchIntegratedServer(saveDirName, java::trim(worldNameField->getText()), &worldsettings);
    } else if (button.id == 3) {
        toggleMoreWorldOptions();
    } else if (button.id == 2) {
        if (gameMode == "survival") {
            if (!allowCheatsWasSetByUser)
                allowCheats = false;
            hardCoreMode = false;
            gameMode = "hardcore";
            hardCoreMode = true;
            btnAllowCommands->enabled = false;
            btnBonusItems->enabled = false;
            updateDisplayState();
        } else if (gameMode == "hardcore") {
            if (!allowCheatsWasSetByUser)
                allowCheats = true;
            hardCoreMode = false;
            gameMode = "creative";
            updateDisplayState();
            hardCoreMode = false;
            btnAllowCommands->enabled = true;
            btnBonusItems->enabled = true;
        } else {
            if (!allowCheatsWasSetByUser)
                allowCheats = false;
            gameMode = "survival";
            updateDisplayState();
            btnAllowCommands->enabled = true;
            btnBonusItems->enabled = true;
            hardCoreMode = false;
        }
        updateDisplayState();
    } else if (button.id == 4) {
        generateStructuresEnabled = !generateStructuresEnabled;
        updateDisplayState();
    } else if (button.id == 7) {
        bonusChestEnabled = !bonusChestEnabled;
        updateDisplayState();
    } else if (button.id == 5) {
        const int count = static_cast<int>(WorldType::worldTypes.size());
        ++selectedIndex;
        if (selectedIndex >= count)
            selectedIndex = 0;
        while (!canSelectCurWorldType()) {
            ++selectedIndex;
            if (selectedIndex >= count)
                selectedIndex = 0;
        }
        chunkProviderSettingsJson.clear();
        updateDisplayState();
        showMoreWorldOptions(inMoreWorldOptionsDisplay);
    } else if (button.id == 6) {
        allowCheatsWasSetByUser = true;
        allowCheats = !allowCheats;
        updateDisplayState();
    } else if (button.id == 8) {
        if (WorldType::worldTypes[static_cast<size_t>(selectedIndex)] == &WorldType::FLAT) {
            // Java: new GuiCreateFlatWorld(this, chunkProviderSettingsJson)
            mc->displayGuiScreen(GuiErrorScreen::notPortedYet(I18n::format("createWorld.customize.flat.title"), "stage 7"));
        } else {
            mc->displayGuiScreen(std::make_shared<GuiCustomizeWorldScreen>(std::static_pointer_cast<GuiCreateWorld>(self), chunkProviderSettingsJson));
        }
    }
}

bool GuiCreateWorld::canSelectCurWorldType() const
{
    const WorldType* worldtype = WorldType::worldTypes[static_cast<size_t>(selectedIndex)];
    if (!worldtype || !worldtype->getCanBeCreated())
        return false;
    return worldtype == &WorldType::DEBUG_WORLD ? isShiftKeyDown() : true;
}

void GuiCreateWorld::showMoreWorldOptions(bool toggle)
{
    inMoreWorldOptionsDisplay = toggle;
    const WorldType* type = WorldType::worldTypes[static_cast<size_t>(selectedIndex)];
    if (type == &WorldType::DEBUG_WORLD) {
        btnGameMode->visible = !inMoreWorldOptionsDisplay;
        btnGameMode->enabled = false;
        if (!savedGameMode)
            savedGameMode = gameMode;
        gameMode = "spectator";
        btnMapFeatures->visible = false;
        btnBonusItems->visible = false;
        btnMapType->visible = inMoreWorldOptionsDisplay;
        btnAllowCommands->visible = false;
        btnCustomizeType->visible = false;
    } else {
        btnGameMode->visible = !inMoreWorldOptionsDisplay;
        btnGameMode->enabled = true;
        if (savedGameMode) {
            gameMode = *savedGameMode;
            savedGameMode.reset();
        }
        btnMapFeatures->visible = inMoreWorldOptionsDisplay && type != &WorldType::CUSTOMIZED;
        btnBonusItems->visible = inMoreWorldOptionsDisplay;
        btnMapType->visible = inMoreWorldOptionsDisplay;
        btnAllowCommands->visible = inMoreWorldOptionsDisplay;
        btnCustomizeType->visible = inMoreWorldOptionsDisplay && (type == &WorldType::FLAT || type == &WorldType::CUSTOMIZED);
    }
    updateDisplayState();
    btnMoreOptions->displayString = inMoreWorldOptionsDisplay ? I18n::format("gui.done") : I18n::format("selectWorld.moreWorldOptions");
}

void GuiCreateWorld::keyTyped(char16_t typedChar, int keyCode)
{
    if (worldNameField->isFocused() && !inMoreWorldOptionsDisplay) {
        worldNameField->textboxKeyTyped(typedChar, keyCode);
        worldName = worldNameField->getText();
    } else if (worldSeedField->isFocused() && inMoreWorldOptionsDisplay) {
        worldSeedField->textboxKeyTyped(typedChar, keyCode);
        worldSeed = worldSeedField->getText();
    }
    if (keyCode == 28 || keyCode == 156)
        actionPerformed(*buttonList[0]);
    buttonList[0]->enabled = !worldNameField->getText().empty();
    calcSaveDirName();
}

void GuiCreateWorld::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    GuiScreen::mouseClicked(mouseX, mouseY, mouseButton);
    if (inMoreWorldOptionsDisplay)
        worldSeedField->mouseClicked(mouseX, mouseY, mouseButton);
    else
        worldNameField->mouseClicked(mouseX, mouseY, mouseButton);
}

void GuiCreateWorld::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    drawCenteredString(*fontRendererObj, I18n::format("selectWorld.create"), width / 2, 20, -1);
    if (inMoreWorldOptionsDisplay) {
        drawString(*fontRendererObj, I18n::format("selectWorld.enterSeed"), width / 2 - 100, 47, -6250336);
        drawString(*fontRendererObj, I18n::format("selectWorld.seedInfo"), width / 2 - 100, 85, -6250336);
        if (btnMapFeatures->visible)
            drawString(*fontRendererObj, I18n::format("selectWorld.mapFeatures.info"), width / 2 - 150, 122, -6250336);
        if (btnAllowCommands->visible)
            drawString(*fontRendererObj, I18n::format("selectWorld.allowCommands.info"), width / 2 - 150, 172, -6250336);
        worldSeedField->drawTextBox();
        const WorldType* type = WorldType::worldTypes[static_cast<size_t>(selectedIndex)];
        if (type->showWorldInfoNotice())
            fontRendererObj->drawSplitString(I18n::format(type->getTranslatedInfo()), btnMapType->xPosition + 2, btnMapType->yPosition + 22, btnMapType->getButtonWidth(),
                10526880);
    } else {
        drawString(*fontRendererObj, I18n::format("selectWorld.enterName"), width / 2 - 100, 47, -6250336);
        drawString(*fontRendererObj, I18n::format("selectWorld.resultFolder") + " " + saveDirName, width / 2 - 100, 85, -6250336);
        worldNameField->drawTextBox();
        drawString(*fontRendererObj, gameModeDesc1, width / 2 - 100, 137, -6250336);
        drawString(*fontRendererObj, gameModeDesc2, width / 2 - 100, 149, -6250336);
    }
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

void GuiCreateWorld::recreateFromExistingWorld(const WorldInfo& original)
{
    worldName = I18n::format("selectWorld.newWorld.copyOf", original.getWorldName());
    worldSeed = std::to_string(original.getSeed());
    selectedIndex = original.getTerrainType()->getWorldTypeID();
    chunkProviderSettingsJson = original.getGeneratorOptions();
    generateStructuresEnabled = original.isMapFeaturesEnabled();
    allowCheats = original.areCommandsAllowed();
    if (original.isHardcoreModeEnabled())
        gameMode = "hardcore";
    else if (isSurvivalOrAdventure(original.getGameType()))
        gameMode = "survival";
    else if (isCreative(original.getGameType()))
        gameMode = "creative";
}

} // namespace mc
