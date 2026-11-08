#pragma once

// Original: net/minecraft/client/gui/GuiCreateWorld.java
// The superflat customization (GuiCreateFlatWorld, GuiFlatPresets) draws block items and comes with item
// rendering (stage 7).

#include "client/gui/GuiScreen.h"

#include <memory>
#include <optional>
#include <string>

namespace mc {

class GuiTextField;
class ISaveFormat;
class WorldInfo;

class GuiCreateWorld : public GuiScreen {
public:
    explicit GuiCreateWorld(std::shared_ptr<GuiScreen> parentScreenIn);
    ~GuiCreateWorld() override;

    void updateScreen() override;
    void initGui() override;
    static std::string getUncollidingSaveDirName(ISaveFormat& saveLoader, std::string name);
    void onGuiClosed() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void recreateFromExistingWorld(const WorldInfo& original);

    std::string chunkProviderSettingsJson;

protected:
    void actionPerformed(GuiButton& button) override;
    void keyTyped(char16_t typedChar, int keyCode) override;
    void mouseClicked(int mouseX, int mouseY, int mouseButton) override;

private:
    void calcSaveDirName();
    void updateDisplayState();
    bool canSelectCurWorldType() const;
    void toggleMoreWorldOptions() { showMoreWorldOptions(!inMoreWorldOptionsDisplay); }
    void showMoreWorldOptions(bool toggle);

    std::shared_ptr<GuiScreen> parentScreen;
    std::unique_ptr<GuiTextField> worldNameField;
    std::unique_ptr<GuiTextField> worldSeedField;
    std::string saveDirName;
    std::string gameMode = "survival";
    std::optional<std::string> savedGameMode;
    bool generateStructuresEnabled = true;
    bool allowCheats = false;
    bool allowCheatsWasSetByUser = false;
    bool bonusChestEnabled = false;
    bool hardCoreMode = false;
    bool alreadyGenerated = false;
    bool inMoreWorldOptionsDisplay = false;
    GuiButton* btnGameMode = nullptr;
    GuiButton* btnMoreOptions = nullptr;
    GuiButton* btnMapFeatures = nullptr;
    GuiButton* btnBonusItems = nullptr;
    GuiButton* btnMapType = nullptr;
    GuiButton* btnAllowCommands = nullptr;
    GuiButton* btnCustomizeType = nullptr;
    std::string gameModeDesc1;
    std::string gameModeDesc2;
    std::string worldSeed;
    std::string worldName;
    int selectedIndex = 0;
};

} // namespace mc
