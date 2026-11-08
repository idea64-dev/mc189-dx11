#pragma once

// Original: net/minecraft/client/resources/model/IBakedModel.java, SimpleBakedModel.java,
// WeightedBakedModel.java and BuiltInModel.java
// Face quads are culled against the neighbour on their side; general quads are always drawn.

#include "client/renderer/block/model/BakedQuad.h"
#include "client/renderer/block/model/ModelBlock.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace mc {

class TextureAtlasSprite;

class IBakedModel {
public:
    virtual ~IBakedModel() = default;

    virtual const std::vector<BakedQuad>& getFaceQuads(EnumFacing facing) const = 0;
    virtual const std::vector<BakedQuad>& getGeneralQuads() const = 0;
    virtual bool isAmbientOcclusion() const = 0;
    virtual bool isGui3d() const = 0;
    virtual bool isBuiltInRenderer() const = 0;
    virtual TextureAtlasSprite* getParticleTexture() const = 0;
    virtual const ItemCameraTransforms& getItemCameraTransforms() const = 0;
};

class SimpleBakedModel : public IBakedModel {
public:
    class Builder {
    public:
        explicit Builder(const ModelBlock& model);
        // Java: Builder(IBakedModel, TextureAtlasSprite) -- the model with every quad remapped onto the sprite
        // (the block breaking overlay).
        Builder(const IBakedModel& bakedModel, const TextureAtlasSprite& texture);

        Builder& addFaceQuad(EnumFacing facing, BakedQuad quad);
        Builder& addGeneralQuad(BakedQuad quad);
        Builder& setTexture(TextureAtlasSprite* texture);
        // Throws std::runtime_error("Missing particle!") without a texture.
        std::shared_ptr<IBakedModel> makeBakedModel();

    private:
        Builder(bool ambientOcclusion, bool gui3d, ItemCameraTransforms cameraTransforms);

        std::vector<BakedQuad> builderGeneralQuads;
        std::array<std::vector<BakedQuad>, 6> builderFaceQuads;
        bool builderAmbientOcclusion;
        TextureAtlasSprite* builderTexture = nullptr;
        bool builderGui3d;
        ItemCameraTransforms builderCameraTransforms;
    };

    SimpleBakedModel(std::vector<BakedQuad> generalQuadsIn, std::array<std::vector<BakedQuad>, 6> faceQuadsIn, bool ambientOcclusionIn, bool gui3dIn,
        TextureAtlasSprite* textureIn, ItemCameraTransforms cameraTransformsIn);

    const std::vector<BakedQuad>& getFaceQuads(EnumFacing facing) const override { return faceQuads[static_cast<size_t>(ordinal(facing))]; }
    const std::vector<BakedQuad>& getGeneralQuads() const override { return generalQuads; }
    bool isAmbientOcclusion() const override { return ambientOcclusion; }
    bool isGui3d() const override { return gui3d; }
    bool isBuiltInRenderer() const override { return false; }
    TextureAtlasSprite* getParticleTexture() const override { return texture; }
    const ItemCameraTransforms& getItemCameraTransforms() const override { return cameraTransforms; }

private:
    std::vector<BakedQuad> generalQuads;
    std::array<std::vector<BakedQuad>, 6> faceQuads;
    bool ambientOcclusion;
    bool gui3d;
    TextureAtlasSprite* texture;
    ItemCameraTransforms cameraTransforms;
};

class WeightedBakedModel : public IBakedModel {
public:
    struct Item {
        std::shared_ptr<IBakedModel> model;
        int itemWeight;

        int getCountQuads() const;
    };

    class Builder {
    public:
        Builder& add(std::shared_ptr<IBakedModel> model, int weight);
        // Sorted by weight, heaviest first, then by quad count (stable, like Collections.sort).
        std::shared_ptr<WeightedBakedModel> build();
        std::shared_ptr<IBakedModel> first() const { return listItems.at(0).model; }

    private:
        std::vector<Item> listItems;
    };

    explicit WeightedBakedModel(std::vector<Item> items);

    const std::vector<BakedQuad>& getFaceQuads(EnumFacing facing) const override { return baseModel->getFaceQuads(facing); }
    const std::vector<BakedQuad>& getGeneralQuads() const override { return baseModel->getGeneralQuads(); }
    bool isAmbientOcclusion() const override { return baseModel->isAmbientOcclusion(); }
    bool isGui3d() const override { return baseModel->isGui3d(); }
    bool isBuiltInRenderer() const override { return baseModel->isBuiltInRenderer(); }
    TextureAtlasSprite* getParticleTexture() const override { return baseModel->getParticleTexture(); }
    const ItemCameraTransforms& getItemCameraTransforms() const override { return baseModel->getItemCameraTransforms(); }

    // The model picked by a position hash (MathHelper.getPositionRandom).
    const IBakedModel* getAlternativeModel(int64_t seed) const;
    const std::vector<Item>& getModels() const { return models; }

private:
    int totalWeight;
    std::vector<Item> models;
    std::shared_ptr<IBakedModel> baseModel;
};

// Rendered by code (chests, banners, skulls as items): no quads.
class BuiltInModel : public IBakedModel {
public:
    explicit BuiltInModel(ItemCameraTransforms transforms)
        : cameraTransforms(std::move(transforms))
    {
    }

    // Java returns null lists; nothing should ask for them.
    const std::vector<BakedQuad>& getFaceQuads(EnumFacing) const override { return noQuads; }
    const std::vector<BakedQuad>& getGeneralQuads() const override { return noQuads; }
    bool isAmbientOcclusion() const override { return false; }
    bool isGui3d() const override { return true; }
    bool isBuiltInRenderer() const override { return true; }
    TextureAtlasSprite* getParticleTexture() const override { return nullptr; }
    const ItemCameraTransforms& getItemCameraTransforms() const override { return cameraTransforms; }

private:
    ItemCameraTransforms cameraTransforms;
    static inline const std::vector<BakedQuad> noQuads;
};

} // namespace mc
