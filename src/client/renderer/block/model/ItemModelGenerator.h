#pragma once

// Original: net/minecraft/client/renderer/block/model/ItemModelGenerator.java
// Builds the model of a flat item (builtin/generated): a front and back face per layer texture plus one thin
// edge face for every run of opaque pixels along a transparent border.

#include "client/renderer/block/model/ModelBlock.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class TextureAtlasSprite;
class TextureMap;

class ItemModelGenerator {
public:
    static const std::array<std::string, 5> LAYERS;

    // Null when the model has no layer0 texture.
    std::shared_ptr<ModelBlock> makeItemModel(const TextureMap& textureMap, const ModelBlock& blockModel) const;

private:
    enum class SpanFacing { UP, DOWN, LEFT, RIGHT };

    struct Span {
        SpanFacing spanFacing;
        int min;
        int max;
        int anchor;

        void expand(int value)
        {
            if (value < min)
                min = value;
            else if (value > max)
                max = value;
        }
    };

    std::vector<BlockPart> func_178394_a(int tintIndex, const std::string& texture, const TextureAtlasSprite& sprite) const;
    std::vector<BlockPart> func_178397_a(const TextureAtlasSprite& sprite, const std::string& texture, int tintIndex) const;
    std::vector<Span> func_178393_a(const TextureAtlasSprite& sprite) const;
    void func_178396_a(SpanFacing facing, std::vector<Span>& spans, const std::vector<uint32_t>& pixels, int x, int y, int width, int height,
        bool opaque) const;
    void func_178395_a(std::vector<Span>& spans, SpanFacing facing, int x, int y) const;
    static bool func_178391_a(const std::vector<uint32_t>& pixels, int x, int y, int width, int height);
};

} // namespace mc
