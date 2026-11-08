#include "client/renderer/block/model/ItemModelGenerator.h"

#include "client/renderer/texture/TextureAtlasSprite.h"
#include "client/renderer/texture/TextureMap.h"

namespace mc {

const std::array<std::string, 5> ItemModelGenerator::LAYERS { "layer0", "layer1", "layer2", "layer3", "layer4" };

namespace {

BlockPartFace makeFace(int tintIndex, const std::string& texture, const std::array<float, 4>& uvs)
{
    BlockPartFace face;
    face.tintIndex = tintIndex;
    face.texture = texture;
    face.blockFaceUV.uvs = uvs;
    return face;
}

} // namespace

std::shared_ptr<ModelBlock> ItemModelGenerator::makeItemModel(const TextureMap& textureMap, const ModelBlock& blockModel) const
{
    std::unordered_map<std::string, std::string> textures;
    std::vector<BlockPart> list;
    for (size_t i = 0; i < LAYERS.size(); ++i) {
        const std::string& s = LAYERS[i];
        if (!blockModel.isTexturePresent(s))
            break;
        const std::string s1 = blockModel.resolveTextureName(s);
        textures[s] = s1;
        const TextureAtlasSprite* sprite = textureMap.getAtlasSprite(ResourceLocation(s1).toString());
        std::vector<BlockPart> parts = func_178394_a(static_cast<int>(i), s, *sprite);
        list.insert(list.end(), std::make_move_iterator(parts.begin()), std::make_move_iterator(parts.end()));
    }
    if (list.empty())
        return nullptr;
    textures["particle"] = blockModel.isTexturePresent("particle") ? blockModel.resolveTextureName("particle") : textures["layer0"];
    return std::make_shared<ModelBlock>(std::move(list), std::move(textures), false, false, blockModel.getAllTransforms());
}

std::vector<BlockPart> ItemModelGenerator::func_178394_a(int tintIndex, const std::string& texture, const TextureAtlasSprite& sprite) const
{
    std::map<EnumFacing, BlockPartFace> faces;
    faces.emplace(EnumFacing::SOUTH, makeFace(tintIndex, texture, { 0.0f, 0.0f, 16.0f, 16.0f }));
    faces.emplace(EnumFacing::NORTH, makeFace(tintIndex, texture, { 16.0f, 0.0f, 0.0f, 16.0f }));
    std::vector<BlockPart> list;
    list.emplace_back(lwjgl::Vector3f(0.0f, 0.0f, 7.5f), lwjgl::Vector3f(16.0f, 16.0f, 8.5f), std::move(faces), std::nullopt, true);
    std::vector<BlockPart> edges = func_178397_a(sprite, texture, tintIndex);
    list.insert(list.end(), std::make_move_iterator(edges.begin()), std::make_move_iterator(edges.end()));
    return list;
}

std::vector<BlockPart> ItemModelGenerator::func_178397_a(const TextureAtlasSprite& sprite, const std::string& texture, int tintIndex) const
{
    const float f = static_cast<float>(sprite.getIconWidth());
    const float f1 = static_cast<float>(sprite.getIconHeight());
    std::vector<BlockPart> list;
    for (const Span& span : func_178393_a(sprite)) {
        float f2 = 0.0f;
        float f3 = 0.0f;
        float f4 = 0.0f;
        float f5 = 0.0f;
        float f6 = 0.0f;
        float f7 = 0.0f;
        float f8 = 0.0f;
        float f9 = 0.0f;
        float f10 = 0.0f;
        float f11 = 0.0f;
        const float f12 = static_cast<float>(span.min);
        const float f13 = static_cast<float>(span.max);
        const float f14 = static_cast<float>(span.anchor);
        const SpanFacing facing = span.spanFacing;
        switch (facing) {
        case SpanFacing::UP:
            f6 = f12;
            f2 = f12;
            f4 = f7 = f13 + 1.0f;
            f8 = f14;
            f3 = f14;
            f9 = f14;
            f5 = f14;
            f10 = 16.0f / f;
            f11 = 16.0f / (f1 - 1.0f);
            break;
        case SpanFacing::DOWN:
            f9 = f14;
            f8 = f14;
            f6 = f12;
            f2 = f12;
            f4 = f7 = f13 + 1.0f;
            f3 = f14 + 1.0f;
            f5 = f14 + 1.0f;
            f10 = 16.0f / f;
            f11 = 16.0f / (f1 - 1.0f);
            break;
        case SpanFacing::LEFT:
            f6 = f14;
            f2 = f14;
            f7 = f14;
            f4 = f14;
            f9 = f12;
            f3 = f12;
            f5 = f8 = f13 + 1.0f;
            f10 = 16.0f / (f - 1.0f);
            f11 = 16.0f / f1;
            break;
        case SpanFacing::RIGHT:
            f7 = f14;
            f6 = f14;
            f2 = f14 + 1.0f;
            f4 = f14 + 1.0f;
            f9 = f12;
            f3 = f12;
            f5 = f8 = f13 + 1.0f;
            f10 = 16.0f / (f - 1.0f);
            f11 = 16.0f / f1;
            break;
        }
        const float f15 = 16.0f / f;
        const float f16 = 16.0f / f1;
        f2 = f2 * f15;
        f4 = f4 * f15;
        f3 = f3 * f16;
        f5 = f5 * f16;
        f3 = 16.0f - f3;
        f5 = 16.0f - f5;
        f6 = f6 * f10;
        f7 = f7 * f10;
        f8 = f8 * f11;
        f9 = f9 * f11;

        static constexpr EnumFacing kSpanFacings[4] { EnumFacing::UP, EnumFacing::DOWN, EnumFacing::EAST, EnumFacing::WEST };
        std::map<EnumFacing, BlockPartFace> faces;
        faces.emplace(kSpanFacings[static_cast<int>(facing)], makeFace(tintIndex, texture, { f6, f8, f7, f9 }));
        switch (facing) {
        case SpanFacing::UP:
            list.emplace_back(lwjgl::Vector3f(f2, f3, 7.5f), lwjgl::Vector3f(f4, f3, 8.5f), std::move(faces), std::nullopt, true);
            break;
        case SpanFacing::DOWN:
            list.emplace_back(lwjgl::Vector3f(f2, f5, 7.5f), lwjgl::Vector3f(f4, f5, 8.5f), std::move(faces), std::nullopt, true);
            break;
        case SpanFacing::LEFT:
            list.emplace_back(lwjgl::Vector3f(f2, f3, 7.5f), lwjgl::Vector3f(f2, f5, 8.5f), std::move(faces), std::nullopt, true);
            break;
        case SpanFacing::RIGHT:
            list.emplace_back(lwjgl::Vector3f(f4, f3, 7.5f), lwjgl::Vector3f(f4, f5, 8.5f), std::move(faces), std::nullopt, true);
            break;
        }
    }
    return list;
}

std::vector<ItemModelGenerator::Span> ItemModelGenerator::func_178393_a(const TextureAtlasSprite& sprite) const
{
    const int i = sprite.getIconWidth();
    const int j = sprite.getIconHeight();
    std::vector<Span> list;
    for (int k = 0; k < sprite.getFrameCount(); ++k) {
        const std::vector<uint32_t>& pixels = sprite.getFrameTextureData(k)[0];
        for (int l = 0; l < j; ++l) {
            for (int i1 = 0; i1 < i; ++i1) {
                const bool flag = !func_178391_a(pixels, i1, l, i, j);
                func_178396_a(SpanFacing::UP, list, pixels, i1, l, i, j, flag);
                func_178396_a(SpanFacing::DOWN, list, pixels, i1, l, i, j, flag);
                func_178396_a(SpanFacing::LEFT, list, pixels, i1, l, i, j, flag);
                func_178396_a(SpanFacing::RIGHT, list, pixels, i1, l, i, j, flag);
            }
        }
    }
    return list;
}

void ItemModelGenerator::func_178396_a(SpanFacing facing, std::vector<Span>& spans, const std::vector<uint32_t>& pixels, int x, int y, int width, int height,
    bool opaque) const
{
    static constexpr int kOffsetX[4] { 0, 0, -1, 1 };
    static constexpr int kOffsetY[4] { -1, 1, 0, 0 };
    const int index = static_cast<int>(facing);
    if (func_178391_a(pixels, x + kOffsetX[index], y + kOffsetY[index], width, height) && opaque)
        func_178395_a(spans, facing, x, y);
}

void ItemModelGenerator::func_178395_a(std::vector<Span>& spans, SpanFacing facing, int x, int y) const
{
    const bool vertical = facing == SpanFacing::DOWN || facing == SpanFacing::UP;
    const int anchor = vertical ? y : x;
    Span* found = nullptr;
    for (Span& span : spans) {
        if (span.spanFacing == facing && span.anchor == anchor) {
            found = &span;
            break;
        }
    }
    const int j = vertical ? y : x;
    const int k = vertical ? x : y;
    if (!found)
        spans.push_back({ facing, k, k, j });
    else
        found->expand(k);
}

bool ItemModelGenerator::func_178391_a(const std::vector<uint32_t>& pixels, int x, int y, int width, int height)
{
    if (x >= 0 && y >= 0 && x < width && y < height)
        return ((pixels[static_cast<size_t>(y * width + x)] >> 24) & 255u) == 0;
    return true;
}

} // namespace mc
