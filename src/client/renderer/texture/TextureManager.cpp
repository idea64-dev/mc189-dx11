#include "client/renderer/texture/TextureManager.h"

#include "client/renderer/texture/SimpleTexture.h"
#include "client/renderer/texture/TextureMap.h"
#include "client/renderer/texture/TextureUtil.h"
#include "core/Log.h"

#include <format>

namespace mc {

namespace {

// Java's missingTexture is a shared static; TextureManager stores it in its map like any other texture.
std::shared_ptr<ITextureObject> missingTextureObject()
{
    return std::shared_ptr<ITextureObject>(std::shared_ptr<ITextureObject> {}, &TextureUtil::missingTexture());
}

} // namespace

TextureManager::TextureManager(const IResourceManager& resourceManager)
    : theResourceManager(&resourceManager)
{
}

void TextureManager::bindTexture(const ResourceLocation& resource)
{
    std::shared_ptr<ITextureObject> texture = getTexture(resource);
    if (!texture) {
        texture = std::make_shared<SimpleTexture>(resource);
        loadTexture(resource, texture);
        texture = getTexture(resource); // may have been replaced by the missing texture
    }
    TextureUtil::bindTexture(texture->getGlTextureId());
}

bool TextureManager::loadTexture(const ResourceLocation& textureLocation, std::shared_ptr<ITextureObject> textureObj)
{
    bool loaded = true;
    try {
        textureObj->loadTexture(*theResourceManager);
    } catch (const std::exception& e) {
        // Java: IOException -> missing texture. Decoding and lookup failures are the only ways this throws.
        Log::warn("Failed to load texture: {} ({})", textureLocation.toString(), e.what());
        textureObj = missingTextureObject();
        loaded = false;
    }
    mapTextureObjects[textureLocation] = std::move(textureObj);
    return loaded;
}

bool TextureManager::loadTickableTexture(const ResourceLocation& textureLocation, std::shared_ptr<ITextureObject> textureObj, ITickableTexture* tickable)
{
    if (!loadTexture(textureLocation, std::move(textureObj)))
        return false;
    listTickables.push_back(tickable);
    return true;
}

void TextureManager::tick()
{
    for (ITickableTexture* tickable : listTickables)
        tickable->tick();
}

std::shared_ptr<ITextureObject> TextureManager::getTexture(const ResourceLocation& textureLocation) const
{
    const auto it = mapTextureObjects.find(textureLocation);
    return it == mapTextureObjects.end() ? nullptr : it->second;
}

ResourceLocation TextureManager::getDynamicTextureLocation(const std::string& name, std::shared_ptr<DynamicTexture> texture)
{
    const int counter = ++mapTextureCounters[name];
    ResourceLocation location(std::format("dynamic/{}_{}", name, counter));
    loadTexture(location, std::move(texture));
    return location;
}

void TextureManager::deleteTexture(const ResourceLocation& textureLocation)
{
    if (const std::shared_ptr<ITextureObject> texture = getTexture(textureLocation))
        TextureUtil::deleteTexture(texture->getGlTextureId());
}

void TextureManager::onResourceManagerReload(const IResourceManager& /*resourceManager*/)
{
    for (auto& [location, texture] : mapTextureObjects)
        loadTexture(location, texture);
}

} // namespace mc
