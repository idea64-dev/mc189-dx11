#include "client/renderer/RenderGlobal.h"

#include "block/Block.h"
#include "block/BlockChest.h"
#include "block/BlockEnderChest.h"
#include "block/BlockLeaves.h"
#include "block/BlockSign.h"
#include "block/BlockSkull.h"
#include "block/material/Material.h"
#include "block/state/BlockState.h"
#include "client/Minecraft.h"
#include "client/entity/EntityPlayerSP.h"
#include "client/multiplayer/WorldClient.h"
#include "client/renderer/BlockRendererDispatcher.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/EntityRenderer.h"
#include "client/renderer/RenderHelper.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/ViewFrustum.h"
#include "client/renderer/WorldRenderer.h"
#include "client/renderer/chunk/ChunkRenderDispatcher.h"
#include "client/renderer/chunk/CompiledChunk.h"
#include "client/renderer/chunk/RenderChunk.h"
#include "client/renderer/chunk/VisGraph.h"
#include "client/renderer/culling/Frustum.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/texture/TextureMap.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/renderer/vertex/VertexBuffer.h"
#include "client/settings/GameSettings.h"
#include "core/Java.h"
#include "core/JavaMath.h"
#include "core/StrictMath.h"
#include "entity/Entity.h"
#include "entity/player/EntityPlayer.h"
#include "init/Blocks.h"
#include "platform/Sys.h"
#include "render/gl/GL.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "util/ResourceLocation.h"
#include "world/WorldProvider.h"
#include "world/border/WorldBorder.h"
#include "world/chunk/Chunk.h"

#include <bit>
#include <cmath>
#include <deque>
#include <format>

namespace mc {

namespace {

const ResourceLocation& locationMoonPhasesPng()
{
    static const ResourceLocation location("textures/environment/moon_phases.png");
    return location;
}

const ResourceLocation& locationSunPng()
{
    static const ResourceLocation location("textures/environment/sun.png");
    return location;
}

const ResourceLocation& locationCloudsPng()
{
    static const ResourceLocation location("textures/environment/clouds.png");
    return location;
}

const ResourceLocation& locationEndSkyPng()
{
    static const ResourceLocation location("textures/environment/end_sky.png");
    return location;
}

const ResourceLocation& locationForcefieldPng()
{
    static const ResourceLocation location("textures/misc/forcefield.png");
    return location;
}

constexpr float kPi = 3.1415927f; // (float)Math.PI
constexpr double kPiD = 3.141592653589793;
// Java: Minecraft.renderChunksMany, which nothing changes.
constexpr bool kRenderChunksMany = true;

VertexFormat positionFormat()
{
    VertexFormat format;
    format.addElement(VertexFormatElement(0, VertexFormatElement::EnumType::FLOAT, VertexFormatElement::EnumUsage::POSITION, 3));
    return format;
}

const char* layerName(EnumWorldBlockLayer layer)
{
    switch (layer) {
    case EnumWorldBlockLayer::SOLID: return "SOLID";
    case EnumWorldBlockLayer::CUTOUT_MIPPED: return "CUTOUT_MIPPED";
    case EnumWorldBlockLayer::CUTOUT: return "CUTOUT";
    case EnumWorldBlockLayer::TRANSLUCENT: return "TRANSLUCENT";
    }
    return "";
}

} // namespace

RenderGlobal::RenderGlobal(Minecraft& mcIn)
    : mc(mcIn)
    , renderEngine(mcIn.getTextureManager())
    , renderDispatcher(std::make_unique<ChunkRenderDispatcher>())
    , renderContainer(std::make_unique<ChunkRenderContainer>())
{
    renderEngine.bindTexture(locationForcefieldPng());
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_WRAP_S, GL::GL_REPEAT);
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_WRAP_T, GL::GL_REPEAT);
    DxStateManager::bindTexture(0);
    updateDestroyBlockIcons();
    vboEnabled = mc.gameSettings->useVbo;
    generateStars();
    generateSky();
    generateSky2();
}

RenderGlobal::~RenderGlobal()
{
    if (viewFrustum) {
        stopChunkUpdates();
        viewFrustum->deleteGlResources();
    }
}

void RenderGlobal::onResourceManagerReload(const IResourceManager& /*resourceManager*/)
{
    updateDestroyBlockIcons();
}

void RenderGlobal::updateDestroyBlockIcons()
{
    TextureMap& texturemap = mc.getTextureMapBlocks();
    for (size_t i = 0; i < destroyBlockIcons.size(); ++i)
        destroyBlockIcons[i] = texturemap.getAtlasSprite("minecraft:blocks/destroy_stage_" + std::to_string(i));
}

void RenderGlobal::generateSky2()
{
    WorldRenderer& worldrenderer = Tessellator::getInstance().getWorldRenderer();
    sky2VBO = std::make_unique<VertexBuffer>(positionFormat());
    renderSky(worldrenderer, -16.0f, true);
    worldrenderer.finishDrawing();
    sky2VBO->bufferData(worldrenderer);
    worldrenderer.reset();
}

void RenderGlobal::generateSky()
{
    WorldRenderer& worldrenderer = Tessellator::getInstance().getWorldRenderer();
    skyVBO = std::make_unique<VertexBuffer>(positionFormat());
    renderSky(worldrenderer, 16.0f, false);
    worldrenderer.finishDrawing();
    skyVBO->bufferData(worldrenderer);
    worldrenderer.reset();
}

void RenderGlobal::renderSky(WorldRenderer& worldRendererIn, float posY, bool reverseX)
{
    worldRendererIn.begin(7, DefaultVertexFormats::POSITION);
    for (int k = -384; k <= 384; k += 64) {
        for (int l = -384; l <= 384; l += 64) {
            float f = static_cast<float>(k);
            float f1 = static_cast<float>(k + 64);
            if (reverseX) {
                f1 = static_cast<float>(k);
                f = static_cast<float>(k + 64);
            }
            worldRendererIn.pos(f, posY, l).endVertex();
            worldRendererIn.pos(f1, posY, l).endVertex();
            worldRendererIn.pos(f1, posY, l + 64).endVertex();
            worldRendererIn.pos(f, posY, l + 64).endVertex();
        }
    }
}

void RenderGlobal::generateStars()
{
    WorldRenderer& worldrenderer = Tessellator::getInstance().getWorldRenderer();
    starVBO = std::make_unique<VertexBuffer>(positionFormat());
    renderStars(worldrenderer);
    worldrenderer.finishDrawing();
    starVBO->bufferData(worldrenderer);
    worldrenderer.reset();
}

void RenderGlobal::renderStars(WorldRenderer& worldRendererIn)
{
    Random random(10842LL);
    worldRendererIn.begin(7, DefaultVertexFormats::POSITION);

    for (int i = 0; i < 1500; ++i) {
        double d0 = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
        double d1 = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
        double d2 = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
        const double d3 = static_cast<double>(0.15f + random.nextFloat() * 0.1f);
        double d4 = d0 * d0 + d1 * d1 + d2 * d2;

        if (d4 < 1.0 && d4 > 0.01) {
            d4 = 1.0 / std::sqrt(d4);
            d0 = d0 * d4;
            d1 = d1 * d4;
            d2 = d2 * d4;
            const double d5 = d0 * 100.0;
            const double d6 = d1 * 100.0;
            const double d7 = d2 * 100.0;
            const double d8 = StrictMath::atan2(d0, d2);
            const double d9 = java::mathSin(d8);
            const double d10 = java::mathCos(d8);
            const double d11 = StrictMath::atan2(std::sqrt(d0 * d0 + d2 * d2), d1);
            const double d12 = java::mathSin(d11);
            const double d13 = java::mathCos(d11);
            const double d14 = random.nextDouble() * kPiD * 2.0;
            const double d15 = java::mathSin(d14);
            const double d16 = java::mathCos(d14);

            for (int j = 0; j < 4; ++j) {
                const double d18 = static_cast<double>((j & 2) - 1) * d3;
                const double d19 = static_cast<double>(((j + 1) & 2) - 1) * d3;
                const double d21 = d18 * d16 - d19 * d15;
                const double d22 = d19 * d16 + d18 * d15;
                const double d23 = d21 * d12 + 0.0 * d13;
                const double d24 = 0.0 * d12 - d21 * d13;
                const double d25 = d24 * d9 - d22 * d10;
                const double d26 = d22 * d9 + d24 * d10;
                worldRendererIn.pos(d5 + d25, d6 + d23, d7 + d26).endVertex();
            }
        }
    }
}

void RenderGlobal::setWorldAndLoadRenderers(WorldClient* worldClientIn)
{
    if (theWorld != nullptr)
        theWorld->removeWorldAccess(this);

    frustumUpdatePosX = 4.9E-324;
    frustumUpdatePosY = 4.9E-324;
    frustumUpdatePosZ = 4.9E-324;
    frustumUpdatePosChunkX = INT32_MIN;
    frustumUpdatePosChunkY = INT32_MIN;
    frustumUpdatePosChunkZ = INT32_MIN;
    // Java: renderManager.set(worldClientIn) -- with the entity renderers (stage 10).
    theWorld = worldClientIn;

    if (worldClientIn != nullptr) {
        worldClientIn->addWorldAccess(this);
        loadRenderers();
    } else if (viewFrustum) {
        // Java keeps the old chunks until the next world; they point at the world being unloaded, so drop them.
        stopChunkUpdates();
        viewFrustum->deleteGlResources();
        viewFrustum.reset();
        renderInfos.clear();
        visibleFacingsCache.reset();
    }
}

void RenderGlobal::loadRenderers()
{
    if (theWorld == nullptr)
        return;

    displayListEntitiesDirty = true;
    Blocks::leaves->setGraphicsLevel(mc.gameSettings->fancyGraphics);
    Blocks::leaves2->setGraphicsLevel(mc.gameSettings->fancyGraphics);
    renderDistanceChunks = mc.gameSettings->renderDistanceChunks;
    // Java switches between VBOs and display lists here and rebuilds the sky; both draw the same, only the
    // last sky plane differs (renderSky).
    vboEnabled = mc.gameSettings->useVbo;

    if (viewFrustum)
        viewFrustum->deleteGlResources();

    stopChunkUpdates();
    renderInfos.clear();
    visibleFacingsCache.reset();
    viewFrustum = std::make_unique<ViewFrustum>(theWorld, mc.gameSettings->renderDistanceChunks, this);

    if (Entity* entity = mc.getRenderViewEntity())
        viewFrustum->updateChunkPositions(entity->posX, entity->posZ);

    renderEntitiesStartupCounter = 2;
}

void RenderGlobal::stopChunkUpdates()
{
    chunksToUpdate.clear();
    chunksToUpdateSet.clear();
    renderDispatcher->stopChunkUpdates();
}

void RenderGlobal::renderEntities(Entity* /*renderViewEntity*/, ICamera& /*camera*/, float /*partialTicks*/)
{
    if (renderEntitiesStartupCounter > 0) {
        --renderEntitiesStartupCounter;
        return;
    }
    // Java: the weather effects, entities and tile entities with their renderers (stage 10).
    countEntitiesTotal = 0;
    countEntitiesRendered = 0;
    countEntitiesHidden = 0;
}

std::string RenderGlobal::getDebugInfoRenders()
{
    const int i = viewFrustum ? static_cast<int>(viewFrustum->renderChunks.size()) : 0;
    int j = 0;
    for (const ContainerLocalRenderInformation& info : renderInfos) {
        const CompiledChunk* compiledchunk = info.renderChunk->getCompiledChunkOnClientThread();
        if (compiledchunk != CompiledChunk::DUMMY().get() && !compiledchunk->isEmpty())
            ++j;
    }
    return std::format("C: {}/{} {}D: {}, {}", j, i, kRenderChunksMany ? "(s) " : "", renderDistanceChunks, renderDispatcher->getDebugInfo());
}

std::string RenderGlobal::getDebugInfoEntities() const
{
    return "E: " + std::to_string(countEntitiesRendered) + "/" + std::to_string(countEntitiesTotal) + ", B: " + std::to_string(countEntitiesHidden)
        + ", I: " + std::to_string(countEntitiesTotal - countEntitiesHidden - countEntitiesRendered);
}

void RenderGlobal::setupTerrain(Entity* viewEntity, double partialTicks, ICamera& camera, int frameCount, bool playerSpectator)
{
    if (mc.gameSettings->renderDistanceChunks != renderDistanceChunks)
        loadRenderers();

    const double d0 = viewEntity->posX - frustumUpdatePosX;
    const double d1 = viewEntity->posY - frustumUpdatePosY;
    const double d2 = viewEntity->posZ - frustumUpdatePosZ;

    if (frustumUpdatePosChunkX != viewEntity->chunkCoordX || frustumUpdatePosChunkY != viewEntity->chunkCoordY || frustumUpdatePosChunkZ != viewEntity->chunkCoordZ
        || d0 * d0 + d1 * d1 + d2 * d2 > 16.0) {
        frustumUpdatePosX = viewEntity->posX;
        frustumUpdatePosY = viewEntity->posY;
        frustumUpdatePosZ = viewEntity->posZ;
        frustumUpdatePosChunkX = viewEntity->chunkCoordX;
        frustumUpdatePosChunkY = viewEntity->chunkCoordY;
        frustumUpdatePosChunkZ = viewEntity->chunkCoordZ;
        viewFrustum->updateChunkPositions(viewEntity->posX, viewEntity->posZ);
    }

    const double d3 = viewEntity->lastTickPosX + (viewEntity->posX - viewEntity->lastTickPosX) * partialTicks;
    const double d4 = viewEntity->lastTickPosY + (viewEntity->posY - viewEntity->lastTickPosY) * partialTicks;
    const double d5 = viewEntity->lastTickPosZ + (viewEntity->posZ - viewEntity->lastTickPosZ) * partialTicks;
    renderContainer->initialize(d3, d4, d5);

    const BlockPos blockpos1(d3, d4 + static_cast<double>(viewEntity->getEyeHeight()), d5);
    RenderChunk* renderchunk = viewFrustum->getRenderChunk(blockpos1);
    const BlockPos blockpos(MathHelper::floor_double(d3 / 16.0) * 16, MathHelper::floor_double(d4 / 16.0) * 16, MathHelper::floor_double(d5 / 16.0) * 16);
    displayListEntitiesDirty = displayListEntitiesDirty || !chunksToUpdate.empty() || viewEntity->posX != lastViewEntityX || viewEntity->posY != lastViewEntityY
        || viewEntity->posZ != lastViewEntityZ || static_cast<double>(viewEntity->rotationPitch) != lastViewEntityPitch
        || static_cast<double>(viewEntity->rotationYaw) != lastViewEntityYaw;
    lastViewEntityX = viewEntity->posX;
    lastViewEntityY = viewEntity->posY;
    lastViewEntityZ = viewEntity->posZ;
    lastViewEntityPitch = static_cast<double>(viewEntity->rotationPitch);
    lastViewEntityYaw = static_cast<double>(viewEntity->rotationYaw);

    if (displayListEntitiesDirty) {
        displayListEntitiesDirty = false;
        renderInfos.clear();
        // Java: an ArrayDeque used as a FIFO; a vector read from the front does the same.
        std::vector<ContainerLocalRenderInformation>& queue = visibilityQueue;
        queue.clear();
        size_t queueHead = 0;
        bool flag1 = kRenderChunksMany;

        if (renderchunk != nullptr) {
            bool flag2 = false;
            ContainerLocalRenderInformation start { renderchunk, std::nullopt, 0, 0 };
            uint8_t set1 = getVisibleFacings(blockpos1);

            if (std::popcount(static_cast<unsigned>(set1)) == 1) {
                const std::array<float, 3> vector3f = getViewVector(viewEntity, partialTicks);
                const EnumFacing enumfacing = getOpposite(getFacingFromVector(vector3f[0], vector3f[1], vector3f[2]));
                set1 = static_cast<uint8_t>(set1 & ~facingBit(enumfacing));
            }
            if (set1 == 0)
                flag2 = true;

            if (flag2 && !playerSpectator) {
                renderInfos.push_back(start);
            } else {
                if (playerSpectator && theWorld->getBlockState(blockpos1)->getBlock()->isOpaqueCube())
                    flag1 = false;
                renderchunk->setFrameIndex(frameCount);
                queue.push_back(start);
            }
        } else {
            const int i = blockpos1.getY() > 0 ? 248 : 8;
            for (int j = -renderDistanceChunks; j <= renderDistanceChunks; ++j) {
                for (int k = -renderDistanceChunks; k <= renderDistanceChunks; ++k) {
                    RenderChunk* renderchunk1 = viewFrustum->getRenderChunk(BlockPos((j << 4) + 8, i, (k << 4) + 8));
                    if (renderchunk1 != nullptr && camera.isBoundingBoxInFrustum(renderchunk1->boundingBox)) {
                        renderchunk1->setFrameIndex(frameCount);
                        queue.push_back(ContainerLocalRenderInformation { renderchunk1, std::nullopt, 0, 0 });
                    }
                }
            }
        }

        while (queueHead < queue.size()) {
            const ContainerLocalRenderInformation info1 = queue[queueHead++];
            RenderChunk* renderchunk3 = info1.renderChunk;
            renderInfos.push_back(info1);
            const CompiledChunk* compiled = renderchunk3->getCompiledChunkOnClientThread();

            for (EnumFacing enumfacing1 : kFacings) {
                RenderChunk* renderchunk2 = getRenderChunkOffset(blockpos, *renderchunk3, enumfacing1);
                if ((!flag1 || !(info1.setFacing & facingBit(getOpposite(enumfacing1))))
                    && (!flag1 || !info1.facing || compiled->isVisible(getOpposite(*info1.facing), enumfacing1)) && renderchunk2 != nullptr
                    && renderchunk2->setFrameIndex(frameCount) && camera.isBoundingBoxInFrustum(renderchunk2->boundingBox)) {
                    ContainerLocalRenderInformation info { renderchunk2, enumfacing1, static_cast<uint8_t>(info1.setFacing | facingBit(enumfacing1)), info1.counter + 1 };
                    queue.push_back(info);
                }
            }
        }
    }

    renderDispatcher->clearChunkUpdates();
    const std::vector<RenderChunk*> set = std::move(chunksToUpdate);
    const std::unordered_set<RenderChunk*> setLookup = std::move(chunksToUpdateSet);
    chunksToUpdate.clear();
    chunksToUpdateSet.clear();

    for (const ContainerLocalRenderInformation& info2 : renderInfos) {
        RenderChunk* renderchunk4 = info2.renderChunk;
        if (renderchunk4->isNeedsUpdate() || setLookup.contains(renderchunk4)) {
            displayListEntitiesDirty = true;
            if (isPositionInRenderChunk(blockpos, *renderchunk4)) {
                renderDispatcher->updateChunkNow(renderchunk4);
                renderchunk4->setNeedsUpdate(false);
            } else if (chunksToUpdateSet.insert(renderchunk4).second) {
                chunksToUpdate.push_back(renderchunk4);
            }
        }
    }

    for (RenderChunk* chunk : set) {
        if (chunksToUpdateSet.insert(chunk).second)
            chunksToUpdate.push_back(chunk);
    }
}

bool RenderGlobal::isPositionInRenderChunk(const BlockPos& pos, const RenderChunk& renderChunkIn)
{
    const BlockPos& blockpos = renderChunkIn.getPosition();
    return MathHelper::abs_int(pos.getX() - blockpos.getX()) > 16
        ? false
        : (MathHelper::abs_int(pos.getY() - blockpos.getY()) > 16 ? false : MathHelper::abs_int(pos.getZ() - blockpos.getZ()) <= 16);
}

uint8_t RenderGlobal::getVisibleFacings(const BlockPos& pos)
{
    // The flood fill over the camera's section is redone only when the camera moves to another block or the
    // section was rebuilt (a block change marks it for an update, which the camera's section gets at once).
    RenderChunk* section = viewFrustum->getRenderChunk(pos);
    const CompiledChunk* compiled = section ? section->getCompiledChunkOnClientThread() : nullptr;
    if (visibleFacingsCache && visibleFacingsCache->pos == pos && visibleFacingsCache->section == section && visibleFacingsCache->compiled == compiled
        && section && !section->isNeedsUpdate() && section->getPosition() == BlockPos(pos.getX() >> 4 << 4, pos.getY() >> 4 << 4, pos.getZ() >> 4 << 4))
        return visibleFacingsCache->facings;
    const uint8_t facings = computeVisibleFacings(pos);
    visibleFacingsCache = VisibleFacingsCache { pos, section, compiled, facings };
    return facings;
}

uint8_t RenderGlobal::computeVisibleFacings(const BlockPos& pos)
{
    VisGraph visgraph;
    const BlockPos blockpos(pos.getX() >> 4 << 4, pos.getY() >> 4 << 4, pos.getZ() >> 4 << 4);
    Chunk* chunk = theWorld->getChunkFromBlockCoords(blockpos);

    for (const BlockPos& p : BlockPos::getAllInBox(blockpos, blockpos.add(15, 15, 15))) {
        if (chunk->getBlockState(p)->getBlock()->isOpaqueCube())
            visgraph.setOpaque(p);
    }
    return visgraph.getVisibleFacings(pos);
}

RenderChunk* RenderGlobal::getRenderChunkOffset(const BlockPos& playerPos, const RenderChunk& renderChunkBase, EnumFacing facing)
{
    const BlockPos& blockpos = renderChunkBase.getBlockPosOffset16(facing);
    if (MathHelper::abs_int(playerPos.getX() - blockpos.getX()) > renderDistanceChunks * 16)
        return nullptr;
    if (blockpos.getY() < 0 || blockpos.getY() >= 256)
        return nullptr;
    if (MathHelper::abs_int(playerPos.getZ() - blockpos.getZ()) > renderDistanceChunks * 16)
        return nullptr;
    if (!renderChunkBase.hasCachedNeighbor(facing))
        renderChunkBase.setCachedNeighbor(facing, viewFrustum->getRenderChunk(blockpos));
    return renderChunkBase.cachedNeighbor(facing);
}

std::array<float, 3> RenderGlobal::getViewVector(const Entity* entityIn, double partialTicks)
{
    float f = static_cast<float>(static_cast<double>(entityIn->prevRotationPitch) + static_cast<double>(entityIn->rotationPitch - entityIn->prevRotationPitch) * partialTicks);
    const float f1 = static_cast<float>(static_cast<double>(entityIn->prevRotationYaw) + static_cast<double>(entityIn->rotationYaw - entityIn->prevRotationYaw) * partialTicks);

    if (Minecraft::getMinecraft()->gameSettings->thirdPersonView == 2)
        f += 180.0f;

    const float f2 = MathHelper::cos(-f1 * 0.017453292f - kPi);
    const float f3 = MathHelper::sin(-f1 * 0.017453292f - kPi);
    const float f4 = -MathHelper::cos(-f * 0.017453292f);
    const float f5 = MathHelper::sin(-f * 0.017453292f);
    return { f3 * f4, f5, f2 * f4 };
}

int RenderGlobal::renderBlockLayer(EnumWorldBlockLayer blockLayerIn, double /*partialTicks*/, int /*pass*/, Entity* entityIn)
{
    RenderHelper::disableStandardItemLighting();

    if (blockLayerIn == EnumWorldBlockLayer::TRANSLUCENT) {
        const double d0 = entityIn->posX - prevRenderSortX;
        const double d1 = entityIn->posY - prevRenderSortY;
        const double d2 = entityIn->posZ - prevRenderSortZ;

        if (d0 * d0 + d1 * d1 + d2 * d2 > 1.0) {
            prevRenderSortX = entityIn->posX;
            prevRenderSortY = entityIn->posY;
            prevRenderSortZ = entityIn->posZ;
            int k = 0;
            for (const ContainerLocalRenderInformation& info : renderInfos) {
                if (info.renderChunk->getCompiledChunkOnClientThread()->isLayerStarted(blockLayerIn) && k++ < 15)
                    renderDispatcher->updateTransparencyLater(info.renderChunk);
            }
        }
    }

    int l = 0;
    const bool flag = blockLayerIn == EnumWorldBlockLayer::TRANSLUCENT;
    const int i1 = flag ? static_cast<int>(renderInfos.size()) - 1 : 0;
    const int i = flag ? -1 : static_cast<int>(renderInfos.size());
    const int j1 = flag ? -1 : 1;

    for (int j = i1; j != i; j += j1) {
        RenderChunk* renderchunk = renderInfos[static_cast<size_t>(j)].renderChunk;
        if (!renderchunk->getCompiledChunkOnClientThread()->isLayerEmpty(blockLayerIn)) {
            ++l;
            renderContainer->addRenderChunk(renderchunk, blockLayerIn);
        }
    }

    (void)layerName; // Java: the profiler section "render_" + layer
    renderBlockLayer(blockLayerIn);
    return l;
}

void RenderGlobal::renderBlockLayer(EnumWorldBlockLayer blockLayerIn)
{
    mc.entityRenderer->enableLightmap();
    renderContainer->renderChunkLayer(blockLayerIn);
    // Java: disabling the colour array resets the colour cache (VBO path).
    if (vboEnabled)
        DxStateManager::resetColor();
    mc.entityRenderer->disableLightmap();
}

void RenderGlobal::cleanupDamagedBlocks()
{
    std::vector<int32_t> expired;
    for (const auto& [id, progress] : damagedBlocks) {
        if (cloudTickCounter - progress.getCreationCloudUpdateTick() > 400)
            expired.push_back(id);
    }
    for (int32_t id : expired)
        damagedBlocks.erase(id);
}

void RenderGlobal::updateClouds()
{
    ++cloudTickCounter;
    if (cloudTickCounter % 20 == 0)
        cleanupDamagedBlocks();
}

void RenderGlobal::renderSkyEnd()
{
    DxStateManager::disableFog();
    DxStateManager::disableAlpha();
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    RenderHelper::disableStandardItemLighting();
    DxStateManager::depthMask(false);
    renderEngine.bindTexture(locationEndSkyPng());
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();

    for (int i = 0; i < 6; ++i) {
        DxStateManager::pushMatrix();
        if (i == 1)
            DxStateManager::rotate(90.0f, 1.0f, 0.0f, 0.0f);
        if (i == 2)
            DxStateManager::rotate(-90.0f, 1.0f, 0.0f, 0.0f);
        if (i == 3)
            DxStateManager::rotate(180.0f, 1.0f, 0.0f, 0.0f);
        if (i == 4)
            DxStateManager::rotate(90.0f, 0.0f, 0.0f, 1.0f);
        if (i == 5)
            DxStateManager::rotate(-90.0f, 0.0f, 0.0f, 1.0f);

        worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
        worldrenderer.pos(-100.0, -100.0, -100.0).tex(0.0, 0.0).color(40, 40, 40, 255).endVertex();
        worldrenderer.pos(-100.0, -100.0, 100.0).tex(0.0, 16.0).color(40, 40, 40, 255).endVertex();
        worldrenderer.pos(100.0, -100.0, 100.0).tex(16.0, 16.0).color(40, 40, 40, 255).endVertex();
        worldrenderer.pos(100.0, -100.0, -100.0).tex(16.0, 0.0).color(40, 40, 40, 255).endVertex();
        tessellator.draw();
        DxStateManager::popMatrix();
    }

    DxStateManager::depthMask(true);
    DxStateManager::enableTexture2D();
    DxStateManager::enableAlpha();
}

void RenderGlobal::renderSky(float partialTicks, int pass)
{
    if (mc.theWorld->provider->getDimensionId() == 1) {
        renderSkyEnd();
        return;
    }
    if (!mc.theWorld->provider->isSurfaceWorld())
        return;

    DxStateManager::disableTexture2D();
    const Vec3 vec3 = theWorld->getSkyColor(mc.getRenderViewEntity(), partialTicks);
    float f = static_cast<float>(vec3.xCoord);
    float f1 = static_cast<float>(vec3.yCoord);
    float f2 = static_cast<float>(vec3.zCoord);

    if (pass != 2) {
        const float f3 = (f * 30.0f + f1 * 59.0f + f2 * 11.0f) / 100.0f;
        const float f4 = (f * 30.0f + f1 * 70.0f) / 100.0f;
        const float f5 = (f * 30.0f + f2 * 70.0f) / 100.0f;
        f = f3;
        f1 = f4;
        f2 = f5;
    }

    DxStateManager::color(f, f1, f2);
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    DxStateManager::depthMask(false);
    DxStateManager::enableFog();
    DxStateManager::color(f, f1, f2);
    skyVBO->drawArrays(7);
    DxStateManager::disableFog();
    DxStateManager::disableAlpha();
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    RenderHelper::disableStandardItemLighting();
    const std::optional<std::array<float, 4>> afloat = theWorld->provider->calcSunriseSunsetColors(theWorld->getCelestialAngle(partialTicks), partialTicks);

    if (afloat) {
        DxStateManager::disableTexture2D();
        DxStateManager::shadeModel(7425);
        DxStateManager::pushMatrix();
        DxStateManager::rotate(90.0f, 1.0f, 0.0f, 0.0f);
        DxStateManager::rotate(MathHelper::sin(theWorld->getCelestialAngleRadians(partialTicks)) < 0.0f ? 180.0f : 0.0f, 0.0f, 0.0f, 1.0f);
        DxStateManager::rotate(90.0f, 0.0f, 0.0f, 1.0f);
        float f6 = (*afloat)[0];
        float f7 = (*afloat)[1];
        float f8 = (*afloat)[2];

        if (pass != 2) {
            const float f9 = (f6 * 30.0f + f7 * 59.0f + f8 * 11.0f) / 100.0f;
            const float f10 = (f6 * 30.0f + f7 * 70.0f) / 100.0f;
            const float f11 = (f6 * 30.0f + f8 * 70.0f) / 100.0f;
            f6 = f9;
            f7 = f10;
            f8 = f11;
        }

        worldrenderer.begin(6, DefaultVertexFormats::POSITION_COLOR);
        worldrenderer.pos(0.0, 100.0, 0.0).color(f6, f7, f8, (*afloat)[3]).endVertex();
        for (int l = 0; l <= 16; ++l) {
            const float f21 = static_cast<float>(l) * kPi * 2.0f / 16.0f;
            const float f12 = MathHelper::sin(f21);
            const float f13 = MathHelper::cos(f21);
            worldrenderer.pos(static_cast<double>(f12 * 120.0f), static_cast<double>(f13 * 120.0f), static_cast<double>(-f13 * 40.0f * (*afloat)[3]))
                .color((*afloat)[0], (*afloat)[1], (*afloat)[2], 0.0f)
                .endVertex();
        }
        tessellator.draw();
        DxStateManager::popMatrix();
        DxStateManager::shadeModel(7424);
    }

    DxStateManager::enableTexture2D();
    DxStateManager::tryBlendFuncSeparate(770, 1, 1, 0);
    DxStateManager::pushMatrix();
    const float f16 = 1.0f - theWorld->getRainStrength(partialTicks);
    DxStateManager::color(1.0f, 1.0f, 1.0f, f16);
    DxStateManager::rotate(-90.0f, 0.0f, 1.0f, 0.0f);
    DxStateManager::rotate(theWorld->getCelestialAngle(partialTicks) * 360.0f, 1.0f, 0.0f, 0.0f);
    float f17 = 30.0f;
    renderEngine.bindTexture(locationSunPng());
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(static_cast<double>(-f17), 100.0, static_cast<double>(-f17)).tex(0.0, 0.0).endVertex();
    worldrenderer.pos(static_cast<double>(f17), 100.0, static_cast<double>(-f17)).tex(1.0, 0.0).endVertex();
    worldrenderer.pos(static_cast<double>(f17), 100.0, static_cast<double>(f17)).tex(1.0, 1.0).endVertex();
    worldrenderer.pos(static_cast<double>(-f17), 100.0, static_cast<double>(f17)).tex(0.0, 1.0).endVertex();
    tessellator.draw();
    f17 = 20.0f;
    renderEngine.bindTexture(locationMoonPhasesPng());
    const int i = theWorld->getMoonPhase();
    const int k = i % 4;
    const int i1 = i / 4 % 2;
    const float f22 = static_cast<float>(k + 0) / 4.0f;
    const float f23 = static_cast<float>(i1 + 0) / 2.0f;
    const float f24 = static_cast<float>(k + 1) / 4.0f;
    const float f14 = static_cast<float>(i1 + 1) / 2.0f;
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(static_cast<double>(-f17), -100.0, static_cast<double>(f17)).tex(f24, f14).endVertex();
    worldrenderer.pos(static_cast<double>(f17), -100.0, static_cast<double>(f17)).tex(f22, f14).endVertex();
    worldrenderer.pos(static_cast<double>(f17), -100.0, static_cast<double>(-f17)).tex(f22, f23).endVertex();
    worldrenderer.pos(static_cast<double>(-f17), -100.0, static_cast<double>(-f17)).tex(f24, f23).endVertex();
    tessellator.draw();
    DxStateManager::disableTexture2D();
    const float f15 = theWorld->getStarBrightness(partialTicks) * f16;

    if (f15 > 0.0f) {
        DxStateManager::color(f15, f15, f15, f15);
        starVBO->drawArrays(7);
    }

    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    DxStateManager::disableBlend();
    DxStateManager::enableAlpha();
    DxStateManager::enableFog();
    DxStateManager::popMatrix();
    DxStateManager::disableTexture2D();
    DxStateManager::color(0.0f, 0.0f, 0.0f);
    const double d0 = mc.thePlayer->getPositionEyes(partialTicks).yCoord - theWorld->getHorizon();

    if (d0 < 0.0) {
        DxStateManager::pushMatrix();
        DxStateManager::translate(0.0f, 12.0f, 0.0f);
        sky2VBO->drawArrays(7);
        DxStateManager::popMatrix();
        const float f19 = -static_cast<float>(d0 + 65.0);
        worldrenderer.begin(7, DefaultVertexFormats::POSITION_COLOR);
        worldrenderer.pos(-1.0, static_cast<double>(f19), 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, static_cast<double>(f19), 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, -1.0, 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(-1.0, -1.0, 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(-1.0, -1.0, -1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, -1.0, -1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, static_cast<double>(f19), -1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(-1.0, static_cast<double>(f19), -1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, -1.0, -1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, -1.0, 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, static_cast<double>(f19), 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, static_cast<double>(f19), -1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(-1.0, static_cast<double>(f19), -1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(-1.0, static_cast<double>(f19), 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(-1.0, -1.0, 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(-1.0, -1.0, -1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(-1.0, -1.0, -1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(-1.0, -1.0, 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, -1.0, 1.0).color(0, 0, 0, 255).endVertex();
        worldrenderer.pos(1.0, -1.0, -1.0).color(0, 0, 0, 255).endVertex();
        tessellator.draw();
    }

    if (theWorld->provider->isSkyColored())
        DxStateManager::color(f * 0.2f + 0.04f, f1 * 0.2f + 0.04f, f2 * 0.6f + 0.1f);
    else
        DxStateManager::color(f, f1, f2);

    DxStateManager::pushMatrix();
    DxStateManager::translate(0.0f, -static_cast<float>(d0 - 16.0), 0.0f);
    // Java: GlStateManager.callList(glSkyList2), which with VBOs on is list -1 and draws nothing.
    if (!vboEnabled)
        sky2VBO->drawArrays(7);
    DxStateManager::popMatrix();
    DxStateManager::enableTexture2D();
    DxStateManager::depthMask(true);
}

void RenderGlobal::renderClouds(float partialTicks, int pass)
{
    if (!mc.theWorld->provider->isSurfaceWorld())
        return;

    if (mc.gameSettings->shouldRenderClouds() == 2) {
        renderCloudsFancy(partialTicks, pass);
        return;
    }

    DxStateManager::disableCull();
    Entity* view = mc.getRenderViewEntity();
    const float f = static_cast<float>(view->lastTickPosY + (view->posY - view->lastTickPosY) * static_cast<double>(partialTicks));
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    renderEngine.bindTexture(locationCloudsPng());
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    const Vec3 vec3 = theWorld->getCloudColour(partialTicks);
    float f1 = static_cast<float>(vec3.xCoord);
    float f2 = static_cast<float>(vec3.yCoord);
    float f3 = static_cast<float>(vec3.zCoord);

    if (pass != 2) {
        const float f4 = (f1 * 30.0f + f2 * 59.0f + f3 * 11.0f) / 100.0f;
        const float f5 = (f1 * 30.0f + f2 * 70.0f) / 100.0f;
        const float f6 = (f1 * 30.0f + f3 * 70.0f) / 100.0f;
        f1 = f4;
        f2 = f5;
        f3 = f6;
    }

    const double d2 = static_cast<double>(static_cast<float>(cloudTickCounter) + partialTicks);
    double d0 = view->prevPosX + (view->posX - view->prevPosX) * static_cast<double>(partialTicks) + d2 * 0.029999999329447746;
    double d1 = view->prevPosZ + (view->posZ - view->prevPosZ) * static_cast<double>(partialTicks);
    const int k = MathHelper::floor_double(d0 / 2048.0);
    const int l = MathHelper::floor_double(d1 / 2048.0);
    d0 = d0 - static_cast<double>(k * 2048);
    d1 = d1 - static_cast<double>(l * 2048);
    const float f7 = theWorld->provider->getCloudHeight() - f + 0.33f;
    const float f8 = static_cast<float>(d0 * 4.8828125E-4);
    const float f9 = static_cast<float>(d1 * 4.8828125E-4);
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);

    for (int i1 = -256; i1 < 256; i1 += 32) {
        for (int j1 = -256; j1 < 256; j1 += 32) {
            worldrenderer.pos(i1 + 0, f7, j1 + 32)
                .tex(static_cast<float>(i1 + 0) * 4.8828125E-4f + f8, static_cast<float>(j1 + 32) * 4.8828125E-4f + f9)
                .color(f1, f2, f3, 0.8f)
                .endVertex();
            worldrenderer.pos(i1 + 32, f7, j1 + 32)
                .tex(static_cast<float>(i1 + 32) * 4.8828125E-4f + f8, static_cast<float>(j1 + 32) * 4.8828125E-4f + f9)
                .color(f1, f2, f3, 0.8f)
                .endVertex();
            worldrenderer.pos(i1 + 32, f7, j1 + 0)
                .tex(static_cast<float>(i1 + 32) * 4.8828125E-4f + f8, static_cast<float>(j1 + 0) * 4.8828125E-4f + f9)
                .color(f1, f2, f3, 0.8f)
                .endVertex();
            worldrenderer.pos(i1 + 0, f7, j1 + 0)
                .tex(static_cast<float>(i1 + 0) * 4.8828125E-4f + f8, static_cast<float>(j1 + 0) * 4.8828125E-4f + f9)
                .color(f1, f2, f3, 0.8f)
                .endVertex();
        }
    }

    tessellator.draw();
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    DxStateManager::disableBlend();
    DxStateManager::enableCull();
}

void RenderGlobal::renderCloudsFancy(float partialTicks, int pass)
{
    DxStateManager::disableCull();
    Entity* view = mc.getRenderViewEntity();
    const float f = static_cast<float>(view->lastTickPosY + (view->posY - view->lastTickPosY) * static_cast<double>(partialTicks));
    const double d0 = static_cast<double>(static_cast<float>(cloudTickCounter) + partialTicks);
    double d1 = (view->prevPosX + (view->posX - view->prevPosX) * static_cast<double>(partialTicks) + d0 * 0.029999999329447746) / 12.0;
    double d2 = (view->prevPosZ + (view->posZ - view->prevPosZ) * static_cast<double>(partialTicks)) / 12.0 + 0.33000001311302185;
    const float f3 = theWorld->provider->getCloudHeight() - f + 0.33f;
    const int i = MathHelper::floor_double(d1 / 2048.0);
    const int j = MathHelper::floor_double(d2 / 2048.0);
    d1 = d1 - static_cast<double>(i * 2048);
    d2 = d2 - static_cast<double>(j * 2048);
    renderEngine.bindTexture(locationCloudsPng());
    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    const Vec3 vec3 = theWorld->getCloudColour(partialTicks);
    float f4 = static_cast<float>(vec3.xCoord);
    float f5 = static_cast<float>(vec3.yCoord);
    float f6 = static_cast<float>(vec3.zCoord);

    if (pass != 2) {
        const float f7 = (f4 * 30.0f + f5 * 59.0f + f6 * 11.0f) / 100.0f;
        const float f8 = (f4 * 30.0f + f5 * 70.0f) / 100.0f;
        const float f9 = (f4 * 30.0f + f6 * 70.0f) / 100.0f;
        f4 = f7;
        f5 = f8;
        f6 = f9;
    }

    const float f26 = f4 * 0.9f;
    const float f27 = f5 * 0.9f;
    const float f28 = f6 * 0.9f;
    const float f10 = f4 * 0.7f;
    const float f11 = f5 * 0.7f;
    const float f12 = f6 * 0.7f;
    const float f13 = f4 * 0.8f;
    const float f14 = f5 * 0.8f;
    const float f15 = f6 * 0.8f;
    const float f17 = static_cast<float>(MathHelper::floor_double(d1)) * 0.00390625f;
    const float f18 = static_cast<float>(MathHelper::floor_double(d2)) * 0.00390625f;
    const float f19 = static_cast<float>(d1 - static_cast<double>(MathHelper::floor_double(d1)));
    const float f20 = static_cast<float>(d2 - static_cast<double>(MathHelper::floor_double(d2)));
    const float f21 = 9.765625E-4f;
    DxStateManager::scale(12.0f, 1.0f, 12.0f);

    const auto u = [&](float x) { return static_cast<double>(x * 0.00390625f + f17); };
    const auto v = [&](float z) { return static_cast<double>(z * 0.00390625f + f18); };

    // The vertices go straight to GLVertex with the values WorldRenderer and the attribute fetch would give for
    // POSITION_TEX_COLOR_NORMAL (colour bytes, byte normals), and both passes draw the same ones: built once.
    const auto quantizeColor = [](float r, float g, float b, float a) {
        const auto q = [](float c) { return static_cast<float>(static_cast<uint8_t>(java::f2i(c * 255.0f))) / 255.0f; };
        return std::array<float, 4> { q(r), q(g), q(b), q(a) };
    };
    const auto quantizeNormal = [](float x, float y, float z) {
        const auto q = [](float n) {
            const auto b = static_cast<int8_t>(java::mul(java::f2i(n), 127) & 255);
            return (2.0f * static_cast<float>(b) + 1.0f) / 255.0f;
        };
        return std::array<float, 3> { q(x), q(y), q(z) };
    };
    const std::array<float, 4> colorBottom = quantizeColor(f10, f11, f12, 0.8f);
    const std::array<float, 4> colorTop = quantizeColor(f4, f5, f6, 0.8f);
    const std::array<float, 4> colorX = quantizeColor(f26, f27, f28, 0.8f);
    const std::array<float, 4> colorZ = quantizeColor(f13, f14, f15, 0.8f);
    const std::array<float, 3> normalDown = quantizeNormal(0.0f, -1.0f, 0.0f);
    const std::array<float, 3> normalUp = quantizeNormal(0.0f, 1.0f, 0.0f);
    const std::array<float, 3> normalWest = quantizeNormal(-1.0f, 0.0f, 0.0f);
    const std::array<float, 3> normalEast = quantizeNormal(1.0f, 0.0f, 0.0f);
    const std::array<float, 3> normalNorth = quantizeNormal(0.0f, 0.0f, -1.0f);
    const std::array<float, 3> normalSouth = quantizeNormal(0.0f, 0.0f, 1.0f);
    const float* lightmap = GL::currentTexCoord(1);
    const auto vertex = [&](float x, float y, float z, double tu, double tv, const std::array<float, 4>& c, const std::array<float, 3>& n) {
        GLVertex& out = cloudVertices.emplace_back();
        out.x = x;
        out.y = y;
        out.z = z;
        out.r = c[0];
        out.g = c[1];
        out.b = c[2];
        out.a = c[3];
        out.u0 = static_cast<float>(tu);
        out.v0 = static_cast<float>(tv);
        out.u1 = lightmap[0];
        out.v1 = lightmap[1];
        out.nx = n[0];
        out.ny = n[1];
        out.nz = n[2];
    };

    cloudVertices.clear();
    for (int j1 = -3; j1 <= 4; ++j1) {
        for (int k1 = -3; k1 <= 4; ++k1) {
            const float f22 = static_cast<float>(j1 * 8);
            const float f23 = static_cast<float>(k1 * 8);
            const float f24 = f22 - f19;
            const float f25 = f23 - f20;

            if (f3 > -5.0f) {
                vertex(f24 + 0.0f, f3 + 0.0f, f25 + 8.0f, u(f22 + 0.0f), v(f23 + 8.0f), colorBottom, normalDown);
                vertex(f24 + 8.0f, f3 + 0.0f, f25 + 8.0f, u(f22 + 8.0f), v(f23 + 8.0f), colorBottom, normalDown);
                vertex(f24 + 8.0f, f3 + 0.0f, f25 + 0.0f, u(f22 + 8.0f), v(f23 + 0.0f), colorBottom, normalDown);
                vertex(f24 + 0.0f, f3 + 0.0f, f25 + 0.0f, u(f22 + 0.0f), v(f23 + 0.0f), colorBottom, normalDown);
            }

            if (f3 <= 5.0f) {
                const float top = f3 + 4.0f - f21;
                vertex(f24 + 0.0f, top, f25 + 8.0f, u(f22 + 0.0f), v(f23 + 8.0f), colorTop, normalUp);
                vertex(f24 + 8.0f, top, f25 + 8.0f, u(f22 + 8.0f), v(f23 + 8.0f), colorTop, normalUp);
                vertex(f24 + 8.0f, top, f25 + 0.0f, u(f22 + 8.0f), v(f23 + 0.0f), colorTop, normalUp);
                vertex(f24 + 0.0f, top, f25 + 0.0f, u(f22 + 0.0f), v(f23 + 0.0f), colorTop, normalUp);
            }

            if (j1 > -1) {
                for (int l1 = 0; l1 < 8; ++l1) {
                    const float x = f24 + static_cast<float>(l1) + 0.0f;
                    const double tu = u(f22 + static_cast<float>(l1) + 0.5f);
                    vertex(x, f3 + 0.0f, f25 + 8.0f, tu, v(f23 + 8.0f), colorX, normalWest);
                    vertex(x, f3 + 4.0f, f25 + 8.0f, tu, v(f23 + 8.0f), colorX, normalWest);
                    vertex(x, f3 + 4.0f, f25 + 0.0f, tu, v(f23 + 0.0f), colorX, normalWest);
                    vertex(x, f3 + 0.0f, f25 + 0.0f, tu, v(f23 + 0.0f), colorX, normalWest);
                }
            }

            if (j1 <= 1) {
                for (int i2 = 0; i2 < 8; ++i2) {
                    const float x = f24 + static_cast<float>(i2) + 1.0f - f21;
                    const double tu = u(f22 + static_cast<float>(i2) + 0.5f);
                    vertex(x, f3 + 0.0f, f25 + 8.0f, tu, v(f23 + 8.0f), colorX, normalEast);
                    vertex(x, f3 + 4.0f, f25 + 8.0f, tu, v(f23 + 8.0f), colorX, normalEast);
                    vertex(x, f3 + 4.0f, f25 + 0.0f, tu, v(f23 + 0.0f), colorX, normalEast);
                    vertex(x, f3 + 0.0f, f25 + 0.0f, tu, v(f23 + 0.0f), colorX, normalEast);
                }
            }

            if (k1 > -1) {
                for (int j2 = 0; j2 < 8; ++j2) {
                    const float z = f25 + static_cast<float>(j2) + 0.0f;
                    const double tv = v(f23 + static_cast<float>(j2) + 0.5f);
                    vertex(f24 + 0.0f, f3 + 4.0f, z, u(f22 + 0.0f), tv, colorZ, normalNorth);
                    vertex(f24 + 8.0f, f3 + 4.0f, z, u(f22 + 8.0f), tv, colorZ, normalNorth);
                    vertex(f24 + 8.0f, f3 + 0.0f, z, u(f22 + 8.0f), tv, colorZ, normalNorth);
                    vertex(f24 + 0.0f, f3 + 0.0f, z, u(f22 + 0.0f), tv, colorZ, normalNorth);
                }
            }

            if (k1 <= 1) {
                for (int k2 = 0; k2 < 8; ++k2) {
                    const float z = f25 + static_cast<float>(k2) + 1.0f - f21;
                    const double tv = v(f23 + static_cast<float>(k2) + 0.5f);
                    vertex(f24 + 0.0f, f3 + 4.0f, z, u(f22 + 0.0f), tv, colorZ, normalSouth);
                    vertex(f24 + 8.0f, f3 + 4.0f, z, u(f22 + 8.0f), tv, colorZ, normalSouth);
                    vertex(f24 + 8.0f, f3 + 0.0f, z, u(f22 + 8.0f), tv, colorZ, normalSouth);
                    vertex(f24 + 0.0f, f3 + 0.0f, z, u(f22 + 0.0f), tv, colorZ, normalSouth);
                }
            }
        }
    }

    for (int i1 = 0; i1 < 2; ++i1) {
        if (i1 == 0) {
            DxStateManager::colorMask(false, false, false, false);
        } else {
            switch (pass) {
            case 0: DxStateManager::colorMask(false, true, true, true); break;
            case 1: DxStateManager::colorMask(true, false, false, true); break;
            case 2: DxStateManager::colorMask(true, true, true, true); break;
            default: break;
            }
        }
        // Java: a tessellator draw per cell, in this order; the cells' quads together are the same draw.
        if (!cloudVertices.empty()) {
            GL::drawVertices(7, cloudVertices);
            DxStateManager::resetColor();
        }
    }

    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    DxStateManager::disableBlend();
    DxStateManager::enableCull();
}

void RenderGlobal::updateChunks(int64_t finishTimeNano)
{
    displayListEntitiesDirty |= renderDispatcher->runChunkUploads(finishTimeNano);

    size_t processed = 0;
    for (; processed < chunksToUpdate.size(); ++processed) {
        RenderChunk* renderchunk = chunksToUpdate[processed];
        if (!renderDispatcher->updateChunkLater(renderchunk))
            break;
        renderchunk->setNeedsUpdate(false);
        chunksToUpdateSet.erase(renderchunk);
        if (finishTimeNano - Sys::nanoTime() < 0) {
            ++processed;
            break;
        }
    }
    chunksToUpdate.erase(chunksToUpdate.begin(), chunksToUpdate.begin() + static_cast<std::ptrdiff_t>(processed));
}

void RenderGlobal::preRenderDamagedBlocks()
{
    DxStateManager::tryBlendFuncSeparate(774, 768, 1, 0);
    DxStateManager::enableBlend();
    DxStateManager::color(1.0f, 1.0f, 1.0f, 0.5f);
    DxStateManager::doPolygonOffset(-3.0f, -3.0f);
    DxStateManager::enablePolygonOffset();
    DxStateManager::alphaFunc(516, 0.1f);
    DxStateManager::enableAlpha();
    DxStateManager::pushMatrix();
}

void RenderGlobal::postRenderDamagedBlocks()
{
    DxStateManager::disableAlpha();
    DxStateManager::doPolygonOffset(0.0f, 0.0f);
    DxStateManager::disablePolygonOffset();
    DxStateManager::enableAlpha();
    DxStateManager::depthMask(true);
    DxStateManager::popMatrix();
}

void RenderGlobal::drawBlockDamageTexture(Tessellator& tessellatorIn, WorldRenderer& worldRendererIn, Entity* entityIn, float partialTicks)
{
    const double d0 = entityIn->lastTickPosX + (entityIn->posX - entityIn->lastTickPosX) * static_cast<double>(partialTicks);
    const double d1 = entityIn->lastTickPosY + (entityIn->posY - entityIn->lastTickPosY) * static_cast<double>(partialTicks);
    const double d2 = entityIn->lastTickPosZ + (entityIn->posZ - entityIn->lastTickPosZ) * static_cast<double>(partialTicks);

    if (damagedBlocks.empty())
        return;

    renderEngine.bindTexture(TextureMap::locationBlocksTexture);
    preRenderDamagedBlocks();
    worldRendererIn.begin(7, DefaultVertexFormats::BLOCK);
    worldRendererIn.setTranslation(-d0, -d1, -d2);
    worldRendererIn.noColor();
    std::vector<int32_t> removed;

    for (const auto& [id, destroyblockprogress] : damagedBlocks) {
        const BlockPos& blockpos = destroyblockprogress.getPosition();
        const double d3 = static_cast<double>(blockpos.getX()) - d0;
        const double d4 = static_cast<double>(blockpos.getY()) - d1;
        const double d5 = static_cast<double>(blockpos.getZ()) - d2;
        Block* block = theWorld->getBlockState(blockpos)->getBlock();

        if (!dynamic_cast<BlockChest*>(block) && !dynamic_cast<BlockEnderChest*>(block) && !dynamic_cast<BlockSign*>(block) && !dynamic_cast<BlockSkull*>(block)) {
            if (d3 * d3 + d4 * d4 + d5 * d5 > 1024.0) {
                removed.push_back(id);
            } else {
                const IBlockState* iblockstate = theWorld->getBlockState(blockpos);
                if (&iblockstate->getBlock()->getMaterial() != &Material::air) {
                    const int i = destroyblockprogress.getPartialBlockDamage();
                    const TextureAtlasSprite* textureatlassprite = destroyBlockIcons[static_cast<size_t>(i)];
                    mc.getBlockRendererDispatcher().renderBlockDamage(iblockstate, blockpos, *textureatlassprite, theWorld);
                }
            }
        }
    }
    for (int32_t id : removed)
        damagedBlocks.erase(id);

    tessellatorIn.draw();
    worldRendererIn.setTranslation(0.0, 0.0, 0.0);
    postRenderDamagedBlocks();
}

void RenderGlobal::drawSelectionBox(EntityPlayer* player, const MovingObjectPosition& movingObjectPositionIn, int execute, float partialTicks)
{
    if (execute != 0 || movingObjectPositionIn.typeOfHit != MovingObjectPosition::MovingObjectType::BLOCK)
        return;

    DxStateManager::enableBlend();
    DxStateManager::tryBlendFuncSeparate(770, 771, 1, 0);
    DxStateManager::color(0.0f, 0.0f, 0.0f, 0.4f);
    GL::glLineWidth(2.0f);
    DxStateManager::disableTexture2D();
    DxStateManager::depthMask(false);
    const BlockPos& blockpos = *movingObjectPositionIn.getBlockPos();
    Block* block = theWorld->getBlockState(blockpos)->getBlock();

    if (&block->getMaterial() != &Material::air && theWorld->getWorldBorder().contains(blockpos)) {
        block->setBlockBoundsBasedOnState(theWorld, blockpos);
        const double d0 = player->lastTickPosX + (player->posX - player->lastTickPosX) * static_cast<double>(partialTicks);
        const double d1 = player->lastTickPosY + (player->posY - player->lastTickPosY) * static_cast<double>(partialTicks);
        const double d2 = player->lastTickPosZ + (player->posZ - player->lastTickPosZ) * static_cast<double>(partialTicks);
        drawSelectionBoundingBox(block->getSelectedBoundingBox(theWorld, blockpos)
                                     .expand(0.0020000000949949026, 0.0020000000949949026, 0.0020000000949949026)
                                     .offset(-d0, -d1, -d2));
    }

    DxStateManager::depthMask(true);
    DxStateManager::enableTexture2D();
    DxStateManager::disableBlend();
}

void RenderGlobal::drawSelectionBoundingBox(const AxisAlignedBB& b)
{
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(3, DefaultVertexFormats::POSITION);
    worldrenderer.pos(b.minX, b.minY, b.minZ).endVertex();
    worldrenderer.pos(b.maxX, b.minY, b.minZ).endVertex();
    worldrenderer.pos(b.maxX, b.minY, b.maxZ).endVertex();
    worldrenderer.pos(b.minX, b.minY, b.maxZ).endVertex();
    worldrenderer.pos(b.minX, b.minY, b.minZ).endVertex();
    tessellator.draw();
    worldrenderer.begin(3, DefaultVertexFormats::POSITION);
    worldrenderer.pos(b.minX, b.maxY, b.minZ).endVertex();
    worldrenderer.pos(b.maxX, b.maxY, b.minZ).endVertex();
    worldrenderer.pos(b.maxX, b.maxY, b.maxZ).endVertex();
    worldrenderer.pos(b.minX, b.maxY, b.maxZ).endVertex();
    worldrenderer.pos(b.minX, b.maxY, b.minZ).endVertex();
    tessellator.draw();
    worldrenderer.begin(1, DefaultVertexFormats::POSITION);
    worldrenderer.pos(b.minX, b.minY, b.minZ).endVertex();
    worldrenderer.pos(b.minX, b.maxY, b.minZ).endVertex();
    worldrenderer.pos(b.maxX, b.minY, b.minZ).endVertex();
    worldrenderer.pos(b.maxX, b.maxY, b.minZ).endVertex();
    worldrenderer.pos(b.maxX, b.minY, b.maxZ).endVertex();
    worldrenderer.pos(b.maxX, b.maxY, b.maxZ).endVertex();
    worldrenderer.pos(b.minX, b.minY, b.maxZ).endVertex();
    worldrenderer.pos(b.minX, b.maxY, b.maxZ).endVertex();
    tessellator.draw();
}

void RenderGlobal::drawOutlinedBoundingBox(const AxisAlignedBB& b, int red, int green, int blue, int alpha)
{
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(3, DefaultVertexFormats::POSITION_COLOR);
    worldrenderer.pos(b.minX, b.minY, b.minZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.maxX, b.minY, b.minZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.maxX, b.minY, b.maxZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.minX, b.minY, b.maxZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.minX, b.minY, b.minZ).color(red, green, blue, alpha).endVertex();
    tessellator.draw();
    worldrenderer.begin(3, DefaultVertexFormats::POSITION_COLOR);
    worldrenderer.pos(b.minX, b.maxY, b.minZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.maxX, b.maxY, b.minZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.maxX, b.maxY, b.maxZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.minX, b.maxY, b.maxZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.minX, b.maxY, b.minZ).color(red, green, blue, alpha).endVertex();
    tessellator.draw();
    worldrenderer.begin(1, DefaultVertexFormats::POSITION_COLOR);
    worldrenderer.pos(b.minX, b.minY, b.minZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.minX, b.maxY, b.minZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.maxX, b.minY, b.minZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.maxX, b.maxY, b.minZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.maxX, b.minY, b.maxZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.maxX, b.maxY, b.maxZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.minX, b.minY, b.maxZ).color(red, green, blue, alpha).endVertex();
    worldrenderer.pos(b.minX, b.maxY, b.maxZ).color(red, green, blue, alpha).endVertex();
    tessellator.draw();
}

void RenderGlobal::markBlocksForUpdate(int x1, int y1, int z1, int x2, int y2, int z2)
{
    if (viewFrustum)
        viewFrustum->markBlocksForUpdate(x1, y1, z1, x2, y2, z2);
}

void RenderGlobal::markBlockForUpdate(const BlockPos& pos)
{
    const int i = pos.getX();
    const int j = pos.getY();
    const int k = pos.getZ();
    markBlocksForUpdate(i - 1, j - 1, k - 1, i + 1, j + 1, k + 1);
}

void RenderGlobal::notifyLightSet(const BlockPos& pos)
{
    const int i = pos.getX();
    const int j = pos.getY();
    const int k = pos.getZ();
    markBlocksForUpdate(i - 1, j - 1, k - 1, i + 1, j + 1, k + 1);
}

void RenderGlobal::markBlockRangeForRenderUpdate(int x1, int y1, int z1, int x2, int y2, int z2)
{
    markBlocksForUpdate(x1 - 1, y1 - 1, z1 - 1, x2 + 1, y2 + 1, z2 + 1);
}

void RenderGlobal::sendBlockBreakProgress(int breakerId, const BlockPos& pos, int progress)
{
    if (progress >= 0 && progress < 10) {
        DestroyBlockProgress* destroyblockprogress = damagedBlocks.find(breakerId);
        if (destroyblockprogress == nullptr || destroyblockprogress->getPosition() != pos) {
            damagedBlocks.put(breakerId, DestroyBlockProgress(breakerId, pos));
            destroyblockprogress = damagedBlocks.find(breakerId);
        }
        destroyblockprogress->setPartialBlockDamage(progress);
        destroyblockprogress->setCloudUpdateTick(cloudTickCounter);
    } else {
        damagedBlocks.erase(breakerId);
    }
}

} // namespace mc
