#include "item/Item.h"

#include "block/Block.h"
#include "client/resources/Locale.h"
#include "core/JavaString.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/ItemBlock.h"
#include "item/ItemStack.h"
#include "util/Random.h"

#include <stdexcept>
#include <unordered_map>

namespace mc {

namespace {

std::vector<std::unique_ptr<Item>>& ownedItems()
{
    static std::vector<std::unique_ptr<Item>> items;
    return items;
}

// Java: BLOCK_TO_ITEM, a HashMap only used for lookups.
std::unordered_map<const Block*, Item*>& blockToItem()
{
    static std::unordered_map<const Block*, Item*> map;
    return map;
}

} // namespace

// --- ToolMaterial ---

const Item::ToolMaterial Item::ToolMaterial::WOOD("WOOD", 0, 59, 2.0f, 0.0f, 15);
const Item::ToolMaterial Item::ToolMaterial::STONE("STONE", 1, 131, 4.0f, 1.0f, 5);
const Item::ToolMaterial Item::ToolMaterial::IRON("IRON", 2, 250, 6.0f, 2.0f, 14);
const Item::ToolMaterial Item::ToolMaterial::EMERALD("EMERALD", 3, 1561, 8.0f, 3.0f, 10);
const Item::ToolMaterial Item::ToolMaterial::GOLD("GOLD", 0, 32, 12.0f, 0.0f, 22);

const Item* Item::ToolMaterial::getRepairItem() const
{
    if (this == &WOOD)
        return Item::getItemFromBlock(Blocks::planks);
    if (this == &STONE)
        return Item::getItemFromBlock(Blocks::cobblestone);
    if (this == &GOLD)
        return Items::gold_ingot;
    if (this == &IRON)
        return Items::iron_ingot;
    if (this == &EMERALD)
        return Items::diamond;
    return nullptr;
}

// --- Registry ---

RegistryNamespaced<ResourceLocation, Item> Item::itemRegistry;

Random& Item::itemRand()
{
    static Random random;
    return random;
}

int Item::getIdFromItem(const Item* item)
{
    return item ? itemRegistry.getIDForObject(item) : 0;
}

Item* Item::getItemById(int id)
{
    return itemRegistry.getObjectById(id);
}

Item* Item::getItemFromBlock(const Block* block)
{
    const auto it = blockToItem().find(block);
    return it == blockToItem().end() ? nullptr : it->second;
}

Item* Item::getByNameOrId(std::string_view id)
{
    Item* item = itemRegistry.getObject(ResourceLocation(id));
    if (!item) {
        try {
            return getItemById(java::parseInt(id));
        } catch (const std::invalid_argument&) {
        }
    }
    return item;
}

void Item::registerItemBlock(Block* block)
{
    registerItemBlock(block, new ItemBlock(block));
}

void Item::registerItemBlock(Block* block, Item* item)
{
    registerItem(Block::getIdFromBlock(block), *Block::blockRegistry.getNameForObject(block), item);
    blockToItem()[block] = item;
}

void Item::registerItem(int id, const std::string& textualID, Item* item)
{
    registerItem(id, ResourceLocation(textualID), item);
}

void Item::registerItem(int id, const ResourceLocation& textualID, Item* item)
{
    ownedItems().emplace_back(item);
    itemRegistry.registerObject(id, textualID, item);
}

// --- Item ---

float Item::getStrVsBlock(const ItemStack&, const Block*) const
{
    return 1.0f;
}

int Item::getMetadata(int) const
{
    return 0;
}

bool Item::canHarvestBlock(const Block*) const
{
    return false;
}

std::string Item::getUnlocalizedNameInefficiently(const ItemStack& stack) const
{
    return StatCollector::translateToLocal(getUnlocalizedName(stack));
}

std::string Item::getUnlocalizedName(const ItemStack&) const
{
    return "item." + unlocalizedName;
}

int Item::getColorFromItemStack(const ItemStack&, int) const
{
    return 16777215;
}

EnumAction Item::getItemUseAction(const ItemStack&) const
{
    return EnumAction::NONE;
}

int Item::getMaxItemUseDuration(const ItemStack&) const
{
    return 0;
}

std::optional<std::string> Item::getPotionEffect(const ItemStack&) const
{
    return potionEffect;
}

std::string Item::getItemStackDisplayName(const ItemStack& stack) const
{
    return java::trim(StatCollector::translateToLocal(getUnlocalizedNameInefficiently(stack) + ".name"));
}

bool Item::hasEffect(const ItemStack& stack) const
{
    return stack.isItemEnchanted();
}

EnumRarity Item::getRarity(const ItemStack& stack) const
{
    return stack.isItemEnchanted() ? EnumRarity::RARE : EnumRarity::COMMON;
}

bool Item::isItemTool(const ItemStack&) const
{
    return getItemStackLimit() == 1 && isDamageable();
}

void Item::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    subItems.emplace_back(item, 1, 0);
}

bool Item::getIsRepairable(const ItemStack&, const ItemStack&) const
{
    return false;
}

} // namespace mc
