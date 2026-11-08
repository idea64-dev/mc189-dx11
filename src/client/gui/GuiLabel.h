#pragma once

// Original: net/minecraft/client/gui/GuiLabel.java -- lines of text, vertically centered in a box.

#include "client/gui/Gui.h"

#include <string>
#include <vector>

namespace mc {

class FontRenderer;
class Minecraft;

class GuiLabel : public Gui {
public:
    GuiLabel(FontRenderer& fontRendererObj, int id, int x, int y, int widthIn, int heightIn, int color);

    // Java: func_175202_a -- adds a line, translated.
    void addLine(const std::string& key);
    GuiLabel& setCentered()
    {
        centered = true;
        return *this;
    }
    void drawLabel(Minecraft& mc, int mouseX, int mouseY);

    int field_146162_g; // x
    int field_146174_h; // y
    int field_175204_i; // id
    bool visible = true;

protected:
    void drawLabelBackground(Minecraft& mc, int mouseX, int mouseY);

    int field_146167_a; // width
    int field_146161_f; // height

private:
    std::vector<std::string> field_146173_k;
    bool centered = false;
    bool labelBgEnabled = false;
    int field_146168_n; // text color
    int field_146169_o = -1; // background
    int field_146166_p = -1; // upper-left border
    int field_146165_q = -1; // lower-right border
    FontRenderer& fontRenderer;
    int field_146163_s = 0; // border
};

} // namespace mc
