#pragma once

// Original: net/minecraft/client/resources/FallbackResourceManager.java and SimpleReloadableResourceManager.java

#include "client/resources/IResourceManager.h"

#include <map>
#include <vector>

namespace mc {

class FallbackResourceManager : public IResourceManager {
public:
    void addResourcePack(std::shared_ptr<IResourcePack> resourcePack);

    std::set<std::string> getResourceDomains() const override { return {}; }
    // The last added pack that has the resource wins; its .mcmeta comes from the last pack that has one.
    std::shared_ptr<IResource> getResource(const ResourceLocation& location) const override;
    std::vector<std::shared_ptr<IResource>> getAllResources(const ResourceLocation& location) const override;

    static ResourceLocation getLocationMcmeta(const ResourceLocation& location);

private:
    std::vector<std::shared_ptr<IResourcePack>> resourcePacks;
};

class SimpleReloadableResourceManager : public IReloadableResourceManager {
public:
    void reloadResourcePack(std::shared_ptr<IResourcePack> resourcePack);

    std::set<std::string> getResourceDomains() const override;
    std::shared_ptr<IResource> getResource(const ResourceLocation& location) const override;
    std::vector<std::shared_ptr<IResource>> getAllResources(const ResourceLocation& location) const override;

    void reloadResources(const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks) override;
    void registerReloadListener(IResourceManagerReloadListener& reloadListener) override;

private:
    void clearResources();
    void notifyReloadListeners();

    std::map<std::string, FallbackResourceManager> domainResourceManagers;
    std::vector<IResourceManagerReloadListener*> reloadListeners;
    std::vector<std::string> setResourceDomains; // insertion-ordered set (Java LinkedHashSet)
};

} // namespace mc
