// The item registry against the original: tests/reference/items.txt is written by the MCP code
// (tools/reference/net/minecraft/item/ItemDump.java); this writes the same text from the port and compares it
// item by item, then the block-to-item table.

#include "block/Block.h"
#include "core/JavaString.h"
#include "creativetab/CreativeTabs.h"
#include "init/Bootstrap.h"
#include "item/AllItems.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

using namespace mc;

namespace {

const char* b(bool value)
{
    return value ? "true" : "false";
}

std::string f(float value)
{
    return java::toString(value);
}

std::string itemName(const Item* item)
{
    const auto name = item ? Item::getNameForItem(item) : std::nullopt;
    return name ? name->toString() : "null";
}

std::string dumpItem(Item& item)
{
    std::ostringstream out;
    const int id = Item::getIdFromItem(&item);
    const ItemStack stack(&item, 1, 0);
    const CreativeTabs* tab = item.getCreativeTab();
    out << "item " << id << " " << itemName(&item) << " " << item.getClassName() << "\n";
    out << " unlocalized=" << item.getUnlocalizedName() << " tab=" << (tab ? tab->getTabLabel() : "none") << " stack=" << item.getItemStackLimit()
        << " damage=" << item.getMaxDamage() << " subtypes=" << b(item.getHasSubtypes()) << " full3d=" << b(item.isFull3D())
        << " rotate=" << b(item.shouldRotateAroundWhenRendering()) << " container=" << itemName(item.getContainerItem())
        << " enchantability=" << item.getItemEnchantability() << " map=" << b(item.isMap()) << " shareTag=" << b(item.getShareTag())
        << " damageable=" << b(item.isDamageable()) << " tool=" << b(item.isItemTool(stack)) << " editBlocks=" << b(item.canItemEditBlocks()) << "\n";
    const auto potion = item.getPotionEffect(stack);
    out << " use=" << getActionName(item.getItemUseAction(stack)) << " duration=" << item.getMaxItemUseDuration(stack)
        << " rarity=" << info(item.getRarity(stack)).enumName << " effect=" << b(item.hasEffect(stack)) << " potion=" << (potion ? *potion : "null") << "\n";
    if (const auto* itemBlock = dynamic_cast<const ItemBlock*>(&item))
        out << " block=" << Block::getNameForBlock(itemBlock->getBlock()) << "\n";
    if (const auto* food = dynamic_cast<const ItemFood*>(&item)) {
        out << " food heal=" << food->getHealAmount(stack) << " saturation=" << f(food->getSaturationModifier(stack)) << " wolf=" << b(food->isWolfsFavoriteMeat())
            << " always=" << b(food->isAlwaysEdible()) << " potionId=" << food->getPotionId() << " potionDuration=" << food->getPotionDuration()
            << " potionAmplifier=" << food->getPotionAmplifier() << " potionChance=" << f(food->getPotionEffectProbability()) << "\n";
    }
    if (const auto* armor = dynamic_cast<const ItemArmor*>(&item)) {
        const ItemArmor::ArmorMaterial& material = armor->getArmorMaterial();
        out << " armor type=" << armor->armorType << " reduce=" << armor->damageReduceAmount << " renderIndex=" << armor->renderIndex
            << " material=" << material.name() << " durability=" << material.getDurability(armor->armorType) << " enchantability=" << material.getEnchantability()
            << "\n";
    }
    if (const auto* tool = dynamic_cast<const ItemTool*>(&item)) {
        out << " tool efficiency=" << f(tool->getEfficiencyOnProperMaterial()) << " damageVsEntity=" << f(tool->getDamageVsEntityValue())
            << " material=" << tool->getToolMaterial().name() << "\n";
    }
    if (const auto* sword = dynamic_cast<const ItemSword*>(&item))
        out << " sword damage=" << f(sword->getDamageVsEntity()) << " material=" << sword->getToolMaterialName() << "\n";
    if (const auto* hoe = dynamic_cast<const ItemHoe*>(&item))
        out << " hoe material=" << hoe->getMaterialName() << "\n";
    std::string speeds;
    std::string harvest;
    for (const Block* block : Block::blockRegistry.values()) {
        const std::string path = Block::blockRegistry.getNameForObject(block)->getResourcePath();
        const float speed = item.getStrVsBlock(stack, block);
        if (speed != 1.0f)
            speeds += " " + path + "=" + f(speed);
        if (item.canHarvestBlock(block))
            harvest += " " + path;
    }
    if (!speeds.empty())
        out << " speeds" << speeds << "\n";
    if (!harvest.empty())
        out << " harvest" << harvest << "\n";
    if (tab) {
        std::vector<ItemStack> subs;
        item.getSubItems(&item, tab, subs);
        for (const ItemStack& sub : subs) {
            out << " sub damage=" << sub.getItemDamage() << " meta=" << sub.getMetadata() << " count=" << sub.stackSize << " name=" << sub.getUnlocalizedName()
                << " color0=" << sub.getItem()->getColorFromItemStack(sub, 0) << " color1=" << sub.getItem()->getColorFromItemStack(sub, 1)
                << " effect=" << b(sub.hasEffect()) << " rarity=" << info(sub.getRarity()).enumName
                << " tag=" << (sub.getTagCompound() ? sub.getTagCompound()->toString() : "null") << "\n";
        }
    }
    return out.str();
}

std::map<int, std::string> referenceSections(std::string& blockItemLines)
{
    std::ifstream file(std::filesystem::path(__FILE__).parent_path() / "reference" / "items.txt");
    REQUIRE(file);
    std::map<int, std::string> sections;
    std::string line;
    int current = -1;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.starts_with("item ")) {
            current = std::stoi(line.substr(5));
            sections[current] = line + "\n";
        } else if (line.starts_with("blockitem ")) {
            current = -1;
            blockItemLines += line + "\n";
        } else if (current >= 0) {
            sections[current] += line + "\n";
        }
    }
    return sections;
}

} // namespace

TEST_CASE("Item registry matches the original")
{
    Bootstrap::registerAll();
    std::string blockItemLines;
    const auto expected = referenceSections(blockItemLines);

    int ported = 0;
    for (Item* item : Item::itemRegistry.values()) {
        const int id = Item::getIdFromItem(item);
        CAPTURE(id);
        const auto it = expected.find(id);
        REQUIRE(it != expected.end());
        const std::string actual = dumpItem(*item);
        if (actual != it->second) {
            std::istringstream a(actual), e(it->second);
            std::string la, le;
            while (std::getline(a, la) && std::getline(e, le)) {
                if (la != le)
                    break;
            }
            CHECK(la == le);
        }
        ++ported;
    }
    CHECK(ported == static_cast<int>(expected.size()));

    std::string actualBlockItems;
    for (const Block* block : Block::blockRegistry.values())
        actualBlockItems += "blockitem " + Block::getNameForBlock(block) + " " + itemName(Item::getItemFromBlock(block)) + "\n";
    CHECK(actualBlockItems == blockItemLines);
}

TEST_CASE("ItemStack NBT round trip")
{
    Bootstrap::registerAll();
    ItemStack stack(Item::getByNameOrId("minecraft:diamond_sword"), 1, 17);
    stack.setStackDisplayName("Blade");
    stack.addEnchantment(16, 3);
    NBTTagCompound nbt;
    stack.writeToNBT(nbt);
    CHECK(nbt.toString() == "{id:\"minecraft:diamond_sword\",Count:1b,tag:{ench:[0:{lvl:3s,id:16s}],display:{Name:\"Blade\"}},Damage:17s}");
    const auto loaded = ItemStack::loadItemStackFromNBT(nbt);
    REQUIRE(loaded);
    CHECK(ItemStack::areItemStacksEqual(loaded.get(), &stack));
    CHECK(loaded->getDisplayName() == "Blade");
    CHECK(loaded->getRarity() == EnumRarity::RARE);
    CHECK(loaded->toString() == "1xitem.swordDiamond@17");

    NBTTagCompound unknown;
    unknown.setString("id", "minecraft:no_such_item");
    CHECK(ItemStack::loadItemStackFromNBT(unknown) == nullptr);

    NBTTagCompound numeric;
    numeric.setShort("id", 1);
    numeric.setByte("Count", 64);
    const auto stone = ItemStack::loadItemStackFromNBT(numeric);
    REQUIRE(stone);
    CHECK(stone->getUnlocalizedName() == "tile.stone.stone");
    CHECK(stone->stackSize == 64);
}
