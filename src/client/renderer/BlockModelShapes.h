#pragma once

// Original: net/minecraft/client/renderer/BlockModelShapes.java
// The baked model of every block state, and the state-to-variant mapping registered for each block.

#include "client/renderer/block/statemap/StateMapper.h"

#include <unordered_map>

namespace mc {

class IBakedModel;
class ModelManager;
class TextureAtlasSprite;

class BlockModelShapes {
public:
    explicit BlockModelShapes(ModelManager& manager);

    BlockStateMapper& getBlockStateMapper() { return blockStateMapper; }
    const BlockStateMapper& getBlockStateMapper() const { return blockStateMapper; }
    // The particle texture (block breaking, landing dust); fixed sprites for blocks drawn by code.
    TextureAtlasSprite* getTexture(const IBlockState* state) const;
    // Never null: the missing model for states without one.
    IBakedModel* getModelForState(const IBlockState* state) const;
    ModelManager& getModelManager() const { return modelManager; }
    void reloadModels();
    void registerBlockWithStateMapper(const Block* block, std::unique_ptr<IStateMapper> stateMapper);
    void registerBuiltInBlocks(std::initializer_list<const Block*> builtIns);

private:
    void registerAllBlocks();

    std::unordered_map<const IBlockState*, IBakedModel*> bakedModelStore;
    BlockStateMapper blockStateMapper;
    ModelManager& modelManager;
};

} // namespace mc
