#include "client/renderer/block/model/ModelBlock.h"

#include "client/renderer/DxStateManager.h"
#include "core/GsonLenient.h"
#include "core/JavaString.h"
#include "core/Log.h"
#include "util/JsonUtils.h"
#include "util/MathHelper.h"

#include <cmath>
#include <stdexcept>

namespace mc {

using gson::JsonElement;
using gson::JsonSyntaxException;

namespace {

// Java: getAsJsonObject() on a JsonElement, an IllegalStateException when it is not one.
const JsonElement& asObject(const JsonElement& element)
{
    if (!element.isJsonObject())
        throw std::runtime_error("Not a JSON Object: " + JsonUtils::toString(element));
    return element;
}

// JsonElement.getAsString(): primitives only (UnsupportedOperationException / IllegalStateException otherwise).
std::string elementAsString(const JsonElement& element)
{
    if (!element.isJsonPrimitive())
        throw std::runtime_error("JsonElement.getAsString on " + JsonUtils::toString(element));
    return JsonUtils::getAsString(element);
}

lwjgl::Vector3f parseVector(const JsonElement& object, const std::string& key)
{
    const JsonElement& array = JsonUtils::getJsonArray(object, key);
    if (array.array.size() != 3)
        throw JsonSyntaxException("Expected 3 " + key + " values, found: " + std::to_string(array.array.size()));
    float values[3];
    for (size_t i = 0; i < 3; ++i)
        values[i] = JsonUtils::asFloat(*array.array[i], key + "[" + std::to_string(i) + "]");
    return { values[0], values[1], values[2] };
}

std::string vectorToString(const lwjgl::Vector3f& v)
{
    return "Vector3f[" + java::toString(v.x) + ", " + java::toString(v.y) + ", " + java::toString(v.z) + "]";
}

lwjgl::Vector3f parseBoundedPosition(const JsonElement& object, const std::string& key)
{
    const lwjgl::Vector3f v = parseVector(object, key);
    if (v.x >= -16.0f && v.y >= -16.0f && v.z >= -16.0f && v.x <= 32.0f && v.y <= 32.0f && v.z <= 32.0f)
        return v;
    throw JsonSyntaxException("'" + key + "' specifier exceeds the allowed boundaries: " + vectorToString(v));
}

bool startsWithHash(const std::string& name)
{
    if (name.empty())
        throw std::out_of_range("String index out of range: 0"); // Java: charAt(0) on ""
    return name[0] == '#';
}

} // namespace

// --- BlockFaceUV ---

float BlockFaceUV::func_178348_a(int vertex) const
{
    if (!uvs)
        throw std::runtime_error("uvs");
    const int i = func_178347_d(vertex);
    return i != 0 && i != 1 ? (*uvs)[2] : (*uvs)[0];
}

float BlockFaceUV::func_178346_b(int vertex) const
{
    if (!uvs)
        throw std::runtime_error("uvs");
    const int i = func_178347_d(vertex);
    return i != 0 && i != 3 ? (*uvs)[3] : (*uvs)[1];
}

BlockFaceUV BlockFaceUV::deserialize(const JsonElement& json)
{
    const JsonElement& object = asObject(json);
    BlockFaceUV uv;
    if (object.has("uv")) {
        const JsonElement& array = JsonUtils::getJsonArray(object, "uv");
        if (array.array.size() != 4)
            throw JsonSyntaxException("Expected 4 uv values, found: " + std::to_string(array.array.size()));
        std::array<float, 4> values {};
        for (size_t i = 0; i < 4; ++i)
            values[i] = JsonUtils::asFloat(*array.array[i], "uv[" + std::to_string(i) + "]");
        uv.uvs = values;
    }
    const int rotation = JsonUtils::getInt(object, "rotation", 0);
    if (rotation < 0 || rotation % 90 != 0 || rotation / 90 > 3)
        throw JsonSyntaxException("Invalid rotation " + std::to_string(rotation) + " found, only 0/90/180/270 allowed");
    uv.rotation = rotation;
    return uv;
}

// --- BlockPartFace ---

BlockPartFace BlockPartFace::deserialize(const JsonElement& json)
{
    const JsonElement& object = asObject(json);
    BlockPartFace face;
    face.cullFace = facingByName(JsonUtils::getString(object, "cullface", ""));
    face.tintIndex = JsonUtils::getInt(object, "tintindex", -1);
    face.texture = JsonUtils::getString(object, "texture");
    face.blockFaceUV = BlockFaceUV::deserialize(object);
    return face;
}

// --- BlockPart ---

BlockPart::BlockPart(const lwjgl::Vector3f& from, const lwjgl::Vector3f& to, std::map<EnumFacing, BlockPartFace> faces,
    std::optional<BlockPartRotation> rotation, bool shadeIn)
    : positionFrom(from)
    , positionTo(to)
    , mapFaces(std::move(faces))
    , partRotation(rotation)
    , shade(shadeIn)
{
    for (auto& [facing, face] : mapFaces)
        face.blockFaceUV.setUvs(getFaceUvs(facing));
}

std::array<float, 4> BlockPart::getFaceUvs(EnumFacing facing) const
{
    switch (facing) {
    case EnumFacing::DOWN:
    case EnumFacing::UP:
        return { positionFrom.x, positionFrom.z, positionTo.x, positionTo.z };
    case EnumFacing::NORTH:
    case EnumFacing::SOUTH:
        return { positionFrom.x, 16.0f - positionTo.y, positionTo.x, 16.0f - positionFrom.y };
    case EnumFacing::WEST:
    case EnumFacing::EAST:
        return { positionFrom.z, 16.0f - positionTo.y, positionTo.z, 16.0f - positionFrom.y };
    }
    throw std::runtime_error("NullPointerException");
}

BlockPart BlockPart::deserialize(const JsonElement& json)
{
    const JsonElement& object = asObject(json);
    const lwjgl::Vector3f from = parseBoundedPosition(object, "from");
    const lwjgl::Vector3f to = parseBoundedPosition(object, "to");

    std::optional<BlockPartRotation> rotation;
    if (object.has("rotation")) {
        const JsonElement& rotationObject = JsonUtils::getJsonObject(object, "rotation");
        lwjgl::Vector3f origin = parseVector(rotationObject, "origin");
        origin.scale(0.0625f);
        const std::string axisName = JsonUtils::getString(rotationObject, "axis");
        const std::optional<Axis> axis = axisByName(axisName);
        if (!axis)
            throw JsonSyntaxException("Invalid rotation axis: " + axisName);
        const float angle = JsonUtils::getFloat(rotationObject, "angle");
        if (angle != 0.0f && std::fabs(angle) != 22.5f && std::fabs(angle) != 45.0f)
            throw JsonSyntaxException("Invalid rotation " + java::toString(angle) + " found, only -45/-22.5/0/22.5/45 allowed");
        const bool rescale = JsonUtils::getBoolean(rotationObject, "rescale", false);
        rotation = BlockPartRotation { origin, *axis, angle, rescale };
    }

    std::map<EnumFacing, BlockPartFace> faces;
    const JsonElement& facesObject = JsonUtils::getJsonObject(object, "faces");
    for (const auto& [key, value] : facesObject.object) {
        const std::optional<EnumFacing> facing = facingByName(key);
        if (!facing)
            throw JsonSyntaxException("Unknown facing: " + key);
        faces.insert_or_assign(*facing, BlockPartFace::deserialize(*value));
    }
    if (faces.empty())
        throw JsonSyntaxException("Expected between 1 and 6 unique faces, got 0");

    if (object.has("shade") && !JsonUtils::isBoolean(object, "shade"))
        throw JsonSyntaxException("Expected shade to be a Boolean");
    const bool shade = JsonUtils::getBoolean(object, "shade", true);
    return BlockPart(from, to, std::move(faces), rotation, shade);
}

// --- ItemTransformVec3f / ItemCameraTransforms ---

const std::shared_ptr<const ItemTransformVec3f> ItemTransformVec3f::DEFAULT = std::make_shared<const ItemTransformVec3f>();

std::shared_ptr<const ItemTransformVec3f> ItemTransformVec3f::deserialize(const JsonElement& json)
{
    const JsonElement& object = asObject(json);
    auto transform = std::make_shared<ItemTransformVec3f>();
    if (object.has("rotation"))
        transform->rotation = parseVector(object, "rotation");
    lwjgl::Vector3f translation = object.has("translation") ? parseVector(object, "translation") : lwjgl::Vector3f();
    translation.scale(0.0625f);
    translation.x = MathHelper::clamp_float(translation.x, -1.5f, 1.5f);
    translation.y = MathHelper::clamp_float(translation.y, -1.5f, 1.5f);
    translation.z = MathHelper::clamp_float(translation.z, -1.5f, 1.5f);
    transform->translation = translation;
    lwjgl::Vector3f scale = object.has("scale") ? parseVector(object, "scale") : lwjgl::Vector3f(1.0f, 1.0f, 1.0f);
    scale.x = MathHelper::clamp_float(scale.x, -4.0f, 4.0f);
    scale.y = MathHelper::clamp_float(scale.y, -4.0f, 4.0f);
    scale.z = MathHelper::clamp_float(scale.z, -4.0f, 4.0f);
    transform->scale = scale;
    return transform;
}

void ItemCameraTransforms::applyTransform(TransformType type) const
{
    const std::shared_ptr<const ItemTransformVec3f>& t = getTransform(type);
    if (t != ItemTransformVec3f::DEFAULT) {
        DxStateManager::translate(t->translation.x + 0.0f, t->translation.y + 0.0f, t->translation.z + 0.0f);
        DxStateManager::rotate(t->rotation.y + 0.0f, 0.0f, 1.0f, 0.0f);
        DxStateManager::rotate(t->rotation.x + 0.0f, 1.0f, 0.0f, 0.0f);
        DxStateManager::rotate(t->rotation.z + 0.0f, 0.0f, 0.0f, 1.0f);
        DxStateManager::scale(t->scale.x + 0.0f, t->scale.y + 0.0f, t->scale.z + 0.0f);
    }
}

const std::shared_ptr<const ItemTransformVec3f>& ItemCameraTransforms::getTransform(TransformType type) const
{
    switch (type) {
    case TransformType::THIRD_PERSON:
        return thirdPerson;
    case TransformType::FIRST_PERSON:
        return firstPerson;
    case TransformType::HEAD:
        return head;
    case TransformType::GUI:
        return gui;
    case TransformType::GROUND:
        return ground;
    case TransformType::FIXED:
        return fixed;
    default:
        return ItemTransformVec3f::DEFAULT;
    }
}

ItemCameraTransforms ItemCameraTransforms::deserialize(const JsonElement& json)
{
    const JsonElement& object = asObject(json);
    const auto read = [&](const char* key) {
        return object.has(key) ? ItemTransformVec3f::deserialize(object.get(key)) : ItemTransformVec3f::DEFAULT;
    };
    ItemCameraTransforms transforms;
    transforms.thirdPerson = read("thirdperson");
    transforms.firstPerson = read("firstperson");
    transforms.head = read("head");
    transforms.gui = read("gui");
    transforms.ground = read("ground");
    transforms.fixed = read("fixed");
    return transforms;
}

// --- ModelBlock ---

ModelBlock::ModelBlock(std::vector<BlockPart> elementsIn, std::unordered_map<std::string, std::string> texturesIn, bool ambientOcclusionIn, bool gui3dIn,
    ItemCameraTransforms cameraTransformsIn)
    : elements(std::move(elementsIn))
    , gui3d(gui3dIn)
    , ambientOcclusion(ambientOcclusionIn)
    , cameraTransforms(std::move(cameraTransformsIn))
    , textures(std::move(texturesIn))
{
}

ModelBlock::ModelBlock(ResourceLocation parentLocationIn, std::unordered_map<std::string, std::string> texturesIn, bool ambientOcclusionIn, bool gui3dIn,
    ItemCameraTransforms cameraTransformsIn)
    : gui3d(gui3dIn)
    , ambientOcclusion(ambientOcclusionIn)
    , cameraTransforms(std::move(cameraTransformsIn))
    , textures(std::move(texturesIn))
    , parentLocation(std::move(parentLocationIn))
{
}

std::shared_ptr<ModelBlock> ModelBlock::deserialize(std::string_view json)
{
    const std::shared_ptr<JsonElement> root = gson::parse(json);
    const JsonElement& object = asObject(*root);

    std::vector<BlockPart> elements;
    if (object.has("elements")) {
        for (const auto& element : JsonUtils::getJsonArray(object, "elements").array)
            elements.push_back(BlockPart::deserialize(*element));
    }
    const std::string parent = JsonUtils::getString(object, "parent", "");
    const bool noParent = parent.empty();
    const bool noElements = elements.empty();
    if (noElements && noParent)
        throw JsonSyntaxException("BlockModel requires either elements or parent, found neither");
    if (!noParent && !noElements)
        throw JsonSyntaxException("BlockModel requires either elements or parent, found both");

    std::unordered_map<std::string, std::string> textures;
    if (object.has("textures")) {
        for (const auto& [key, value] : asObject(object.get("textures")).object)
            textures.insert_or_assign(key, elementAsString(*value));
    }
    const bool ambientOcclusion = JsonUtils::getBoolean(object, "ambientocclusion", true);
    ItemCameraTransforms transforms;
    if (object.has("display"))
        transforms = ItemCameraTransforms::deserialize(JsonUtils::getJsonObject(object, "display"));

    if (noElements)
        return std::make_shared<ModelBlock>(ResourceLocation(parent), std::move(textures), ambientOcclusion, true, std::move(transforms));
    return std::make_shared<ModelBlock>(std::move(elements), std::move(textures), ambientOcclusion, true, std::move(transforms));
}

void ModelBlock::getParentFromMap(const std::unordered_map<ResourceLocation, std::shared_ptr<ModelBlock>>& models)
{
    if (parentLocation) {
        const auto it = models.find(*parentLocation);
        parent = it == models.end() ? nullptr : it->second.get();
    }
}

std::string ModelBlock::resolveTextureName(std::string textureName) const
{
    if (!startsWithHash(textureName))
        textureName = '#' + textureName;
    Bookkeep bookkeep { this };
    return resolveTextureName(textureName, bookkeep);
}

std::string ModelBlock::resolveTextureName(const std::string& textureName, Bookkeep& bookkeep) const
{
    if (!startsWithHash(textureName))
        return textureName;
    if (this == bookkeep.modelExt) {
        Log::warn("Unable to resolve texture due to upward reference: {} in {}", textureName, name);
        return "missingno";
    }
    std::optional<std::string> s;
    if (const auto it = textures.find(textureName.substr(1)); it != textures.end())
        s = it->second;
    if (!s && parent)
        s = parent->resolveTextureName(textureName, bookkeep);
    bookkeep.modelExt = this;
    if (s && startsWithHash(*s))
        s = bookkeep.model->resolveTextureName(*s, bookkeep);
    return s && !startsWithHash(*s) ? *s : "missingno";
}

std::shared_ptr<const ItemTransformVec3f> ModelBlock::getTransform(TransformType type) const
{
    return parent && !cameraTransforms.func_181687_c(type) ? parent->getTransform(type) : cameraTransforms.getTransform(type);
}

ItemCameraTransforms ModelBlock::getAllTransforms() const
{
    ItemCameraTransforms transforms;
    transforms.thirdPerson = getTransform(TransformType::THIRD_PERSON);
    transforms.firstPerson = getTransform(TransformType::FIRST_PERSON);
    transforms.head = getTransform(TransformType::HEAD);
    transforms.gui = getTransform(TransformType::GUI);
    transforms.ground = getTransform(TransformType::GROUND);
    transforms.fixed = getTransform(TransformType::FIXED);
    return transforms;
}

void checkModelHierarchy(const std::vector<const ModelBlock*>& models)
{
    // Java walks with a tortoise and a hare and relies on the NullPointerException at the end of a chain.
    for (const ModelBlock* model : models) {
        if (!model)
            continue; // Java: NPE on a null model, swallowed
        const ModelBlock* slow = model->getParent();
        if (!slow)
            continue;
        const ModelBlock* fast = slow->getParent();
        while (true) {
            if (!fast)
                break;
            if (slow == fast)
                throw ModelLoopException();
            slow = slow->getParent();
            if (!fast->getParent())
                break;
            fast = fast->getParent()->getParent();
        }
    }
}

} // namespace mc
