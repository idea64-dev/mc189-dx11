#include "client/gui/GuiSelectWorld.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiCreateWorld.h"
#include "client/gui/GuiErrorScreen.h"
#include "client/gui/GuiTextField.h"
#include "client/gui/GuiYesNo.h"
#include "client/resources/Locale.h"
#include "core/JavaLocale.h"
#include "core/JavaString.h"
#include "core/Log.h"
#include "platform/Keyboard.h"
#include "util/ChatComponent.h"
#include "world/storage/ISaveHandler.h"
#include "world/storage/WorldInfo.h"

#include <algorithm>

namespace mc {

// --- GuiSelectWorld ---

GuiSelectWorld::GuiSelectWorld(std::shared_ptr<GuiScreen> parentScreenIn)
    : parentScreen(std::move(parentScreenIn))
{
}

void GuiSelectWorld::initGui()
{
    screenTitle = I18n::format("selectWorld.title");
    try {
        loadLevelList();
    } catch (const AnvilConverterException& e) {
        Log::error("Couldn't load level list: {}", e.what());
        mc->displayGuiScreen(std::make_shared<GuiErrorScreen>("Unable to load worlds", e.what()));
        return;
    }
    field_146637_u = I18n::format("selectWorld.world");
    field_146636_v = I18n::format("selectWorld.conversion");
    field_146635_w[static_cast<size_t>(getGameTypeID(GameType::SURVIVAL))] = I18n::format("gameMode.survival");
    field_146635_w[static_cast<size_t>(getGameTypeID(GameType::CREATIVE))] = I18n::format("gameMode.creative");
    field_146635_w[static_cast<size_t>(getGameTypeID(GameType::ADVENTURE))] = I18n::format("gameMode.adventure");
    field_146635_w[static_cast<size_t>(getGameTypeID(GameType::SPECTATOR))] = I18n::format("gameMode.spectator");
    availableWorlds = std::make_shared<List>(*this, *mc);
    availableWorlds->registerScrollButtons(4, 5);
    addWorldSelectionButtons();
}

void GuiSelectWorld::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    const std::shared_ptr<List> list = availableWorlds;
    if (list)
        list->handleMouseInput();
}

void GuiSelectWorld::loadLevelList()
{
    saveList = mc->getSaveLoader().getSaveList();
    std::stable_sort(saveList.begin(), saveList.end(), [](const SaveFormatComparator& a, const SaveFormatComparator& b) { return a.compareTo(b) < 0; });
    selectedIndex = -1;
}

std::string GuiSelectWorld::getSaveFileName(int index) const
{
    return saveList.at(static_cast<size_t>(index)).getFileName();
}

std::string GuiSelectWorld::getSaveName(int index) const
{
    std::string s = saveList.at(static_cast<size_t>(index)).getDisplayName();
    if (s.empty())
        s = I18n::format("selectWorld.world") + " " + std::to_string(index + 1);
    return s;
}

void GuiSelectWorld::addWorldSelectionButtons()
{
    selectButton = &addButton<GuiButton>(1, width / 2 - 154, height - 52, 150, 20, I18n::format("selectWorld.select"));
    addButton<GuiButton>(3, width / 2 + 4, height - 52, 150, 20, I18n::format("selectWorld.create"));
    renameButton = &addButton<GuiButton>(6, width / 2 - 154, height - 28, 72, 20, I18n::format("selectWorld.rename"));
    deleteButton = &addButton<GuiButton>(2, width / 2 - 76, height - 28, 72, 20, I18n::format("selectWorld.delete"));
    recreateButton = &addButton<GuiButton>(7, width / 2 + 4, height - 28, 72, 20, I18n::format("selectWorld.recreate"));
    addButton<GuiButton>(0, width / 2 + 82, height - 28, 72, 20, I18n::format("gui.cancel"));
    selectButton->enabled = false;
    deleteButton->enabled = false;
    renameButton->enabled = false;
    recreateButton->enabled = false;
}

void GuiSelectWorld::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;
    const auto self = shared_from_this();
    if (button.id == 2) {
        const std::string s = getSaveName(selectedIndex);
        confirmingDelete = true;
        mc->displayGuiScreen(makeDeleteWorldYesNo(self, s, selectedIndex));
    } else if (button.id == 1) {
        selectWorld(selectedIndex);
    } else if (button.id == 3) {
        mc->displayGuiScreen(std::make_shared<GuiCreateWorld>(self));
    } else if (button.id == 6) {
        mc->displayGuiScreen(std::make_shared<GuiRenameWorld>(self, getSaveFileName(selectedIndex)));
    } else if (button.id == 0) {
        mc->displayGuiScreen(parentScreen);
    } else if (button.id == 7) {
        auto guicreateworld = std::make_shared<GuiCreateWorld>(self);
        const std::shared_ptr<ISaveHandler> isavehandler = mc->getSaveLoader().getSaveLoader(getSaveFileName(selectedIndex), false);
        const std::unique_ptr<WorldInfo> worldinfo = isavehandler->loadWorldInfo();
        isavehandler->flush();
        // Java dereferences the WorldInfo; a world without a readable level.dat throws there.
        if (!worldinfo)
            throw std::runtime_error("No level.dat in " + getSaveFileName(selectedIndex));
        guicreateworld->recreateFromExistingWorld(*worldinfo);
        mc->displayGuiScreen(guicreateworld);
    } else {
        availableWorlds->actionPerformed(button);
    }
}

void GuiSelectWorld::selectWorld(int index)
{
    const auto keepAlive = shared_from_this();
    mc->displayGuiScreen(nullptr);
    if (field_146634_i)
        return;
    field_146634_i = true;
    const std::string s = getSaveFileName(index);
    const std::string s1 = getSaveName(index);
    if (mc->getSaveLoader().canLoadWorld(s))
        mc->launchIntegratedServer(s, s1, nullptr);
}

void GuiSelectWorld::confirmClicked(bool result, int id)
{
    if (!confirmingDelete)
        return;
    confirmingDelete = false;
    if (result) {
        ISaveFormat& isaveformat = mc->getSaveLoader();
        isaveformat.flushCache();
        isaveformat.deleteWorldDirectory(getSaveFileName(id));
        try {
            loadLevelList();
        } catch (const AnvilConverterException& e) {
            Log::error("Couldn't load level list: {}", e.what());
        }
    }
    mc->displayGuiScreen(shared_from_this());
}

void GuiSelectWorld::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    availableWorlds->drawScreen(mouseX, mouseY, partialTicks);
    drawCenteredString(*fontRendererObj, screenTitle, width / 2, 20, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

std::shared_ptr<GuiYesNo> GuiSelectWorld::makeDeleteWorldYesNo(std::shared_ptr<GuiYesNoCallback> selectWorld, const std::string& name, int id)
{
    const std::string s = I18n::format("selectWorld.deleteQuestion");
    const std::string s1 = "'" + name + "' " + I18n::format("selectWorld.deleteWarning");
    const std::string s2 = I18n::format("selectWorld.deleteButton");
    const std::string s3 = I18n::format("gui.cancel");
    return std::make_shared<GuiYesNo>(std::move(selectWorld), s, s1, s2, s3, id);
}

GuiSelectWorld::List::List(GuiSelectWorld& screenIn, Minecraft& mcIn)
    : GuiSlot(mcIn, screenIn.width, screenIn.height, 32, screenIn.height - 64, 36)
    , screen(screenIn)
{
}

void GuiSelectWorld::List::elementClicked(int slotIndex, bool isDoubleClick, int, int)
{
    screen.selectedIndex = slotIndex;
    const bool flag = screen.selectedIndex >= 0 && screen.selectedIndex < getSize();
    screen.selectButton->enabled = flag;
    screen.deleteButton->enabled = flag;
    screen.renameButton->enabled = flag;
    screen.recreateButton->enabled = flag;
    if (isDoubleClick && flag)
        screen.selectWorld(slotIndex);
}

void GuiSelectWorld::List::drawSlot(int entryID, int insideLeft, int yPos, int, int, int)
{
    const SaveFormatComparator& save = screen.saveList[static_cast<size_t>(entryID)];
    std::string s = save.getDisplayName();
    if (s.empty())
        s = screen.field_146637_u + " " + std::to_string(entryID + 1);
    std::string s1 = save.getFileName();
    s1 += " (" + java::formatDefaultDateTime(save.getLastTimePlayed());
    s1 += ")";
    std::string s2;
    if (save.requiresConversion()) {
        s2 = screen.field_146636_v + " " + s2;
    } else {
        s2 = screen.field_146635_w[static_cast<size_t>(getGameTypeID(save.getEnumGameType()))];
        if (save.isHardcoreModeEnabled())
            s2 = EnumChatFormatting::toString('4') + I18n::format("gameMode.hardcore") + EnumChatFormatting::toString('r');
        if (save.getCheatsEnabled())
            s2 += ", " + I18n::format("selectWorld.cheats");
    }
    screen.drawString(*screen.fontRendererObj, s, insideLeft + 2, yPos + 1, 16777215);
    screen.drawString(*screen.fontRendererObj, s1, insideLeft + 2, yPos + 12, 8421504);
    screen.drawString(*screen.fontRendererObj, s2, insideLeft + 2, yPos + 12 + 10, 8421504);
}

// --- GuiRenameWorld ---

GuiRenameWorld::GuiRenameWorld(std::shared_ptr<GuiScreen> parentScreenIn, std::string saveNameIn)
    : parentScreen(std::move(parentScreenIn))
    , saveName(std::move(saveNameIn))
{
}

GuiRenameWorld::~GuiRenameWorld() = default;

void GuiRenameWorld::updateScreen()
{
    field_146583_f->updateCursorCounter();
}

void GuiRenameWorld::initGui()
{
    Keyboard::enableRepeatEvents(true);
    buttonList.clear();
    addButton<GuiButton>(0, width / 2 - 100, height / 4 + 96 + 12, I18n::format("selectWorld.renameButton"));
    addButton<GuiButton>(1, width / 2 - 100, height / 4 + 120 + 12, I18n::format("gui.cancel"));
    const std::unique_ptr<WorldInfo> worldinfo = mc->getSaveLoader().getWorldInfo(saveName);
    if (!worldinfo)
        throw std::runtime_error("No level.dat in " + saveName); // Java: NullPointerException
    const std::string s = worldinfo->getWorldName();
    field_146583_f = std::make_unique<GuiTextField>(2, *fontRendererObj, width / 2 - 100, 60, 200, 20);
    field_146583_f->setFocused(true);
    field_146583_f->setText(s);
}

void GuiRenameWorld::onGuiClosed()
{
    Keyboard::enableRepeatEvents(false);
}

void GuiRenameWorld::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;
    if (button.id == 1) {
        mc->displayGuiScreen(parentScreen);
    } else if (button.id == 0) {
        mc->getSaveLoader().renameWorld(saveName, java::trim(field_146583_f->getText()));
        mc->displayGuiScreen(parentScreen);
    }
}

void GuiRenameWorld::keyTyped(char16_t typedChar, int keyCode)
{
    field_146583_f->textboxKeyTyped(typedChar, keyCode);
    buttonList[0]->enabled = !java::trim(field_146583_f->getText()).empty();
    if (keyCode == 28 || keyCode == 156)
        actionPerformed(*buttonList[0]);
}

void GuiRenameWorld::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    GuiScreen::mouseClicked(mouseX, mouseY, mouseButton);
    field_146583_f->mouseClicked(mouseX, mouseY, mouseButton);
}

void GuiRenameWorld::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    drawCenteredString(*fontRendererObj, I18n::format("selectWorld.renameTitle"), width / 2, 20, 16777215);
    drawString(*fontRendererObj, I18n::format("selectWorld.enterName"), width / 2 - 100, 47, 10526880);
    field_146583_f->drawTextBox();
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
