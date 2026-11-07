#pragma once

// Original: net/minecraft/client/resources/IResourceManager.java, IReloadableResourceManager.java,
// IResourceManagerReloadListener.java

#include "client/resources/IResource.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace mc {

class IResourcePack;

class IResourceManager {
public:
    virtual ~IResourceManager() = default;

    virtual std::set<std::string> getResourceDomains() const = 0;
    // Throws ResourceNotFoundException (Java: FileNotFoundException) when no pack has the resource.
    virtual std::shared_ptr<IResource> getResource(const ResourceLocation& location) const = 0;
    virtual std::vector<std::shared_ptr<IResource>> getAllResources(const ResourceLocation& location) const = 0;
};

class IResourceManagerReloadListener {
public:
    virtual ~IResourceManagerReloadListener() = default;
    virtual void onResourceManagerReload(const IResourceManager& resourceManager) = 0;
};

class IReloadableResourceManager : public IResourceManager {
public:
    virtual void reloadResources(const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks) = 0;
    // Not owned; the listener must outlive the manager or never be destroyed (true for the game's managers).
    virtual void registerReloadListener(IResourceManagerReloadListener& reloadListener) = 0;
};

} // namespace mc
