#pragma once

// Original: net/minecraft/entity/SharedMonsterAttributes.java -- the common attributes and their NBT.

#include "entity/ai/attributes/Attributes.h"

#include <memory>
#include <optional>

namespace mc {

class NBTTagCompound;
class NBTTagList;

class SharedMonsterAttributes {
public:
    static const RangedAttribute& maxHealth();
    static const RangedAttribute& followRange();
    static const RangedAttribute& knockbackResistance();
    static const RangedAttribute& movementSpeed();
    static const RangedAttribute& attackDamage();

    static std::shared_ptr<NBTTagList> writeBaseAttributeMapToNBT(const BaseAttributeMap& map);
    static void setAttributeModifiers(BaseAttributeMap& map, const NBTTagList& list);
    // Java returns null (with a warning) for an invalid modifier.
    static std::optional<AttributeModifier> readAttributeModifierFromNBT(const NBTTagCompound& compound);
    static std::shared_ptr<NBTTagCompound> writeAttributeModifierToNBT(const AttributeModifier& modifier);
};

} // namespace mc
