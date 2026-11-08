#include "item/ItemArmor.h"

#include "creativetab/CreativeTabs.h"
#include "init/Items.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"

#include <stdexcept>

namespace mc {

namespace {

constexpr int kMaxDamageArray[4] { 11, 16, 15, 13 };

} // namespace

const ItemArmor::ArmorMaterial ItemArmor::ArmorMaterial::LEATHER("LEATHER", "leather", 5, { 1, 3, 2, 1 }, 15);
const ItemArmor::ArmorMaterial ItemArmor::ArmorMaterial::CHAIN("CHAIN", "chainmail", 15, { 2, 5, 4, 1 }, 12);
const ItemArmor::ArmorMaterial ItemArmor::ArmorMaterial::IRON("IRON", "iron", 15, { 2, 6, 5, 2 }, 9);
const ItemArmor::ArmorMaterial ItemArmor::ArmorMaterial::GOLD("GOLD", "gold", 7, { 2, 5, 3, 1 }, 25);
const ItemArmor::ArmorMaterial ItemArmor::ArmorMaterial::DIAMOND("DIAMOND", "diamond", 33, { 3, 8, 6, 3 }, 10);

int ItemArmor::ArmorMaterial::getDurability(int armorType) const
{
    return kMaxDamageArray[armorType] * maxDamageFactor;
}

const Item* ItemArmor::ArmorMaterial::getRepairItem() const
{
    if (this == &LEATHER)
        return Items::leather;
    if (this == &CHAIN)
        return Items::iron_ingot;
    if (this == &GOLD)
        return Items::gold_ingot;
    if (this == &IRON)
        return Items::iron_ingot;
    if (this == &DIAMOND)
        return Items::diamond;
    return nullptr;
}

ItemArmor::ItemArmor(const ArmorMaterial& materialIn, int renderIndexIn, int armorTypeIn)
    : armorType(armorTypeIn)
    , damageReduceAmount(materialIn.getDamageReductionAmount(armorTypeIn))
    , renderIndex(renderIndexIn)
    , material(materialIn)
{
    setMaxDamage(materialIn.getDurability(armorTypeIn));
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabCombat);
}

int ItemArmor::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    if (renderPass > 0)
        return 16777215;
    int i = getColor(stack);
    if (i < 0)
        i = 16777215;
    return i;
}

bool ItemArmor::hasColor(const ItemStack& stack) const
{
    if (&material != &ArmorMaterial::LEATHER || !stack.hasTagCompound())
        return false;
    const auto& tag = stack.getTagCompound();
    return tag->hasKey("display", 10) && tag->getCompoundTag("display")->hasKey("color", 3);
}

int ItemArmor::getColor(const ItemStack& stack) const
{
    if (&material != &ArmorMaterial::LEATHER)
        return -1;
    if (const auto& nbttagcompound = stack.getTagCompound()) {
        const auto nbttagcompound1 = nbttagcompound->getCompoundTag("display");
        if (nbttagcompound1->hasKey("color", 3))
            return nbttagcompound1->getInteger("color");
    }
    return 10511680;
}

void ItemArmor::removeColor(ItemStack& stack) const
{
    if (&material == &ArmorMaterial::LEATHER) {
        if (const auto& nbttagcompound = stack.getTagCompound()) {
            const auto nbttagcompound1 = nbttagcompound->getCompoundTag("display");
            if (nbttagcompound1->hasKey("color"))
                nbttagcompound1->removeTag("color");
        }
    }
}

void ItemArmor::setColor(ItemStack& stack, int color) const
{
    if (&material != &ArmorMaterial::LEATHER)
        throw std::logic_error("Can't dye non-leather!");
    auto nbttagcompound = stack.getTagCompound();
    if (!nbttagcompound) {
        nbttagcompound = std::make_shared<NBTTagCompound>();
        stack.setTagCompound(nbttagcompound);
    }
    auto nbttagcompound1 = nbttagcompound->getCompoundTag("display");
    if (!nbttagcompound->hasKey("display", 10))
        nbttagcompound->setTag("display", nbttagcompound1);
    nbttagcompound1->setInteger("color", color);
}

bool ItemArmor::getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const
{
    return material.getRepairItem() == repair.getItem() ? true : Item::getIsRepairable(toRepair, repair);
}

} // namespace mc
