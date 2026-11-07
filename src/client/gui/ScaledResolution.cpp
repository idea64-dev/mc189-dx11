#include "client/gui/ScaledResolution.h"

#include "client/Minecraft.h"
#include "util/MathHelper.h"

namespace mc {

ScaledResolution::ScaledResolution(const Minecraft& minecraft)
    : scaledWidth(minecraft.displayWidth)
    , scaledHeight(minecraft.displayHeight)
    , scaleFactor(1)
{
    const bool unicode = minecraft.isUnicode();
    int i = minecraft.gameSettings->guiScale;
    if (i == 0)
        i = 1000;

    while (scaleFactor < i && scaledWidth / (scaleFactor + 1) >= 320 && scaledHeight / (scaleFactor + 1) >= 240)
        ++scaleFactor;

    if (unicode && scaleFactor % 2 != 0 && scaleFactor != 1)
        --scaleFactor;

    scaledWidthD = static_cast<double>(scaledWidth) / static_cast<double>(scaleFactor);
    scaledHeightD = static_cast<double>(scaledHeight) / static_cast<double>(scaleFactor);
    scaledWidth = MathHelper::ceiling_double_int(scaledWidthD);
    scaledHeight = MathHelper::ceiling_double_int(scaledHeightD);
}

} // namespace mc
