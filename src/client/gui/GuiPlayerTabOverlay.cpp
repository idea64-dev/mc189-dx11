#include "client/gui/GuiPlayerTabOverlay.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/multiplayer/WorldClient.h"
#include "client/network/NetHandlerPlayClient.h"
#include "client/network/NetworkPlayerInfo.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/texture/TextureManager.h"
#include "core/JavaString.h"
#include "entity/player/EntityPlayer.h"
#include "entity/player/EntityPlayerSP.h"
#include "entity/player/PlayerEnums.h"
#include "util/ChatComponent.h"

#include <algorithm>
#include <climits>
#include <vector>

namespace mc {

std::string GuiPlayerTabOverlay::getPlayerName(const NetworkPlayerInfo& networkPlayerInfoIn) const
{
    // Java: ScorePlayerTeam.formatPlayerName(team, name) -- the name itself without a team.
    return networkPlayerInfoIn.getDisplayName() != nullptr ? networkPlayerInfoIn.getDisplayName()->getFormattedText() : networkPlayerInfoIn.getGameProfile().getName();
}

void GuiPlayerTabOverlay::updatePlayerList(bool willBeRendered)
{
    if (willBeRendered && !isBeingRendered)
        lastTimeOpened = Minecraft::getSystemTime();

    isBeingRendered = willBeRendered;
}

void GuiPlayerTabOverlay::renderPlayerlist(int width)
{
    NetHandlerPlayClient* nethandlerplayclient = mc->thePlayer->sendQueue;
    std::vector<NetworkPlayerInfo*> list = nethandlerplayclient->getPlayerInfoMap();
    // Java: PlayerComparator -- non-spectators first, then the team name (none yet), then the name.
    std::stable_sort(list.begin(), list.end(), [](const NetworkPlayerInfo* a, const NetworkPlayerInfo* b) {
        const bool aPlaying = a->getGameType() != GameType::SPECTATOR;
        const bool bPlaying = b->getGameType() != GameType::SPECTATOR;
        if (aPlaying != bPlaying)
            return aPlaying;
        return java::compareTo(a->getGameProfile().getName(), b->getGameProfile().getName()) < 0;
    });
    int i = 0;

    for (const NetworkPlayerInfo* networkplayerinfo : list)
        i = std::max(i, mc->fontRendererObj->getStringWidth(getPlayerName(*networkplayerinfo)));

    if (list.size() > 80)
        list.resize(80);
    const int l3 = static_cast<int>(list.size());
    int i4 = l3;
    int j4;

    for (j4 = 1; i4 > 20; i4 = (l3 + j4 - 1) / j4)
        ++j4;

    // Java: integrated server or an encrypted connection -- the local connection is always integrated.
    const bool flag = mc->isIntegratedServerRunning();
    const int l = 0;
    const int i1 = std::min(j4 * ((flag ? 9 : 0) + i + l + 13), width - 50) / j4;
    const int j1 = width / 2 - (i1 * j4 + (j4 - 1) * 5) / 2;
    int k1 = 10;
    int l1 = i1 * j4 + (j4 - 1) * 5;
    std::vector<std::string> list1;
    std::vector<std::string> list2;

    if (header != nullptr) {
        list1 = mc->fontRendererObj->listFormattedStringToWidth(header->getFormattedText(), width - 50);
        for (const std::string& s : list1)
            l1 = std::max(l1, mc->fontRendererObj->getStringWidth(s));
    }

    if (footer != nullptr) {
        list2 = mc->fontRendererObj->listFormattedStringToWidth(footer->getFormattedText(), width - 50);
        for (const std::string& s2 : list2)
            l1 = std::max(l1, mc->fontRendererObj->getStringWidth(s2));
    }

    if (header != nullptr) {
        drawRect(width / 2 - l1 / 2 - 1, k1 - 1, width / 2 + l1 / 2 + 1, k1 + static_cast<int>(list1.size()) * mc->fontRendererObj->FONT_HEIGHT, INT_MIN);

        for (const std::string& s3 : list1) {
            const int i2 = mc->fontRendererObj->getStringWidth(s3);
            mc->fontRendererObj->drawStringWithShadow(s3, static_cast<float>(width / 2 - i2 / 2), static_cast<float>(k1), -1);
            k1 += mc->fontRendererObj->FONT_HEIGHT;
        }

        ++k1;
    }

    drawRect(width / 2 - l1 / 2 - 1, k1 - 1, width / 2 + l1 / 2 + 1, k1 + i4 * 9, INT_MIN);

    for (int k4 = 0; k4 < l3; ++k4) {
        const int l4 = k4 / i4;
        const int i5 = k4 % i4;
        int j2 = j1 + l4 * i1 + l4 * 5;
        const int k2 = k1 + i5 * 9;
        drawRect(j2, k2, j2 + i1, k2 + 8, 553648127);
        DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
        DxStateManager::enableAlpha();
        DxStateManager::enableBlend();
        DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);

        if (k4 < static_cast<int>(list.size())) {
            const NetworkPlayerInfo& networkplayerinfo1 = *list[static_cast<size_t>(k4)];
            std::string s1 = getPlayerName(networkplayerinfo1);
            const GameProfile& gameprofile = networkplayerinfo1.getGameProfile();

            if (flag) {
                EntityPlayer* entityplayer = gameprofile.getId() ? mc->theWorld->getPlayerEntityByUUID(*gameprofile.getId()) : nullptr;
                const bool flag1 = entityplayer != nullptr && entityplayer->isWearing(EnumPlayerModelParts::CAPE)
                    && (gameprofile.getName() == "Dinnerbone" || gameprofile.getName() == "Grumm");
                mc->getTextureManager().bindTexture(networkplayerinfo1.getLocationSkin());
                const int l2 = 8 + (flag1 ? 8 : 0);
                const int i3 = 8 * (flag1 ? -1 : 1);
                Gui::drawScaledCustomSizeModalRect(j2, k2, 8.0f, static_cast<float>(l2), 8, i3, 8, 8, 64.0f, 64.0f);

                if (entityplayer != nullptr && entityplayer->isWearing(EnumPlayerModelParts::HAT)) {
                    const int j3 = 8 + (flag1 ? 8 : 0);
                    const int k3 = 8 * (flag1 ? -1 : 1);
                    Gui::drawScaledCustomSizeModalRect(j2, k2, 40.0f, static_cast<float>(j3), 8, k3, 8, 8, 64.0f, 64.0f);
                }

                j2 += 9;
            }

            if (networkplayerinfo1.getGameType() == GameType::SPECTATOR) {
                s1 = EnumChatFormatting::toString('o') + s1;
                mc->fontRendererObj->drawStringWithShadow(s1, static_cast<float>(j2), static_cast<float>(k2), -1862270977);
            } else {
                mc->fontRendererObj->drawStringWithShadow(s1, static_cast<float>(j2), static_cast<float>(k2), -1);
            }

            drawPing(i1, j2 - (flag ? 9 : 0), k2, networkplayerinfo1);
        }
    }

    if (footer != nullptr) {
        k1 = k1 + i4 * 9 + 1;
        drawRect(width / 2 - l1 / 2 - 1, k1 - 1, width / 2 + l1 / 2 + 1, k1 + static_cast<int>(list2.size()) * mc->fontRendererObj->FONT_HEIGHT, INT_MIN);

        for (const std::string& s4 : list2) {
            const int j5 = mc->fontRendererObj->getStringWidth(s4);
            mc->fontRendererObj->drawStringWithShadow(s4, static_cast<float>(width / 2 - j5 / 2), static_cast<float>(k1), -1);
            k1 += mc->fontRendererObj->FONT_HEIGHT;
        }
    }
}

void GuiPlayerTabOverlay::drawPing(int width, int x, int y, const NetworkPlayerInfo& networkPlayerInfoIn)
{
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    mc->getTextureManager().bindTexture(icons);
    const int i = 0;
    int j;

    if (networkPlayerInfoIn.getResponseTime() < 0)
        j = 5;
    else if (networkPlayerInfoIn.getResponseTime() < 150)
        j = 0;
    else if (networkPlayerInfoIn.getResponseTime() < 300)
        j = 1;
    else if (networkPlayerInfoIn.getResponseTime() < 600)
        j = 2;
    else if (networkPlayerInfoIn.getResponseTime() < 1000)
        j = 3;
    else
        j = 4;

    zLevel += 100.0f;
    drawTexturedModalRect(x + width - 11, y, 0 + i * 10, 176 + j * 8, 10, 8);
    zLevel -= 100.0f;
}

} // namespace mc
