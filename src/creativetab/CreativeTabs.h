#pragma once

// Original: net/minecraft/creativetab/CreativeTabs.java
// Icons (getTabIconItem / getIconItemStack), enchantment types and displayAllReleventItems come with the items
// and the creative inventory (stages 4 and 10).

#include <array>
#include <string>

namespace mc {

class CreativeTabs {
public:
    static std::array<const CreativeTabs*, 12> creativeTabArray;
    static const CreativeTabs tabBlock, tabDecorations, tabRedstone, tabTransport, tabMisc, tabAllSearch, tabFood, tabTools, tabCombat,
        tabBrewing, tabMaterials, tabInventory;

    int getTabIndex() const { return tabIndex; }
    const std::string& getTabLabel() const { return tabLabel; }
    std::string getTranslatedTabLabel() const { return "itemGroup." + tabLabel; }
    const std::string& getBackgroundImageName() const { return theTexture; }
    bool drawInForegroundOfTab() const { return drawTitle; }
    bool shouldHidePlayerInventory() const { return hasScrollbar; }
    int getTabColumn() const { return tabIndex % 6; }
    bool isTabInFirstRow() const { return tabIndex < 6; }

private:
    CreativeTabs(int index, std::string label, std::string texture = "items.png", bool scrollbar = true, bool title = true);

    int tabIndex;
    std::string tabLabel;
    std::string theTexture;
    bool hasScrollbar;
    bool drawTitle;
};

} // namespace mc
