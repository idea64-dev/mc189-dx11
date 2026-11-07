#pragma once

// Original: net/minecraft/client/gui/ScaledResolution.java

namespace mc {

class Minecraft;

class ScaledResolution {
public:
    explicit ScaledResolution(const Minecraft& minecraft);

    int getScaledWidth() const { return scaledWidth; }
    int getScaledHeight() const { return scaledHeight; }
    double getScaledWidth_double() const { return scaledWidthD; }
    double getScaledHeight_double() const { return scaledHeightD; }
    int getScaleFactor() const { return scaleFactor; }

private:
    double scaledWidthD;
    double scaledHeightD;
    int scaledWidth;
    int scaledHeight;
    int scaleFactor;
};

} // namespace mc
