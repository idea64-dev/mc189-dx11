#pragma once

// Original: net/minecraft/client/renderer/RenderGlobal.java
// The world renderer: the render chunks around the camera (visibility flood fill, frustum culling, rebuild
// scheduling), the terrain layers, sky, sun, moon, stars, clouds, the selection box and the block cracks.
// Not ported yet: entities and tile entities (renderEntities -- stage 10), the entity outline shader (stage 15),
// the world border wall, and the sounds and particles of playAuxSFX / broadcastSound / spawnParticle /
// playRecord (stages 13 and 15).

#include "client/renderer/DestroyBlockProgress.h"
#include "client/resources/IResourceManager.h"
#include "core/JavaHashMap.h"
#include "render/gl/GL.h"
#include "util/Vec3.h"
#include "world/IWorldAccess.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace mc {

class ChunkRenderContainer;
class ChunkRenderDispatcher;
class Entity;
class EntityPlayer;
class ICamera;
class Minecraft;
class CompiledChunk;
class RenderChunk;
class Tessellator;
class TextureAtlasSprite;
class TextureManager;
class VertexBuffer;
class ViewFrustum;
class WorldClient;
class WorldRenderer;
enum class EnumWorldBlockLayer;

class RenderGlobal : public IWorldAccess, public IResourceManagerReloadListener {
public:
    explicit RenderGlobal(Minecraft& mcIn);
    ~RenderGlobal() override;

    void onResourceManagerReload(const IResourceManager& resourceManager) override;
    // Null clears the world.
    void setWorldAndLoadRenderers(WorldClient* worldClientIn);
    void loadRenderers();
    void renderEntities(Entity* renderViewEntity, ICamera& camera, float partialTicks);
    std::string getDebugInfoRenders();
    std::string getDebugInfoEntities() const;
    void setupTerrain(Entity* viewEntity, double partialTicks, ICamera& camera, int frameCount, bool playerSpectator);
    int renderBlockLayer(EnumWorldBlockLayer blockLayerIn, double partialTicks, int pass, Entity* entityIn);
    void updateClouds();
    void renderSky(float partialTicks, int pass);
    void renderClouds(float partialTicks, int pass);
    bool hasCloudFog(double, double, double, float) const { return false; }
    void updateChunks(int64_t finishTimeNano);
    void drawBlockDamageTexture(Tessellator& tessellatorIn, WorldRenderer& worldRendererIn, Entity* entityIn, float partialTicks);
    void drawSelectionBox(EntityPlayer* player, const MovingObjectPosition& movingObjectPositionIn, int execute, float partialTicks);
    static void drawSelectionBoundingBox(const AxisAlignedBB& boundingBox);
    static void drawOutlinedBoundingBox(const AxisAlignedBB& boundingBox, int red, int green, int blue, int alpha);
    void setDisplayListEntitiesDirty() { displayListEntitiesDirty = true; }
    void deleteAllDisplayLists() { }

    // IWorldAccess
    void markBlockForUpdate(const BlockPos& pos) override;
    void notifyLightSet(const BlockPos& pos) override;
    void markBlockRangeForRenderUpdate(int x1, int y1, int z1, int x2, int y2, int z2) override;
    void sendBlockBreakProgress(int breakerId, const BlockPos& pos, int progress) override;

private:
    struct ContainerLocalRenderInformation {
        RenderChunk* renderChunk;
        // Java: null for the starting chunks.
        std::optional<EnumFacing> facing;
        uint8_t setFacing = 0;
        int counter;
    };

    void updateDestroyBlockIcons();
    void generateSky2();
    void generateSky();
    static void renderSky(WorldRenderer& worldRendererIn, float posY, bool reverseX);
    void generateStars();
    static void renderStars(WorldRenderer& worldRendererIn);
    void stopChunkUpdates();
    static bool isPositionInRenderChunk(const BlockPos& pos, const RenderChunk& renderChunkIn);
    uint8_t getVisibleFacings(const BlockPos& pos);
    uint8_t computeVisibleFacings(const BlockPos& pos);
    RenderChunk* getRenderChunkOffset(const BlockPos& playerPos, const RenderChunk& renderChunkBase, EnumFacing facing);
    static std::array<float, 3> getViewVector(const Entity* entityIn, double partialTicks);
    void renderBlockLayer(EnumWorldBlockLayer blockLayerIn);
    void cleanupDamagedBlocks();
    void renderSkyEnd();
    void renderCloudsFancy(float partialTicks, int pass);
    void preRenderDamagedBlocks();
    void postRenderDamagedBlocks();
    void markBlocksForUpdate(int x1, int y1, int z1, int x2, int y2, int z2);

    Minecraft& mc;
    TextureManager& renderEngine;
    WorldClient* theWorld = nullptr;
    // Java: a LinkedHashSet -- insertion order, no duplicates.
    std::vector<RenderChunk*> chunksToUpdate;
    std::unordered_set<RenderChunk*> chunksToUpdateSet;
    std::vector<ContainerLocalRenderInformation> renderInfos;
    std::vector<ContainerLocalRenderInformation> visibilityQueue;
    struct VisibleFacingsCache {
        BlockPos pos;
        const RenderChunk* section;
        const CompiledChunk* compiled;
        uint8_t facings;
    };
    std::optional<VisibleFacingsCache> visibleFacingsCache;
    std::vector<GLVertex> cloudVertices;
    std::unique_ptr<ViewFrustum> viewFrustum;
    std::unique_ptr<VertexBuffer> starVBO;
    std::unique_ptr<VertexBuffer> skyVBO;
    std::unique_ptr<VertexBuffer> sky2VBO;
    int cloudTickCounter = 0;
    JavaHashMap<int32_t, DestroyBlockProgress> damagedBlocks;
    std::array<TextureAtlasSprite*, 10> destroyBlockIcons {};
    double frustumUpdatePosX = 4.9E-324;
    double frustumUpdatePosY = 4.9E-324;
    double frustumUpdatePosZ = 4.9E-324;
    int frustumUpdatePosChunkX = INT32_MIN;
    int frustumUpdatePosChunkY = INT32_MIN;
    int frustumUpdatePosChunkZ = INT32_MIN;
    double lastViewEntityX = 4.9E-324;
    double lastViewEntityY = 4.9E-324;
    double lastViewEntityZ = 4.9E-324;
    double lastViewEntityPitch = 4.9E-324;
    double lastViewEntityYaw = 4.9E-324;
    std::unique_ptr<ChunkRenderDispatcher> renderDispatcher;
    std::unique_ptr<ChunkRenderContainer> renderContainer;
    int renderDistanceChunks = -1;
    int renderEntitiesStartupCounter = 2;
    int countEntitiesTotal = 0;
    int countEntitiesRendered = 0;
    int countEntitiesHidden = 0;
    bool vboEnabled = false;
    double prevRenderSortX = 0.0;
    double prevRenderSortY = 0.0;
    double prevRenderSortZ = 0.0;
    bool displayListEntitiesDirty = true;
};

} // namespace mc
