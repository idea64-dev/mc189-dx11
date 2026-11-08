#pragma once

// Original: net/minecraft/client/resources/ResourcePackListEntry.java, ResourcePackListEntryDefault.java and
// ResourcePackListEntryFound.java

#include "client/gui/GuiSlot.h"
#include "client/resources/ResourcePackRepository.h"
#include "util/ResourceLocation.h"

#include <memory>
#include <string>

namespace mc {

class GuiScreenResourcePacks;
class IResourcePack;
class Minecraft;

class ResourcePackListEntry : public GuiListExtended::IGuiListEntry, public std::enable_shared_from_this<ResourcePackListEntry> {
public:
    explicit ResourcePackListEntry(GuiScreenResourcePacks& resourcePacksGUI);

    void drawEntry(int slotIndex, int x, int y, int listWidth, int slotHeight, int mouseX, int mouseY, bool isSelected) override;
    bool mousePressed(int slotIndex, int x, int y, int mouseEvent, int relativeX, int relativeY) override;
    void setSelected(int /*slotIndex*/, int /*insideLeft*/, int /*yPos*/) override { }
    void mouseReleased(int /*slotIndex*/, int /*x*/, int /*y*/, int /*mouseEvent*/, int /*relativeX*/, int /*relativeY*/) override { }

protected:
    // Java: func_183019_a, the pack format (1 is compatible)
    virtual int getPackFormat() const = 0;
    // Java: func_148311_a
    virtual std::string getDescription() const = 0;
    // Java: func_148312_b
    virtual std::string getName() const = 0;
    // Java: func_148313_c
    virtual void bindIcon() = 0;
    // Java: func_148310_d
    virtual bool canBeMoved() const { return true; }
    // Java: func_148309_e
    virtual bool canBeSelected() const;
    // Java: func_148308_f
    virtual bool canBeDeselected() const;
    // Java: func_148314_g
    virtual bool canMoveUp() const;
    // Java: func_148307_h
    virtual bool canMoveDown() const;

    Minecraft& mc;
    GuiScreenResourcePacks& resourcePacksGUI;
};

class ResourcePackListEntryDefault : public ResourcePackListEntry {
public:
    explicit ResourcePackListEntryDefault(GuiScreenResourcePacks& resourcePacksGUI);

protected:
    int getPackFormat() const override { return 1; }
    std::string getDescription() const override;
    std::string getName() const override { return "Default"; }
    void bindIcon() override;
    bool canBeMoved() const override { return false; }
    bool canBeSelected() const override { return false; }
    bool canBeDeselected() const override { return false; }
    bool canMoveUp() const override { return false; }
    bool canMoveDown() const override { return false; }

private:
    std::shared_ptr<IResourcePack> defaultPack;
    ResourceLocation resourcePackIcon;
};

class ResourcePackListEntryFound : public ResourcePackListEntry {
public:
    ResourcePackListEntryFound(GuiScreenResourcePacks& resourcePacksGUI, std::shared_ptr<ResourcePackRepository::Entry> entry);

    // Java: func_148318_i
    const std::shared_ptr<ResourcePackRepository::Entry>& getEntry() const { return entry; }

protected:
    int getPackFormat() const override { return entry->getPackFormat(); }
    std::string getDescription() const override { return entry->getTexturePackDescription(); }
    std::string getName() const override { return entry->getResourcePackName(); }
    void bindIcon() override;

private:
    std::shared_ptr<ResourcePackRepository::Entry> entry;
};

} // namespace mc
