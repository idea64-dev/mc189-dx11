#include "client/resources/model/IBakedModel.h"

#include "core/Java.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace mc {

// --- SimpleBakedModel ---

SimpleBakedModel::SimpleBakedModel(std::vector<BakedQuad> generalQuadsIn, std::array<std::vector<BakedQuad>, 6> faceQuadsIn, bool ambientOcclusionIn,
    bool gui3dIn, TextureAtlasSprite* textureIn, ItemCameraTransforms cameraTransformsIn)
    : generalQuads(std::move(generalQuadsIn))
    , faceQuads(std::move(faceQuadsIn))
    , ambientOcclusion(ambientOcclusionIn)
    , gui3d(gui3dIn)
    , texture(textureIn)
    , cameraTransforms(std::move(cameraTransformsIn))
{
}

SimpleBakedModel::Builder::Builder(bool ambientOcclusion, bool gui3d, ItemCameraTransforms cameraTransforms)
    : builderAmbientOcclusion(ambientOcclusion)
    , builderGui3d(gui3d)
    , builderCameraTransforms(std::move(cameraTransforms))
{
}

SimpleBakedModel::Builder::Builder(const ModelBlock& model)
    : Builder(model.isAmbientOcclusion(), model.isGui3d(), model.getAllTransforms())
{
}

SimpleBakedModel::Builder::Builder(const IBakedModel& bakedModel, const TextureAtlasSprite& texture)
    : Builder(bakedModel.isAmbientOcclusion(), bakedModel.isGui3d(), bakedModel.getItemCameraTransforms())
{
    builderTexture = bakedModel.getParticleTexture();
    for (const EnumFacing facing : kFacings) {
        for (const BakedQuad& quad : bakedModel.getFaceQuads(facing))
            addFaceQuad(facing, makeBreakingFour(quad, texture));
    }
    for (const BakedQuad& quad : bakedModel.getGeneralQuads())
        addGeneralQuad(makeBreakingFour(quad, texture));
}

SimpleBakedModel::Builder& SimpleBakedModel::Builder::addFaceQuad(EnumFacing facing, BakedQuad quad)
{
    builderFaceQuads[static_cast<size_t>(ordinal(facing))].push_back(std::move(quad));
    return *this;
}

SimpleBakedModel::Builder& SimpleBakedModel::Builder::addGeneralQuad(BakedQuad quad)
{
    builderGeneralQuads.push_back(std::move(quad));
    return *this;
}

SimpleBakedModel::Builder& SimpleBakedModel::Builder::setTexture(TextureAtlasSprite* texture)
{
    builderTexture = texture;
    return *this;
}

std::shared_ptr<IBakedModel> SimpleBakedModel::Builder::makeBakedModel()
{
    if (!builderTexture)
        throw std::runtime_error("Missing particle!");
    return std::make_shared<SimpleBakedModel>(std::move(builderGeneralQuads), std::move(builderFaceQuads), builderAmbientOcclusion, builderGui3d,
        builderTexture, builderCameraTransforms);
}

// --- WeightedBakedModel ---

int WeightedBakedModel::Item::getCountQuads() const
{
    int i = static_cast<int>(model->getGeneralQuads().size());
    for (const EnumFacing facing : kFacings)
        i += static_cast<int>(model->getFaceQuads(facing).size());
    return i;
}

WeightedBakedModel::Builder& WeightedBakedModel::Builder::add(std::shared_ptr<IBakedModel> model, int weight)
{
    listItems.push_back({ std::move(model), weight });
    return *this;
}

std::shared_ptr<WeightedBakedModel> WeightedBakedModel::Builder::build()
{
    // compareTo: ComparisonChain.start().compare(other.weight, this.weight).compare(this.quads, other.quads)
    std::stable_sort(listItems.begin(), listItems.end(), [](const Item& a, const Item& b) {
        if (a.itemWeight != b.itemWeight)
            return b.itemWeight < a.itemWeight;
        return a.getCountQuads() < b.getCountQuads();
    });
    return std::make_shared<WeightedBakedModel>(listItems);
}

WeightedBakedModel::WeightedBakedModel(std::vector<Item> items)
    : totalWeight(0)
    , models(std::move(items))
    , baseModel(models.at(0).model)
{
    for (const Item& item : models)
        totalWeight += item.itemWeight;
}

const IBakedModel* WeightedBakedModel::getAlternativeModel(int64_t seed) const
{
    int weight = std::abs(java::l2i(seed) >> 16) % totalWeight;
    for (const Item& item : models) {
        weight -= item.itemWeight;
        if (weight < 0)
            return item.model.get();
    }
    return nullptr; // Java returns null and the caller fails
}

} // namespace mc
