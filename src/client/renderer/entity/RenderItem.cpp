#include "client/renderer/entity/RenderItem.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/EntityRenderer.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/WorldRenderer.h"
#include "client/renderer/block/model/BakedQuad.h"
#include "client/renderer/block/model/ModelBlock.h"
#include "client/renderer/texture/ITextureObject.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/texture/TextureMap.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/resources/model/IBakedModel.h"
#include "core/Java.h"
#include "item/ItemStack.h"
#include "util/ResourceLocation.h"

namespace mc {

namespace {

const ResourceLocation RES_ITEM_GLINT("textures/misc/enchanted_item_glint.png");

} // namespace

void RenderItem::renderItem(const ItemStack* stack, const IBakedModel& model)
{
    if (stack == nullptr)
        return;

    DxStateManager::pushMatrix();
    DxStateManager::scale(0.5f, 0.5f, 0.5f);

    if (model.isBuiltInRenderer()) {
        // Java: TileEntityItemStackRenderer draws chests, banners and skulls -- stage 10.
        DxStateManager::rotate(180.0f, 0.0f, 1.0f, 0.0f);
        DxStateManager::translate(-0.5f, -0.5f, -0.5f);
        DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
        DxStateManager::enableRescaleNormal();
    } else {
        DxStateManager::translate(-0.5f, -0.5f, -0.5f);
        renderModel(model, -1, stack);
        if (stack->hasEffect())
            renderEffect(model);
    }

    DxStateManager::popMatrix();
}

void RenderItem::renderModel(const IBakedModel& model, int color, const ItemStack* stack)
{
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::ITEM);
    for (EnumFacing enumfacing : kFacings)
        renderQuads(worldrenderer, model.getFaceQuads(enumfacing), color, stack);
    renderQuads(worldrenderer, model.getGeneralQuads(), color, stack);
    tessellator.draw();
}

void RenderItem::renderEffect(const IBakedModel& model)
{
    DxStateManager::depthMask(false);
    DxStateManager::depthFunc(514);
    DxStateManager::disableLighting();
    DxStateManager::blendFunc(768, 1);
    textureManager.bindTexture(RES_ITEM_GLINT);
    DxStateManager::matrixMode(5890);
    DxStateManager::pushMatrix();
    DxStateManager::scale(8.0f, 8.0f, 8.0f);
    const float f = static_cast<float>(Minecraft::getSystemTime() % 3000LL) / 3000.0f / 8.0f;
    DxStateManager::translate(f, 0.0f, 0.0f);
    DxStateManager::rotate(-50.0f, 0.0f, 0.0f, 1.0f);
    renderModel(model, -8372020, nullptr);
    DxStateManager::popMatrix();
    DxStateManager::pushMatrix();
    DxStateManager::scale(8.0f, 8.0f, 8.0f);
    const float f1 = static_cast<float>(Minecraft::getSystemTime() % 4873LL) / 4873.0f / 8.0f;
    DxStateManager::translate(-f1, 0.0f, 0.0f);
    DxStateManager::rotate(10.0f, 0.0f, 0.0f, 1.0f);
    renderModel(model, -8372020, nullptr);
    DxStateManager::popMatrix();
    DxStateManager::matrixMode(5888);
    DxStateManager::blendFunc(770, 771);
    DxStateManager::enableLighting();
    DxStateManager::depthFunc(515);
    DxStateManager::depthMask(true);
    textureManager.bindTexture(TextureMap::locationBlocksTexture);
}

void RenderItem::renderQuads(WorldRenderer& renderer, const std::vector<BakedQuad>& quads, int color, const ItemStack* stack)
{
    const bool flag = color == -1 && stack != nullptr;

    for (const BakedQuad& bakedquad : quads) {
        int k = color;
        if (flag && bakedquad.hasTintIndex()) {
            k = stack->getItem()->getColorFromItemStack(*stack, bakedquad.getTintIndex());
            if (EntityRenderer::anaglyphEnable)
                k = static_cast<int>(TextureUtil::anaglyphColor(static_cast<uint32_t>(k)));
            k = k | -16777216;
        }
        renderQuad(renderer, bakedquad, k);
    }
}

void RenderItem::renderQuad(WorldRenderer& renderer, const BakedQuad& quad, int color)
{
    renderer.addVertexData(quad.getVertexData());
    renderer.putColor4(color);
    const Vec3i vec3i = getDirectionVec(quad.getFace());
    renderer.putNormal(static_cast<float>(vec3i.getX()), static_cast<float>(vec3i.getY()), static_cast<float>(vec3i.getZ()));
}

bool RenderItem::shouldRenderItemIn3D(const ItemStack& stack) const
{
    const IBakedModel* ibakedmodel = itemModelMesher.getItemModel(stack);
    return ibakedmodel != nullptr && ibakedmodel->isGui3d();
}

void RenderItem::renderItemIntoGUI(const ItemStack& stack, int x, int y)
{
    const IBakedModel* ibakedmodel = itemModelMesher.getItemModel(stack);
    DxStateManager::pushMatrix();
    textureManager.bindTexture(TextureMap::locationBlocksTexture);
    textureManager.getTexture(TextureMap::locationBlocksTexture)->setBlurMipmap(false, false);
    DxStateManager::enableRescaleNormal();
    DxStateManager::enableAlpha();
    DxStateManager::alphaFunc(516, 0.1f);
    DxStateManager::enableBlend();
    DxStateManager::blendFunc(770, 771);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    setupGuiTransform(x, y, ibakedmodel->isGui3d());
    ibakedmodel->getItemCameraTransforms().applyTransform(TransformType::GUI);
    renderItem(&stack, *ibakedmodel);
    DxStateManager::disableAlpha();
    DxStateManager::disableRescaleNormal();
    DxStateManager::disableLighting();
    DxStateManager::popMatrix();
    textureManager.bindTexture(TextureMap::locationBlocksTexture);
    textureManager.getTexture(TextureMap::locationBlocksTexture)->restoreLastBlurMipmap();
}

void RenderItem::setupGuiTransform(int xPosition, int yPosition, bool isGui3d)
{
    DxStateManager::translate(static_cast<float>(xPosition), static_cast<float>(yPosition), 100.0f + zLevel);
    DxStateManager::translate(8.0f, 8.0f, 0.0f);
    DxStateManager::scale(1.0f, 1.0f, -1.0f);
    DxStateManager::scale(0.5f, 0.5f, 0.5f);

    if (isGui3d) {
        DxStateManager::scale(40.0f, 40.0f, 40.0f);
        DxStateManager::rotate(210.0f, 1.0f, 0.0f, 0.0f);
        DxStateManager::rotate(-135.0f, 0.0f, 1.0f, 0.0f);
        DxStateManager::enableLighting();
    } else {
        DxStateManager::scale(64.0f, 64.0f, 64.0f);
        DxStateManager::rotate(180.0f, 1.0f, 0.0f, 0.0f);
        DxStateManager::disableLighting();
    }
}

void RenderItem::renderItemAndEffectIntoGUI(const ItemStack* stack, int xPosition, int yPosition)
{
    if (stack != nullptr && stack->getItem() != nullptr) {
        zLevel += 50.0f;
        renderItemIntoGUI(*stack, xPosition, yPosition);
        zLevel -= 50.0f;
    }
}

void RenderItem::renderItemOverlayIntoGUI(FontRenderer& fr, const ItemStack* stack, int xPosition, int yPosition, const std::string* text)
{
    if (stack == nullptr)
        return;

    if (stack->stackSize != 1 || text != nullptr) {
        std::string s = text == nullptr ? std::to_string(stack->stackSize) : *text;
        if (text == nullptr && stack->stackSize < 1)
            s = "§c" + std::to_string(stack->stackSize);
        DxStateManager::disableLighting();
        DxStateManager::disableDepth();
        DxStateManager::disableBlend();
        fr.drawStringWithShadow(s, static_cast<float>(xPosition + 19 - 2 - fr.getStringWidth(s)), static_cast<float>(yPosition + 6 + 3), 16777215);
        DxStateManager::enableLighting();
        DxStateManager::enableDepth();
    }

    if (stack->isItemDamaged()) {
        const int j = static_cast<int>(java::round(13.0 - static_cast<double>(stack->getItemDamage()) * 13.0 / static_cast<double>(stack->getMaxDamage())));
        const int i = static_cast<int>(java::round(255.0 - static_cast<double>(stack->getItemDamage()) * 255.0 / static_cast<double>(stack->getMaxDamage())));
        DxStateManager::disableLighting();
        DxStateManager::disableDepth();
        DxStateManager::disableTexture2D();
        DxStateManager::disableAlpha();
        DxStateManager::disableBlend();
        WorldRenderer& worldrenderer = Tessellator::getInstance().getWorldRenderer();
        draw(worldrenderer, xPosition + 2, yPosition + 13, 13, 2, 0, 0, 0, 255);
        draw(worldrenderer, xPosition + 2, yPosition + 13, 12, 1, (255 - i) / 4, 64, 0, 255);
        draw(worldrenderer, xPosition + 2, yPosition + 13, j, 1, 255 - i, i, 0, 255);
        DxStateManager::enableBlend();
        DxStateManager::enableAlpha();
        DxStateManager::enableTexture2D();
        DxStateManager::enableLighting();
        DxStateManager::enableDepth();
    }
}

void RenderItem::draw(WorldRenderer& renderer, int x, int y, int width, int height, int red, int green, int blue, int alpha)
{
    renderer.begin(7, DefaultVertexFormats::POSITION_COLOR);
    renderer.pos(x + 0, y + 0, 0.0).color(red, green, blue, alpha).endVertex();
    renderer.pos(x + 0, y + height, 0.0).color(red, green, blue, alpha).endVertex();
    renderer.pos(x + width, y + height, 0.0).color(red, green, blue, alpha).endVertex();
    renderer.pos(x + width, y + 0, 0.0).color(red, green, blue, alpha).endVertex();
    Tessellator::getInstance().draw();
}

} // namespace mc
