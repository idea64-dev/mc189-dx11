#include "client/gui/GuiIngame.h"

#include "block/BlockGlass.h"
#include "block/material/Material.h"
#include "block/state/BlockState.h"
#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/ScaledResolution.h"
#include "client/multiplayer/PlayerControllerMP.h"
#include "client/multiplayer/WorldClient.h"
#include "client/network/NetHandlerPlayClient.h"
#include "client/renderer/BlockModelShapes.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/EntityRenderer.h"
#include "client/renderer/RenderHelper.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/WorldRenderer.h"
#include "client/renderer/entity/RenderItem.h"
#include "client/renderer/texture/TextureAtlasSprite.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/texture/TextureMap.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/resources/Locale.h"
#include "client/resources/model/ModelManager.h"
#include "client/settings/GameSettings.h"
#include "core/Java.h"
#include "entity/EntityLivingBase.h"
#include "entity/SharedMonsterAttributes.h"
#include "entity/ai/attributes/Attributes.h"
#include "entity/player/EntityPlayer.h"
#include "entity/player/EntityPlayerSP.h"
#include "entity/player/InventoryPlayer.h"
#include "init/Blocks.h"
#include "item/Item.h"
#include "item/ItemStack.h"
#include "potion/Potion.h"
#include "tileentity/TileEntity.h"
#include "util/ChatComponent.h"
#include "util/FoodStats.h"
#include "util/MathHelper.h"
#include "world/border/WorldBorder.h"
#include "world/storage/WorldInfo.h"

#include <algorithm>
#include <cmath>

namespace mc {

GuiIngame::GuiIngame(Minecraft* mcIn)
    : mc(mcIn)
    , itemRenderer(&mcIn->getRenderItem())
    , persistantChatGUI(mcIn)
    , overlayDebug(mcIn)
    , overlayPlayerList(mcIn, this)
{
    setDefaultTitlesTimes();
}

void GuiIngame::setDefaultTitlesTimes()
{
    titleFadeIn = 10;
    titleDisplayTime = 70;
    titleFadeOut = 20;
}

FontRenderer& GuiIngame::getFontRenderer()
{
    return *mc->fontRendererObj;
}

void GuiIngame::renderGameOverlay(float partialTicks)
{
    const ScaledResolution scaledresolution(*mc);
    const int i = scaledresolution.getScaledWidth();
    const int j = scaledresolution.getScaledHeight();
    mc->entityRenderer->setupOverlayRendering();
    DxStateManager::enableBlend();

    if (mc->gameSettings->fancyGraphics)
        renderVignette(mc->thePlayer->getBrightness(partialTicks), scaledresolution);
    else
        DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);

    const std::shared_ptr<ItemStack> itemstack = mc->thePlayer->inventory.armorItemInSlot(3);

    if (mc->gameSettings->thirdPersonView == 0 && itemstack != nullptr && itemstack->getItem() == Item::getItemFromBlock(Blocks::pumpkin))
        renderPumpkinOverlay(scaledresolution);

    if (!mc->thePlayer->isPotionActive(Potion::confusion)) {
        const float f = mc->thePlayer->prevTimeInPortal + (mc->thePlayer->timeInPortal - mc->thePlayer->prevTimeInPortal) * partialTicks;

        if (f > 0.0f)
            renderPortal(f, scaledresolution);
    }

    // Java: GuiSpectator.renderTooltip for spectators (not ported yet).
    if (!mc->playerController->isSpectator())
        renderTooltip(scaledresolution, partialTicks);

    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    mc->getTextureManager().bindTexture(icons);
    DxStateManager::enableBlend();

    if (showCrosshair()) {
        DxStateManager::tryBlendFuncSeparate(775, 769, 1, 0);
        DxStateManager::enableAlpha();
        drawTexturedModalRect(i / 2 - 7, j / 2 - 7, 0, 0, 16, 16);
    }

    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    mc->mcProfiler.startSection("bossHealth");
    // Java: renderBossHealth -- BossStatus (no bosses yet).
    mc->mcProfiler.endSection();

    if (mc->playerController->shouldDrawHUD())
        renderPlayerStats(scaledresolution);

    DxStateManager::disableBlend();

    if (mc->thePlayer->getSleepTimer() > 0) {
        mc->mcProfiler.startSection("sleep");
        DxStateManager::disableDepth();
        DxStateManager::disableAlpha();
        const int j1 = mc->thePlayer->getSleepTimer();
        float f1 = static_cast<float>(j1) / 100.0f;

        if (f1 > 1.0f)
            f1 = 1.0f - static_cast<float>(j1 - 100) / 10.0f;

        const int k = java::f2i(220.0f * f1) << 24 | 1052704;
        drawRect(0, 0, i, j, k);
        DxStateManager::enableAlpha();
        DxStateManager::enableDepth();
        mc->mcProfiler.endSection();
    }

    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    const int k1 = i / 2 - 91;

    if (mc->thePlayer->isRidingHorse())
        renderHorseJumpBar(scaledresolution, k1);
    else if (mc->playerController->gameIsSurvivalOrAdventure())
        renderExpBar(scaledresolution, k1);

    // Java: else if spectating, GuiSpectator.renderSelectedItem (not ported yet).
    if (mc->gameSettings->heldItemTooltips && !mc->playerController->isSpectator())
        renderSelectedItem(scaledresolution);

    if (mc->gameSettings->showDebugInfo)
        overlayDebug.renderDebugInfo(scaledresolution);

    if (recordPlayingUpFor > 0) {
        mc->mcProfiler.startSection("overlayMessage");
        const float f2 = static_cast<float>(recordPlayingUpFor) - partialTicks;
        int l1 = java::f2i(f2 * 255.0f / 20.0f);

        if (l1 > 255)
            l1 = 255;

        if (l1 > 8) {
            DxStateManager::pushMatrix();
            DxStateManager::translate(static_cast<float>(i / 2), static_cast<float>(j - 68), 0.0f);
            DxStateManager::enableBlend();
            DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
            int l = 16777215;

            if (recordIsPlaying)
                l = MathHelper::hsvToRGB(f2 / 50.0f, 0.7f, 0.6f) & 16777215;

            getFontRenderer().drawString(recordPlaying, -getFontRenderer().getStringWidth(recordPlaying) / 2, -4, l + (l1 << 24 & -16777216));
            DxStateManager::disableBlend();
            DxStateManager::popMatrix();
        }

        mc->mcProfiler.endSection();
    }

    if (titlesTimer > 0) {
        mc->mcProfiler.startSection("titleAndSubtitle");
        const float f3 = static_cast<float>(titlesTimer) - partialTicks;
        int i2 = 255;

        if (titlesTimer > titleFadeOut + titleDisplayTime) {
            const float f4 = static_cast<float>(titleFadeIn + titleDisplayTime + titleFadeOut) - f3;
            i2 = java::f2i(f4 * 255.0f / static_cast<float>(titleFadeIn));
        }

        if (titlesTimer <= titleFadeOut)
            i2 = java::f2i(f3 * 255.0f / static_cast<float>(titleFadeOut));

        i2 = MathHelper::clamp_int(i2, 0, 255);

        if (i2 > 8) {
            DxStateManager::pushMatrix();
            DxStateManager::translate(static_cast<float>(i / 2), static_cast<float>(j / 2), 0.0f);
            DxStateManager::enableBlend();
            DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
            DxStateManager::pushMatrix();
            DxStateManager::scale(4.0f, 4.0f, 4.0f);
            const int j2 = i2 << 24 & -16777216;
            getFontRenderer().drawString(displayedTitle, static_cast<float>(-getFontRenderer().getStringWidth(displayedTitle) / 2), -10.0f, 16777215 | j2, true);
            DxStateManager::popMatrix();
            DxStateManager::pushMatrix();
            DxStateManager::scale(2.0f, 2.0f, 2.0f);
            getFontRenderer().drawString(displayedSubTitle, static_cast<float>(-getFontRenderer().getStringWidth(displayedSubTitle) / 2), 5.0f, 16777215 | j2, true);
            DxStateManager::popMatrix();
            DxStateManager::disableBlend();
            DxStateManager::popMatrix();
        }

        mc->mcProfiler.endSection();
    }

    // Java: the scoreboard sidebar (stage 12).

    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::disableAlpha();
    DxStateManager::pushMatrix();
    DxStateManager::translate(0.0f, static_cast<float>(j - 48), 0.0f);
    mc->mcProfiler.startSection("chat");
    persistantChatGUI.drawChat(updateCounter);
    mc->mcProfiler.endSection();
    DxStateManager::popMatrix();

    // Java: the list objective in display slot 0 (none without the scoreboard).
    if (!mc->gameSettings->keyBindPlayerList.isKeyDown() || (mc->isIntegratedServerRunning() && mc->thePlayer->sendQueue->getPlayerInfoMap().size() <= 1)) {
        overlayPlayerList.updatePlayerList(false);
    } else {
        overlayPlayerList.updatePlayerList(true);
        overlayPlayerList.renderPlayerlist(i);
    }

    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    DxStateManager::disableLighting();
    DxStateManager::enableAlpha();
}

void GuiIngame::renderTooltip(const ScaledResolution& sr, float partialTicks)
{
    auto* entityplayer = dynamic_cast<EntityPlayer*>(mc->getRenderViewEntity());
    if (entityplayer == nullptr)
        return;

    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    mc->getTextureManager().bindTexture(widgetsTexPath);
    const int i = sr.getScaledWidth() / 2;
    const float f = zLevel;
    zLevel = -90.0f;
    drawTexturedModalRect(i - 91, sr.getScaledHeight() - 22, 0, 0, 182, 22);
    drawTexturedModalRect(i - 91 - 1 + entityplayer->inventory.currentItem * 20, sr.getScaledHeight() - 22 - 1, 0, 22, 24, 22);
    zLevel = f;
    DxStateManager::enableRescaleNormal();
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    RenderHelper::enableGUIStandardItemLighting();

    for (int j = 0; j < 9; ++j) {
        const int k = sr.getScaledWidth() / 2 - 90 + j * 20 + 2;
        const int l = sr.getScaledHeight() - 16 - 3;
        renderHotbarItem(j, k, l, partialTicks, entityplayer);
    }

    RenderHelper::disableStandardItemLighting();
    DxStateManager::disableRescaleNormal();
    DxStateManager::disableBlend();
}

void GuiIngame::renderHorseJumpBar(const ScaledResolution& scaledRes, int x)
{
    mc->mcProfiler.startSection("jumpBar");
    mc->getTextureManager().bindTexture(Gui::icons);
    const float f = mc->thePlayer->getHorseJumpPower();
    const int i = 182;
    const int j = java::f2i(f * static_cast<float>(i + 1));
    const int k = scaledRes.getScaledHeight() - 32 + 3;
    drawTexturedModalRect(x, k, 0, 84, i, 5);

    if (j > 0)
        drawTexturedModalRect(x, k, 0, 89, j, 5);

    mc->mcProfiler.endSection();
}

void GuiIngame::renderExpBar(const ScaledResolution& scaledRes, int x)
{
    mc->mcProfiler.startSection("expBar");
    mc->getTextureManager().bindTexture(Gui::icons);
    const int i = mc->thePlayer->xpBarCap();

    if (i > 0) {
        const int j = 182;
        const int k = java::f2i(mc->thePlayer->experience * static_cast<float>(j + 1));
        const int l = scaledRes.getScaledHeight() - 32 + 3;
        drawTexturedModalRect(x, l, 0, 64, j, 5);

        if (k > 0)
            drawTexturedModalRect(x, l, 0, 69, k, 5);
    }

    mc->mcProfiler.endSection();

    if (mc->thePlayer->experienceLevel > 0) {
        mc->mcProfiler.startSection("expLevel");
        const int k1 = 8453920;
        const std::string s = std::to_string(mc->thePlayer->experienceLevel);
        const int l1 = (scaledRes.getScaledWidth() - getFontRenderer().getStringWidth(s)) / 2;
        const int i1 = scaledRes.getScaledHeight() - 31 - 4;
        getFontRenderer().drawString(s, l1 + 1, i1, 0);
        getFontRenderer().drawString(s, l1 - 1, i1, 0);
        getFontRenderer().drawString(s, l1, i1 + 1, 0);
        getFontRenderer().drawString(s, l1, i1 - 1, 0);
        getFontRenderer().drawString(s, l1, i1, k1);
        mc->mcProfiler.endSection();
    }
}

void GuiIngame::renderSelectedItem(const ScaledResolution& scaledRes)
{
    mc->mcProfiler.startSection("selectedItemName");

    if (remainingHighlightTicks > 0 && highlightingItemStack != nullptr) {
        std::string s = highlightingItemStack->getDisplayName();

        if (highlightingItemStack->hasDisplayName())
            s = EnumChatFormatting::toString('o') + s;

        const int i = (scaledRes.getScaledWidth() - getFontRenderer().getStringWidth(s)) / 2;
        int j = scaledRes.getScaledHeight() - 59;

        if (!mc->playerController->shouldDrawHUD())
            j += 14;

        int k = java::f2i(static_cast<float>(remainingHighlightTicks) * 256.0f / 10.0f);

        if (k > 255)
            k = 255;

        if (k > 0) {
            DxStateManager::pushMatrix();
            DxStateManager::enableBlend();
            DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
            getFontRenderer().drawStringWithShadow(s, static_cast<float>(i), static_cast<float>(j), 16777215 + (k << 24));
            DxStateManager::disableBlend();
            DxStateManager::popMatrix();
        }
    }

    mc->mcProfiler.endSection();
}

bool GuiIngame::showCrosshair()
{
    if (mc->gameSettings->showDebugInfo && !mc->thePlayer->hasReducedDebug() && !mc->gameSettings->reducedDebugInfo)
        return false;

    if (mc->playerController->isSpectator()) {
        if (mc->pointedEntity != nullptr)
            return true;

        if (mc->objectMouseOver && mc->objectMouseOver->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK) {
            const BlockPos& blockpos = *mc->objectMouseOver->getBlockPos();

            if (dynamic_cast<IInventory*>(mc->theWorld->getTileEntity(blockpos)) != nullptr)
                return true;
        }

        return false;
    }

    return true;
}

void GuiIngame::renderPlayerStats(const ScaledResolution& scaledRes)
{
    auto* entityplayer = dynamic_cast<EntityPlayer*>(mc->getRenderViewEntity());
    if (entityplayer == nullptr)
        return;

    const int i = MathHelper::ceiling_float_int(entityplayer->getHealth());
    const bool flag = healthUpdateCounter > static_cast<int64_t>(updateCounter) && (healthUpdateCounter - static_cast<int64_t>(updateCounter)) / 3 % 2 == 1;

    if (i < playerHealth && entityplayer->hurtResistantTime > 0) {
        lastSystemTime = Minecraft::getSystemTime();
        healthUpdateCounter = static_cast<int64_t>(updateCounter + 20);
    } else if (i > playerHealth && entityplayer->hurtResistantTime > 0) {
        lastSystemTime = Minecraft::getSystemTime();
        healthUpdateCounter = static_cast<int64_t>(updateCounter + 10);
    }

    if (Minecraft::getSystemTime() - lastSystemTime > 1000) {
        playerHealth = i;
        lastPlayerHealth = i;
        lastSystemTime = Minecraft::getSystemTime();
    }

    playerHealth = i;
    const int j = lastPlayerHealth;
    rand.setSeed(static_cast<int64_t>(java::mul(updateCounter, 312871)));
    const bool flag1 = false;
    FoodStats& foodstats = entityplayer->getFoodStats();
    const int k = foodstats.getFoodLevel();
    const int l = foodstats.getPrevFoodLevel();
    IAttributeInstance* iattributeinstance = entityplayer->getEntityAttribute(SharedMonsterAttributes::maxHealth());
    const int i1 = scaledRes.getScaledWidth() / 2 - 91;
    const int j1 = scaledRes.getScaledWidth() / 2 + 91;
    const int k1 = scaledRes.getScaledHeight() - 39;
    const float f = static_cast<float>(iattributeinstance->getAttributeValue());
    const float f1 = entityplayer->getAbsorptionAmount();
    const int l1 = MathHelper::ceiling_float_int((f + f1) / 2.0f / 10.0f);
    const int i2 = std::max(10 - (l1 - 2), 3);
    const int j2 = k1 - (l1 - 1) * i2 - 10;
    float f2 = f1;
    const int k2 = entityplayer->getTotalArmorValue();
    int l2 = -1;

    if (entityplayer->isPotionActive(Potion::regeneration))
        l2 = updateCounter % MathHelper::ceiling_float_int(f + 5.0f);

    mc->mcProfiler.startSection("armor");

    for (int i3 = 0; i3 < 10; ++i3) {
        if (k2 > 0) {
            const int j3 = i1 + i3 * 8;

            if (i3 * 2 + 1 < k2)
                drawTexturedModalRect(j3, j2, 34, 9, 9, 9);

            if (i3 * 2 + 1 == k2)
                drawTexturedModalRect(j3, j2, 25, 9, 9, 9);

            if (i3 * 2 + 1 > k2)
                drawTexturedModalRect(j3, j2, 16, 9, 9, 9);
        }
    }

    mc->mcProfiler.endStartSection("health");

    for (int i6 = MathHelper::ceiling_float_int((f + f1) / 2.0f) - 1; i6 >= 0; --i6) {
        int j6 = 16;

        if (entityplayer->isPotionActive(Potion::poison))
            j6 += 36;
        else if (entityplayer->isPotionActive(Potion::wither))
            j6 += 72;

        int k3 = 0;

        if (flag)
            k3 = 1;

        const int l3 = MathHelper::ceiling_float_int(static_cast<float>(i6 + 1) / 10.0f) - 1;
        const int i4 = i1 + i6 % 10 * 8;
        int j4 = k1 - l3 * i2;

        if (i <= 4)
            j4 += rand.nextInt(2);

        if (i6 == l2)
            j4 -= 2;

        int k4 = 0;

        if (entityplayer->worldObj->getWorldInfo().isHardcoreModeEnabled())
            k4 = 5;

        drawTexturedModalRect(i4, j4, 16 + k3 * 9, 9 * k4, 9, 9);

        if (flag) {
            if (i6 * 2 + 1 < j)
                drawTexturedModalRect(i4, j4, j6 + 54, 9 * k4, 9, 9);

            if (i6 * 2 + 1 == j)
                drawTexturedModalRect(i4, j4, j6 + 63, 9 * k4, 9, 9);
        }

        if (f2 > 0.0f) {
            if (f2 == f1 && std::fmod(f1, 2.0f) == 1.0f)
                drawTexturedModalRect(i4, j4, j6 + 153, 9 * k4, 9, 9);
            else
                drawTexturedModalRect(i4, j4, j6 + 144, 9 * k4, 9, 9);

            f2 -= 2.0f;
        } else {
            if (i6 * 2 + 1 < i)
                drawTexturedModalRect(i4, j4, j6 + 36, 9 * k4, 9, 9);

            if (i6 * 2 + 1 == i)
                drawTexturedModalRect(i4, j4, j6 + 45, 9 * k4, 9, 9);
        }
    }

    Entity* entity = entityplayer->ridingEntity.get();

    if (entity == nullptr) {
        mc->mcProfiler.endStartSection("food");

        for (int k6 = 0; k6 < 10; ++k6) {
            int i7 = k1;
            int l7 = 16;
            int j8 = 0;

            if (entityplayer->isPotionActive(Potion::hunger)) {
                l7 += 36;
                j8 = 13;
            }

            if (entityplayer->getFoodStats().getSaturationLevel() <= 0.0f && updateCounter % (k * 3 + 1) == 0)
                i7 = k1 + (rand.nextInt(3) - 1);

            if (flag1)
                j8 = 1;

            const int i9 = j1 - k6 * 8 - 9;
            drawTexturedModalRect(i9, i7, 16 + j8 * 9, 27, 9, 9);

            if (flag1) {
                if (k6 * 2 + 1 < l)
                    drawTexturedModalRect(i9, i7, l7 + 54, 27, 9, 9);

                if (k6 * 2 + 1 == l)
                    drawTexturedModalRect(i9, i7, l7 + 63, 27, 9, 9);
            }

            if (k6 * 2 + 1 < k)
                drawTexturedModalRect(i9, i7, l7 + 36, 27, 9, 9);

            if (k6 * 2 + 1 == k)
                drawTexturedModalRect(i9, i7, l7 + 45, 27, 9, 9);
        }
    } else if (auto* entitylivingbase = dynamic_cast<EntityLivingBase*>(entity)) {
        mc->mcProfiler.endStartSection("mountHealth");
        const int j7 = static_cast<int>(std::ceil(static_cast<double>(entitylivingbase->getHealth())));
        const float f3 = entitylivingbase->getMaxHealth();
        int k8 = java::f2i(f3 + 0.5f) / 2;

        if (k8 > 30)
            k8 = 30;

        int j9 = k1;

        for (int k9 = 0; k8 > 0; k9 += 20) {
            const int l4 = std::min(k8, 10);
            k8 -= l4;

            for (int i5 = 0; i5 < l4; ++i5) {
                const int j5 = 52;
                int k5 = 0;

                if (flag1)
                    k5 = 1;

                const int l5 = j1 - i5 * 8 - 9;
                drawTexturedModalRect(l5, j9, j5 + k5 * 9, 9, 9, 9);

                if (i5 * 2 + 1 + k9 < j7)
                    drawTexturedModalRect(l5, j9, j5 + 36, 9, 9, 9);

                if (i5 * 2 + 1 + k9 == j7)
                    drawTexturedModalRect(l5, j9, j5 + 45, 9, 9, 9);
            }

            j9 -= 10;
        }
    }

    mc->mcProfiler.endStartSection("air");

    if (entityplayer->isInsideOfMaterial(Material::water)) {
        const int l6 = mc->thePlayer->getAir();
        const int k7 = MathHelper::ceiling_double_int(static_cast<double>(l6 - 2) * 10.0 / 300.0);
        const int i8 = MathHelper::ceiling_double_int(static_cast<double>(l6) * 10.0 / 300.0) - k7;

        for (int l8 = 0; l8 < k7 + i8; ++l8) {
            if (l8 < k7)
                drawTexturedModalRect(j1 - l8 * 8 - 9, j2, 16, 18, 9, 9);
            else
                drawTexturedModalRect(j1 - l8 * 8 - 9, j2, 25, 18, 9, 9);
        }
    }

    mc->mcProfiler.endSection();
}

void GuiIngame::renderPumpkinOverlay(const ScaledResolution& scaledRes)
{
    DxStateManager::disableDepth();
    DxStateManager::depthMask(false);
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    DxStateManager::disableAlpha();
    mc->getTextureManager().bindTexture(pumpkinBlurTexPath);
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(0.0, static_cast<double>(scaledRes.getScaledHeight()), -90.0).tex(0.0, 1.0).endVertex();
    worldrenderer.pos(static_cast<double>(scaledRes.getScaledWidth()), static_cast<double>(scaledRes.getScaledHeight()), -90.0).tex(1.0, 1.0).endVertex();
    worldrenderer.pos(static_cast<double>(scaledRes.getScaledWidth()), 0.0, -90.0).tex(1.0, 0.0).endVertex();
    worldrenderer.pos(0.0, 0.0, -90.0).tex(0.0, 0.0).endVertex();
    tessellator.draw();
    DxStateManager::depthMask(true);
    DxStateManager::enableDepth();
    DxStateManager::enableAlpha();
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
}

void GuiIngame::renderVignette(float lightLevel, const ScaledResolution& scaledRes)
{
    lightLevel = 1.0f - lightLevel;
    lightLevel = MathHelper::clamp_float(lightLevel, 0.0f, 1.0f);
    WorldBorder& worldborder = mc->theWorld->getWorldBorder();
    // Java: getClosestDistance(Entity) -- by the entity's x and z.
    float f = static_cast<float>(worldborder.getClosestDistance(mc->thePlayer->posX, mc->thePlayer->posZ));
    const double d0 = std::min(worldborder.getResizeSpeed() * static_cast<double>(worldborder.getWarningTime()) * 1000.0,
        std::abs(worldborder.getTargetSize() - worldborder.getDiameter()));
    const double d1 = std::max(static_cast<double>(worldborder.getWarningDistance()), d0);

    if (static_cast<double>(f) < d1)
        f = 1.0f - static_cast<float>(static_cast<double>(f) / d1);
    else
        f = 0.0f;

    prevVignetteBrightness = static_cast<float>(static_cast<double>(prevVignetteBrightness) + static_cast<double>(lightLevel - prevVignetteBrightness) * 0.01);
    DxStateManager::disableDepth();
    DxStateManager::depthMask(false);
    DxStateManager::tryBlendFuncSeparate(0, 769, 1, 0);

    if (f > 0.0f)
        DxStateManager::color(0.0f, f, f, 1.0f);
    else
        DxStateManager::color(prevVignetteBrightness, prevVignetteBrightness, prevVignetteBrightness, 1.0f);

    mc->getTextureManager().bindTexture(vignetteTexPath);
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(0.0, static_cast<double>(scaledRes.getScaledHeight()), -90.0).tex(0.0, 1.0).endVertex();
    worldrenderer.pos(static_cast<double>(scaledRes.getScaledWidth()), static_cast<double>(scaledRes.getScaledHeight()), -90.0).tex(1.0, 1.0).endVertex();
    worldrenderer.pos(static_cast<double>(scaledRes.getScaledWidth()), 0.0, -90.0).tex(1.0, 0.0).endVertex();
    worldrenderer.pos(0.0, 0.0, -90.0).tex(0.0, 0.0).endVertex();
    tessellator.draw();
    DxStateManager::depthMask(true);
    DxStateManager::enableDepth();
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
}

void GuiIngame::renderPortal(float timeInPortal, const ScaledResolution& scaledRes)
{
    if (timeInPortal < 1.0f) {
        timeInPortal = timeInPortal * timeInPortal;
        timeInPortal = timeInPortal * timeInPortal;
        timeInPortal = timeInPortal * 0.8f + 0.2f;
    }

    DxStateManager::disableAlpha();
    DxStateManager::disableDepth();
    DxStateManager::depthMask(false);
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::color(1.0f, 1.0f, 1.0f, timeInPortal);
    mc->getTextureManager().bindTexture(TextureMap::locationBlocksTexture);
    // Java: mc.getBlockRendererDispatcher().getBlockModelShapes() -- the same instance as the model manager's.
    TextureAtlasSprite* textureatlassprite = mc->getModelManager().getBlockModelShapes().getTexture(Blocks::portal->getDefaultState());
    const float f = textureatlassprite->getMinU();
    const float f1 = textureatlassprite->getMinV();
    const float f2 = textureatlassprite->getMaxU();
    const float f3 = textureatlassprite->getMaxV();
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(0.0, static_cast<double>(scaledRes.getScaledHeight()), -90.0).tex(static_cast<double>(f), static_cast<double>(f3)).endVertex();
    worldrenderer.pos(static_cast<double>(scaledRes.getScaledWidth()), static_cast<double>(scaledRes.getScaledHeight()), -90.0)
        .tex(static_cast<double>(f2), static_cast<double>(f3))
        .endVertex();
    worldrenderer.pos(static_cast<double>(scaledRes.getScaledWidth()), 0.0, -90.0).tex(static_cast<double>(f2), static_cast<double>(f1)).endVertex();
    worldrenderer.pos(0.0, 0.0, -90.0).tex(static_cast<double>(f), static_cast<double>(f1)).endVertex();
    tessellator.draw();
    DxStateManager::depthMask(true);
    DxStateManager::enableDepth();
    DxStateManager::enableAlpha();
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
}

void GuiIngame::renderHotbarItem(int index, int xPos, int yPos, float partialTicks, EntityPlayer* player)
{
    const std::shared_ptr<ItemStack>& itemstack = player->inventory.mainInventory[static_cast<size_t>(index)];

    if (itemstack == nullptr)
        return;

    const float f = static_cast<float>(itemstack->animationsToGo) - partialTicks;

    if (f > 0.0f) {
        DxStateManager::pushMatrix();
        const float f1 = 1.0f + f / 5.0f;
        DxStateManager::translate(static_cast<float>(xPos + 8), static_cast<float>(yPos + 12), 0.0f);
        DxStateManager::scale(1.0f / f1, (f1 + 1.0f) / 2.0f, 1.0f);
        DxStateManager::translate(static_cast<float>(-(xPos + 8)), static_cast<float>(-(yPos + 12)), 0.0f);
    }

    itemRenderer->renderItemAndEffectIntoGUI(itemstack.get(), xPos, yPos);

    if (f > 0.0f)
        DxStateManager::popMatrix();

    itemRenderer->renderItemOverlays(*mc->fontRendererObj, itemstack.get(), xPos, yPos);
}

void GuiIngame::updateTick()
{
    if (recordPlayingUpFor > 0)
        --recordPlayingUpFor;

    if (titlesTimer > 0) {
        --titlesTimer;

        if (titlesTimer <= 0) {
            displayedTitle.clear();
            displayedSubTitle.clear();
        }
    }

    ++updateCounter;

    if (mc->thePlayer != nullptr) {
        const std::shared_ptr<ItemStack> itemstack = mc->thePlayer->inventory.getCurrentItem();

        if (itemstack == nullptr) {
            remainingHighlightTicks = 0;
        } else if (highlightingItemStack != nullptr && itemstack->getItem() == highlightingItemStack->getItem()
            && ItemStack::areItemStackTagsEqual(itemstack.get(), highlightingItemStack.get())
            && (itemstack->isItemStackDamageable() || itemstack->getMetadata() == highlightingItemStack->getMetadata())) {
            if (remainingHighlightTicks > 0)
                --remainingHighlightTicks;
        } else {
            remainingHighlightTicks = 40;
        }

        highlightingItemStack = itemstack;
    }
}

void GuiIngame::setRecordPlayingMessage(const std::string& recordName)
{
    setRecordPlaying(I18n::format("record.nowPlaying", recordName), true);
}

void GuiIngame::setRecordPlaying(const std::string& message, bool isPlaying)
{
    recordPlaying = message;
    recordPlayingUpFor = 60;
    recordIsPlaying = isPlaying;
}

void GuiIngame::setRecordPlaying(const std::shared_ptr<IChatComponent>& component, bool isPlaying)
{
    setRecordPlaying(component->getUnformattedText(), isPlaying);
}

void GuiIngame::displayTitle(const std::optional<std::string>& title, const std::optional<std::string>& subTitle, int timeFadeIn, int displayTime, int timeFadeOut)
{
    if (!title && !subTitle && timeFadeIn < 0 && displayTime < 0 && timeFadeOut < 0) {
        displayedTitle.clear();
        displayedSubTitle.clear();
        titlesTimer = 0;
    } else if (title) {
        displayedTitle = *title;
        titlesTimer = titleFadeIn + titleDisplayTime + titleFadeOut;
    } else if (subTitle) {
        displayedSubTitle = *subTitle;
    } else {
        if (timeFadeIn >= 0)
            titleFadeIn = timeFadeIn;

        if (displayTime >= 0)
            titleDisplayTime = displayTime;

        if (timeFadeOut >= 0)
            titleFadeOut = timeFadeOut;

        if (titlesTimer > 0)
            titlesTimer = titleFadeIn + titleDisplayTime + titleFadeOut;
    }
}

} // namespace mc
