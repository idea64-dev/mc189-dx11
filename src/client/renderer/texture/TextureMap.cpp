#include "client/renderer/texture/TextureMap.h"

#include "client/renderer/texture/Stitcher.h"
#include "client/renderer/texture/TextureAtlasSprite.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/resources/IResourceManager.h"
#include "client/resources/IResourcePack.h"
#include "client/resources/data/MetadataSections.h"
#include "core/Log.h"
#include "util/MathHelper.h"

#include <algorithm>
#include <bit>
#include <climits>

namespace mc {

const ResourceLocation TextureMap::LOCATION_MISSING_TEXTURE("missingno");
const ResourceLocation TextureMap::locationBlocksTexture("textures/atlas/blocks.png");

namespace {

// Java: Minecraft.getGLMaximumTextureSize(); Direct3D 11 guarantees 16384.
constexpr int kMaximumTextureSize = 16384;

// Java: TextureUtil.missingTextureData, the magenta and black checkerboard.
std::vector<uint32_t> missingTextureData()
{
    std::vector<uint32_t> data(256);
    for (int row = 0; row < 16; ++row) {
        for (int column = 0; column < 16; ++column)
            data[static_cast<size_t>(row * 16 + column)] = ((row < 8) == (column < 8)) ? 0xFFF800F8u : 0xFF000000u;
    }
    return data;
}

int lowestOneBit(int value)
{
    return value & -value;
}

} // namespace

TextureMap::TextureMap(std::string basePathIn, IIconCreator iconCreatorIn)
    : basePath(std::move(basePathIn))
    , iconCreator(std::move(iconCreatorIn))
    , missingImage(std::make_shared<TextureAtlasSprite>("missingno"))
{
}

void TextureMap::initMissingImage()
{
    missingImage->setIconWidth(16);
    missingImage->setIconHeight(16);
    SpriteFrame frame(static_cast<size_t>(mipmapLevels) + 1);
    frame[0] = missingTextureData();
    missingImage->setFramesTextureData({ std::move(frame) });
}

void TextureMap::loadTexture(const IResourceManager& resourceManager)
{
    if (iconCreator)
        loadSprites(resourceManager, iconCreator);
}

void TextureMap::loadSprites(const IResourceManager& resourceManager, const IIconCreator& creator)
{
    mapRegisteredSprites.clear();
    creator(*this);
    initMissingImage();
    deleteGlTexture();
    loadTextureAtlas(resourceManager);
}

void TextureMap::loadTextureAtlas(const IResourceManager& resourceManager)
{
    Stitcher stitcher(kMaximumTextureSize, kMaximumTextureSize, true, 0, mipmapLevels);
    mapUploadedSprites.clear();
    listAnimatedSprites.clear();
    uploadedInOrder.clear();
    int j = INT_MAX;
    int k = 1 << mipmapLevels;

    for (const auto& [name, sprite] : mapRegisteredSprites) {
        const ResourceLocation location(sprite->getIconName());
        const ResourceLocation fileLocation = completeResourceLocation(location, 0);
        try {
            const std::shared_ptr<IResource> resource = resourceManager.getResource(fileLocation);
            std::vector<BufferedImage> images(static_cast<size_t>(1 + mipmapLevels));
            images[0] = TextureUtil::readBufferedImage(resource->getInputStream());
            if (const std::optional<TextureMetadataSection> texture = TextureMetadataSection::fromResource(*resource)) {
                if (!texture->listMipmaps.empty()) {
                    const int l = images[0].width;
                    const int i1 = images[0].height;
                    if (MathHelper::roundUpToPowerOfTwo(l) != l || MathHelper::roundUpToPowerOfTwo(i1) != i1)
                        throw std::runtime_error("Unable to load extra miplevels, source-texture is not power of two");
                }
                for (const int i2 : texture->listMipmaps) {
                    // Java's bound check skips the last level too.
                    if (i2 > 0 && i2 < static_cast<int>(images.size()) - 1 && images[static_cast<size_t>(i2)].width == 0) {
                        const ResourceLocation mipLocation = completeResourceLocation(location, i2);
                        try {
                            images[static_cast<size_t>(i2)] = TextureUtil::readBufferedImage(resourceManager.getResource(mipLocation)->getInputStream());
                        } catch (const std::exception& e) {
                            Log::error("Unable to load miplevel {} from: {} ({})", i2, mipLocation.toString(), e.what());
                        }
                    }
                }
            }
            const std::optional<AnimationMetadataSection> animation = AnimationMetadataSection::fromResource(*resource);
            sprite->loadSprite(images, animation);
        } catch (const ResourceNotFoundException& e) {
            Log::error("Using missing texture, unable to load {} ({})", fileLocation.toString(), e.what());
            continue;
        } catch (const std::exception& e) {
            Log::error("Unable to parse metadata from {} ({})", fileLocation.toString(), e.what());
            continue;
        }

        j = std::min(j, std::min(sprite->getIconWidth(), sprite->getIconHeight()));
        const int l1 = std::min(lowestOneBit(sprite->getIconWidth()), lowestOneBit(sprite->getIconHeight()));
        if (l1 < k) {
            Log::warn("Texture {} with size {}x{} limits mip level from {} to {}", fileLocation.toString(), sprite->getIconWidth(), sprite->getIconHeight(),
                MathHelper::calculateLogBaseTwo(k), MathHelper::calculateLogBaseTwo(l1));
            k = l1;
        }
        stitcher.addSprite(sprite.get());
    }

    const int j1 = std::min(j, k);
    const int k1 = MathHelper::calculateLogBaseTwo(j1);
    if (k1 < mipmapLevels) {
        Log::warn("{}: dropping miplevel from {} to {}, because of minimum power of two: {}", basePath, mipmapLevels, k1, j1);
        mipmapLevels = k1;
    }

    for (const auto& [name, sprite] : mapRegisteredSprites)
        sprite->generateMipmaps(mipmapLevels);

    missingImage->generateMipmaps(mipmapLevels);
    stitcher.addSprite(missingImage.get());
    stitcher.doStitch();

    Log::info("Created: {}x{} {}-atlas", stitcher.getCurrentWidth(), stitcher.getCurrentHeight(), basePath);
    atlasWidth = stitcher.getCurrentWidth();
    atlasHeight = stitcher.getCurrentHeight();
    if (cpuAtlas) {
        cpuAtlas->assign(static_cast<size_t>(mipmapLevels) + 1, {});
        for (int level = 0; level <= mipmapLevels; ++level)
            (*cpuAtlas)[static_cast<size_t>(level)].assign(static_cast<size_t>((atlasWidth >> level) * (atlasHeight >> level)), 0u);
    } else {
        TextureUtil::allocateTextureImpl(getGlTextureId(), mipmapLevels, atlasWidth, atlasHeight);
    }

    std::unordered_map<std::string, TextureAtlasSprite*> notUploaded;
    for (const auto& [name, sprite] : mapRegisteredSprites)
        notUploaded.emplace(name, sprite.get());

    for (TextureAtlasSprite* sprite : stitcher.getStichSlots()) {
        const std::string& s = sprite->getIconName();
        notUploaded.erase(s);
        mapUploadedSprites[s] = sprite;
        uploadedInOrder.push_back(sprite);
        if (cpuAtlas)
            copyToCpuAtlas(*sprite);
        else
            TextureUtil::uploadTextureMipmap(sprite->getFrameTextureData(0), sprite->getIconWidth(), sprite->getIconHeight(), sprite->getOriginX(),
                sprite->getOriginY(), false, false);
        if (sprite->hasAnimationMetadata())
            listAnimatedSprites.push_back(sprite);
    }

    for (const auto& [name, sprite] : notUploaded)
        sprite->copyFrom(*missingImage);
}

void TextureMap::copyToCpuAtlas(const TextureAtlasSprite& sprite)
{
    const SpriteFrame& frame = sprite.getFrameTextureData(0);
    for (size_t level = 0; level < frame.size() && level < cpuAtlas->size(); ++level) {
        const int w = sprite.getIconWidth() >> level;
        const int h = sprite.getIconHeight() >> level;
        const int x0 = sprite.getOriginX() >> level;
        const int y0 = sprite.getOriginY() >> level;
        const int atlasLevelWidth = atlasWidth >> level;
        std::vector<uint32_t>& target = (*cpuAtlas)[level];
        for (int y = 0; y < h; ++y) {
            std::copy_n(frame[level].begin() + static_cast<std::ptrdiff_t>(y * w), w,
                target.begin() + static_cast<std::ptrdiff_t>((y0 + y) * atlasLevelWidth + x0));
        }
    }
}

ResourceLocation TextureMap::completeResourceLocation(const ResourceLocation& location, int mipLevel) const
{
    if (mipLevel == 0)
        return ResourceLocation(location.getResourceDomain(), basePath + "/" + location.getResourcePath() + ".png");
    return ResourceLocation(location.getResourceDomain(), basePath + "/mipmaps/" + location.getResourcePath() + "." + std::to_string(mipLevel) + ".png");
}

TextureAtlasSprite* TextureMap::getAtlasSprite(const std::string& iconName) const
{
    const auto it = mapUploadedSprites.find(iconName);
    return it == mapUploadedSprites.end() ? missingImage.get() : it->second;
}

void TextureMap::updateAnimations()
{
    TextureUtil::bindTexture(getGlTextureId());
    for (TextureAtlasSprite* sprite : listAnimatedSprites)
        sprite->updateAnimation();
}

std::shared_ptr<TextureAtlasSprite> TextureMap::registerSprite(const ResourceLocation& location)
{
    // Java looks the map up by the ResourceLocation although it is keyed by strings, so this always makes a new
    // sprite and replaces any earlier one of the same name.
    std::shared_ptr<TextureAtlasSprite> sprite = TextureAtlasSprite::makeAtlasSprite(location);
    mapRegisteredSprites[location.toString()] = sprite;
    return sprite;
}

} // namespace mc
