#include "item/ItemPotion.h"

#include "client/resources/Locale.h"
#include "core/JavaString.h"
#include "creativetab/CreativeTabs.h"
#include "entity/player/EntityPlayer.h"
#include "init/Items.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "potion/PotionHelper.h"
#include "world/World.h"

namespace mc {

ItemPotion::ItemPotion()
{
    setMaxStackSize(1);
    setHasSubtypes(true);
    setMaxDamage(0);
    setCreativeTab(&CreativeTabs::tabBrewing);
}

std::vector<PotionEffect> ItemPotion::getEffects(const ItemStack& stack) const
{
    if (stack.hasTagCompound() && stack.getTagCompound()->hasKey("CustomPotionEffects", 9)) {
        std::vector<PotionEffect> list1;
        const auto nbttaglist = stack.getTagCompound()->getTagList("CustomPotionEffects", 10);
        for (int i = 0; i < nbttaglist->tagCount(); ++i) {
            if (const auto potioneffect = PotionEffect::readCustomPotionEffectFromNBT(*nbttaglist->getCompoundTagAt(i)))
                list1.push_back(*potioneffect);
        }
        return list1;
    }
    return getEffects(stack.getMetadata());
}

const std::vector<PotionEffect>& ItemPotion::getEffects(int meta) const
{
    auto it = effectCache.find(meta);
    if (it == effectCache.end())
        it = effectCache.emplace(meta, PotionHelper::getPotionEffects(meta, false)).first;
    return it->second;
}

int ItemPotion::getColorFromDamage(int meta) const
{
    return PotionHelper::getLiquidColor(meta, false);
}

int ItemPotion::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    return renderPass > 0 ? 16777215 : getColorFromDamage(stack.getMetadata());
}

bool ItemPotion::isEffectInstant(int meta) const
{
    for (const PotionEffect& potioneffect : getEffects(meta)) {
        if (Potion::potionTypes[static_cast<size_t>(potioneffect.getPotionID())]->isInstant())
            return true;
    }
    return false;
}

std::string ItemPotion::getItemStackDisplayName(const ItemStack& stack) const
{
    if (stack.getMetadata() == 0)
        return java::trim(StatCollector::translateToLocal("item.emptyPotion.name"));
    std::string s;
    if (isSplash(stack.getMetadata()))
        s = java::trim(StatCollector::translateToLocal("potion.prefix.grenade")) + " ";
    const std::vector<PotionEffect> list = Items::potionitem->getEffects(stack);
    if (!list.empty())
        return s + java::trim(StatCollector::translateToLocal(list.front().getEffectName() + ".postfix"));
    const std::string s1 = PotionHelper::getPotionPrefix(stack.getMetadata());
    return java::trim(StatCollector::translateToLocal(s1)) + " " + Item::getItemStackDisplayName(stack);
}

bool ItemPotion::hasEffect(const ItemStack& stack) const
{
    return !getEffects(stack).empty();
}

void ItemPotion::getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const
{
    Item::getSubItems(item, tab, subItems);
    // Java: a static LinkedHashMap keyed by the effect list -- a repeated list keeps its first position and takes
    // the newer damage value.
    static std::vector<std::pair<std::vector<PotionEffect>, int>> subItemsCache;
    if (subItemsCache.empty()) {
        for (int i = 0; i <= 15; ++i) {
            for (int j = 0; j <= 1; ++j) {
                const int base = j == 0 ? i | 8192 : i | 16384;
                for (int l = 0; l <= 2; ++l) {
                    int i1 = base;
                    if (l == 1)
                        i1 = base | 32;
                    else if (l == 2)
                        i1 = base | 64;
                    std::vector<PotionEffect> list = PotionHelper::getPotionEffects(i1, false);
                    if (!list.empty()) {
                        bool found = false;
                        for (auto& [key, value] : subItemsCache) {
                            if (key == list) {
                                value = i1;
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                            subItemsCache.emplace_back(std::move(list), i1);
                    }
                }
            }
        }
    }
    for (const auto& entry : subItemsCache)
        subItems.emplace_back(item, 1, entry.second);
}

std::shared_ptr<ItemStack> ItemPotion::onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn)
{
    if (!playerIn->capabilities.isCreativeMode)
        --stack->stackSize;

    if (!worldIn->isRemote) {
        for (const PotionEffect& potioneffect : getEffects(*stack))
            playerIn->addPotionEffect(PotionEffect::copyOf(potioneffect));
    }

    // Java: triggerAchievement(StatList.objectUseStats[...]) -- stage 12.
    if (!playerIn->capabilities.isCreativeMode) {
        if (stack->stackSize <= 0)
            return std::make_shared<ItemStack>(Items::glass_bottle);
        playerIn->inventory.addItemStackToInventory(std::make_shared<ItemStack>(Items::glass_bottle));
    }

    return stack;
}

std::shared_ptr<ItemStack> ItemPotion::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    if (isSplash(itemStackIn->getMetadata())) {
        if (!playerIn->capabilities.isCreativeMode)
            --itemStackIn->stackSize;
        worldIn->playSoundAtEntity(playerIn, "random.bow", 0.5f, 0.4f / (itemRand().nextFloat() * 0.4f + 0.8f));
        // Java: spawns an EntityPotion (server side) -- the thrown entities, stage 10.
        return itemStackIn;
    }

    playerIn->setItemInUse(itemStackIn, getMaxItemUseDuration(*itemStackIn));
    return itemStackIn;
}

} // namespace mc
