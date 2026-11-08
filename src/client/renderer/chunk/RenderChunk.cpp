#include "client/renderer/chunk/RenderChunk.h"

#include "block/state/BlockState.h"
#include "client/Minecraft.h"
#include "client/renderer/BlockRendererDispatcher.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/RegionRenderCache.h"
#include "client/renderer/chunk/ChunkCompileTaskGenerator.h"
#include "client/renderer/chunk/CompiledChunk.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"

namespace mc {

std::atomic<int> RenderChunk::renderChunksUpdated { 0 };

std::shared_ptr<RenderChunk> RenderChunk::create(World* worldIn, RenderGlobal* renderGlobalIn, const BlockPos& blockPosIn, int indexIn)
{
    auto renderChunk = std::make_shared<RenderChunk>(worldIn, renderGlobalIn, indexIn);
    if (blockPosIn != renderChunk->getPosition())
        renderChunk->setPosition(blockPosIn);
    return renderChunk;
}

RenderChunk::RenderChunk(World* worldIn, RenderGlobal* renderGlobalIn, int indexIn)
    : boundingBox(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)
    , world(worldIn)
    , renderGlobal(renderGlobalIn)
    , compiledChunk(CompiledChunk::DUMMY())
    , index(indexIn)
{
    for (auto& vertexBuffer : vertexBuffers)
        vertexBuffer = std::make_unique<VertexBuffer>(DefaultVertexFormats::BLOCK);
}

RenderChunk::~RenderChunk() = default;

bool RenderChunk::setFrameIndex(int frameIndexIn)
{
    if (frameIndex == frameIndexIn)
        return false;
    frameIndex = frameIndexIn;
    return true;
}

void RenderChunk::setPosition(const BlockPos& pos)
{
    stopCompileTask();
    position = pos;
    neighborsCached = 0;
    boundingBox = AxisAlignedBB(pos, pos.add(16, 16, 16));
    for (EnumFacing enumfacing : kFacings)
        mapEnumFacing[static_cast<size_t>(ordinal(enumfacing))] = pos.offset(enumfacing, 16);
    initModelviewMatrix();
}

void RenderChunk::resortTransparency(float x, float y, float z, ChunkCompileTaskGenerator& generator)
{
    CompiledChunk& compiledchunk = *generator.getCompiledChunk();
    if (compiledchunk.getState() && !compiledchunk.isLayerEmpty(EnumWorldBlockLayer::TRANSLUCENT)) {
        WorldRenderer& worldrenderer = generator.getRegionRenderCacheBuilder()->getWorldRendererByLayer(EnumWorldBlockLayer::TRANSLUCENT);
        preRenderBlocks(worldrenderer, position);
        worldrenderer.setVertexState(*compiledchunk.getState());
        postRenderBlocks(EnumWorldBlockLayer::TRANSLUCENT, x, y, z, worldrenderer, compiledchunk);
    }
}

void RenderChunk::rebuildChunk(float x, float y, float z, ChunkCompileTaskGenerator& generator)
{
    auto compiledchunk = std::make_shared<CompiledChunk>();
    const BlockPos blockpos = position;
    const BlockPos blockpos1 = blockpos.add(15, 15, 15);
    RegionRenderCache* iblockaccess;
    {
        std::lock_guard guard(generator.getLock());
        if (generator.getStatus() != ChunkCompileTaskGenerator::Status::COMPILING)
            return;
        iblockaccess = generator.getRegionCache();
        generator.setCompiledChunk(compiledchunk);
    }

    VisGraph visgraph;
    if (!iblockaccess->extendedLevelsInChunkCache()) {
        ++renderChunksUpdated;
        bool layersUsed[4] {};
        BlockRendererDispatcher& blockrendererdispatcher = Minecraft::getMinecraft()->getBlockRendererDispatcher();

        for (const BlockPos& blockposMutable : BlockPos::getAllInBox(blockpos, blockpos1)) {
            const IBlockState* iblockstate = iblockaccess->getBlockState(blockposMutable);
            Block* block = iblockstate->getBlock();
            if (block->isOpaqueCube())
                visgraph.setOpaque(blockposMutable);

            // Java: the tile entities with special renderers are collected here (stage 10).

            const EnumWorldBlockLayer layer = block->getBlockLayer();
            const int j = static_cast<int>(layer);
            if (block->getRenderType() != -1) {
                WorldRenderer& worldrenderer = generator.getRegionRenderCacheBuilder()->getWorldRendererByLayerId(j);
                if (!compiledchunk->isLayerStarted(layer)) {
                    compiledchunk->setLayerStarted(layer);
                    preRenderBlocks(worldrenderer, blockpos);
                }
                layersUsed[j] |= blockrendererdispatcher.renderBlock(iblockstate, blockposMutable, iblockaccess, worldrenderer);
            }
        }

        for (int l = 0; l < 4; ++l) {
            const auto layer = static_cast<EnumWorldBlockLayer>(l);
            if (layersUsed[l])
                compiledchunk->setLayerUsed(layer);
            if (compiledchunk->isLayerStarted(layer))
                postRenderBlocks(layer, x, y, z, generator.getRegionRenderCacheBuilder()->getWorldRendererByLayer(layer), *compiledchunk);
        }
    }

    compiledchunk->setVisibility(visgraph.computeVisibility());
    // Java's region cache is a local of this method; here the task carries it, and the task outlives the
    // build as the render chunk's compileTask: drop the copy of the blocks now.
    generator.setRegionCache(nullptr);
}

void RenderChunk::finishCompileTask()
{
    std::lock_guard guard(lockCompileTask);
    if (compileTask && compileTask->getStatus() != ChunkCompileTaskGenerator::Status::DONE) {
        compileTask->finish();
        compileTask.reset();
    }
}

std::shared_ptr<ChunkCompileTaskGenerator> RenderChunk::makeCompileTaskChunk()
{
    std::lock_guard guard(lockCompileTask);
    finishCompileTask();
    compileTask = std::make_shared<ChunkCompileTaskGenerator>(shared_from_this(), ChunkCompileTaskGenerator::Type::REBUILD_CHUNK);
    // Java builds the region cache on the worker (rebuildChunk); see ChunkCompileTaskGenerator.
    compileTask->setRegionCache(std::make_unique<RegionRenderCache>(world, position.add(-1, -1, -1), position.add(15, 15, 15).add(1, 1, 1), 1));
    return compileTask;
}

std::shared_ptr<ChunkCompileTaskGenerator> RenderChunk::makeCompileTaskTransparency()
{
    std::lock_guard guard(lockCompileTask);
    if (!compileTask || compileTask->getStatus() != ChunkCompileTaskGenerator::Status::PENDING) {
        if (compileTask && compileTask->getStatus() != ChunkCompileTaskGenerator::Status::DONE) {
            compileTask->finish();
            compileTask.reset();
        }
        compileTask = std::make_shared<ChunkCompileTaskGenerator>(shared_from_this(), ChunkCompileTaskGenerator::Type::RESORT_TRANSPARENCY);
        compileTask->setCompiledChunk(getCompiledChunk());
        return compileTask;
    }
    return nullptr;
}

void RenderChunk::preRenderBlocks(WorldRenderer& worldRendererIn, const BlockPos& pos)
{
    worldRendererIn.begin(7, DefaultVertexFormats::BLOCK);
    worldRendererIn.setTranslation(static_cast<double>(-pos.getX()), static_cast<double>(-pos.getY()), static_cast<double>(-pos.getZ()));
}

void RenderChunk::postRenderBlocks(EnumWorldBlockLayer layer, float x, float y, float z, WorldRenderer& worldRendererIn, CompiledChunk& compiledChunkIn)
{
    if (layer == EnumWorldBlockLayer::TRANSLUCENT && !compiledChunkIn.isLayerEmpty(layer)) {
        worldRendererIn.sortVertexData(x, y, z);
        compiledChunkIn.setState(worldRendererIn.getVertexState());
    }
    worldRendererIn.finishDrawing();
}

void RenderChunk::initModelviewMatrix()
{
    DxStateManager::pushMatrix();
    DxStateManager::loadIdentity();
    const float f = 1.000001f;
    DxStateManager::translate(-8.0f, -8.0f, -8.0f);
    DxStateManager::scale(f, f, f);
    DxStateManager::translate(8.0f, 8.0f, 8.0f);
    DxStateManager::getFloat(2982, modelviewMatrix.data());
    DxStateManager::popMatrix();
}

void RenderChunk::multModelviewMatrix()
{
    DxStateManager::multMatrix(modelviewMatrix.data());
}

std::shared_ptr<CompiledChunk> RenderChunk::getCompiledChunk()
{
    std::lock_guard guard(lockCompiledChunk);
    return compiledChunk;
}

void RenderChunk::setCompiledChunk(std::shared_ptr<CompiledChunk> compiledChunkIn)
{
    std::lock_guard guard(lockCompiledChunk);
    compiledChunk = std::move(compiledChunkIn);
}

void RenderChunk::stopCompileTask()
{
    finishCompileTask();
    setCompiledChunk(CompiledChunk::DUMMY());
}

void RenderChunk::deleteGlResources()
{
    stopCompileTask();
    {
        // A finished task still points back here; drop it so the render chunk can go.
        std::lock_guard guard(lockCompileTask);
        compileTask.reset();
    }
    world = nullptr;
    for (auto& vertexBuffer : vertexBuffers) {
        if (vertexBuffer)
            vertexBuffer->deleteGlBuffers();
    }
}

} // namespace mc
