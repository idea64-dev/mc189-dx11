#include "client/resources/ResourcePackListEntry.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/Gui.h"
#include "client/gui/GuiScreenResourcePacks.h"
#include "client/gui/GuiYesNo.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/texture/SimpleTexture.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/resources/IResourcePack.h"
#include "client/resources/Locale.h"
#include "core/Log.h"
#include "util/ChatComponent.h"

#include <algorithm>

namespace mc {

namespace {

const ResourceLocation RESOURCE_PACKS_TEXTURE("textures/gui/resource_packs.png");

std::string formatted(const std::string& key)
{
    return ChatComponentTranslation(key).getFormattedText();
}

using EntryList = std::vector<std::shared_ptr<ResourcePackListEntry>>;

ptrdiff_t indexOf(const EntryList& list, const ResourcePackListEntry* entry)
{
    const auto it = std::find_if(list.begin(), list.end(), [&](const auto& e) { return e.get() == entry; });
    return it == list.end() ? -1 : it - list.begin();
}

// The confirmation for selecting an incompatible pack (an anonymous GuiYesNoCallback in Java).
class IncompatibleCallback : public GuiYesNoCallback {
public:
    IncompatibleCallback(std::shared_ptr<GuiScreen> screen, std::shared_ptr<ResourcePackListEntry> entry)
        : screen(std::move(screen))
        , entry(std::move(entry))
    {
    }

    void confirmClicked(bool result, int /*id*/) override
    {
        auto& gui = static_cast<GuiScreenResourcePacks&>(*screen);
        EntryList& list2 = gui.getListContaining(entry.get());
        Minecraft::getMinecraft()->displayGuiScreen(screen);
        if (result) {
            std::erase(list2, entry);
            gui.getSelectedResourcePacks().insert(gui.getSelectedResourcePacks().begin(), entry);
        }
    }

private:
    std::shared_ptr<GuiScreen> screen;
    std::shared_ptr<ResourcePackListEntry> entry;
};

} // namespace

ResourcePackListEntry::ResourcePackListEntry(GuiScreenResourcePacks& resourcePacksGUIIn)
    : mc(*Minecraft::getMinecraft())
    , resourcePacksGUI(resourcePacksGUIIn)
{
}

void ResourcePackListEntry::drawEntry(int /*slotIndex*/, int x, int y, int listWidth, int slotHeight, int mouseX, int mouseY, bool isSelected)
{
    const int i = getPackFormat();
    if (i != 1) {
        DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
        Gui::drawRect(x - 1, y - 1, x + listWidth - 9, y + slotHeight + 1, -8978432);
    }

    bindIcon();
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    Gui::drawModalRectWithCustomSizedTexture(x, y, 0.0f, 0.0f, 32, 32, 32.0f, 32.0f);
    std::string s = getName();
    std::string s1 = getDescription();

    if ((mc.gameSettings->touchscreen || isSelected) && canBeMoved()) {
        mc.getTextureManager().bindTexture(RESOURCE_PACKS_TEXTURE);
        Gui::drawRect(x, y, x + 32, y + 32, -1601138544);
        DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
        const int j = mouseX - x;
        const int k = mouseY - y;

        if (i < 1) {
            s = formatted("resourcePack.incompatible");
            s1 = formatted("resourcePack.incompatible.old");
        } else if (i > 1) {
            s = formatted("resourcePack.incompatible");
            s1 = formatted("resourcePack.incompatible.new");
        }

        if (canBeSelected()) {
            if (j < 32)
                Gui::drawModalRectWithCustomSizedTexture(x, y, 0.0f, 32.0f, 32, 32, 256.0f, 256.0f);
            else
                Gui::drawModalRectWithCustomSizedTexture(x, y, 0.0f, 0.0f, 32, 32, 256.0f, 256.0f);
        } else {
            if (canBeDeselected()) {
                if (j < 16)
                    Gui::drawModalRectWithCustomSizedTexture(x, y, 32.0f, 32.0f, 32, 32, 256.0f, 256.0f);
                else
                    Gui::drawModalRectWithCustomSizedTexture(x, y, 32.0f, 0.0f, 32, 32, 256.0f, 256.0f);
            }

            if (canMoveUp()) {
                if (j < 32 && j > 16 && k < 16)
                    Gui::drawModalRectWithCustomSizedTexture(x, y, 96.0f, 32.0f, 32, 32, 256.0f, 256.0f);
                else
                    Gui::drawModalRectWithCustomSizedTexture(x, y, 96.0f, 0.0f, 32, 32, 256.0f, 256.0f);
            }

            if (canMoveDown()) {
                if (j < 32 && j > 16 && k > 16)
                    Gui::drawModalRectWithCustomSizedTexture(x, y, 64.0f, 32.0f, 32, 32, 256.0f, 256.0f);
                else
                    Gui::drawModalRectWithCustomSizedTexture(x, y, 64.0f, 0.0f, 32, 32, 256.0f, 256.0f);
            }
        }
    }

    FontRenderer& font = *mc.fontRendererObj;
    const int i1 = font.getStringWidth(s);
    if (i1 > 157)
        s = font.trimStringToWidth(s, 157 - font.getStringWidth("...")) + "...";

    font.drawStringWithShadow(s, static_cast<float>(x + 32 + 2), static_cast<float>(y + 1), 16777215);
    const std::vector<std::string> list = font.listFormattedStringToWidth(s1, 157);
    for (size_t l = 0; l < 2 && l < list.size(); ++l)
        font.drawStringWithShadow(list[l], static_cast<float>(x + 32 + 2), static_cast<float>(y + 12 + 10 * static_cast<int>(l)), 8421504);
}

bool ResourcePackListEntry::canBeSelected() const
{
    return !resourcePacksGUI.hasResourcePackEntry(this);
}

bool ResourcePackListEntry::canBeDeselected() const
{
    return resourcePacksGUI.hasResourcePackEntry(this);
}

bool ResourcePackListEntry::canMoveUp() const
{
    const EntryList& list = resourcePacksGUI.getListContaining(this);
    const ptrdiff_t i = indexOf(list, this);
    return i > 0 && list[static_cast<size_t>(i - 1)]->canBeMoved();
}

bool ResourcePackListEntry::canMoveDown() const
{
    const EntryList& list = resourcePacksGUI.getListContaining(this);
    const ptrdiff_t i = indexOf(list, this);
    return i >= 0 && i < static_cast<ptrdiff_t>(list.size()) - 1 && list[static_cast<size_t>(i + 1)]->canBeMoved();
}

bool ResourcePackListEntry::mousePressed(int /*slotIndex*/, int /*x*/, int /*y*/, int /*mouseEvent*/, int relativeX, int relativeY)
{
    if (!canBeMoved() || relativeX > 32)
        return false;

    // The lists hold the entries; keep this one alive while it moves between them.
    const std::shared_ptr<ResourcePackListEntry> self = shared_from_this();

    if (canBeSelected()) {
        resourcePacksGUI.markChanged();
        const int j = getPackFormat();
        if (j != 1) {
            const std::string s1 = I18n::format("resourcePack.incompatible.confirm.title");
            const std::string s = I18n::format("resourcePack.incompatible.confirm." + std::string(j > 1 ? "new" : "old"));
            mc.displayGuiScreen(std::make_shared<GuiYesNo>(std::make_shared<IncompatibleCallback>(resourcePacksGUI.shared_from_this(), self), s1, s, 0));
        } else {
            std::erase(resourcePacksGUI.getListContaining(this), self);
            resourcePacksGUI.getSelectedResourcePacks().insert(resourcePacksGUI.getSelectedResourcePacks().begin(), self);
        }
        return true;
    }

    if (relativeX < 16 && canBeDeselected()) {
        std::erase(resourcePacksGUI.getListContaining(this), self);
        resourcePacksGUI.getAvailableResourcePacks().insert(resourcePacksGUI.getAvailableResourcePacks().begin(), self);
        resourcePacksGUI.markChanged();
        return true;
    }

    if (relativeX > 16 && relativeY < 16 && canMoveUp()) {
        EntryList& list1 = resourcePacksGUI.getListContaining(this);
        const ptrdiff_t k = indexOf(list1, this);
        list1.erase(list1.begin() + k);
        list1.insert(list1.begin() + (k - 1), self);
        resourcePacksGUI.markChanged();
        return true;
    }

    if (relativeX > 16 && relativeY > 16 && canMoveDown()) {
        EntryList& list = resourcePacksGUI.getListContaining(this);
        const ptrdiff_t i = indexOf(list, this);
        list.erase(list.begin() + i);
        list.insert(list.begin() + (i + 1), self);
        resourcePacksGUI.markChanged();
        return true;
    }

    return false;
}

// --- ResourcePackListEntryDefault ---

ResourcePackListEntryDefault::ResourcePackListEntryDefault(GuiScreenResourcePacks& resourcePacksGUIIn)
    : ResourcePackListEntry(resourcePacksGUIIn)
    , defaultPack(mc.getResourcePackRepository().rprDefaultResourcePack)
    , resourcePackIcon([&] {
        std::shared_ptr<DynamicTexture> dynamictexture;
        try {
            dynamictexture = std::make_shared<DynamicTexture>(defaultPack->getPackImage());
        } catch (const std::exception&) {
            dynamictexture = std::shared_ptr<DynamicTexture>(std::shared_ptr<DynamicTexture> {}, &TextureUtil::missingTexture());
        }
        return mc.getTextureManager().getDynamicTextureLocation("texturepackicon", dynamictexture);
    }())
{
}

std::string ResourcePackListEntryDefault::getDescription() const
{
    try {
        if (const auto section = defaultPack->getPackMetadata("pack"))
            return PackMetadataSection::deserialize(*section).packDescription->getFormattedText();
    } catch (const std::exception& e) {
        Log::error("Couldn't load metadata info ({})", e.what());
    }
    return EnumChatFormatting::toString('c') + "Missing pack.mcmeta :(";
}

void ResourcePackListEntryDefault::bindIcon()
{
    mc.getTextureManager().bindTexture(resourcePackIcon);
}

// --- ResourcePackListEntryFound ---

ResourcePackListEntryFound::ResourcePackListEntryFound(GuiScreenResourcePacks& resourcePacksGUIIn, std::shared_ptr<ResourcePackRepository::Entry> entryIn)
    : ResourcePackListEntry(resourcePacksGUIIn)
    , entry(std::move(entryIn))
{
}

void ResourcePackListEntryFound::bindIcon()
{
    entry->bindTexturePackIcon(mc.getTextureManager());
}

} // namespace mc
