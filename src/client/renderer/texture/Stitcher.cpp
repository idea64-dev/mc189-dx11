#include "client/renderer/texture/Stitcher.h"

#include "client/renderer/texture/TextureAtlasSprite.h"
#include "core/Java.h"
#include "core/JavaString.h"
#include "util/MathHelper.h"

#include <algorithm>

namespace mc {

Stitcher::Stitcher(int maxTextureWidth, int maxTextureHeight, bool forcePowerOf2In, int maxTileDimensionIn, int mipmapLevel)
    : mipmapLevelStitcher(mipmapLevel)
    , maxWidth(maxTextureWidth)
    , maxHeight(maxTextureHeight)
    , forcePowerOf2(forcePowerOf2In)
    , maxTileDimension(maxTileDimensionIn)
{
}

void Stitcher::addSprite(TextureAtlasSprite* sprite)
{
    auto holder = std::make_unique<Holder>(sprite, mipmapLevelStitcher);
    if (maxTileDimension > 0)
        holder->setNewDimension(maxTileDimension);
    setStitchHolders.push_back(std::move(holder));
}

void Stitcher::doStitch()
{
    std::vector<Holder*> holders;
    holders.reserve(setStitchHolders.size());
    for (const auto& holder : setStitchHolders)
        holders.push_back(holder.get());
    // Arrays.sort on objects is stable; the order is total anyway (names are unique).
    std::stable_sort(holders.begin(), holders.end(), [](const Holder* a, const Holder* b) { return a->compareTo(*b) < 0; });

    for (Holder* holder : holders) {
        if (!allocateSlot(holder)) {
            const TextureAtlasSprite* sprite = holder->getAtlasSprite();
            throw StitcherException("Unable to fit: " + sprite->getIconName() + " - size: " + std::to_string(sprite->getIconWidth()) + "x"
                + std::to_string(sprite->getIconHeight()) + " - Maybe try a lowerresolution resourcepack?");
        }
    }

    if (forcePowerOf2) {
        currentWidth = MathHelper::roundUpToPowerOfTwo(currentWidth);
        currentHeight = MathHelper::roundUpToPowerOfTwo(currentHeight);
    }
}

std::vector<TextureAtlasSprite*> Stitcher::getStichSlots()
{
    std::vector<const Slot*> list;
    for (const Slot& slot : stitchSlots)
        slot.getAllStitchSlots(list);

    std::vector<TextureAtlasSprite*> sprites;
    sprites.reserve(list.size());
    for (const Slot* slot : list) {
        Holder* holder = slot->getStitchHolder();
        TextureAtlasSprite* sprite = holder->getAtlasSprite();
        sprite->initSprite(currentWidth, currentHeight, slot->getOriginX(), slot->getOriginY(), holder->isRotated());
        sprites.push_back(sprite);
    }
    return sprites;
}

int Stitcher::getMipmapDimension(int size, int level)
{
    return ((size >> level) + ((size & ((1 << level) - 1)) == 0 ? 0 : 1)) << level;
}

bool Stitcher::allocateSlot(Holder* holder)
{
    for (Slot& slot : stitchSlots) {
        if (slot.addSlot(holder))
            return true;
        holder->rotate();
        if (slot.addSlot(holder))
            return true;
        holder->rotate();
    }
    return expandAndAllocateSlot(holder);
}

bool Stitcher::expandAndAllocateSlot(Holder* holder)
{
    const int i = std::min(holder->getWidth(), holder->getHeight());
    const bool flag = currentWidth == 0 && currentHeight == 0;
    bool flag1;

    if (forcePowerOf2) {
        const int j = MathHelper::roundUpToPowerOfTwo(currentWidth);
        const int k = MathHelper::roundUpToPowerOfTwo(currentHeight);
        const int l = MathHelper::roundUpToPowerOfTwo(currentWidth + i);
        const int i1 = MathHelper::roundUpToPowerOfTwo(currentHeight + i);
        const bool flag2 = l <= maxWidth;
        const bool flag3 = i1 <= maxHeight;
        if (!flag2 && !flag3)
            return false;
        const bool flag4 = j != l;
        const bool flag5 = k != i1;
        if (flag4 ^ flag5)
            flag1 = !flag4;
        else
            flag1 = flag2 && j <= k;
    } else {
        const bool flag6 = currentWidth + i <= maxWidth;
        const bool flag7 = currentHeight + i <= maxHeight;
        if (!flag6 && !flag7)
            return false;
        flag1 = flag6 && (flag || currentWidth <= currentHeight);
    }

    const int j1 = std::max(holder->getWidth(), holder->getHeight());
    if (MathHelper::roundUpToPowerOfTwo((flag1 ? currentHeight : currentWidth) + j1) > (flag1 ? maxHeight : maxWidth))
        return false;

    if (flag1) {
        if (holder->getWidth() > holder->getHeight())
            holder->rotate();
        if (currentHeight == 0)
            currentHeight = holder->getHeight();
        stitchSlots.emplace_back(currentWidth, 0, holder->getWidth(), currentHeight);
        currentWidth += holder->getWidth();
    } else {
        stitchSlots.emplace_back(0, currentHeight, currentWidth, holder->getHeight());
        currentHeight += holder->getHeight();
    }
    stitchSlots.back().addSlot(holder);
    return true;
}

Stitcher::Holder::Holder(TextureAtlasSprite* texture, int mipmapLevel)
    : theTexture(texture)
    , width(texture->getIconWidth())
    , height(texture->getIconHeight())
    , mipmapLevelHolder(mipmapLevel)
    , rotated(getMipmapDimension(height, mipmapLevel) > getMipmapDimension(width, mipmapLevel))
{
}

int Stitcher::Holder::getWidth() const
{
    return rotated ? getMipmapDimension(java::f2i(static_cast<float>(height) * scaleFactor), mipmapLevelHolder)
                   : getMipmapDimension(java::f2i(static_cast<float>(width) * scaleFactor), mipmapLevelHolder);
}

int Stitcher::Holder::getHeight() const
{
    return rotated ? getMipmapDimension(java::f2i(static_cast<float>(width) * scaleFactor), mipmapLevelHolder)
                   : getMipmapDimension(java::f2i(static_cast<float>(height) * scaleFactor), mipmapLevelHolder);
}

void Stitcher::Holder::setNewDimension(int dimension)
{
    if (width > dimension && height > dimension)
        scaleFactor = static_cast<float>(dimension) / static_cast<float>(std::min(width, height));
}

int Stitcher::Holder::compareTo(const Holder& other) const
{
    if (getHeight() == other.getHeight()) {
        if (getWidth() == other.getWidth())
            return java::compareTo(theTexture->getIconName(), other.theTexture->getIconName());
        return getWidth() < other.getWidth() ? 1 : -1;
    }
    return getHeight() < other.getHeight() ? 1 : -1;
}

bool Stitcher::Slot::addSlot(Holder* holderIn)
{
    if (holder)
        return false;
    const int i = holderIn->getWidth();
    const int j = holderIn->getHeight();
    if (i > width || j > height)
        return false;
    if (i == width && j == height) {
        holder = holderIn;
        return true;
    }
    if (!hasSubSlots) {
        hasSubSlots = true;
        subSlots.reserve(3);
        subSlots.emplace_back(originX, originY, i, j);
        const int k = width - i;
        const int l = height - j;
        if (l > 0 && k > 0) {
            const int i1 = std::max(height, k);
            const int j1 = std::max(width, l);
            if (i1 >= j1) {
                subSlots.emplace_back(originX, originY + j, i, l);
                subSlots.emplace_back(originX + i, originY, k, height);
            } else {
                subSlots.emplace_back(originX + i, originY, k, j);
                subSlots.emplace_back(originX, originY + j, width, l);
            }
        } else if (k == 0) {
            subSlots.emplace_back(originX, originY + j, i, l);
        } else if (l == 0) {
            subSlots.emplace_back(originX + i, originY, k, j);
        }
    }
    for (Slot& slot : subSlots) {
        if (slot.addSlot(holderIn))
            return true;
    }
    return false;
}

void Stitcher::Slot::getAllStitchSlots(std::vector<const Slot*>& out) const
{
    if (holder) {
        out.push_back(this);
    } else if (hasSubSlots) {
        for (const Slot& slot : subSlots)
            slot.getAllStitchSlots(out);
    }
}

} // namespace mc
