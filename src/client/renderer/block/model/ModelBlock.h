#pragma once

// Original: net/minecraft/client/renderer/block/model/ModelBlock.java, BlockPart.java, BlockPartFace.java,
// BlockFaceUV.java, BlockPartRotation.java, ItemTransformVec3f.java and ItemCameraTransforms.java
// A block or item model JSON (models/**.json) before baking: elements, textures by variable, the parent chain.

#include "client/renderer/vector/LwjglVector.h"
#include "util/EnumFacing.h"
#include "util/ResourceLocation.h"

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mc {

namespace gson {
struct JsonElement;
}

struct BlockFaceUV {
    // Empty until set: Java's null uvs, filled in from the element bounds.
    std::optional<std::array<float, 4>> uvs;
    int rotation = 0;

    float func_178348_a(int vertex) const;
    float func_178346_b(int vertex) const;
    int func_178345_c(int vertex) const { return (vertex + (4 - rotation / 90)) % 4; }
    void setUvs(const std::array<float, 4>& uvsIn)
    {
        if (!uvs)
            uvs = uvsIn;
    }

    static BlockFaceUV deserialize(const gson::JsonElement& json);

private:
    int func_178347_d(int vertex) const { return (vertex + rotation / 90) % 4; }
};

struct BlockPartFace {
    std::optional<EnumFacing> cullFace;
    int tintIndex = -1;
    std::string texture;
    BlockFaceUV blockFaceUV;

    static BlockPartFace deserialize(const gson::JsonElement& json);
};

struct BlockPartRotation {
    lwjgl::Vector3f origin;
    Axis axis;
    float angle;
    bool rescale;
};

struct BlockPart {
    lwjgl::Vector3f positionFrom;
    lwjgl::Vector3f positionTo;
    // Java: an EnumMap, so faces iterate in EnumFacing order. (ItemModelGenerator's HashMap of two faces
    // iterates by identity hash in Java, an order that changes between runs; this uses the enum order.)
    std::map<EnumFacing, BlockPartFace> mapFaces;
    std::optional<BlockPartRotation> partRotation;
    bool shade = true;

    BlockPart(const lwjgl::Vector3f& from, const lwjgl::Vector3f& to, std::map<EnumFacing, BlockPartFace> faces, std::optional<BlockPartRotation> rotation,
        bool shadeIn);

    static BlockPart deserialize(const gson::JsonElement& json);

private:
    std::array<float, 4> getFaceUvs(EnumFacing facing) const;
};

struct ItemTransformVec3f {
    lwjgl::Vector3f rotation;
    lwjgl::Vector3f translation;
    lwjgl::Vector3f scale { 1.0f, 1.0f, 1.0f };

    bool operator==(const ItemTransformVec3f& other) const = default;

    // Java compares against this instance by reference in ItemCameraTransforms.applyTransform.
    static const std::shared_ptr<const ItemTransformVec3f> DEFAULT;
    static std::shared_ptr<const ItemTransformVec3f> deserialize(const gson::JsonElement& json);
};

enum class TransformType { NONE, THIRD_PERSON, FIRST_PERSON, HEAD, GUI, GROUND, FIXED };

struct ItemCameraTransforms {
    std::shared_ptr<const ItemTransformVec3f> thirdPerson = ItemTransformVec3f::DEFAULT;
    std::shared_ptr<const ItemTransformVec3f> firstPerson = ItemTransformVec3f::DEFAULT;
    std::shared_ptr<const ItemTransformVec3f> head = ItemTransformVec3f::DEFAULT;
    std::shared_ptr<const ItemTransformVec3f> gui = ItemTransformVec3f::DEFAULT;
    std::shared_ptr<const ItemTransformVec3f> ground = ItemTransformVec3f::DEFAULT;
    std::shared_ptr<const ItemTransformVec3f> fixed = ItemTransformVec3f::DEFAULT;

    // Java: the static tweak fields (all zero; debugging leftovers) are folded away.
    void applyTransform(TransformType type) const;
    const std::shared_ptr<const ItemTransformVec3f>& getTransform(TransformType type) const;
    bool func_181687_c(TransformType type) const { return !(*getTransform(type) == *ItemTransformVec3f::DEFAULT); }

    static ItemCameraTransforms deserialize(const gson::JsonElement& json);
};

// Thrown by checkModelHierarchy for a parent cycle (Java: ModelBlock.LoopException).
class ModelLoopException : public std::runtime_error {
public:
    ModelLoopException()
        : std::runtime_error("ModelBlock.LoopException")
    {
    }
};

class ModelBlock {
public:
    // Throws gson::JsonSyntaxException (Java: JsonParseException) and std::runtime_error for bad models.
    static std::shared_ptr<ModelBlock> deserialize(std::string_view json);

    ModelBlock(std::vector<BlockPart> elementsIn, std::unordered_map<std::string, std::string> texturesIn, bool ambientOcclusionIn, bool gui3dIn,
        ItemCameraTransforms cameraTransformsIn);
    ModelBlock(ResourceLocation parentLocationIn, std::unordered_map<std::string, std::string> texturesIn, bool ambientOcclusionIn, bool gui3dIn,
        ItemCameraTransforms cameraTransformsIn);

    const std::vector<BlockPart>& getElements() const { return parent ? parent->getElements() : elements; }
    bool isAmbientOcclusion() const { return parent ? parent->isAmbientOcclusion() : ambientOcclusion; }
    bool isGui3d() const { return gui3d; }
    bool isResolved() const { return !parentLocation || (parent && parent->isResolved()); }
    void getParentFromMap(const std::unordered_map<ResourceLocation, std::shared_ptr<ModelBlock>>& models);
    bool isTexturePresent(const std::string& textureName) const { return resolveTextureName(textureName) != "missingno"; }
    std::string resolveTextureName(std::string textureName) const;
    const std::optional<ResourceLocation>& getParentLocation() const { return parentLocation; }
    const ModelBlock* getRootModel() const { return parent ? parent->getRootModel() : this; }
    ItemCameraTransforms getAllTransforms() const;
    const ModelBlock* getParent() const { return parent; }

    std::string name;

private:
    struct Bookkeep {
        const ModelBlock* model;
        const ModelBlock* modelExt = nullptr;
    };

    std::string resolveTextureName(const std::string& textureName, Bookkeep& bookkeep) const;
    std::shared_ptr<const ItemTransformVec3f> getTransform(TransformType type) const;

    std::vector<BlockPart> elements;
    bool gui3d;
    bool ambientOcclusion;
    ItemCameraTransforms cameraTransforms;
    std::unordered_map<std::string, std::string> textures;
    const ModelBlock* parent = nullptr;
    std::optional<ResourceLocation> parentLocation;
};

// Java: ModelBlock.checkModelHierarchy -- throws ModelLoopException for a parent cycle.
void checkModelHierarchy(const std::vector<const ModelBlock*>& models);

} // namespace mc
