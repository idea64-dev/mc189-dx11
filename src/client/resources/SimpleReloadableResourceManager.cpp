#include "client/resources/SimpleReloadableResourceManager.h"

#include "client/resources/IResourcePack.h"
#include "core/Log.h"

#include <algorithm>

namespace mc {

// --- FallbackResourceManager ---

void FallbackResourceManager::addResourcePack(std::shared_ptr<IResourcePack> resourcePack)
{
    resourcePacks.push_back(std::move(resourcePack));
}

ResourceLocation FallbackResourceManager::getLocationMcmeta(const ResourceLocation& location)
{
    return ResourceLocation(location.getResourceDomain(), location.getResourcePath() + ".mcmeta");
}

std::shared_ptr<IResource> FallbackResourceManager::getResource(const ResourceLocation& location) const
{
    const IResourcePack* metadataPack = nullptr;
    const ResourceLocation mcmetaLocation = getLocationMcmeta(location);

    for (auto it = resourcePacks.rbegin(); it != resourcePacks.rend(); ++it) {
        const IResourcePack& pack = **it;
        if (!metadataPack && pack.resourceExists(mcmetaLocation))
            metadataPack = &pack;

        if (pack.resourceExists(location)) {
            std::optional<std::vector<uint8_t>> mcmeta;
            if (metadataPack)
                mcmeta = metadataPack->getInputStream(mcmetaLocation);
            return std::make_shared<SimpleResource>(pack.getPackName(), location, pack.getInputStream(location), std::move(mcmeta));
        }
    }
    throw ResourceNotFoundException(location.toString());
}

std::vector<std::shared_ptr<IResource>> FallbackResourceManager::getAllResources(const ResourceLocation& location) const
{
    std::vector<std::shared_ptr<IResource>> list;
    const ResourceLocation mcmetaLocation = getLocationMcmeta(location);

    for (const auto& pack : resourcePacks) {
        if (pack->resourceExists(location)) {
            std::optional<std::vector<uint8_t>> mcmeta;
            if (pack->resourceExists(mcmetaLocation))
                mcmeta = pack->getInputStream(mcmetaLocation);
            list.push_back(std::make_shared<SimpleResource>(pack->getPackName(), location, pack->getInputStream(location), std::move(mcmeta)));
        }
    }
    if (list.empty())
        throw ResourceNotFoundException(location.toString());
    return list;
}

// --- SimpleReloadableResourceManager ---

void SimpleReloadableResourceManager::reloadResourcePack(std::shared_ptr<IResourcePack> resourcePack)
{
    for (const std::string& domain : resourcePack->getResourceDomains()) {
        if (std::find(setResourceDomains.begin(), setResourceDomains.end(), domain) == setResourceDomains.end())
            setResourceDomains.push_back(domain);
        domainResourceManagers[domain].addResourcePack(resourcePack);
    }
}

std::set<std::string> SimpleReloadableResourceManager::getResourceDomains() const
{
    return { setResourceDomains.begin(), setResourceDomains.end() };
}

std::shared_ptr<IResource> SimpleReloadableResourceManager::getResource(const ResourceLocation& location) const
{
    const auto it = domainResourceManagers.find(location.getResourceDomain());
    if (it == domainResourceManagers.end())
        throw ResourceNotFoundException(location.toString());
    return it->second.getResource(location);
}

std::vector<std::shared_ptr<IResource>> SimpleReloadableResourceManager::getAllResources(const ResourceLocation& location) const
{
    const auto it = domainResourceManagers.find(location.getResourceDomain());
    if (it == domainResourceManagers.end())
        throw ResourceNotFoundException(location.toString());
    return it->second.getAllResources(location);
}

void SimpleReloadableResourceManager::clearResources()
{
    domainResourceManagers.clear();
    setResourceDomains.clear();
}

void SimpleReloadableResourceManager::reloadResources(const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks)
{
    clearResources();

    std::string names;
    for (const auto& pack : resourcePacks)
        names += (names.empty() ? "" : ", ") + pack->getPackName();
    Log::info("Reloading ResourceManager: {}", names);

    for (const auto& pack : resourcePacks)
        reloadResourcePack(pack);
    notifyReloadListeners();
}

void SimpleReloadableResourceManager::registerReloadListener(IResourceManagerReloadListener& reloadListener)
{
    reloadListeners.push_back(&reloadListener);
    reloadListener.onResourceManagerReload(*this);
}

void SimpleReloadableResourceManager::notifyReloadListeners()
{
    for (IResourceManagerReloadListener* listener : reloadListeners)
        listener->onResourceManagerReload(*this);
}

} // namespace mc
