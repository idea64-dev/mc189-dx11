#pragma once

// Original: net/minecraft/nbt/JsonToNBT.java -- the NBT literal parser of /give, /summon, /blockdata...
// ("{Count:1b,display:{Name:\"x\"}}"). Its quirks are kept: it isn't JSON, works on Java chars, and anything
// that fails to parse as a number becomes a string.

#include "nbt/NBTTagCompound.h"

#include <memory>
#include <string_view>

namespace mc {

class JsonToNBT {
public:
    // Throws NBTException.
    static std::shared_ptr<NBTTagCompound> getTagFromJson(std::string_view jsonString);
};

} // namespace mc
