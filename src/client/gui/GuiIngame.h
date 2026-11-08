#pragma once

// Original: net/minecraft/client/gui/GuiIngame.java
// The in-game HUD. Not ported yet: the spectator hotbar (GuiSpectator) and the scoreboard sidebar (stage 12), the
// boss bar (BossStatus, with the bosses of stage 10/12), the demo timer and the Twitch stream indicator (out of
// scope).

#include "client/gui/Gui.h"
#include "client/gui/GuiNewChat.h"
#include "client/gui/GuiOverlayDebug.h"
#include "client/gui/GuiPlayerTabOverlay.h"
#include "util/Random.h"
#include "util/ResourceLocation.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace mc {

class EntityPlayer;
class FontRenderer;
class IChatComponent;
class ItemStack;
class Minecraft;
class RenderItem;
class ScaledResolution;

class GuiIngame : public Gui {
public:
    explicit GuiIngame(Minecraft* mcIn);

    void setDefaultTitlesTimes();
    void renderGameOverlay(float partialTicks);
    void renderHorseJumpBar(const ScaledResolution& scaledRes, int x);
    void renderExpBar(const ScaledResolution& scaledRes, int x);
    void renderSelectedItem(const ScaledResolution& scaledRes);
    void updateTick();
    void setRecordPlayingMessage(const std::string& recordName);
    void setRecordPlaying(const std::string& message, bool isPlaying);
    void setRecordPlaying(const std::shared_ptr<IChatComponent>& component, bool isPlaying);
    // Java: null strings are "no change"; nullopt here.
    void displayTitle(const std::optional<std::string>& title, const std::optional<std::string>& subTitle, int timeFadeIn, int displayTime, int timeFadeOut);
    GuiNewChat& getChatGUI() { return persistantChatGUI; }
    int getUpdateCounter() const { return updateCounter; }
    FontRenderer& getFontRenderer();
    GuiPlayerTabOverlay& getTabList() { return overlayPlayerList; }
    void resetPlayersOverlayFooterHeader() { overlayPlayerList.resetFooterHeader(); }

    float prevVignetteBrightness = 1.0f;

protected:
    void renderTooltip(const ScaledResolution& sr, float partialTicks);
    bool showCrosshair();

private:
    void renderPlayerStats(const ScaledResolution& scaledRes);
    void renderPumpkinOverlay(const ScaledResolution& scaledRes);
    void renderVignette(float lightLevel, const ScaledResolution& scaledRes);
    void renderPortal(float timeInPortal, const ScaledResolution& scaledRes);
    void renderHotbarItem(int index, int xPos, int yPos, float partialTicks, EntityPlayer* player);

    static inline const ResourceLocation vignetteTexPath { "textures/misc/vignette.png" };
    static inline const ResourceLocation widgetsTexPath { "textures/gui/widgets.png" };
    static inline const ResourceLocation pumpkinBlurTexPath { "textures/misc/pumpkinblur.png" };

    Random rand;
    Minecraft* mc;
    RenderItem* itemRenderer;
    GuiNewChat persistantChatGUI;
    int updateCounter = 0;
    std::string recordPlaying;
    int recordPlayingUpFor = 0;
    bool recordIsPlaying = false;
    int remainingHighlightTicks = 0;
    std::shared_ptr<ItemStack> highlightingItemStack;
    GuiOverlayDebug overlayDebug;
    GuiPlayerTabOverlay overlayPlayerList;
    int titlesTimer = 0;
    std::string displayedTitle;
    std::string displayedSubTitle;
    int titleFadeIn = 0;
    int titleDisplayTime = 0;
    int titleFadeOut = 0;
    int playerHealth = 0;
    int lastPlayerHealth = 0;
    int64_t lastSystemTime = 0;
    int64_t healthUpdateCounter = 0;
};

} // namespace mc
