#pragma once

// Original: net/minecraft/client/renderer/entity/RenderItem.java
// The ItemModelMesher with the vanilla registrations (rebuilt on resource reload) and drawing items in the GUI
// (with the enchantment glint, the stack size and the damage bar).
// Not ported yet: the built-in renderers (chests, banners, skulls: TileEntityItemStackRenderer) and the
// in-world item renderers (held, dropped, item frames) -- stage 10.

#include "client/renderer/ItemModelMesher.h"
#include "client/resources/IResourceManager.h"

#include <string>
#include <vector>

namespace mc {

class FontRenderer;
class IBakedModel;
class ItemStack;
class ModelManager;
class TextureManager;
class WorldRenderer;
struct BakedQuad;

class RenderItem : public IResourceManagerReloadListener {
public:
    RenderItem(TextureManager& textureManagerIn, ModelManager& modelManager)
        : itemModelMesher(modelManager)
        , textureManager(textureManagerIn)
    {
        itemModelMesher.registerVanillaItems();
    }

    ItemModelMesher& getItemModelMesher() { return itemModelMesher; }
    void onResourceManagerReload(const IResourceManager&) override { itemModelMesher.rebuildCache(); }

    void renderItem(const ItemStack* stack, const IBakedModel& model);
    bool shouldRenderItemIn3D(const ItemStack& stack) const;
    void renderItemIntoGUI(const ItemStack& stack, int x, int y);
    void renderItemAndEffectIntoGUI(const ItemStack* stack, int xPosition, int yPosition);
    void renderItemOverlays(FontRenderer& fr, const ItemStack* stack, int xPosition, int yPosition) { renderItemOverlayIntoGUI(fr, stack, xPosition, yPosition, nullptr); }
    // A null text shows the stack size.
    void renderItemOverlayIntoGUI(FontRenderer& fr, const ItemStack* stack, int xPosition, int yPosition, const std::string* text);

    float zLevel = 0.0f;

private:
    void renderModel(const IBakedModel& model, int color, const ItemStack* stack);
    void renderEffect(const IBakedModel& model);
    void renderQuads(WorldRenderer& renderer, const std::vector<BakedQuad>& quads, int color, const ItemStack* stack);
    void renderQuad(WorldRenderer& renderer, const BakedQuad& quad, int color);
    void setupGuiTransform(int xPosition, int yPosition, bool isGui3d);
    static void draw(WorldRenderer& renderer, int x, int y, int width, int height, int red, int green, int blue, int alpha);

    ItemModelMesher itemModelMesher;
    TextureManager& textureManager;
};

} // namespace mc
