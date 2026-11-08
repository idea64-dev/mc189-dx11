#include "client/renderer/BlockFluidRenderer.h"

#include "block/BlockLiquid.h"
#include "block/material/Material.h"
#include "block/state/BlockState.h"
#include "client/Minecraft.h"
#include "client/renderer/WorldRenderer.h"
#include "client/renderer/texture/TextureAtlasSprite.h"
#include "client/renderer/texture/TextureMap.h"
#include "util/MathHelper.h"
#include "world/IBlockAccess.h"

namespace mc {

BlockFluidRenderer::BlockFluidRenderer()
{
    initAtlasSprites();
}

void BlockFluidRenderer::initAtlasSprites()
{
    TextureMap& texturemap = Minecraft::getMinecraft()->getTextureMapBlocks();
    atlasSpritesLava[0] = texturemap.getAtlasSprite("minecraft:blocks/lava_still");
    atlasSpritesLava[1] = texturemap.getAtlasSprite("minecraft:blocks/lava_flow");
    atlasSpritesWater[0] = texturemap.getAtlasSprite("minecraft:blocks/water_still");
    atlasSpritesWater[1] = texturemap.getAtlasSprite("minecraft:blocks/water_flow");
}

bool BlockFluidRenderer::renderFluid(IBlockAccess* blockAccess, const IBlockState* blockStateIn, const BlockPos& blockPosIn, WorldRenderer& worldRendererIn)
{
    auto* blockliquid = static_cast<BlockLiquid*>(blockStateIn->getBlock());
    blockliquid->setBlockBoundsBasedOnState(blockAccess, blockPosIn);
    const std::array<TextureAtlasSprite*, 2>& sprites = &blockliquid->getMaterial() == &Material::lava ? atlasSpritesLava : atlasSpritesWater;
    // Java: colorMultiplier(IBlockAccess, BlockPos) -- render pass 0.
    const int i = blockliquid->colorMultiplier(blockAccess, blockPosIn, 0);
    const float f = static_cast<float>(i >> 16 & 255) / 255.0f;
    const float f1 = static_cast<float>(i >> 8 & 255) / 255.0f;
    const float f2 = static_cast<float>(i & 255) / 255.0f;
    const bool flag = blockliquid->shouldSideBeRendered(blockAccess, blockPosIn.up(), EnumFacing::UP);
    const bool flag1 = blockliquid->shouldSideBeRendered(blockAccess, blockPosIn.down(), EnumFacing::DOWN);
    const bool sides[4] { blockliquid->shouldSideBeRendered(blockAccess, blockPosIn.north(), EnumFacing::NORTH),
        blockliquid->shouldSideBeRendered(blockAccess, blockPosIn.south(), EnumFacing::SOUTH),
        blockliquid->shouldSideBeRendered(blockAccess, blockPosIn.west(), EnumFacing::WEST),
        blockliquid->shouldSideBeRendered(blockAccess, blockPosIn.east(), EnumFacing::EAST) };

    if (!flag && !flag1 && !sides[0] && !sides[1] && !sides[2] && !sides[3])
        return false;

    bool flag2 = false;
    const float f3 = 0.5f;
    const float f4 = 1.0f;
    const float f5 = 0.8f;
    const float f6 = 0.6f;
    const Material& material = blockliquid->getMaterial();
    float f7 = getFluidHeight(blockAccess, blockPosIn, material);
    float f8 = getFluidHeight(blockAccess, blockPosIn.south(), material);
    float f9 = getFluidHeight(blockAccess, blockPosIn.east().south(), material);
    float f10 = getFluidHeight(blockAccess, blockPosIn.east(), material);
    const double d0 = static_cast<double>(blockPosIn.getX());
    const double d1 = static_cast<double>(blockPosIn.getY());
    const double d2 = static_cast<double>(blockPosIn.getZ());
    const float f11 = 0.001f;

    if (flag) {
        flag2 = true;
        TextureAtlasSprite* textureatlassprite = sprites[0];
        const float f12 = static_cast<float>(BlockLiquid::getFlowDirection(blockAccess, blockPosIn, material));
        if (f12 > -999.0f)
            textureatlassprite = sprites[1];

        f7 -= f11;
        f8 -= f11;
        f9 -= f11;
        f10 -= f11;
        float f13, f14, f15, f16, f17, f18, f19, f20;

        if (f12 < -999.0f) {
            f13 = textureatlassprite->getInterpolatedU(0.0);
            f17 = textureatlassprite->getInterpolatedV(0.0);
            f14 = f13;
            f18 = textureatlassprite->getInterpolatedV(16.0);
            f15 = textureatlassprite->getInterpolatedU(16.0);
            f19 = f18;
            f16 = f15;
            f20 = f17;
        } else {
            const float f21 = MathHelper::sin(f12) * 0.25f;
            const float f22 = MathHelper::cos(f12) * 0.25f;
            f13 = textureatlassprite->getInterpolatedU(static_cast<double>(8.0f + (-f22 - f21) * 16.0f));
            f17 = textureatlassprite->getInterpolatedV(static_cast<double>(8.0f + (-f22 + f21) * 16.0f));
            f14 = textureatlassprite->getInterpolatedU(static_cast<double>(8.0f + (-f22 + f21) * 16.0f));
            f18 = textureatlassprite->getInterpolatedV(static_cast<double>(8.0f + (f22 + f21) * 16.0f));
            f15 = textureatlassprite->getInterpolatedU(static_cast<double>(8.0f + (f22 + f21) * 16.0f));
            f19 = textureatlassprite->getInterpolatedV(static_cast<double>(8.0f + (f22 - f21) * 16.0f));
            f16 = textureatlassprite->getInterpolatedU(static_cast<double>(8.0f + (f22 - f21) * 16.0f));
            f20 = textureatlassprite->getInterpolatedV(static_cast<double>(8.0f + (-f22 - f21) * 16.0f));
        }

        const int k2 = blockliquid->getMixedBrightnessForBlock(blockAccess, blockPosIn);
        const int l2 = k2 >> 16 & 65535;
        const int i3 = k2 & 65535;
        const float f24 = f4 * f;
        const float f25 = f4 * f1;
        const float f26 = f4 * f2;
        worldRendererIn.pos(d0 + 0.0, d1 + static_cast<double>(f7), d2 + 0.0).color(f24, f25, f26, 1.0f).tex(f13, f17).lightmap(l2, i3).endVertex();
        worldRendererIn.pos(d0 + 0.0, d1 + static_cast<double>(f8), d2 + 1.0).color(f24, f25, f26, 1.0f).tex(f14, f18).lightmap(l2, i3).endVertex();
        worldRendererIn.pos(d0 + 1.0, d1 + static_cast<double>(f9), d2 + 1.0).color(f24, f25, f26, 1.0f).tex(f15, f19).lightmap(l2, i3).endVertex();
        worldRendererIn.pos(d0 + 1.0, d1 + static_cast<double>(f10), d2 + 0.0).color(f24, f25, f26, 1.0f).tex(f16, f20).lightmap(l2, i3).endVertex();

        if (blockliquid->shouldRenderSides(blockAccess, blockPosIn.up())) {
            worldRendererIn.pos(d0 + 0.0, d1 + static_cast<double>(f7), d2 + 0.0).color(f24, f25, f26, 1.0f).tex(f13, f17).lightmap(l2, i3).endVertex();
            worldRendererIn.pos(d0 + 1.0, d1 + static_cast<double>(f10), d2 + 0.0).color(f24, f25, f26, 1.0f).tex(f16, f20).lightmap(l2, i3).endVertex();
            worldRendererIn.pos(d0 + 1.0, d1 + static_cast<double>(f9), d2 + 1.0).color(f24, f25, f26, 1.0f).tex(f15, f19).lightmap(l2, i3).endVertex();
            worldRendererIn.pos(d0 + 0.0, d1 + static_cast<double>(f8), d2 + 1.0).color(f24, f25, f26, 1.0f).tex(f14, f18).lightmap(l2, i3).endVertex();
        }
    }

    if (flag1) {
        const float f35 = sprites[0]->getMinU();
        const float f36 = sprites[0]->getMaxU();
        const float f37 = sprites[0]->getMinV();
        const float f38 = sprites[0]->getMaxV();
        const int l1 = blockliquid->getMixedBrightnessForBlock(blockAccess, blockPosIn.down());
        const int i2 = l1 >> 16 & 65535;
        const int j2 = l1 & 65535;
        worldRendererIn.pos(d0, d1, d2 + 1.0).color(f3, f3, f3, 1.0f).tex(f35, f38).lightmap(i2, j2).endVertex();
        worldRendererIn.pos(d0, d1, d2).color(f3, f3, f3, 1.0f).tex(f35, f37).lightmap(i2, j2).endVertex();
        worldRendererIn.pos(d0 + 1.0, d1, d2).color(f3, f3, f3, 1.0f).tex(f36, f37).lightmap(i2, j2).endVertex();
        worldRendererIn.pos(d0 + 1.0, d1, d2 + 1.0).color(f3, f3, f3, 1.0f).tex(f36, f38).lightmap(i2, j2).endVertex();
        flag2 = true;
    }

    for (int i1 = 0; i1 < 4; ++i1) {
        int j1 = 0;
        int k1 = 0;
        if (i1 == 0)
            --k1;
        if (i1 == 1)
            ++k1;
        if (i1 == 2)
            --j1;
        if (i1 == 3)
            ++j1;

        const BlockPos blockpos = blockPosIn.add(j1, 0, k1);
        TextureAtlasSprite* textureatlassprite1 = sprites[1];

        if (sides[i1]) {
            float f39, f40;
            double d3, d4, d5, d6;
            if (i1 == 0) {
                f39 = f7;
                f40 = f10;
                d3 = d0;
                d5 = d0 + 1.0;
                d4 = d2 + static_cast<double>(f11);
                d6 = d2 + static_cast<double>(f11);
            } else if (i1 == 1) {
                f39 = f9;
                f40 = f8;
                d3 = d0 + 1.0;
                d5 = d0;
                d4 = d2 + 1.0 - static_cast<double>(f11);
                d6 = d2 + 1.0 - static_cast<double>(f11);
            } else if (i1 == 2) {
                f39 = f8;
                f40 = f7;
                d3 = d0 + static_cast<double>(f11);
                d5 = d0 + static_cast<double>(f11);
                d4 = d2 + 1.0;
                d6 = d2;
            } else {
                f39 = f10;
                f40 = f9;
                d3 = d0 + 1.0 - static_cast<double>(f11);
                d5 = d0 + 1.0 - static_cast<double>(f11);
                d4 = d2;
                d6 = d2 + 1.0;
            }

            flag2 = true;
            const float f41 = textureatlassprite1->getInterpolatedU(0.0);
            const float f27 = textureatlassprite1->getInterpolatedU(8.0);
            const float f28 = textureatlassprite1->getInterpolatedV(static_cast<double>((1.0f - f39) * 16.0f * 0.5f));
            const float f29 = textureatlassprite1->getInterpolatedV(static_cast<double>((1.0f - f40) * 16.0f * 0.5f));
            const float f30 = textureatlassprite1->getInterpolatedV(8.0);
            const int j = blockliquid->getMixedBrightnessForBlock(blockAccess, blockpos);
            const int k = j >> 16 & 65535;
            const int l = j & 65535;
            const float f31 = i1 < 2 ? f5 : f6;
            const float f32 = f4 * f31 * f;
            const float f33 = f4 * f31 * f1;
            const float f34 = f4 * f31 * f2;
            worldRendererIn.pos(d3, d1 + static_cast<double>(f39), d4).color(f32, f33, f34, 1.0f).tex(f41, f28).lightmap(k, l).endVertex();
            worldRendererIn.pos(d5, d1 + static_cast<double>(f40), d6).color(f32, f33, f34, 1.0f).tex(f27, f29).lightmap(k, l).endVertex();
            worldRendererIn.pos(d5, d1 + 0.0, d6).color(f32, f33, f34, 1.0f).tex(f27, f30).lightmap(k, l).endVertex();
            worldRendererIn.pos(d3, d1 + 0.0, d4).color(f32, f33, f34, 1.0f).tex(f41, f30).lightmap(k, l).endVertex();
            worldRendererIn.pos(d3, d1 + 0.0, d4).color(f32, f33, f34, 1.0f).tex(f41, f30).lightmap(k, l).endVertex();
            worldRendererIn.pos(d5, d1 + 0.0, d6).color(f32, f33, f34, 1.0f).tex(f27, f30).lightmap(k, l).endVertex();
            worldRendererIn.pos(d5, d1 + static_cast<double>(f40), d6).color(f32, f33, f34, 1.0f).tex(f27, f29).lightmap(k, l).endVertex();
            worldRendererIn.pos(d3, d1 + static_cast<double>(f39), d4).color(f32, f33, f34, 1.0f).tex(f41, f28).lightmap(k, l).endVertex();
        }
    }

    return flag2;
}

float BlockFluidRenderer::getFluidHeight(IBlockAccess* blockAccess, const BlockPos& blockPosIn, const Material& blockMaterial)
{
    int i = 0;
    float f = 0.0f;

    for (int j = 0; j < 4; ++j) {
        const BlockPos blockpos = blockPosIn.add(-(j & 1), 0, -(j >> 1 & 1));
        if (&blockAccess->getBlockState(blockpos.up())->getBlock()->getMaterial() == &blockMaterial)
            return 1.0f;

        const IBlockState* iblockstate = blockAccess->getBlockState(blockpos);
        const Material& material = iblockstate->getBlock()->getMaterial();
        if (&material != &blockMaterial) {
            if (!material.isSolid()) {
                ++f;
                ++i;
            }
        } else {
            const int k = iblockstate->getValue(BlockLiquid::LEVEL);
            if (k >= 8 || k == 0) {
                f += BlockLiquid::getLiquidHeightPercent(k) * 10.0f;
                i += 10;
            }
            f += BlockLiquid::getLiquidHeightPercent(k);
            ++i;
        }
    }

    return 1.0f - f / static_cast<float>(i);
}

} // namespace mc
