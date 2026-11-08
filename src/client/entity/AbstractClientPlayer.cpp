#include "client/entity/AbstractClientPlayer.h"

#include "client/Minecraft.h"
#include "client/network/NetHandlerPlayClient.h"
#include "client/network/NetworkPlayerInfo.h"
#include "entity/SharedMonsterAttributes.h"
#include "init/Items.h"
#include "item/ItemMisc.h"
#include "item/ItemStack.h"

#include <cctype>
#include <cmath>

namespace mc {

namespace {

// Java: StringUtils.stripControlCodes -- removes "§" followed by one of 0-9, a-f, k-o, r (any case).
std::string stripControlCodes(const std::string& text)
{
    static const std::string kSection = "\xC2\xA7";
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        if (text.compare(i, kSection.size(), kSection) == 0 && i + kSection.size() < text.size()) {
            const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i + kSection.size()])));
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'k' && c <= 'o') || c == 'r') {
                i += kSection.size() + 1;
                continue;
            }
        }
        out += text[i++];
    }
    return out;
}

} // namespace

bool AbstractClientPlayer::isSpectator()
{
    const std::optional<UUID>& id = getGameProfile().getId();
    NetworkPlayerInfo* networkplayerinfo = id ? Minecraft::getMinecraft()->getNetHandler()->getPlayerInfo(*id) : nullptr;
    return networkplayerinfo && networkplayerinfo->getGameType() == GameType::SPECTATOR;
}

NetworkPlayerInfo* AbstractClientPlayer::getPlayerInfo()
{
    if (!playerInfo)
        playerInfo = Minecraft::getMinecraft()->getNetHandler()->getPlayerInfo(getUniqueID());
    return playerInfo;
}

bool AbstractClientPlayer::hasSkin()
{
    NetworkPlayerInfo* networkplayerinfo = getPlayerInfo();
    return networkplayerinfo && networkplayerinfo->hasLocationSkin();
}

ResourceLocation AbstractClientPlayer::getLocationSkin()
{
    NetworkPlayerInfo* networkplayerinfo = getPlayerInfo();
    return networkplayerinfo ? networkplayerinfo->getLocationSkin() : DefaultPlayerSkin::getDefaultSkin(getUniqueID());
}

std::optional<ResourceLocation> AbstractClientPlayer::getLocationCape()
{
    NetworkPlayerInfo* networkplayerinfo = getPlayerInfo();
    return networkplayerinfo ? networkplayerinfo->getLocationCape() : std::nullopt;
}

ResourceLocation AbstractClientPlayer::getLocationSkin(const std::string& username)
{
    return ResourceLocation("skins/" + stripControlCodes(username));
}

std::string AbstractClientPlayer::getSkinType()
{
    NetworkPlayerInfo* networkplayerinfo = getPlayerInfo();
    return networkplayerinfo ? networkplayerinfo->getSkinType() : DefaultPlayerSkin::getSkinType(getUniqueID());
}

float AbstractClientPlayer::getFovModifier()
{
    float f = 1.0f;
    if (capabilities.isFlying)
        f *= 1.1f;
    IAttributeInstance* iattributeinstance = getEntityAttribute(SharedMonsterAttributes::movementSpeed());
    f = static_cast<float>(static_cast<double>(f) * ((iattributeinstance->getAttributeValue() / static_cast<double>(capabilities.getWalkSpeed()) + 1.0) / 2.0));
    if (capabilities.getWalkSpeed() == 0.0f || std::isnan(f) || std::isinf(f))
        f = 1.0f;
    if (isUsingItem() && getItemInUse()->getItem() == Items::bow) {
        const int i = getItemInUseDuration();
        float f1 = static_cast<float>(i) / 20.0f;
        if (f1 > 1.0f)
            f1 = 1.0f;
        else
            f1 = f1 * f1;
        f *= 1.0f - f1 * 0.15f;
    }
    return f;
}

} // namespace mc
