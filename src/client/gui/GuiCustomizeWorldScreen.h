#pragma once

// Original: net/minecraft/client/gui/GuiCustomizeWorldScreen.java and GuiScreenCustomizePresets.java -- the
// generator settings of the "customized" world type.

#include "client/gui/GuiResponder.h"
#include "client/gui/GuiScreen.h"
#include "client/gui/GuiSlot.h"
#include "util/Random.h"
#include "world/gen/ChunkProviderSettings.h"

#include <array>
#include <memory>
#include <string>

namespace mc {

class GuiCreateWorld;
class GuiPageButtonList;
class GuiTextField;

class GuiCustomizeWorldScreen : public GuiScreen, public GuiResponder {
public:
    GuiCustomizeWorldScreen(std::shared_ptr<GuiCreateWorld> parent, const std::string& settingsJson);
    ~GuiCustomizeWorldScreen() override;

    void initGui() override;
    void handleMouseInput() override;
    // Java: func_175323_a -- the settings as JSON.
    std::string getSettingsJson() const;
    // Java: func_175324_a
    void loadSettingsJson(const std::string& json);
    // GuiResponder (Java: func_175319_a, func_175321_a, onTick)
    void onText(int id, const std::string& text) override;
    void onToggle(int id, bool value) override;
    void onTick(int id, float value) override;
    // GuiSlider.FormatHelper
    std::string getText(int id, const std::string& name, float value) const;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;
    void keyTyped(char16_t typedChar, int keyCode) override;
    void mouseClicked(int mouseX, int mouseY, int mouseButton) override;
    void mouseReleased(int mouseX, int mouseY, int state) override;

    std::string field_175341_a = "Customize World Settings";
    std::string field_175333_f = "Page 1 of 3";
    std::string field_175335_g = "Basic Settings";
    std::array<std::string, 4> field_175342_h;

private:
    void func_175325_f();
    void func_181031_a(bool changed);
    std::string func_175330_b(int id, float value) const;
    void func_175326_g();
    void func_175322_b(int id);
    void func_175331_h();
    void func_175329_a(bool confirming);
    void func_175328_i();
    void func_175327_a(float step);

    std::shared_ptr<GuiCreateWorld> field_175343_i;
    std::unique_ptr<GuiPageButtonList> field_175349_r;
    GuiButton* field_175348_s = nullptr; // done
    GuiButton* field_175347_t = nullptr; // randomize
    GuiButton* field_175346_u = nullptr; // defaults
    GuiButton* field_175345_v = nullptr; // previous page
    GuiButton* field_175344_w = nullptr; // next page
    GuiButton* field_175352_x = nullptr; // confirm yes
    GuiButton* field_175351_y = nullptr; // confirm no
    GuiButton* field_175350_z = nullptr; // presets
    bool field_175338_A = false;
    int field_175339_B = 0;
    bool field_175340_C = false;
    const ChunkProviderSettings::Factory field_175334_E;
    ChunkProviderSettings::Factory field_175336_F;
    Random random;
};

class GuiScreenCustomizePresets : public GuiScreen {
public:
    explicit GuiScreenCustomizePresets(std::shared_ptr<GuiCustomizeWorldScreen> parent);
    ~GuiScreenCustomizePresets() override;

    void initGui() override;
    void handleMouseInput() override;
    void onGuiClosed() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void updateScreen() override;
    void func_175304_a();

protected:
    void mouseClicked(int mouseX, int mouseY, int mouseButton) override;
    void keyTyped(char16_t typedChar, int keyCode) override;
    void actionPerformed(GuiButton& button) override;

    std::string field_175315_a = "Customize World Presets";

private:
    class ListPreset : public GuiSlot {
    public:
        ListPreset(GuiScreenCustomizePresets& screen, Minecraft& mcIn);

        int field_178053_u = -1;

    protected:
        int getSize() const override;
        void elementClicked(int slotIndex, bool isDoubleClick, int mouseX, int mouseY) override;
        bool isSelected(int slotIndex) const override { return slotIndex == field_178053_u; }
        void drawBackground() override { }
        void drawSlot(int entryID, int insideLeft, int yPos, int insideSlotHeight, int mouseXIn, int mouseYIn) override;

    private:
        void func_178051_a(int x, int y, const ResourceLocation& texture);

        GuiScreenCustomizePresets& screen;
    };

    bool func_175305_g() const;

    std::shared_ptr<ListPreset> field_175311_g;
    GuiButton* field_175316_h = nullptr;
    std::unique_ptr<GuiTextField> field_175317_i;
    std::shared_ptr<GuiCustomizeWorldScreen> field_175314_r;
    std::string field_175313_s;
    std::string field_175312_t;
};

} // namespace mc
