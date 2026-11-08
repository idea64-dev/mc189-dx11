#include "client/gui/GuiScreenResourcePacks.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiOptionButton.h"
#include "client/resources/Locale.h"
#include "client/resources/ResourcePackListEntry.h"
#include "client/resources/ResourcePackRepository.h"
#include "client/settings/GameSettings.h"
#include "core/Log.h"

#include <Windows.h>
#include <shellapi.h>

#include <algorithm>

namespace mc {

// --- GuiResourcePackList ---

GuiResourcePackList::GuiResourcePackList(Minecraft& mcIn, int widthIn, int heightIn, std::vector<std::shared_ptr<ResourcePackListEntry>>& entriesIn, std::string headerKeyIn)
    : GuiListExtended(mcIn, widthIn, heightIn, 32, heightIn - 55 + 4, 36)
    , entries(entriesIn)
    , headerKey(std::move(headerKeyIn))
{
    centerListVertically = false;
    setHasListHeader(true, static_cast<int>(static_cast<float>(mcIn.fontRendererObj->FONT_HEIGHT) * 1.5f));
}

GuiListExtended::IGuiListEntry& GuiResourcePackList::getListEntry(int index)
{
    return *entries[static_cast<size_t>(index)];
}

void GuiResourcePackList::drawListHeader(int insideLeft, int insideTop, Tessellator& /*tessellator*/)
{
    // EnumChatFormatting.UNDERLINE + BOLD
    const std::string s = "§n§l" + I18n::format(headerKey);
    FontRenderer& font = *mc.fontRendererObj;
    font.drawString(s, insideLeft + width / 2 - font.getStringWidth(s) / 2, std::min(top + 3, insideTop), 16777215);
}

// --- GuiScreenResourcePacks ---

GuiScreenResourcePacks::GuiScreenResourcePacks(std::shared_ptr<GuiScreen> parentScreenIn)
    : parentScreen(std::move(parentScreenIn))
{
}

void GuiScreenResourcePacks::initGui()
{
    addButton<GuiOptionButton>(2, width / 2 - 154, height - 48, I18n::format("resourcePack.openFolder"));
    addButton<GuiOptionButton>(1, width / 2 + 4, height - 48, I18n::format("gui.done"));

    if (!changed) {
        availableResourcePacks.clear();
        selectedResourcePacks.clear();
        ResourcePackRepository& repository = mc->getResourcePackRepository();
        repository.updateRepositoryEntriesAll();
        std::vector<std::shared_ptr<ResourcePackRepository::Entry>> list = repository.getRepositoryEntriesAll();
        const auto selected = repository.getRepositoryEntries();
        // Java: list.removeAll(getRepositoryEntries()), equality by Entry.toString()
        std::erase_if(list, [&](const auto& entry) {
            return std::any_of(selected.begin(), selected.end(), [&](const auto& other) { return other->toString() == entry->toString(); });
        });

        for (const auto& entry : list)
            availableResourcePacks.push_back(std::make_shared<ResourcePackListEntryFound>(*this, entry));
        for (auto it = selected.rbegin(); it != selected.rend(); ++it)
            selectedResourcePacks.push_back(std::make_shared<ResourcePackListEntryFound>(*this, *it));
        selectedResourcePacks.push_back(std::make_shared<ResourcePackListEntryDefault>(*this));
    }

    availableResourcePacksList = std::make_shared<GuiResourcePackList>(*mc, 200, height, availableResourcePacks, "resourcePack.available.title");
    availableResourcePacksList->setSlotXBoundsFromLeft(width / 2 - 4 - 200);
    availableResourcePacksList->registerScrollButtons(7, 8);
    selectedResourcePacksList = std::make_shared<GuiResourcePackList>(*mc, 200, height, selectedResourcePacks, "resourcePack.selected.title");
    selectedResourcePacksList->setSlotXBoundsFromLeft(width / 2 + 4);
    selectedResourcePacksList->registerScrollButtons(7, 8);
}

void GuiScreenResourcePacks::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    const std::shared_ptr<GuiResourcePackList> selected = selectedResourcePacksList;
    const std::shared_ptr<GuiResourcePackList> available = availableResourcePacksList;
    selected->handleMouseInput();
    available->handleMouseInput();
}

bool GuiScreenResourcePacks::hasResourcePackEntry(const ResourcePackListEntry* entry) const
{
    return std::any_of(selectedResourcePacks.begin(), selectedResourcePacks.end(), [&](const auto& e) { return e.get() == entry; });
}

GuiScreenResourcePacks::EntryList& GuiScreenResourcePacks::getListContaining(const ResourcePackListEntry* entry)
{
    return hasResourcePackEntry(entry) ? selectedResourcePacks : availableResourcePacks;
}

void GuiScreenResourcePacks::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;

    if (button.id == 2) {
        // Java: cmd.exe /C start "Open file" "<folder>"
        const std::wstring folder = std::filesystem::absolute(mc->getResourcePackRepository().getDirResourcepacks()).wstring();
        if (reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) <= 32)
            Log::error("Couldn't open file");
    } else if (button.id == 1) {
        if (changed) {
            std::vector<std::shared_ptr<ResourcePackRepository::Entry>> list;
            for (const auto& entry : selectedResourcePacks) {
                if (const auto* found = dynamic_cast<const ResourcePackListEntryFound*>(entry.get()))
                    list.push_back(found->getEntry());
            }
            std::reverse(list.begin(), list.end());
            mc->getResourcePackRepository().setRepositories(list);
            mc->gameSettings->resourcePacks.clear();
            mc->gameSettings->incompatibleResourcePacks.clear();
            for (const auto& entry : list) {
                mc->gameSettings->resourcePacks.push_back(entry->getResourcePackName());
                if (entry->getPackFormat() != 1)
                    mc->gameSettings->incompatibleResourcePacks.push_back(entry->getResourcePackName());
            }
            mc->gameSettings->saveOptions();
            mc->refreshResources();
        }
        mc->displayGuiScreen(parentScreen);
    }
}

void GuiScreenResourcePacks::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    GuiScreen::mouseClicked(mouseX, mouseY, mouseButton);
    const std::shared_ptr<GuiResourcePackList> available = availableResourcePacksList;
    const std::shared_ptr<GuiResourcePackList> selected = selectedResourcePacksList;
    available->mouseClicked(mouseX, mouseY, mouseButton);
    selected->mouseClicked(mouseX, mouseY, mouseButton);
}

void GuiScreenResourcePacks::mouseReleased(int mouseX, int mouseY, int state)
{
    GuiScreen::mouseReleased(mouseX, mouseY, state);
}

void GuiScreenResourcePacks::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawBackground(0);
    availableResourcePacksList->drawScreen(mouseX, mouseY, partialTicks);
    selectedResourcePacksList->drawScreen(mouseX, mouseY, partialTicks);
    drawCenteredString(*fontRendererObj, I18n::format("resourcePack.title"), width / 2, 16, 16777215);
    drawCenteredString(*fontRendererObj, I18n::format("resourcePack.folderInfo"), width / 2 - 77, height - 26, 8421504);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

} // namespace mc
