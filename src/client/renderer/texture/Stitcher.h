#pragma once

// Original: net/minecraft/client/renderer/texture/Stitcher.java and client/renderer/StitcherException.java
// Packs the sprites of an atlas: sorted biggest first (then by name), each placed into the first slot of the
// slot tree that fits, growing the atlas when none does.

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace mc {

class TextureAtlasSprite;

class StitcherException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Stitcher {
public:
    class Holder {
    public:
        Holder(TextureAtlasSprite* texture, int mipmapLevel);

        TextureAtlasSprite* getAtlasSprite() const { return theTexture; }
        int getWidth() const;
        int getHeight() const;
        void rotate() { rotated = !rotated; }
        bool isRotated() const { return rotated; }
        void setNewDimension(int dimension);
        int compareTo(const Holder& other) const;

    private:
        TextureAtlasSprite* theTexture;
        int width;
        int height;
        int mipmapLevelHolder;
        bool rotated;
        float scaleFactor = 1.0f;
    };

    class Slot {
    public:
        Slot(int originXIn, int originYIn, int widthIn, int heightIn)
            : originX(originXIn)
            , originY(originYIn)
            , width(widthIn)
            , height(heightIn)
        {
        }

        Holder* getStitchHolder() const { return holder; }
        int getOriginX() const { return originX; }
        int getOriginY() const { return originY; }
        bool addSlot(Holder* holderIn);
        void getAllStitchSlots(std::vector<const Slot*>& out) const;

    private:
        int originX;
        int originY;
        int width;
        int height;
        std::vector<Slot> subSlots;
        bool hasSubSlots = false;
        Holder* holder = nullptr;
    };

    Stitcher(int maxTextureWidth, int maxTextureHeight, bool forcePowerOf2In, int maxTileDimensionIn, int mipmapLevel);

    int getCurrentWidth() const { return currentWidth; }
    int getCurrentHeight() const { return currentHeight; }
    void addSprite(TextureAtlasSprite* sprite);
    // Throws StitcherException when a sprite doesn't fit.
    void doStitch();
    // Every placed sprite, initialised with its atlas position.
    std::vector<TextureAtlasSprite*> getStichSlots();

private:
    static int getMipmapDimension(int size, int level);
    bool allocateSlot(Holder* holder);
    bool expandAndAllocateSlot(Holder* holder);

    int mipmapLevelStitcher;
    std::vector<std::unique_ptr<Holder>> setStitchHolders;
    std::vector<Slot> stitchSlots;
    int currentWidth = 0;
    int currentHeight = 0;
    int maxWidth;
    int maxHeight;
    bool forcePowerOf2;
    int maxTileDimension;
};

} // namespace mc
