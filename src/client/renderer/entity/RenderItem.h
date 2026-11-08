#pragma once

// Original: net/minecraft/client/renderer/entity/RenderItem.java
// So far the model side only: the ItemModelMesher with the vanilla registrations, rebuilt on resource reload.
// Drawing items (GUI, held, dropped) arrives with the item renderers (stages 7/8).

#include "client/renderer/ItemModelMesher.h"
#include "client/resources/IResourceManager.h"

namespace mc {

class ModelManager;
class TextureManager;

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

    float zLevel = 0.0f;

private:
    ItemModelMesher itemModelMesher;
    TextureManager& textureManager;
};

} // namespace mc
