#pragma once

// Original: net/minecraft/client/renderer/RenderHelper.java -- the two directional lights of entities and items.

namespace mc {

class RenderHelper {
public:
    static void disableStandardItemLighting();
    static void enableStandardItemLighting();
    static void enableGUIStandardItemLighting();
};

} // namespace mc
