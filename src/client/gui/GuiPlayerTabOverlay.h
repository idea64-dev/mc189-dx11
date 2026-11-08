#pragma once

// Original: net/minecraft/client/gui/GuiPlayerTabOverlay.java
// The tab list. The scoreboard objective column and the team names come with the scoreboard (stage 12); until
// then players are listed by name with their ping, as Java does without an objective or teams.

#include "client/gui/Gui.h"

#include <cstdint>
#include <memory>
#include <string>

namespace mc {

class GuiIngame;
class IChatComponent;
class Minecraft;
class NetworkPlayerInfo;

class GuiPlayerTabOverlay : public Gui {
public:
    GuiPlayerTabOverlay(Minecraft* mcIn, GuiIngame* guiIngameIn)
        : mc(mcIn)
        , guiIngame(guiIngameIn)
    {
    }

    std::string getPlayerName(const NetworkPlayerInfo& networkPlayerInfoIn) const;
    void updatePlayerList(bool willBeRendered);
    // Java: renderPlayerlist(width, scoreboard, objective) -- without an objective.
    void renderPlayerlist(int width);
    void setFooter(std::shared_ptr<IChatComponent> footerIn) { footer = std::move(footerIn); }
    void setHeader(std::shared_ptr<IChatComponent> headerIn) { header = std::move(headerIn); }
    void resetFooterHeader()
    {
        header = nullptr;
        footer = nullptr;
    }

protected:
    void drawPing(int width, int x, int y, const NetworkPlayerInfo& networkPlayerInfoIn);

private:
    Minecraft* mc;
    GuiIngame* guiIngame;
    std::shared_ptr<IChatComponent> footer;
    std::shared_ptr<IChatComponent> header;
    int64_t lastTimeOpened = 0;
    bool isBeingRendered = false;
};

} // namespace mc
