#pragma once

// Original: net/minecraft/client/renderer/chunk/CompiledChunk.java
// The result of building a render chunk: which layers have geometry, the face visibility and the sorted
// translucent vertex state. DUMMY is the not-yet-built chunk (sees nothing).

#include "block/Block.h"
#include "client/renderer/WorldRenderer.h"
#include "client/renderer/chunk/VisGraph.h"

#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

class TileEntity;

class CompiledChunk {
public:
    static const std::shared_ptr<CompiledChunk>& DUMMY()
    {
        static const std::shared_ptr<CompiledChunk> dummyChunk = [] {
            auto chunk = std::make_shared<CompiledChunk>();
            chunk->dummy = true;
            return chunk;
        }();
        return dummyChunk;
    }

    bool isEmpty() const { return empty; }
    void setLayerUsed(EnumWorldBlockLayer layer)
    {
        empty = false;
        layersUsed[static_cast<size_t>(layer)] = true;
    }
    bool isLayerEmpty(EnumWorldBlockLayer layer) const { return !layersUsed[static_cast<size_t>(layer)]; }
    void setLayerStarted(EnumWorldBlockLayer layer) { layersStarted[static_cast<size_t>(layer)] = true; }
    bool isLayerStarted(EnumWorldBlockLayer layer) const { return layersStarted[static_cast<size_t>(layer)]; }
    const std::vector<TileEntity*>& getTileEntities() const { return tileEntities; }
    void addTileEntity(TileEntity* tileEntityIn) { tileEntities.push_back(tileEntityIn); }
    bool isVisible(EnumFacing facing, EnumFacing facing2) const { return !dummy && visibility.isVisible(facing, facing2); }
    void setVisibility(const SetVisibility& visibilityIn) { visibility = visibilityIn; }
    const std::optional<WorldRenderer::State>& getState() const { return state; }
    void setState(WorldRenderer::State stateIn) { state = std::move(stateIn); }

private:
    std::array<bool, 4> layersUsed {};
    std::array<bool, 4> layersStarted {};
    bool empty = true;
    bool dummy = false;
    std::vector<TileEntity*> tileEntities;
    SetVisibility visibility;
    std::optional<WorldRenderer::State> state;
};

} // namespace mc
