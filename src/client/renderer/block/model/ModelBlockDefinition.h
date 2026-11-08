#pragma once

// Original: net/minecraft/client/renderer/block/model/ModelBlockDefinition.java
// A blockstates/*.json file: per variant name ("facing=north,half=top"), the weighted list of models with their
// rotation and uv lock. Definitions from several resource packs merge, later packs replacing whole variants.

#include "util/ResourceLocation.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mc {

class ModelRotation;

class ModelBlockDefinition {
public:
    struct Variant {
        ResourceLocation modelLocation;
        const ModelRotation* modelRotation;
        bool uvLock;
        int weight;

        const ResourceLocation& getModelLocation() const { return modelLocation; }
        const ModelRotation& getRotation() const { return *modelRotation; }
        bool isUvLocked() const { return uvLock; }
        int getWeight() const { return weight; }
    };

    struct Variants {
        std::string name;
        std::vector<Variant> listVariants;

        const std::vector<Variant>& getVariants() const { return listVariants; }
    };

    class MissingVariantException : public std::runtime_error {
    public:
        MissingVariantException()
            : std::runtime_error("MissingVariantException")
        {
        }
    };

    ModelBlockDefinition() = default;
    explicit ModelBlockDefinition(const std::vector<ModelBlockDefinition>& definitions);

    // Throws gson::JsonSyntaxException / std::runtime_error on bad JSON.
    static ModelBlockDefinition parseFromReader(std::string_view json);

    // Throws MissingVariantException.
    const Variants& getVariants(const std::string& name) const;

private:
    std::unordered_map<std::string, Variants> mapVariants;
};

} // namespace mc
