#pragma once

// Original: net/minecraft/client/gui/GuiSelectWorld.java and GuiRenameWorld.java

#include "client/gui/GuiScreen.h"
#include "client/gui/GuiSlot.h"
#include "world/storage/SaveFormat.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class GuiTextField;
class GuiYesNo;

class GuiSelectWorld : public GuiScreen {
public:
    explicit GuiSelectWorld(std::shared_ptr<GuiScreen> parentScreenIn);

    void initGui() override;
    void handleMouseInput() override;
    void addWorldSelectionButtons();
    // Java: func_146615_e -- plays the world at the index.
    void selectWorld(int index);
    void confirmClicked(bool result, int id) override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    static std::shared_ptr<GuiYesNo> makeDeleteWorldYesNo(std::shared_ptr<GuiYesNoCallback> selectWorld, const std::string& name, int id);

protected:
    void actionPerformed(GuiButton& button) override;
    // Java: func_146621_a -- the save directory name.
    std::string getSaveFileName(int index) const;
    // Java: func_146614_d -- the display name, "World n" when it has none.
    std::string getSaveName(int index) const;

    std::shared_ptr<GuiScreen> parentScreen;
    std::string screenTitle = "Select world";

private:
    class List : public GuiSlot {
    public:
        List(GuiSelectWorld& screen, Minecraft& mcIn);

    protected:
        int getSize() const override { return static_cast<int>(screen.saveList.size()); }
        void elementClicked(int slotIndex, bool isDoubleClick, int mouseX, int mouseY) override;
        bool isSelected(int slotIndex) const override { return slotIndex == screen.selectedIndex; }
        int getContentHeight() const override { return static_cast<int>(screen.saveList.size()) * 36; }
        void drawBackground() override { screen.drawDefaultBackground(); }
        void drawSlot(int entryID, int insideLeft, int yPos, int insideSlotHeight, int mouseXIn, int mouseYIn) override;

    private:
        GuiSelectWorld& screen;
    };

    void loadLevelList();

    bool field_146634_i = false;
    int selectedIndex = -1;
    std::vector<SaveFormatComparator> saveList;
    std::shared_ptr<List> availableWorlds;
    std::string field_146637_u;
    std::string field_146636_v;
    std::array<std::string, 4> field_146635_w;
    bool confirmingDelete = false;
    GuiButton* deleteButton = nullptr;
    GuiButton* selectButton = nullptr;
    GuiButton* renameButton = nullptr;
    GuiButton* recreateButton = nullptr;
};

class GuiRenameWorld : public GuiScreen {
public:
    GuiRenameWorld(std::shared_ptr<GuiScreen> parentScreenIn, std::string saveNameIn);
    ~GuiRenameWorld() override;

    void updateScreen() override;
    void initGui() override;
    void onGuiClosed() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;
    void keyTyped(char16_t typedChar, int keyCode) override;
    void mouseClicked(int mouseX, int mouseY, int mouseButton) override;

private:
    std::shared_ptr<GuiScreen> parentScreen;
    std::unique_ptr<GuiTextField> field_146583_f;
    const std::string saveName;
};

} // namespace mc
