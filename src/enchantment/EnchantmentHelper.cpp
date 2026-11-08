#include "enchantment/EnchantmentHelper.h"

#include "core/JavaHashMap.h"
#include "init/Items.h"
#include "item/ItemMisc.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "util/Random.h"
#include "util/WeightedRandomChestContent.h"

#include <algorithm>

namespace mc {

namespace {

using EnchantmentMap = JavaHashMap<int, EnchantmentData>;

std::vector<EnchantmentData> valuesOf(const EnchantmentMap& map)
{
    std::vector<EnchantmentData> values;
    values.reserve(map.size());
    for (const auto& entry : map)
        values.push_back(entry.second);
    return values;
}

EnchantmentMap buildMap(int level, const ItemStack& stack)
{
    const Item* item = stack.getItem();
    EnchantmentMap map;
    const bool flag = stack.getItem() == Items::book;
    for (Enchantment* enchantment : Enchantment::getEnchantmentsBookList()) {
        if (enchantment && (canEnchantItem(enchantment->type, item) || flag)) {
            for (int i = enchantment->getMinLevel(); i <= enchantment->getMaxLevel(); ++i) {
                if (level >= enchantment->getMinEnchantability(i) && level <= enchantment->getMaxEnchantability(i))
                    map.put(enchantment->effectId, EnchantmentData(enchantment, i));
            }
        }
    }
    return map;
}

} // namespace

int EnchantmentHelper::getEnchantmentLevel(int enchID, const ItemStack* stack)
{
    if (!stack)
        return 0;
    const auto nbttaglist = stack->getEnchantmentTagList();
    if (!nbttaglist)
        return 0;
    for (int i = 0; i < nbttaglist->tagCount(); ++i) {
        const int j = nbttaglist->getCompoundTagAt(i)->getShort("id");
        const int k = nbttaglist->getCompoundTagAt(i)->getShort("lvl");
        if (j == enchID)
            return k;
    }
    return 0;
}

std::vector<std::pair<int, int>> EnchantmentHelper::getEnchantments(const ItemStack& stack)
{
    std::vector<std::pair<int, int>> map;
    const auto nbttaglist = stack.getItem() == Items::enchanted_book ? Items::enchanted_book->getEnchantments(stack) : stack.getEnchantmentTagList();
    if (nbttaglist) {
        for (int i = 0; i < nbttaglist->tagCount(); ++i) {
            const int j = nbttaglist->getCompoundTagAt(i)->getShort("id");
            const int k = nbttaglist->getCompoundTagAt(i)->getShort("lvl");
            // LinkedHashMap.put: an existing key keeps its place.
            const auto it = std::find_if(map.begin(), map.end(), [j](const auto& e) { return e.first == j; });
            if (it != map.end())
                it->second = k;
            else
                map.emplace_back(j, k);
        }
    }
    return map;
}

void EnchantmentHelper::setEnchantments(const std::vector<std::pair<int, int>>& enchMap, ItemStack& stack)
{
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (const auto& [i, level] : enchMap) {
        Enchantment* enchantment = Enchantment::getEnchantmentById(i);
        if (enchantment) {
            auto nbttagcompound = std::make_shared<NBTTagCompound>();
            nbttagcompound->setShort("id", static_cast<int16_t>(i));
            nbttagcompound->setShort("lvl", static_cast<int16_t>(level));
            nbttaglist->appendTag(nbttagcompound);
            if (stack.getItem() == Items::enchanted_book)
                Items::enchanted_book->addEnchantment(stack, EnchantmentData(enchantment, level));
        }
    }
    if (nbttaglist->tagCount() > 0) {
        if (stack.getItem() != Items::enchanted_book)
            stack.setTagInfo("ench", nbttaglist);
    } else if (stack.hasTagCompound()) {
        stack.getTagCompound()->removeTag("ench");
    }
}

int EnchantmentHelper::getMaxEnchantmentLevel(int enchID, std::span<const ItemStack* const> stacks)
{
    int i = 0;
    for (const ItemStack* itemstack : stacks) {
        const int j = getEnchantmentLevel(enchID, itemstack);
        if (j > i)
            i = j;
    }
    return i;
}

int EnchantmentHelper::calcItemStackEnchantability(Random& rand, int enchantNum, int power, const ItemStack& stack)
{
    const int i = stack.getItem()->getItemEnchantability();
    if (i <= 0)
        return 0;
    if (power > 15)
        power = 15;
    const int a = rand.nextInt(8);
    const int j = a + 1 + (power >> 1) + rand.nextInt(power + 1);
    return enchantNum == 0 ? std::max(j / 3, 1) : (enchantNum == 1 ? j * 2 / 3 + 1 : std::max(j, power * 2));
}

ItemStack& EnchantmentHelper::addRandomEnchantment(Random& rand, ItemStack& stack, int level)
{
    const std::vector<EnchantmentData> list = buildEnchantmentList(rand, stack, level);
    const bool flag = stack.getItem() == Items::book;
    if (flag)
        stack.setItem(Items::enchanted_book);
    for (const EnchantmentData& enchantmentdata : list) {
        if (flag)
            Items::enchanted_book->addEnchantment(stack, enchantmentdata);
        else
            stack.addEnchantment(enchantmentdata.enchantmentobj->effectId, enchantmentdata.enchantmentLevel);
    }
    return stack;
}

std::vector<EnchantmentData> EnchantmentHelper::buildEnchantmentList(Random& randomIn, const ItemStack& itemStackIn, int level)
{
    int i = itemStackIn.getItem()->getItemEnchantability();
    if (i <= 0)
        return {};
    i = i / 2;
    const int a = randomIn.nextInt((i >> 1) + 1);
    i = 1 + a + randomIn.nextInt((i >> 1) + 1);
    const int j = i + level;
    const float f0 = randomIn.nextFloat();
    const float f = (f0 + randomIn.nextFloat() - 1.0f) * 0.15f;
    int k = static_cast<int>(static_cast<float>(j) * (1.0f + f) + 0.5f);
    if (k < 1)
        k = 1;
    std::vector<EnchantmentData> list;
    EnchantmentMap map = buildMap(k, itemStackIn);
    if (!map.empty()) {
        const std::vector<EnchantmentData> values = valuesOf(map);
        const EnchantmentData* enchantmentdata = WeightedRandom::getRandomItem(randomIn, values);
        if (enchantmentdata) {
            list.push_back(*enchantmentdata);
            for (int l = k; randomIn.nextInt(50) <= l; l >>= 1) {
                std::vector<int> removed;
                for (const auto& entry : map) {
                    bool flag = true;
                    for (const EnchantmentData& enchantmentdata1 : list) {
                        if (!enchantmentdata1.enchantmentobj->canApplyTogether(Enchantment::getEnchantmentById(entry.first))) {
                            flag = false;
                            break;
                        }
                    }
                    if (!flag)
                        removed.push_back(entry.first);
                }
                for (const int key : removed)
                    map.erase(key);
                if (!map.empty()) {
                    const std::vector<EnchantmentData> remaining = valuesOf(map);
                    list.push_back(*WeightedRandom::getRandomItem(randomIn, remaining));
                }
            }
        }
    }
    return list;
}

std::vector<EnchantmentData> EnchantmentHelper::mapEnchantmentData(int level, const ItemStack& stack)
{
    return valuesOf(buildMap(level, stack));
}

} // namespace mc
