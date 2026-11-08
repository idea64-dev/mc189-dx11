#pragma once

// Original: net/minecraft/client/gui/GuiSnooper.java

#include "client/gui/GuiScreen.h"
#include "client/gui/GuiSlot.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class GameSettings;

class GuiSnooper : public GuiScreen {
public:
    GuiSnooper(std::shared_ptr<GuiScreen> parentScreen, GameSettings& gameSettings);

    void initGui() override;
    void handleMouseInput() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;

private:
    class List : public GuiSlot {
    public:
        explicit List(GuiSnooper& screen);

    protected:
        int getSize() const override { return static_cast<int>(screen.statNames.size()); }
        void elementClicked(int, bool, int, int) override { }
        bool isSelected(int) const override { return false; }
        void drawBackground() override { }
        void drawSlot(int entryID, int insideLeft, int yPos, int insideSlotHeight, int mouseXIn, int mouseYIn) override;
        int getScrollBarX() const override { return width - 10; }

    private:
        GuiSnooper& screen;
    };

    std::shared_ptr<GuiScreen> parentScreen;
    GameSettings& game_settings_2;
    std::vector<std::string> statNames;
    std::vector<std::string> statValues;
    std::string title;
    std::vector<std::string> description;
    std::shared_ptr<List> list;
    GuiButton* toggleButton = nullptr;
};

} // namespace mc
