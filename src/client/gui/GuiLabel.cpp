#include "client/gui/GuiLabel.h"

#include "client/renderer/DxStateManager.h"
#include "client/resources/Locale.h"

namespace mc {

GuiLabel::GuiLabel(FontRenderer& fontRendererObj, int id, int x, int y, int widthIn, int heightIn, int color)
    : field_146162_g(x)
    , field_146174_h(y)
    , field_175204_i(id)
    , field_146167_a(widthIn)
    , field_146161_f(heightIn)
    , field_146168_n(color)
    , fontRenderer(fontRendererObj)
{
}

void GuiLabel::addLine(const std::string& key)
{
    field_146173_k.push_back(I18n::format(key));
}

void GuiLabel::drawLabel(Minecraft& mc, int mouseX, int mouseY)
{
    if (!visible)
        return;
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    drawLabelBackground(mc, mouseX, mouseY);
    const int i = field_146174_h + field_146161_f / 2 + field_146163_s / 2;
    const int j = i - static_cast<int>(field_146173_k.size()) * 10 / 2;
    for (size_t k = 0; k < field_146173_k.size(); ++k) {
        const int y = j + static_cast<int>(k) * 10;
        if (centered)
            drawCenteredString(fontRenderer, field_146173_k[k], field_146162_g + field_146167_a / 2, y, field_146168_n);
        else
            drawString(fontRenderer, field_146173_k[k], field_146162_g, y, field_146168_n);
    }
}

void GuiLabel::drawLabelBackground(Minecraft&, int, int)
{
    if (!labelBgEnabled)
        return;
    const int i = field_146167_a + field_146163_s * 2;
    const int j = field_146161_f + field_146163_s * 2;
    const int k = field_146162_g - field_146163_s;
    const int l = field_146174_h - field_146163_s;
    drawRect(k, l, k + i, l + j, field_146169_o);
    drawHorizontalLine(k, k + i, l, field_146166_p);
    drawHorizontalLine(k, k + i, l + j, field_146165_q);
    drawVerticalLine(k, l, l + j, field_146166_p);
    drawVerticalLine(k + i, l, l + j, field_146165_q);
}

} // namespace mc
