#include "client/resources/IResource.h"

#include <nlohmann/json.hpp>

namespace mc {

SimpleResource::SimpleResource(std::string packName, ResourceLocation resourceLocation, std::vector<uint8_t> contents,
    std::optional<std::vector<uint8_t>> metadata)
    : resourcePackName(std::move(packName))
    , location(std::move(resourceLocation))
    , data(std::move(contents))
    , mcmeta(std::move(metadata))
{
}

SimpleResource::~SimpleResource() = default;

const nlohmann::json* SimpleResource::getMetadata(const std::string& section) const
{
    if (!hasMetadata())
        return nullptr;

    if (!mcmetaJson && !mcmetaJsonChecked) {
        mcmetaJsonChecked = true;
        mcmetaJson = std::make_unique<nlohmann::json>(nlohmann::json::parse(mcmeta->begin(), mcmeta->end()));
    }
    if (!mcmetaJson || !mcmetaJson->is_object())
        return nullptr;

    const auto it = mcmetaJson->find(section);
    return it == mcmetaJson->end() ? nullptr : &*it;
}

} // namespace mc
