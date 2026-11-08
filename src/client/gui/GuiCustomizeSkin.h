#pragma once

// Original: net/minecraft/client/gui/GuiCustomizeSkin.java

#include "client/gui/GuiScreen.h"
#include "entity/player/PlayerEnums.h"

#include <memory>
#include <string>

namespace mc {

class GuiCustomizeSkin : public GuiScreen {
public:
    explicit GuiCustomizeSkin(std::shared_ptr<GuiScreen> parentScreen);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton& button) override;

private:
    class ButtonPart;

    std::string partLabel(EnumPlayerModelParts playerModelParts) const;

    std::shared_ptr<GuiScreen> parentScreen;
    std::string title;
};

} // namespace mc
