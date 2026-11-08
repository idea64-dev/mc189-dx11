#include "creativetab/CreativeTabs.h"

namespace mc {

std::array<const CreativeTabs*, 12> CreativeTabs::creativeTabArray {};

CreativeTabs::CreativeTabs(int index, std::string label, std::string texture, bool scrollbar, bool title)
    : tabIndex(index)
    , tabLabel(std::move(label))
    , theTexture(std::move(texture))
    , hasScrollbar(scrollbar)
    , drawTitle(title)
{
    creativeTabArray[static_cast<size_t>(index)] = this;
}

const CreativeTabs CreativeTabs::tabBlock(0, "buildingBlocks");
const CreativeTabs CreativeTabs::tabDecorations(1, "decorations");
const CreativeTabs CreativeTabs::tabRedstone(2, "redstone");
const CreativeTabs CreativeTabs::tabTransport(3, "transportation");
const CreativeTabs CreativeTabs::tabMisc(4, "misc");
const CreativeTabs CreativeTabs::tabAllSearch(5, "search", "item_search.png");
const CreativeTabs CreativeTabs::tabFood(6, "food");
const CreativeTabs CreativeTabs::tabTools(7, "tools");
const CreativeTabs CreativeTabs::tabCombat(8, "combat");
const CreativeTabs CreativeTabs::tabBrewing(9, "brewing");
const CreativeTabs CreativeTabs::tabMaterials(10, "materials");
const CreativeTabs CreativeTabs::tabInventory(11, "inventory", "inventory.png", false, false);

} // namespace mc
