#include "client/renderer/BlockModelRenderer.h"

#include "block/state/BlockState.h"
#include "client/Minecraft.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/EntityRenderer.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/WorldRenderer.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/resources/model/IBakedModel.h"
#include "client/settings/GameSettings.h"
#include "core/Java.h"
#include "util/MathHelper.h"
#include "world/IBlockAccess.h"

#include <algorithm>

namespace mc {

namespace {

constexpr int kFacingCount = 6;

// Java: BlockModelRenderer.Orientation -- an index into the quad bounds: the facing, or 6 + facing for 1 - bound.
enum Orientation : int8_t {
    O_DOWN = 0,
    O_UP = 1,
    O_NORTH = 2,
    O_SOUTH = 3,
    O_WEST = 4,
    O_EAST = 5,
    O_FLIP_DOWN = 6,
    O_FLIP_UP = 7,
    O_FLIP_NORTH = 8,
    O_FLIP_SOUTH = 9,
    O_FLIP_WEST = 10,
    O_FLIP_EAST = 11,
};

// Java: BlockModelRenderer.EnumNeighborInfo, indexed by EnumFacing.
struct NeighborInfo {
    std::array<EnumFacing, 4> corners; // field_178276_g
    float shadeWeight; // field_178288_h
    bool doNonCubicWeight; // field_178289_i
    std::array<Orientation, 8> vert0Weights; // field_178286_j
    std::array<Orientation, 8> vert1Weights; // field_178287_k
    std::array<Orientation, 8> vert2Weights; // field_178284_l
    std::array<Orientation, 8> vert3Weights; // field_178285_m
};

using F = EnumFacing;

const NeighborInfo kNeighborInfo[6] {
    // DOWN
    { { F::WEST, F::EAST, F::NORTH, F::SOUTH }, 0.5f, false, {}, {}, {}, {} },
    // UP
    { { F::EAST, F::WEST, F::NORTH, F::SOUTH }, 1.0f, false, {}, {}, {}, {} },
    // NORTH
    { { F::UP, F::DOWN, F::EAST, F::WEST }, 0.8f, true,
        { O_UP, O_FLIP_WEST, O_UP, O_WEST, O_FLIP_UP, O_WEST, O_FLIP_UP, O_FLIP_WEST },
        { O_UP, O_FLIP_EAST, O_UP, O_EAST, O_FLIP_UP, O_EAST, O_FLIP_UP, O_FLIP_EAST },
        { O_DOWN, O_FLIP_EAST, O_DOWN, O_EAST, O_FLIP_DOWN, O_EAST, O_FLIP_DOWN, O_FLIP_EAST },
        { O_DOWN, O_FLIP_WEST, O_DOWN, O_WEST, O_FLIP_DOWN, O_WEST, O_FLIP_DOWN, O_FLIP_WEST } },
    // SOUTH
    { { F::WEST, F::EAST, F::DOWN, F::UP }, 0.8f, true,
        { O_UP, O_FLIP_WEST, O_FLIP_UP, O_FLIP_WEST, O_FLIP_UP, O_WEST, O_UP, O_WEST },
        { O_DOWN, O_FLIP_WEST, O_FLIP_DOWN, O_FLIP_WEST, O_FLIP_DOWN, O_WEST, O_DOWN, O_WEST },
        { O_DOWN, O_FLIP_EAST, O_FLIP_DOWN, O_FLIP_EAST, O_FLIP_DOWN, O_EAST, O_DOWN, O_EAST },
        { O_UP, O_FLIP_EAST, O_FLIP_UP, O_FLIP_EAST, O_FLIP_UP, O_EAST, O_UP, O_EAST } },
    // WEST
    { { F::UP, F::DOWN, F::NORTH, F::SOUTH }, 0.6f, true,
        { O_UP, O_SOUTH, O_UP, O_FLIP_SOUTH, O_FLIP_UP, O_FLIP_SOUTH, O_FLIP_UP, O_SOUTH },
        { O_UP, O_NORTH, O_UP, O_FLIP_NORTH, O_FLIP_UP, O_FLIP_NORTH, O_FLIP_UP, O_NORTH },
        { O_DOWN, O_NORTH, O_DOWN, O_FLIP_NORTH, O_FLIP_DOWN, O_FLIP_NORTH, O_FLIP_DOWN, O_NORTH },
        { O_DOWN, O_SOUTH, O_DOWN, O_FLIP_SOUTH, O_FLIP_DOWN, O_FLIP_SOUTH, O_FLIP_DOWN, O_SOUTH } },
    // EAST
    { { F::DOWN, F::UP, F::NORTH, F::SOUTH }, 0.6f, true,
        { O_FLIP_DOWN, O_SOUTH, O_FLIP_DOWN, O_FLIP_SOUTH, O_DOWN, O_FLIP_SOUTH, O_DOWN, O_SOUTH },
        { O_FLIP_DOWN, O_NORTH, O_FLIP_DOWN, O_FLIP_NORTH, O_DOWN, O_FLIP_NORTH, O_DOWN, O_NORTH },
        { O_FLIP_UP, O_NORTH, O_FLIP_UP, O_FLIP_NORTH, O_UP, O_FLIP_NORTH, O_UP, O_NORTH },
        { O_FLIP_UP, O_SOUTH, O_FLIP_UP, O_FLIP_SOUTH, O_UP, O_FLIP_SOUTH, O_UP, O_SOUTH } },
};

// Java: BlockModelRenderer.VertexTranslations, indexed by EnumFacing.
struct VertexTranslations {
    int v0, v1, v2, v3; // field_178191_g, field_178200_h, field_178201_i, field_178198_j
};

constexpr VertexTranslations kVertexTranslations[6] {
    { 0, 1, 2, 3 }, // DOWN
    { 2, 3, 0, 1 }, // UP
    { 3, 0, 1, 2 }, // NORTH
    { 0, 1, 2, 3 }, // SOUTH
    { 3, 0, 1, 2 }, // WEST
    { 1, 2, 3, 0 }, // EAST
};

bool ambientOcclusionEnabled()
{
    const Minecraft* mc = Minecraft::getMinecraft();
    return mc != nullptr && mc->gameSettings->ambientOcclusion != 0;
}

int tintColor(Block* blockIn, IBlockAccess* blockAccessIn, const BlockPos& blockPosIn, int tintIndex)
{
    int j = blockIn->colorMultiplier(blockAccessIn, blockPosIn, tintIndex);
    if (EntityRenderer::anaglyphEnable)
        j = static_cast<int>(TextureUtil::anaglyphColor(static_cast<uint32_t>(j)));
    return j;
}

} // namespace

bool BlockModelRenderer::renderModel(IBlockAccess* blockAccessIn, const IBakedModel* modelIn, const IBlockState* blockStateIn, const BlockPos& blockPosIn,
    WorldRenderer& worldRendererIn)
{
    Block* block = blockStateIn->getBlock();
    block->setBlockBoundsBasedOnState(blockAccessIn, blockPosIn);
    return renderModel(blockAccessIn, modelIn, blockStateIn, blockPosIn, worldRendererIn, true);
}

bool BlockModelRenderer::renderModel(IBlockAccess* blockAccessIn, const IBakedModel* modelIn, const IBlockState* blockStateIn, const BlockPos& blockPosIn,
    WorldRenderer& worldRendererIn, bool checkSides)
{
    const bool flag = ambientOcclusionEnabled() && blockStateIn->getBlock()->getLightValue() == 0 && modelIn->isAmbientOcclusion();
    Block* block = blockStateIn->getBlock();
    return flag ? renderModelAmbientOcclusion(blockAccessIn, modelIn, block, blockPosIn, worldRendererIn, checkSides)
                : renderModelStandard(blockAccessIn, modelIn, block, blockPosIn, worldRendererIn, checkSides);
}

bool BlockModelRenderer::renderModelAmbientOcclusion(IBlockAccess* blockAccessIn, const IBakedModel* modelIn, Block* blockIn, const BlockPos& blockPosIn,
    WorldRenderer& worldRendererIn, bool checkSides)
{
    bool flag = false;
    Bounds afloat {};
    BoundsFlags bitset;
    AmbientOcclusionFace aoFace;

    for (EnumFacing enumfacing : kFacings) {
        const std::vector<BakedQuad>& list = modelIn->getFaceQuads(enumfacing);
        if (!list.empty()) {
            const BlockPos blockpos = blockPosIn.offset(enumfacing);
            if (!checkSides || blockIn->shouldSideBeRendered(blockAccessIn, blockpos, enumfacing)) {
                renderModelAmbientOcclusionQuads(blockAccessIn, blockIn, blockPosIn, worldRendererIn, list, afloat, bitset, aoFace);
                flag = true;
            }
        }
    }

    const std::vector<BakedQuad>& list1 = modelIn->getGeneralQuads();
    if (!list1.empty()) {
        renderModelAmbientOcclusionQuads(blockAccessIn, blockIn, blockPosIn, worldRendererIn, list1, afloat, bitset, aoFace);
        flag = true;
    }
    return flag;
}

bool BlockModelRenderer::renderModelStandard(IBlockAccess* blockAccessIn, const IBakedModel* modelIn, Block* blockIn, const BlockPos& blockPosIn,
    WorldRenderer& worldRendererIn, bool checkSides)
{
    bool flag = false;
    BoundsFlags bitset;

    for (EnumFacing enumfacing : kFacings) {
        const std::vector<BakedQuad>& list = modelIn->getFaceQuads(enumfacing);
        if (!list.empty()) {
            const BlockPos blockpos = blockPosIn.offset(enumfacing);
            if (!checkSides || blockIn->shouldSideBeRendered(blockAccessIn, blockpos, enumfacing)) {
                const int i = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockpos);
                renderModelStandardQuads(blockAccessIn, blockIn, blockPosIn, enumfacing, i, false, worldRendererIn, list, bitset);
                flag = true;
            }
        }
    }

    const std::vector<BakedQuad>& list1 = modelIn->getGeneralQuads();
    if (!list1.empty()) {
        renderModelStandardQuads(blockAccessIn, blockIn, blockPosIn, EnumFacing::DOWN, -1, true, worldRendererIn, list1, bitset);
        flag = true;
    }
    return flag;
}

void BlockModelRenderer::renderModelAmbientOcclusionQuads(IBlockAccess* blockAccessIn, Block* blockIn, const BlockPos& blockPosIn,
    WorldRenderer& worldRendererIn, const std::vector<BakedQuad>& listQuadsIn, Bounds& quadBounds, BoundsFlags& boundsFlags, AmbientOcclusionFace& aoFaceIn)
{
    double d0 = static_cast<double>(blockPosIn.getX());
    double d1 = static_cast<double>(blockPosIn.getY());
    double d2 = static_cast<double>(blockPosIn.getZ());
    const Block::EnumOffsetType offsetType = blockIn->getOffsetType();

    if (offsetType != Block::EnumOffsetType::NONE) {
        // Java: MathHelper.getPositionRandom(pos)
        const int64_t i = MathHelper::getCoordinateRandom(blockPosIn.getX(), blockPosIn.getY(), blockPosIn.getZ());
        d0 += (static_cast<double>(static_cast<float>(i >> 16 & 15LL) / 15.0f) - 0.5) * 0.5;
        d2 += (static_cast<double>(static_cast<float>(i >> 24 & 15LL) / 15.0f) - 0.5) * 0.5;
        if (offsetType == Block::EnumOffsetType::XYZ)
            d1 += (static_cast<double>(static_cast<float>(i >> 20 & 15LL) / 15.0f) - 1.0) * 0.2;
    }

    for (const BakedQuad& bakedquad : listQuadsIn) {
        fillQuadBounds(blockIn, bakedquad.getVertexData(), bakedquad.getFace(), &quadBounds, boundsFlags);
        aoFaceIn.updateVertexBrightness(blockAccessIn, blockIn, blockPosIn, bakedquad.getFace(), quadBounds, boundsFlags);
        worldRendererIn.addVertexData(bakedquad.getVertexData());
        worldRendererIn.putBrightness4(aoFaceIn.vertexBrightness[0], aoFaceIn.vertexBrightness[1], aoFaceIn.vertexBrightness[2], aoFaceIn.vertexBrightness[3]);
        const auto& m = aoFaceIn.vertexColorMultiplier;

        if (bakedquad.hasTintIndex()) {
            const int j = tintColor(blockIn, blockAccessIn, blockPosIn, bakedquad.getTintIndex());
            const float f = static_cast<float>(j >> 16 & 255) / 255.0f;
            const float f1 = static_cast<float>(j >> 8 & 255) / 255.0f;
            const float f2 = static_cast<float>(j & 255) / 255.0f;
            worldRendererIn.putColorMultiplier(m[0] * f, m[0] * f1, m[0] * f2, 4);
            worldRendererIn.putColorMultiplier(m[1] * f, m[1] * f1, m[1] * f2, 3);
            worldRendererIn.putColorMultiplier(m[2] * f, m[2] * f1, m[2] * f2, 2);
            worldRendererIn.putColorMultiplier(m[3] * f, m[3] * f1, m[3] * f2, 1);
        } else {
            worldRendererIn.putColorMultiplier(m[0], m[0], m[0], 4);
            worldRendererIn.putColorMultiplier(m[1], m[1], m[1], 3);
            worldRendererIn.putColorMultiplier(m[2], m[2], m[2], 2);
            worldRendererIn.putColorMultiplier(m[3], m[3], m[3], 1);
        }

        worldRendererIn.putPosition(d0, d1, d2);
    }
}

void BlockModelRenderer::fillQuadBounds(Block* blockIn, const std::array<int32_t, 28>& vertexData, EnumFacing facingIn, Bounds* quadBounds,
    BoundsFlags& boundsFlags)
{
    float f = 32.0f;
    float f1 = 32.0f;
    float f2 = 32.0f;
    float f3 = -32.0f;
    float f4 = -32.0f;
    float f5 = -32.0f;

    for (int i = 0; i < 4; ++i) {
        const float f6 = java::intBitsToFloat(vertexData[static_cast<size_t>(i * 7)]);
        const float f7 = java::intBitsToFloat(vertexData[static_cast<size_t>(i * 7 + 1)]);
        const float f8 = java::intBitsToFloat(vertexData[static_cast<size_t>(i * 7 + 2)]);
        f = std::min(f, f6);
        f1 = std::min(f1, f7);
        f2 = std::min(f2, f8);
        f3 = std::max(f3, f6);
        f4 = std::max(f4, f7);
        f5 = std::max(f5, f8);
    }

    if (quadBounds != nullptr) {
        Bounds& b = *quadBounds;
        b[static_cast<size_t>(getIndex(EnumFacing::WEST))] = f;
        b[static_cast<size_t>(getIndex(EnumFacing::EAST))] = f3;
        b[static_cast<size_t>(getIndex(EnumFacing::DOWN))] = f1;
        b[static_cast<size_t>(getIndex(EnumFacing::UP))] = f4;
        b[static_cast<size_t>(getIndex(EnumFacing::NORTH))] = f2;
        b[static_cast<size_t>(getIndex(EnumFacing::SOUTH))] = f5;
        b[static_cast<size_t>(getIndex(EnumFacing::WEST) + kFacingCount)] = 1.0f - f;
        b[static_cast<size_t>(getIndex(EnumFacing::EAST) + kFacingCount)] = 1.0f - f3;
        b[static_cast<size_t>(getIndex(EnumFacing::DOWN) + kFacingCount)] = 1.0f - f1;
        b[static_cast<size_t>(getIndex(EnumFacing::UP) + kFacingCount)] = 1.0f - f4;
        b[static_cast<size_t>(getIndex(EnumFacing::NORTH) + kFacingCount)] = 1.0f - f2;
        b[static_cast<size_t>(getIndex(EnumFacing::SOUTH) + kFacingCount)] = 1.0f - f5;
    }

    switch (facingIn) {
    case EnumFacing::DOWN:
        boundsFlags[1] = f >= 1.0E-4f || f2 >= 1.0E-4f || f3 <= 0.9999f || f5 <= 0.9999f;
        boundsFlags[0] = (f1 < 1.0E-4f || blockIn->isFullCube()) && f1 == f4;
        break;
    case EnumFacing::UP:
        boundsFlags[1] = f >= 1.0E-4f || f2 >= 1.0E-4f || f3 <= 0.9999f || f5 <= 0.9999f;
        boundsFlags[0] = (f4 > 0.9999f || blockIn->isFullCube()) && f1 == f4;
        break;
    case EnumFacing::NORTH:
        boundsFlags[1] = f >= 1.0E-4f || f1 >= 1.0E-4f || f3 <= 0.9999f || f4 <= 0.9999f;
        boundsFlags[0] = (f2 < 1.0E-4f || blockIn->isFullCube()) && f2 == f5;
        break;
    case EnumFacing::SOUTH:
        boundsFlags[1] = f >= 1.0E-4f || f1 >= 1.0E-4f || f3 <= 0.9999f || f4 <= 0.9999f;
        boundsFlags[0] = (f5 > 0.9999f || blockIn->isFullCube()) && f2 == f5;
        break;
    case EnumFacing::WEST:
        boundsFlags[1] = f1 >= 1.0E-4f || f2 >= 1.0E-4f || f4 <= 0.9999f || f5 <= 0.9999f;
        boundsFlags[0] = (f < 1.0E-4f || blockIn->isFullCube()) && f == f3;
        break;
    case EnumFacing::EAST:
        boundsFlags[1] = f1 >= 1.0E-4f || f2 >= 1.0E-4f || f4 <= 0.9999f || f5 <= 0.9999f;
        boundsFlags[0] = (f3 > 0.9999f || blockIn->isFullCube()) && f == f3;
        break;
    }
}

void BlockModelRenderer::renderModelStandardQuads(IBlockAccess* blockAccessIn, Block* blockIn, const BlockPos& blockPosIn, EnumFacing /*faceIn*/,
    int brightnessIn, bool ownBrightness, WorldRenderer& worldRendererIn, const std::vector<BakedQuad>& listQuadsIn, BoundsFlags& boundsFlags)
{
    double d0 = static_cast<double>(blockPosIn.getX());
    double d1 = static_cast<double>(blockPosIn.getY());
    double d2 = static_cast<double>(blockPosIn.getZ());
    const Block::EnumOffsetType offsetType = blockIn->getOffsetType();

    if (offsetType != Block::EnumOffsetType::NONE) {
        // Java: the position hash without y here (unlike the smooth path).
        const int i = blockPosIn.getX();
        const int j = blockPosIn.getZ();
        int64_t k = static_cast<int64_t>(java::mul(i, 3129871)) ^ static_cast<int64_t>(static_cast<uint64_t>(static_cast<int64_t>(j)) * 116129781ULL);
        k = static_cast<int64_t>(static_cast<uint64_t>(k) * static_cast<uint64_t>(k) * 42317861ULL + static_cast<uint64_t>(k) * 11ULL);
        d0 += (static_cast<double>(static_cast<float>(k >> 16 & 15LL) / 15.0f) - 0.5) * 0.5;
        d2 += (static_cast<double>(static_cast<float>(k >> 24 & 15LL) / 15.0f) - 0.5) * 0.5;
        if (offsetType == Block::EnumOffsetType::XYZ)
            d1 += (static_cast<double>(static_cast<float>(k >> 20 & 15LL) / 15.0f) - 1.0) * 0.2;
    }

    for (const BakedQuad& bakedquad : listQuadsIn) {
        if (ownBrightness) {
            fillQuadBounds(blockIn, bakedquad.getVertexData(), bakedquad.getFace(), nullptr, boundsFlags);
            brightnessIn = boundsFlags[0] ? blockIn->getMixedBrightnessForBlock(blockAccessIn, blockPosIn.offset(bakedquad.getFace()))
                                          : blockIn->getMixedBrightnessForBlock(blockAccessIn, blockPosIn);
        }

        worldRendererIn.addVertexData(bakedquad.getVertexData());
        worldRendererIn.putBrightness4(brightnessIn, brightnessIn, brightnessIn, brightnessIn);

        if (bakedquad.hasTintIndex()) {
            const int l = tintColor(blockIn, blockAccessIn, blockPosIn, bakedquad.getTintIndex());
            const float f = static_cast<float>(l >> 16 & 255) / 255.0f;
            const float f1 = static_cast<float>(l >> 8 & 255) / 255.0f;
            const float f2 = static_cast<float>(l & 255) / 255.0f;
            worldRendererIn.putColorMultiplier(f, f1, f2, 4);
            worldRendererIn.putColorMultiplier(f, f1, f2, 3);
            worldRendererIn.putColorMultiplier(f, f1, f2, 2);
            worldRendererIn.putColorMultiplier(f, f1, f2, 1);
        }

        worldRendererIn.putPosition(d0, d1, d2);
    }
}

void BlockModelRenderer::renderModelBrightnessColor(const IBakedModel* bakedModel, float brightness, float red, float green, float blue)
{
    for (EnumFacing enumfacing : kFacings)
        renderModelBrightnessColorQuads(brightness, red, green, blue, bakedModel->getFaceQuads(enumfacing));
    renderModelBrightnessColorQuads(brightness, red, green, blue, bakedModel->getGeneralQuads());
}

void BlockModelRenderer::renderModelBrightness(const IBakedModel* model, const IBlockState* state, float brightness, bool useColor)
{
    Block* block = state->getBlock();
    block->setBlockBoundsForItemRender();
    DxStateManager::rotate(90.0f, 0.0f, 1.0f, 0.0f);
    int i = block->getRenderColor(block->getStateForEntityRender(state));
    if (EntityRenderer::anaglyphEnable)
        i = static_cast<int>(TextureUtil::anaglyphColor(static_cast<uint32_t>(i)));

    const float f = static_cast<float>(i >> 16 & 255) / 255.0f;
    const float f1 = static_cast<float>(i >> 8 & 255) / 255.0f;
    const float f2 = static_cast<float>(i & 255) / 255.0f;
    if (!useColor)
        DxStateManager::color(brightness, brightness, brightness, 1.0f);
    renderModelBrightnessColor(model, brightness, f, f1, f2);
}

void BlockModelRenderer::renderModelBrightnessColorQuads(float brightness, float red, float green, float blue, const std::vector<BakedQuad>& listQuads)
{
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();

    for (const BakedQuad& bakedquad : listQuads) {
        worldrenderer.begin(7, DefaultVertexFormats::ITEM);
        worldrenderer.addVertexData(bakedquad.getVertexData());
        if (bakedquad.hasTintIndex())
            worldrenderer.putColorRGB_F4(red * brightness, green * brightness, blue * brightness);
        else
            worldrenderer.putColorRGB_F4(brightness, brightness, brightness);
        const EnumFacing face = bakedquad.getFace();
        worldrenderer.putNormal(static_cast<float>(getFrontOffsetX(face)), static_cast<float>(getFrontOffsetY(face)), static_cast<float>(getFrontOffsetZ(face)));
        tessellator.draw();
    }
}

// --- AmbientOcclusionFace ---

void BlockModelRenderer::AmbientOcclusionFace::updateVertexBrightness(IBlockAccess* blockAccessIn, Block* blockIn, const BlockPos& blockPosIn,
    EnumFacing facingIn, const Bounds& quadBounds, const BoundsFlags& boundsFlags)
{
    const BlockPos blockpos = boundsFlags[0] ? blockPosIn.offset(facingIn) : blockPosIn;
    const NeighborInfo& info = kNeighborInfo[getIndex(facingIn)];
    const BlockPos blockpos1 = blockpos.offset(info.corners[0]);
    const BlockPos blockpos2 = blockpos.offset(info.corners[1]);
    const BlockPos blockpos3 = blockpos.offset(info.corners[2]);
    const BlockPos blockpos4 = blockpos.offset(info.corners[3]);
    const int i = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockpos1);
    const int j = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockpos2);
    const int k = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockpos3);
    const int l = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockpos4);
    const float f = blockAccessIn->getBlockState(blockpos1)->getBlock()->getAmbientOcclusionLightValue();
    const float f1 = blockAccessIn->getBlockState(blockpos2)->getBlock()->getAmbientOcclusionLightValue();
    const float f2 = blockAccessIn->getBlockState(blockpos3)->getBlock()->getAmbientOcclusionLightValue();
    const float f3 = blockAccessIn->getBlockState(blockpos4)->getBlock()->getAmbientOcclusionLightValue();
    const bool flag = blockAccessIn->getBlockState(blockpos1.offset(facingIn))->getBlock()->isTranslucent();
    const bool flag1 = blockAccessIn->getBlockState(blockpos2.offset(facingIn))->getBlock()->isTranslucent();
    const bool flag2 = blockAccessIn->getBlockState(blockpos3.offset(facingIn))->getBlock()->isTranslucent();
    const bool flag3 = blockAccessIn->getBlockState(blockpos4.offset(facingIn))->getBlock()->isTranslucent();

    float f4;
    int i1;
    if (!flag2 && !flag) {
        f4 = f;
        i1 = i;
    } else {
        const BlockPos blockpos5 = blockpos1.offset(info.corners[2]);
        f4 = blockAccessIn->getBlockState(blockpos5)->getBlock()->getAmbientOcclusionLightValue();
        i1 = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockpos5);
    }

    float f5;
    int j1;
    if (!flag3 && !flag) {
        f5 = f;
        j1 = i;
    } else {
        const BlockPos blockpos6 = blockpos1.offset(info.corners[3]);
        f5 = blockAccessIn->getBlockState(blockpos6)->getBlock()->getAmbientOcclusionLightValue();
        j1 = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockpos6);
    }

    float f6;
    int k1;
    if (!flag2 && !flag1) {
        f6 = f1;
        k1 = j;
    } else {
        const BlockPos blockpos7 = blockpos2.offset(info.corners[2]);
        f6 = blockAccessIn->getBlockState(blockpos7)->getBlock()->getAmbientOcclusionLightValue();
        k1 = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockpos7);
    }

    float f7;
    int l1;
    if (!flag3 && !flag1) {
        f7 = f1;
        l1 = j;
    } else {
        const BlockPos blockpos8 = blockpos2.offset(info.corners[3]);
        f7 = blockAccessIn->getBlockState(blockpos8)->getBlock()->getAmbientOcclusionLightValue();
        l1 = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockpos8);
    }

    int i3 = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockPosIn);
    if (boundsFlags[0] || !blockAccessIn->getBlockState(blockPosIn.offset(facingIn))->getBlock()->isOpaqueCube())
        i3 = blockIn->getMixedBrightnessForBlock(blockAccessIn, blockPosIn.offset(facingIn));

    const float f8 = boundsFlags[0] ? blockAccessIn->getBlockState(blockpos)->getBlock()->getAmbientOcclusionLightValue()
                                    : blockAccessIn->getBlockState(blockPosIn)->getBlock()->getAmbientOcclusionLightValue();
    const VertexTranslations& vt = kVertexTranslations[getIndex(facingIn)];

    if (boundsFlags[1] && info.doNonCubicWeight) {
        const float f29 = (f3 + f + f5 + f8) * 0.25f;
        const float f30 = (f2 + f + f4 + f8) * 0.25f;
        const float f31 = (f2 + f1 + f6 + f8) * 0.25f;
        const float f32 = (f3 + f1 + f7 + f8) * 0.25f;
        const auto q = [&](Orientation o) { return quadBounds[static_cast<size_t>(o)]; };
        const auto& w0 = info.vert0Weights;
        const auto& w1 = info.vert1Weights;
        const auto& w2 = info.vert2Weights;
        const auto& w3 = info.vert3Weights;
        const float f13 = q(w0[0]) * q(w0[1]);
        const float f14 = q(w0[2]) * q(w0[3]);
        const float f15 = q(w0[4]) * q(w0[5]);
        const float f16 = q(w0[6]) * q(w0[7]);
        const float f17 = q(w1[0]) * q(w1[1]);
        const float f18 = q(w1[2]) * q(w1[3]);
        const float f19 = q(w1[4]) * q(w1[5]);
        const float f20 = q(w1[6]) * q(w1[7]);
        const float f21 = q(w2[0]) * q(w2[1]);
        const float f22 = q(w2[2]) * q(w2[3]);
        const float f23 = q(w2[4]) * q(w2[5]);
        const float f24 = q(w2[6]) * q(w2[7]);
        const float f25 = q(w3[0]) * q(w3[1]);
        const float f26 = q(w3[2]) * q(w3[3]);
        const float f27 = q(w3[4]) * q(w3[5]);
        const float f28 = q(w3[6]) * q(w3[7]);
        vertexColorMultiplier[static_cast<size_t>(vt.v0)] = f29 * f13 + f30 * f14 + f31 * f15 + f32 * f16;
        vertexColorMultiplier[static_cast<size_t>(vt.v1)] = f29 * f17 + f30 * f18 + f31 * f19 + f32 * f20;
        vertexColorMultiplier[static_cast<size_t>(vt.v2)] = f29 * f21 + f30 * f22 + f31 * f23 + f32 * f24;
        vertexColorMultiplier[static_cast<size_t>(vt.v3)] = f29 * f25 + f30 * f26 + f31 * f27 + f32 * f28;
        const int i2 = getAoBrightness(l, i, j1, i3);
        const int j2 = getAoBrightness(k, i, i1, i3);
        const int k2 = getAoBrightness(k, j, k1, i3);
        const int l2 = getAoBrightness(l, j, l1, i3);
        vertexBrightness[static_cast<size_t>(vt.v0)] = getVertexBrightness(i2, j2, k2, l2, f13, f14, f15, f16);
        vertexBrightness[static_cast<size_t>(vt.v1)] = getVertexBrightness(i2, j2, k2, l2, f17, f18, f19, f20);
        vertexBrightness[static_cast<size_t>(vt.v2)] = getVertexBrightness(i2, j2, k2, l2, f21, f22, f23, f24);
        vertexBrightness[static_cast<size_t>(vt.v3)] = getVertexBrightness(i2, j2, k2, l2, f25, f26, f27, f28);
    } else {
        const float f9 = (f3 + f + f5 + f8) * 0.25f;
        const float f10 = (f2 + f + f4 + f8) * 0.25f;
        const float f11 = (f2 + f1 + f6 + f8) * 0.25f;
        const float f12 = (f3 + f1 + f7 + f8) * 0.25f;
        vertexBrightness[static_cast<size_t>(vt.v0)] = getAoBrightness(l, i, j1, i3);
        vertexBrightness[static_cast<size_t>(vt.v1)] = getAoBrightness(k, i, i1, i3);
        vertexBrightness[static_cast<size_t>(vt.v2)] = getAoBrightness(k, j, k1, i3);
        vertexBrightness[static_cast<size_t>(vt.v3)] = getAoBrightness(l, j, l1, i3);
        vertexColorMultiplier[static_cast<size_t>(vt.v0)] = f9;
        vertexColorMultiplier[static_cast<size_t>(vt.v1)] = f10;
        vertexColorMultiplier[static_cast<size_t>(vt.v2)] = f11;
        vertexColorMultiplier[static_cast<size_t>(vt.v3)] = f12;
    }
}

int BlockModelRenderer::AmbientOcclusionFace::getAoBrightness(int br1, int br2, int br3, int br4)
{
    if (br1 == 0)
        br1 = br4;
    if (br2 == 0)
        br2 = br4;
    if (br3 == 0)
        br3 = br4;
    return (br1 + br2 + br3 + br4) >> 2 & 16711935;
}

int BlockModelRenderer::AmbientOcclusionFace::getVertexBrightness(int b1, int b2, int b3, int b4, float w1, float w2, float w3, float w4)
{
    const int i = java::f2i(static_cast<float>(b1 >> 16 & 255) * w1 + static_cast<float>(b2 >> 16 & 255) * w2 + static_cast<float>(b3 >> 16 & 255) * w3
                      + static_cast<float>(b4 >> 16 & 255) * w4)
        & 255;
    const int j = java::f2i(static_cast<float>(b1 & 255) * w1 + static_cast<float>(b2 & 255) * w2 + static_cast<float>(b3 & 255) * w3 + static_cast<float>(b4 & 255) * w4) & 255;
    return i << 16 | j;
}

} // namespace mc
