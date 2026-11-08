#pragma once

// Original: net/minecraft/client/gui/GuiLanguage.java

#include "client/gui/GuiScreen.h"
#include "client/gui/GuiSlot.h"
#include "client/resources/Locale.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class GameSettings;
class GuiOptionButton;
class LanguageManager;

class GuiLanguage : public GuiScreen {
public:
    GuiLanguage(std::shared_ptr<GuiScreen> parentScreen, GameSettings& gameSettings, LanguageManager& manager);

    void initGui() override;
    void handleMouseInput() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;

    std::shared_ptr<GuiScreen> parentScreen;

private:
    class List : public GuiSlot {
    public:
        List(GuiLanguage& screen, Minecraft& mcIn);

    protected:
        int getSize() const override { return static_cast<int>(langCodeList.size()); }
        void elementClicked(int slotIndex, bool isDoubleClick, int mouseX, int mouseY) override;
        bool isSelected(int slotIndex) const override;
        int getContentHeight() const override { return getSize() * 18; }
        void drawBackground() override;
        void drawSlot(int entryID, int insideLeft, int yPos, int insideSlotHeight, int mouseXIn, int mouseYIn) override;

    private:
        GuiLanguage& screen;
        std::vector<std::string> langCodeList;
        std::map<std::string, Language> languageMap;
    };

    std::shared_ptr<List> list;
    GameSettings& game_settings_3;
    LanguageManager& languageManager;
    GuiOptionButton* forceUnicodeFontBtn = nullptr;
    GuiOptionButton* confirmSettingsBtn = nullptr;
};

} // namespace mc
