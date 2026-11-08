#pragma once

// Original: net/minecraft/client/gui/GuiScreenResourcePacks.java, GuiResourcePackList.java,
// GuiResourcePackAvailable.java and GuiResourcePackSelected.java

#include "client/gui/GuiScreen.h"
#include "client/gui/GuiSlot.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class ResourcePackListEntry;

class GuiResourcePackList : public GuiListExtended {
public:
    GuiResourcePackList(Minecraft& mcIn, int widthIn, int heightIn, std::vector<std::shared_ptr<ResourcePackListEntry>>& entries, std::string headerKey);

    std::vector<std::shared_ptr<ResourcePackListEntry>>& getList() { return entries; }
    IGuiListEntry& getListEntry(int index) override;
    int getListWidth() const override { return width; }

protected:
    void drawListHeader(int insideLeft, int insideTop, Tessellator& tessellator) override;
    int getSize() const override { return static_cast<int>(entries.size()); }
    int getScrollBarX() const override { return right - 6; }

private:
    std::vector<std::shared_ptr<ResourcePackListEntry>>& entries;
    // GuiResourcePackAvailable / GuiResourcePackSelected differ only in this.
    std::string headerKey;
};

class GuiScreenResourcePacks : public GuiScreen {
public:
    using EntryList = std::vector<std::shared_ptr<ResourcePackListEntry>>;

    explicit GuiScreenResourcePacks(std::shared_ptr<GuiScreen> parentScreen);

    void initGui() override;
    void handleMouseInput() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

    bool hasResourcePackEntry(const ResourcePackListEntry* entry) const;
    EntryList& getListContaining(const ResourcePackListEntry* entry);
    EntryList& getAvailableResourcePacks() { return availableResourcePacks; }
    EntryList& getSelectedResourcePacks() { return selectedResourcePacks; }
    void markChanged() { changed = true; }

protected:
    void actionPerformed(GuiButton& button) override;
    void mouseClicked(int mouseX, int mouseY, int mouseButton) override;
    void mouseReleased(int mouseX, int mouseY, int state) override;

private:
    std::shared_ptr<GuiScreen> parentScreen;
    EntryList availableResourcePacks;
    EntryList selectedResourcePacks;
    std::shared_ptr<GuiResourcePackList> availableResourcePacksList;
    std::shared_ptr<GuiResourcePackList> selectedResourcePacksList;
    bool changed = false;
};

} // namespace mc
