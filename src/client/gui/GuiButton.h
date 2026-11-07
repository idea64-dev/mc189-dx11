#pragma once

// Original: net/minecraft/client/gui/GuiButton.java and GuiButtonLanguage.java

#include "client/gui/Gui.h"

#include <string>

namespace mc {

class Minecraft;
class SoundHandler;

class GuiButton : public Gui {
public:
    GuiButton(int buttonId, int x, int y, std::string buttonText);
    GuiButton(int buttonId, int x, int y, int widthIn, int heightIn, std::string buttonText);

    virtual void drawButton(Minecraft& mc, int mouseX, int mouseY);
    virtual void mouseReleased(int mouseX, int mouseY);
    virtual bool mousePressed(Minecraft& mc, int mouseX, int mouseY);
    bool isMouseOver() const { return hovered; }
    virtual void drawButtonForegroundLayer(int mouseX, int mouseY);
    virtual void playPressSound(SoundHandler& soundHandlerIn);
    int getButtonWidth() const { return width; }
    void setWidth(int widthIn) { width = widthIn; }

    int xPosition;
    int yPosition;
    std::string displayString;
    int id;
    bool enabled = true;
    bool visible = true;

protected:
    static inline const ResourceLocation buttonTextures { "textures/gui/widgets.png" };

    virtual int getHoverState(bool mouseOver) const;
    virtual void mouseDragged(Minecraft& mc, int mouseX, int mouseY);

    int width;
    int height;
    bool hovered = false;
};

class GuiButtonLanguage : public GuiButton {
public:
    GuiButtonLanguage(int buttonId, int xPos, int yPos);
    void drawButton(Minecraft& mc, int mouseX, int mouseY) override;
};

} // namespace mc
