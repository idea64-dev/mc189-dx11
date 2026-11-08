#include "entity/SharedMonsterAttributes.h"

#include "core/Log.h"
#include "nbt/NBTTagCompound.h"

#include <stdexcept>

namespace mc {

const RangedAttribute& SharedMonsterAttributes::maxHealth()
{
    static const RangedAttribute attribute = std::move(RangedAttribute(nullptr, "generic.maxHealth", 20.0, 0.0, 1024.0).setDescription("Max Health").setShouldWatch(true));
    return attribute;
}

const RangedAttribute& SharedMonsterAttributes::followRange()
{
    static const RangedAttribute attribute = std::move(RangedAttribute(nullptr, "generic.followRange", 32.0, 0.0, 2048.0).setDescription("Follow Range"));
    return attribute;
}

const RangedAttribute& SharedMonsterAttributes::knockbackResistance()
{
    static const RangedAttribute attribute = std::move(RangedAttribute(nullptr, "generic.knockbackResistance", 0.0, 0.0, 1.0).setDescription("Knockback Resistance"));
    return attribute;
}

const RangedAttribute& SharedMonsterAttributes::movementSpeed()
{
    static const RangedAttribute attribute =
        std::move(RangedAttribute(nullptr, "generic.movementSpeed", 0.699999988079071, 0.0, 1024.0).setDescription("Movement Speed").setShouldWatch(true));
    return attribute;
}

const RangedAttribute& SharedMonsterAttributes::attackDamage()
{
    static const RangedAttribute attribute(nullptr, "generic.attackDamage", 2.0, 0.0, 2048.0);
    return attribute;
}

namespace {

std::shared_ptr<NBTTagCompound> writeAttributeInstanceToNBT(const IAttributeInstance& instance)
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    nbttagcompound->setString("Name", instance.getAttribute().getAttributeUnlocalizedName());
    nbttagcompound->setDouble("Base", instance.getBaseValue());
    const std::vector<AttributeModifier> collection = instance.func_111122_c();
    if (!collection.empty()) {
        auto nbttaglist = std::make_shared<NBTTagList>();
        for (const AttributeModifier& modifier : collection) {
            if (modifier.isSaved())
                nbttaglist->appendTag(SharedMonsterAttributes::writeAttributeModifierToNBT(modifier));
        }
        nbttagcompound->setTag("Modifiers", nbttaglist);
    }
    return nbttagcompound;
}

void applyModifiersToAttributeInstance(IAttributeInstance& instance, const NBTTagCompound& compound)
{
    instance.setBaseValue(compound.getDouble("Base"));
    if (compound.hasKey("Modifiers", 9)) {
        const auto nbttaglist = compound.getTagList("Modifiers", 10);
        for (int i = 0; i < nbttaglist->tagCount(); ++i) {
            if (const auto modifier = SharedMonsterAttributes::readAttributeModifierFromNBT(*nbttaglist->getCompoundTagAt(i))) {
                if (const AttributeModifier* existing = instance.getModifier(modifier->getID())) {
                    const AttributeModifier copy = *existing;
                    instance.removeModifier(copy);
                }
                instance.applyModifier(*modifier);
            }
        }
    }
}

} // namespace

std::shared_ptr<NBTTagList> SharedMonsterAttributes::writeBaseAttributeMapToNBT(const BaseAttributeMap& map)
{
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (const IAttributeInstance* instance : map.getAllAttributes())
        nbttaglist->appendTag(writeAttributeInstanceToNBT(*instance));
    return nbttaglist;
}

std::shared_ptr<NBTTagCompound> SharedMonsterAttributes::writeAttributeModifierToNBT(const AttributeModifier& modifier)
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    nbttagcompound->setString("Name", modifier.getName());
    nbttagcompound->setDouble("Amount", modifier.getAmount());
    nbttagcompound->setInteger("Operation", modifier.getOperation());
    nbttagcompound->setLong("UUIDMost", modifier.getID().getMostSignificantBits());
    nbttagcompound->setLong("UUIDLeast", modifier.getID().getLeastSignificantBits());
    return nbttagcompound;
}

void SharedMonsterAttributes::setAttributeModifiers(BaseAttributeMap& map, const NBTTagList& list)
{
    for (int i = 0; i < list.tagCount(); ++i) {
        const auto nbttagcompound = list.getCompoundTagAt(i);
        if (IAttributeInstance* instance = map.getAttributeInstanceByName(nbttagcompound->getString("Name")))
            applyModifiersToAttributeInstance(*instance, *nbttagcompound);
        else
            Log::warn("Ignoring unknown attribute '{}'", nbttagcompound->getString("Name"));
    }
}

std::optional<AttributeModifier> SharedMonsterAttributes::readAttributeModifierFromNBT(const NBTTagCompound& compound)
{
    const UUID uuid(compound.getLong("UUIDMost"), compound.getLong("UUIDLeast"));
    try {
        return AttributeModifier(uuid, compound.getString("Name"), compound.getDouble("Amount"), compound.getInteger("Operation"));
    } catch (const std::exception& exception) {
        Log::warn("Unable to create attribute: {}", exception.what());
        return std::nullopt;
    }
}

} // namespace mc
