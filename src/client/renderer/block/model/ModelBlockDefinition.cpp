#include "client/renderer/block/model/ModelBlockDefinition.h"

#include "client/resources/model/ModelRotation.h"
#include "core/GsonLenient.h"
#include "util/JsonUtils.h"

namespace mc {

using gson::JsonElement;

namespace {

ModelBlockDefinition::Variant parseVariant(const JsonElement& json)
{
    if (!json.isJsonObject())
        throw std::runtime_error("Not a JSON Object: " + JsonUtils::toString(json));
    const std::string model = JsonUtils::getString(json, "model");
    const int x = JsonUtils::getInt(json, "x", 0);
    const int y = JsonUtils::getInt(json, "y", 0);
    const ModelRotation* rotation = ModelRotation::getModelRotation(x, y);
    if (!rotation)
        throw gson::JsonSyntaxException("Invalid BlockModelRotation x: " + std::to_string(x) + ", y: " + std::to_string(y));
    const bool uvLock = JsonUtils::getBoolean(json, "uvlock", false);
    const int weight = JsonUtils::getInt(json, "weight", 1);
    const ResourceLocation location(model);
    return { ResourceLocation(location.getResourceDomain(), "block/" + location.getResourcePath()), rotation, uvLock, weight };
}

} // namespace

ModelBlockDefinition::ModelBlockDefinition(const std::vector<ModelBlockDefinition>& definitions)
{
    for (const ModelBlockDefinition& definition : definitions) {
        for (const auto& [name, variants] : definition.mapVariants)
            mapVariants.insert_or_assign(name, variants);
    }
}

ModelBlockDefinition ModelBlockDefinition::parseFromReader(std::string_view json)
{
    const std::shared_ptr<JsonElement> root = gson::parse(json);
    if (!root->isJsonObject())
        throw std::runtime_error("Not a JSON Object: " + JsonUtils::toString(*root));
    ModelBlockDefinition definition;
    for (const auto& [name, value] : JsonUtils::getJsonObject(*root, "variants").object) {
        Variants variants { name, {} };
        if (value->type == JsonElement::Type::ARRAY) {
            for (const auto& element : value->array)
                variants.listVariants.push_back(parseVariant(*element));
        } else {
            variants.listVariants.push_back(parseVariant(*value));
        }
        definition.mapVariants.insert_or_assign(name, std::move(variants));
    }
    return definition;
}

const ModelBlockDefinition::Variants& ModelBlockDefinition::getVariants(const std::string& name) const
{
    const auto it = mapVariants.find(name);
    if (it == mapVariants.end())
        throw MissingVariantException();
    return it->second;
}

} // namespace mc
