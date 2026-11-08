#include "client/renderer/ViewFrustum.h"

#include "client/renderer/DxStateManager.h"
#include "client/renderer/chunk/RenderChunk.h"
#include "client/renderer/vertex/VertexBuffer.h"
#include "render/gl/GL.h"
#include "util/MathHelper.h"

namespace mc {

ViewFrustum::ViewFrustum(World* worldIn, int renderDistanceChunks, RenderGlobal* renderGlobalIn)
    : renderGlobal(renderGlobalIn)
    , world(worldIn)
{
    setCountChunksXYZ(renderDistanceChunks);
    createRenderChunks();
}

void ViewFrustum::createRenderChunks()
{
    const int i = countChunksX * countChunksY * countChunksZ;
    renderChunks.assign(static_cast<size_t>(i), nullptr);
    int j = 0;
    for (int k = 0; k < countChunksX; ++k) {
        for (int l = 0; l < countChunksY; ++l) {
            for (int i1 = 0; i1 < countChunksZ; ++i1) {
                const int j1 = (i1 * countChunksY + l) * countChunksX + k;
                const BlockPos blockpos(k * 16, l * 16, i1 * 16);
                renderChunks[static_cast<size_t>(j1)] = RenderChunk::create(world, renderGlobal, blockpos, j++);
            }
        }
    }
}

void ViewFrustum::deleteGlResources()
{
    for (const std::shared_ptr<RenderChunk>& renderchunk : renderChunks)
        renderchunk->deleteGlResources();
}

void ViewFrustum::setCountChunksXYZ(int renderDistanceChunks)
{
    const int i = renderDistanceChunks * 2 + 1;
    countChunksX = i;
    countChunksY = 16;
    countChunksZ = i;
}

void ViewFrustum::updateChunkPositions(double viewEntityX, double viewEntityZ)
{
    const int i = MathHelper::floor_double(viewEntityX) - 8;
    const int j = MathHelper::floor_double(viewEntityZ) - 8;
    const int k = countChunksX * 16;

    for (int l = 0; l < countChunksX; ++l) {
        const int i1 = func_178157_a(i, k, l);
        for (int j1 = 0; j1 < countChunksZ; ++j1) {
            const int k1 = func_178157_a(j, k, j1);
            for (int l1 = 0; l1 < countChunksY; ++l1) {
                const int i2 = l1 * 16;
                RenderChunk& renderchunk = *renderChunks[static_cast<size_t>((j1 * countChunksY + l1) * countChunksX + l)];
                const BlockPos blockpos(i1, i2, k1);
                if (blockpos != renderchunk.getPosition())
                    renderchunk.setPosition(blockpos);
            }
        }
    }
}

int ViewFrustum::func_178157_a(int viewPos, int size, int index)
{
    const int i = index * 16;
    int j = i - viewPos + size / 2;
    if (j < 0)
        j -= size - 1;
    return i - j / size * size;
}

void ViewFrustum::markBlocksForUpdate(int fromX, int fromY, int fromZ, int toX, int toY, int toZ)
{
    const int i = MathHelper::bucketInt(fromX, 16);
    const int j = MathHelper::bucketInt(fromY, 16);
    const int k = MathHelper::bucketInt(fromZ, 16);
    const int l = MathHelper::bucketInt(toX, 16);
    const int i1 = MathHelper::bucketInt(toY, 16);
    const int j1 = MathHelper::bucketInt(toZ, 16);

    for (int k1 = i; k1 <= l; ++k1) {
        int l1 = k1 % countChunksX;
        if (l1 < 0)
            l1 += countChunksX;
        for (int i2 = j; i2 <= i1; ++i2) {
            int j2 = i2 % countChunksY;
            if (j2 < 0)
                j2 += countChunksY;
            for (int k2 = k; k2 <= j1; ++k2) {
                int l2 = k2 % countChunksZ;
                if (l2 < 0)
                    l2 += countChunksZ;
                const int i3 = (l2 * countChunksY + j2) * countChunksX + l1;
                renderChunks[static_cast<size_t>(i3)]->setNeedsUpdate(true);
            }
        }
    }
}

RenderChunk* ViewFrustum::getRenderChunk(const BlockPos& pos)
{
    int i = MathHelper::bucketInt(pos.getX(), 16);
    const int j = MathHelper::bucketInt(pos.getY(), 16);
    int k = MathHelper::bucketInt(pos.getZ(), 16);
    if (j < 0 || j >= countChunksY)
        return nullptr;
    i = i % countChunksX;
    if (i < 0)
        i += countChunksX;
    k = k % countChunksZ;
    if (k < 0)
        k += countChunksZ;
    const int l = (k * countChunksY + j) * countChunksX + i;
    return renderChunks[static_cast<size_t>(l)].get();
}

// --- ChunkRenderContainer ---

void ChunkRenderContainer::initialize(double viewEntityXIn, double viewEntityYIn, double viewEntityZIn)
{
    initialized = true;
    renderChunks.clear();
    viewEntityX = viewEntityXIn;
    viewEntityY = viewEntityYIn;
    viewEntityZ = viewEntityZIn;
}

void ChunkRenderContainer::preRenderChunk(const RenderChunk& renderChunkIn)
{
    const BlockPos& blockpos = renderChunkIn.getPosition();
    DxStateManager::translate(static_cast<float>(static_cast<double>(blockpos.getX()) - viewEntityX), static_cast<float>(static_cast<double>(blockpos.getY()) - viewEntityY),
        static_cast<float>(static_cast<double>(blockpos.getZ()) - viewEntityZ));
}

void ChunkRenderContainer::addRenderChunk(RenderChunk* renderChunkIn, EnumWorldBlockLayer /*layer*/)
{
    renderChunks.push_back(renderChunkIn);
}

void ChunkRenderContainer::renderChunkLayer(EnumWorldBlockLayer layer)
{
    if (!initialized)
        return;
    // The matrices come from the same operations as VboRenderList's matrix stack; the chunks then go to
    // Direct3D together (GL::drawBlockBuffers), in the same order.
    drawBuffers.clear();
    drawModelViews.clear();
    const Matrix4f& modelView = GL::currentMatrix(GL::GL_MODELVIEW);
    for (RenderChunk* renderchunk : renderChunks) {
        VertexBuffer* vertexbuffer = renderchunk->getVertexBufferByLayer(static_cast<int>(layer));
        if (vertexbuffer->isBlockBuffer()) {
            // preRenderChunk and multModelviewMatrix on a copy: glTranslatef, then glMultMatrixf.
            const BlockPos& blockpos = renderchunk->getPosition();
            Matrix4f& matrix = drawModelViews.emplace_back(modelView);
            matrix.translate(static_cast<float>(static_cast<double>(blockpos.getX()) - viewEntityX),
                static_cast<float>(static_cast<double>(blockpos.getY()) - viewEntityY), static_cast<float>(static_cast<double>(blockpos.getZ()) - viewEntityZ));
            matrix = matrix * Matrix4f { renderchunk->getModelviewMatrix() };
            drawBuffers.push_back(vertexbuffer->getGlBufferId());
        } else {
            if (!drawBuffers.empty()) {
                GL::drawBlockBuffers(drawBuffers, drawModelViews);
                drawBuffers.clear();
                drawModelViews.clear();
            }
            DxStateManager::pushMatrix();
            preRenderChunk(*renderchunk);
            renderchunk->multModelviewMatrix();
            vertexbuffer->drawArrays(7);
            DxStateManager::popMatrix();
        }
    }
    if (!drawBuffers.empty())
        GL::drawBlockBuffers(drawBuffers, drawModelViews);
    DxStateManager::resetColor();
    renderChunks.clear();
}

} // namespace mc
