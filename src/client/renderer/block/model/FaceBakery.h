#pragma once

// Original: net/minecraft/client/renderer/block/model/FaceBakery.java
// Turns one face of a model element into a BakedQuad: corner positions from the element bounds, the element
// rotation, the blockstate rotation, UVs on the sprite (optionally locked to the world), the face shade.

#include "client/renderer/block/model/BakedQuad.h"
#include "client/renderer/block/model/ModelBlock.h"

#include <optional>

namespace mc {

class ModelRotation;
class TextureAtlasSprite;

class FaceBakery {
public:
    BakedQuad makeBakedQuad(const lwjgl::Vector3f& posFrom, const lwjgl::Vector3f& posTo, const BlockPartFace& face, const TextureAtlasSprite& sprite,
        EnumFacing facing, const ModelRotation& modelRotation, const std::optional<BlockPartRotation>& partRotation, bool uvLocked, bool shade) const;

    int rotateVertex(lwjgl::Vector3f& position, EnumFacing facing, int vertexIndex, const ModelRotation& modelRotation, bool uvLocked) const;
    static EnumFacing getFacingFromVertexData(const std::array<int32_t, 28>& faceData);
    void lockUv(std::array<int32_t, 28>& faceData, EnumFacing facing, const BlockFaceUV& faceUV, const TextureAtlasSprite& sprite) const;

private:
    void fillVertexData(std::array<int32_t, 28>& faceData, int vertexIndex, EnumFacing facing, const BlockPartFace& partFace, const float* positions,
        const TextureAtlasSprite& sprite, const ModelRotation& modelRotation, const std::optional<BlockPartRotation>& partRotation, bool uvLocked,
        bool shade) const;
    void storeVertexData(std::array<int32_t, 28>& faceData, int storeIndex, int vertexIndex, const lwjgl::Vector3f& position, int shadeColor,
        const TextureAtlasSprite& sprite, const BlockFaceUV& faceUV) const;
    void rotatePart(lwjgl::Vector3f& position, const std::optional<BlockPartRotation>& partRotation) const;
    void rotateScale(lwjgl::Vector3f& position, const lwjgl::Vector3f& rotationOrigin, const lwjgl::Matrix4f& rotationMatrix,
        const lwjgl::Vector3f& scale) const;
    void applyFacing(std::array<int32_t, 28>& faceData, EnumFacing facing) const;
    void lockVertexUv(int vertex, std::array<int32_t, 28>& faceData, EnumFacing facing, const BlockFaceUV& faceUV, const TextureAtlasSprite& sprite) const;
};

} // namespace mc
