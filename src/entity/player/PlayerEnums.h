#pragma once

// Original: net/minecraft/entity/player/EnumPlayerModelParts.java and EntityPlayer.EnumChatVisibility

#include <array>
#include <stdexcept>
#include <string_view>

namespace mc {

enum class EnumPlayerModelParts { CAPE, JACKET, LEFT_SLEEVE, RIGHT_SLEEVE, LEFT_PANTS_LEG, RIGHT_PANTS_LEG, HAT };

inline constexpr std::array<EnumPlayerModelParts, 7> kPlayerModelParts { EnumPlayerModelParts::CAPE, EnumPlayerModelParts::JACKET,
    EnumPlayerModelParts::LEFT_SLEEVE, EnumPlayerModelParts::RIGHT_SLEEVE, EnumPlayerModelParts::LEFT_PANTS_LEG,
    EnumPlayerModelParts::RIGHT_PANTS_LEG, EnumPlayerModelParts::HAT };

constexpr std::string_view getPartName(EnumPlayerModelParts part)
{
    switch (part) {
    case EnumPlayerModelParts::CAPE: return "cape";
    case EnumPlayerModelParts::JACKET: return "jacket";
    case EnumPlayerModelParts::LEFT_SLEEVE: return "left_sleeve";
    case EnumPlayerModelParts::RIGHT_SLEEVE: return "right_sleeve";
    case EnumPlayerModelParts::LEFT_PANTS_LEG: return "left_pants_leg";
    case EnumPlayerModelParts::RIGHT_PANTS_LEG: return "right_pants_leg";
    case EnumPlayerModelParts::HAT: return "hat";
    }
    return "";
}

constexpr int getPartId(EnumPlayerModelParts part)
{
    return static_cast<int>(part);
}

constexpr int getPartMask(EnumPlayerModelParts part)
{
    return 1 << getPartId(part);
}

enum class EnumChatVisibility { FULL, SYSTEM, HIDDEN };

constexpr int getChatVisibility(EnumChatVisibility visibility)
{
    return static_cast<int>(visibility);
}

constexpr std::string_view getResourceKey(EnumChatVisibility visibility)
{
    switch (visibility) {
    case EnumChatVisibility::FULL: return "options.chat.visibility.full";
    case EnumChatVisibility::SYSTEM: return "options.chat.visibility.system";
    case EnumChatVisibility::HIDDEN: return "options.chat.visibility.hidden";
    }
    return "";
}

// Java: ID_LOOKUP[id % ID_LOOKUP.length] -- a negative id throws ArrayIndexOutOfBoundsException.
inline EnumChatVisibility getEnumChatVisibility(int id)
{
    const int index = id % 3;
    if (index < 0)
        throw std::out_of_range("chat visibility");
    return static_cast<EnumChatVisibility>(index);
}

} // namespace mc
