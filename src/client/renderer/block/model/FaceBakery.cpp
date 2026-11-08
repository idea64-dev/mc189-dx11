#include "client/renderer/block/model/FaceBakery.h"

#include "client/renderer/texture/TextureAtlasSprite.h"
#include "client/resources/model/ModelRotation.h"
#include "core/Java.h"
#include "core/JavaMath.h"
#include "util/MathHelper.h"
#include "util/BlockPos.h"

#include <cmath>

namespace mc {

namespace {

constexpr int WEST_INDEX = getIndex(EnumFacing::WEST);
constexpr int DOWN_INDEX = getIndex(EnumFacing::DOWN);
constexpr int NORTH_INDEX = getIndex(EnumFacing::NORTH);
constexpr int EAST_INDEX = getIndex(EnumFacing::EAST);
constexpr int UP_INDEX = getIndex(EnumFacing::UP);
constexpr int SOUTH_INDEX = getIndex(EnumFacing::SOUTH);

constexpr VertexInformation kVertexInfos[6][4] {
    // DOWN
    { { WEST_INDEX, DOWN_INDEX, SOUTH_INDEX }, { WEST_INDEX, DOWN_INDEX, NORTH_INDEX }, { EAST_INDEX, DOWN_INDEX, NORTH_INDEX },
        { EAST_INDEX, DOWN_INDEX, SOUTH_INDEX } },
    // UP
    { { WEST_INDEX, UP_INDEX, NORTH_INDEX }, { WEST_INDEX, UP_INDEX, SOUTH_INDEX }, { EAST_INDEX, UP_INDEX, SOUTH_INDEX }, { EAST_INDEX, UP_INDEX, NORTH_INDEX } },
    // NORTH
    { { EAST_INDEX, UP_INDEX, NORTH_INDEX }, { EAST_INDEX, DOWN_INDEX, NORTH_INDEX }, { WEST_INDEX, DOWN_INDEX, NORTH_INDEX },
        { WEST_INDEX, UP_INDEX, NORTH_INDEX } },
    // SOUTH
    { { WEST_INDEX, UP_INDEX, SOUTH_INDEX }, { WEST_INDEX, DOWN_INDEX, SOUTH_INDEX }, { EAST_INDEX, DOWN_INDEX, SOUTH_INDEX },
        { EAST_INDEX, UP_INDEX, SOUTH_INDEX } },
    // WEST
    { { WEST_INDEX, UP_INDEX, NORTH_INDEX }, { WEST_INDEX, DOWN_INDEX, NORTH_INDEX }, { WEST_INDEX, DOWN_INDEX, SOUTH_INDEX },
        { WEST_INDEX, UP_INDEX, SOUTH_INDEX } },
    // EAST
    { { EAST_INDEX, UP_INDEX, SOUTH_INDEX }, { EAST_INDEX, DOWN_INDEX, SOUTH_INDEX }, { EAST_INDEX, DOWN_INDEX, NORTH_INDEX },
        { EAST_INDEX, UP_INDEX, NORTH_INDEX } },
};

float scaleRotation22_5()
{
    static const float value = 1.0f / static_cast<float>(java::mathCos(0.39269909262657166)) - 1.0f;
    return value;
}

float scaleRotationGeneral()
{
    static const float value = 1.0f / static_cast<float>(java::mathCos(0.7853981633974483)) - 1.0f;
    return value;
}

int getFaceShadeColor(EnumFacing facing)
{
    float f;
    switch (facing) {
    case EnumFacing::DOWN:
        f = 0.5f;
        break;
    case EnumFacing::UP:
        f = 1.0f;
        break;
    case EnumFacing::NORTH:
    case EnumFacing::SOUTH:
        f = 0.8f;
        break;
    case EnumFacing::WEST:
    case EnumFacing::EAST:
        f = 0.6f;
        break;
    default:
        f = 1.0f;
        break;
    }
    const int i = MathHelper::clamp_int(java::f2i(f * 255.0f), 0, 255);
    return static_cast<int>(0xFF000000u | static_cast<uint32_t>(i << 16 | i << 8 | i));
}

float bitsToFloat(int32_t bits)
{
    return java::intBitsToFloat(bits);
}

int32_t floatToBits(float value)
{
    return java::floatToRawIntBits(value);
}

// The (u, v) in 0..16 of a block-space position seen from the given face, as uv lock and BreakingFour use.
void faceUv(EnumFacing facing, float f, float f1, float f2, float& f3, float& f4)
{
    f3 = 0.0f;
    f4 = 0.0f;
    switch (facing) {
    case EnumFacing::DOWN:
        f3 = f * 16.0f;
        f4 = (1.0f - f2) * 16.0f;
        break;
    case EnumFacing::UP:
        f3 = f * 16.0f;
        f4 = f2 * 16.0f;
        break;
    case EnumFacing::NORTH:
        f3 = (1.0f - f) * 16.0f;
        f4 = (1.0f - f1) * 16.0f;
        break;
    case EnumFacing::SOUTH:
        f3 = f * 16.0f;
        f4 = (1.0f - f1) * 16.0f;
        break;
    case EnumFacing::WEST:
        f3 = f2 * 16.0f;
        f4 = (1.0f - f1) * 16.0f;
        break;
    case EnumFacing::EAST:
        f3 = (1.0f - f2) * 16.0f;
        f4 = (1.0f - f1) * 16.0f;
        break;
    }
}

} // namespace

const VertexInformation& getVertexInformation(EnumFacing facing, int index)
{
    return kVertexInfos[getIndex(facing)][index];
}

BakedQuad makeBreakingFour(const BakedQuad& quad, const TextureAtlasSprite& texture)
{
    BakedQuad result;
    result.vertexData = quad.vertexData;
    result.tintIndex = quad.tintIndex;
    result.face = FaceBakery::getFacingFromVertexData(quad.vertexData);
    for (int vertex = 0; vertex < 4; ++vertex) {
        const int i = 7 * vertex;
        float f3;
        float f4;
        faceUv(result.face, bitsToFloat(result.vertexData[static_cast<size_t>(i)]), bitsToFloat(result.vertexData[static_cast<size_t>(i + 1)]),
            bitsToFloat(result.vertexData[static_cast<size_t>(i + 2)]), f3, f4);
        result.vertexData[static_cast<size_t>(i + 4)] = floatToBits(texture.getInterpolatedU(static_cast<double>(f3)));
        result.vertexData[static_cast<size_t>(i + 4 + 1)] = floatToBits(texture.getInterpolatedV(static_cast<double>(f4)));
    }
    return result;
}

BakedQuad FaceBakery::makeBakedQuad(const lwjgl::Vector3f& posFrom, const lwjgl::Vector3f& posTo, const BlockPartFace& face, const TextureAtlasSprite& sprite,
    EnumFacing facing, const ModelRotation& modelRotation, const std::optional<BlockPartRotation>& partRotation, bool uvLocked, bool shade) const
{
    float positions[6];
    positions[WEST_INDEX] = posFrom.x / 16.0f;
    positions[DOWN_INDEX] = posFrom.y / 16.0f;
    positions[NORTH_INDEX] = posFrom.z / 16.0f;
    positions[EAST_INDEX] = posTo.x / 16.0f;
    positions[UP_INDEX] = posTo.y / 16.0f;
    positions[SOUTH_INDEX] = posTo.z / 16.0f;

    BakedQuad quad;
    for (int i = 0; i < 4; ++i)
        fillVertexData(quad.vertexData, i, facing, face, positions, sprite, modelRotation, partRotation, uvLocked, shade);
    const EnumFacing enumfacing = getFacingFromVertexData(quad.vertexData);
    if (uvLocked)
        lockUv(quad.vertexData, enumfacing, face.blockFaceUV, sprite);
    if (!partRotation)
        applyFacing(quad.vertexData, enumfacing);
    quad.tintIndex = face.tintIndex;
    quad.face = enumfacing;
    return quad;
}

void FaceBakery::fillVertexData(std::array<int32_t, 28>& faceData, int vertexIndex, EnumFacing facing, const BlockPartFace& partFace, const float* positions,
    const TextureAtlasSprite& sprite, const ModelRotation& modelRotation, const std::optional<BlockPartRotation>& partRotation, bool uvLocked, bool shade) const
{
    const EnumFacing enumfacing = modelRotation.rotateFace(facing);
    const int i = shade ? getFaceShadeColor(enumfacing) : -1;
    const VertexInformation& info = getVertexInformation(facing, vertexIndex);
    lwjgl::Vector3f vector3f(positions[info.xIndex], positions[info.yIndex], positions[info.zIndex]);
    rotatePart(vector3f, partRotation);
    const int j = rotateVertex(vector3f, facing, vertexIndex, modelRotation, uvLocked);
    storeVertexData(faceData, j, vertexIndex, vector3f, i, sprite, partFace.blockFaceUV);
}

void FaceBakery::storeVertexData(std::array<int32_t, 28>& faceData, int storeIndex, int vertexIndex, const lwjgl::Vector3f& position, int shadeColor,
    const TextureAtlasSprite& sprite, const BlockFaceUV& faceUV) const
{
    const size_t i = static_cast<size_t>(storeIndex * 7);
    faceData[i] = floatToBits(position.x);
    faceData[i + 1] = floatToBits(position.y);
    faceData[i + 2] = floatToBits(position.z);
    faceData[i + 3] = shadeColor;
    faceData[i + 4] = floatToBits(sprite.getInterpolatedU(static_cast<double>(faceUV.func_178348_a(vertexIndex))));
    faceData[i + 4 + 1] = floatToBits(sprite.getInterpolatedV(static_cast<double>(faceUV.func_178346_b(vertexIndex))));
}

void FaceBakery::rotatePart(lwjgl::Vector3f& position, const std::optional<BlockPartRotation>& partRotation) const
{
    if (!partRotation)
        return;
    lwjgl::Matrix4f matrix;
    lwjgl::Vector3f scale(0.0f, 0.0f, 0.0f);
    switch (partRotation->axis) {
    case Axis::X:
        matrix = lwjgl::Matrix4f::rotate(partRotation->angle * 0.017453292f, lwjgl::Vector3f(1.0f, 0.0f, 0.0f), matrix);
        scale.set(0.0f, 1.0f, 1.0f);
        break;
    case Axis::Y:
        matrix = lwjgl::Matrix4f::rotate(partRotation->angle * 0.017453292f, lwjgl::Vector3f(0.0f, 1.0f, 0.0f), matrix);
        scale.set(1.0f, 0.0f, 1.0f);
        break;
    case Axis::Z:
        matrix = lwjgl::Matrix4f::rotate(partRotation->angle * 0.017453292f, lwjgl::Vector3f(0.0f, 0.0f, 1.0f), matrix);
        scale.set(1.0f, 1.0f, 0.0f);
        break;
    }
    if (partRotation->rescale) {
        if (std::fabs(partRotation->angle) == 22.5f)
            scale.scale(scaleRotation22_5());
        else
            scale.scale(scaleRotationGeneral());
        scale = lwjgl::Vector3f::add(scale, lwjgl::Vector3f(1.0f, 1.0f, 1.0f));
    } else {
        scale.set(1.0f, 1.0f, 1.0f);
    }
    rotateScale(position, partRotation->origin, matrix, scale);
}

int FaceBakery::rotateVertex(lwjgl::Vector3f& position, EnumFacing facing, int vertexIndex, const ModelRotation& modelRotation, bool) const
{
    if (&modelRotation == &ModelRotation::X0_Y0)
        return vertexIndex;
    rotateScale(position, lwjgl::Vector3f(0.5f, 0.5f, 0.5f), modelRotation.getMatrix4d(), lwjgl::Vector3f(1.0f, 1.0f, 1.0f));
    return modelRotation.rotateVertex(facing, vertexIndex);
}

void FaceBakery::rotateScale(lwjgl::Vector3f& position, const lwjgl::Vector3f& rotationOrigin, const lwjgl::Matrix4f& rotationMatrix,
    const lwjgl::Vector3f& scale) const
{
    lwjgl::Vector4f vector4f { position.x - rotationOrigin.x, position.y - rotationOrigin.y, position.z - rotationOrigin.z, 1.0f };
    vector4f = lwjgl::Matrix4f::transform(rotationMatrix, vector4f);
    vector4f.x *= scale.x;
    vector4f.y *= scale.y;
    vector4f.z *= scale.z;
    position.set(vector4f.x + rotationOrigin.x, vector4f.y + rotationOrigin.y, vector4f.z + rotationOrigin.z);
}

EnumFacing FaceBakery::getFacingFromVertexData(const std::array<int32_t, 28>& faceData)
{
    const lwjgl::Vector3f vector3f(bitsToFloat(faceData[0]), bitsToFloat(faceData[1]), bitsToFloat(faceData[2]));
    const lwjgl::Vector3f vector3f1(bitsToFloat(faceData[7]), bitsToFloat(faceData[8]), bitsToFloat(faceData[9]));
    const lwjgl::Vector3f vector3f2(bitsToFloat(faceData[14]), bitsToFloat(faceData[15]), bitsToFloat(faceData[16]));
    const lwjgl::Vector3f vector3f3 = lwjgl::Vector3f::sub(vector3f, vector3f1);
    const lwjgl::Vector3f vector3f4 = lwjgl::Vector3f::sub(vector3f2, vector3f1);
    lwjgl::Vector3f vector3f5 = lwjgl::Vector3f::cross(vector3f4, vector3f3);
    const float f = static_cast<float>(std::sqrt(static_cast<double>(vector3f5.x * vector3f5.x + vector3f5.y * vector3f5.y + vector3f5.z * vector3f5.z)));
    vector3f5.x /= f;
    vector3f5.y /= f;
    vector3f5.z /= f;
    std::optional<EnumFacing> enumfacing;
    float f1 = 0.0f;
    for (const EnumFacing facing : kFacings) {
        const Vec3i vec = getDirectionVec(facing);
        const lwjgl::Vector3f vector3f6(static_cast<float>(vec.getX()), static_cast<float>(vec.getY()), static_cast<float>(vec.getZ()));
        const float f2 = lwjgl::Vector3f::dot(vector3f5, vector3f6);
        if (f2 >= 0.0f && f2 > f1) {
            f1 = f2;
            enumfacing = facing;
        }
    }
    return enumfacing.value_or(EnumFacing::UP);
}

void FaceBakery::lockUv(std::array<int32_t, 28>& faceData, EnumFacing facing, const BlockFaceUV& faceUV, const TextureAtlasSprite& sprite) const
{
    for (int i = 0; i < 4; ++i)
        lockVertexUv(i, faceData, facing, faceUV, sprite);
}

void FaceBakery::applyFacing(std::array<int32_t, 28>& faceData, EnumFacing facing) const
{
    const std::array<int32_t, 28> aint = faceData;
    float bounds[6];
    bounds[WEST_INDEX] = 999.0f;
    bounds[DOWN_INDEX] = 999.0f;
    bounds[NORTH_INDEX] = 999.0f;
    bounds[EAST_INDEX] = -999.0f;
    bounds[UP_INDEX] = -999.0f;
    bounds[SOUTH_INDEX] = -999.0f;

    for (int i = 0; i < 4; ++i) {
        const size_t j = static_cast<size_t>(7 * i);
        const float f = bitsToFloat(aint[j]);
        const float f1 = bitsToFloat(aint[j + 1]);
        const float f2 = bitsToFloat(aint[j + 2]);
        if (f < bounds[WEST_INDEX])
            bounds[WEST_INDEX] = f;
        if (f1 < bounds[DOWN_INDEX])
            bounds[DOWN_INDEX] = f1;
        if (f2 < bounds[NORTH_INDEX])
            bounds[NORTH_INDEX] = f2;
        if (f > bounds[EAST_INDEX])
            bounds[EAST_INDEX] = f;
        if (f1 > bounds[UP_INDEX])
            bounds[UP_INDEX] = f1;
        if (f2 > bounds[SOUTH_INDEX])
            bounds[SOUTH_INDEX] = f2;
    }

    for (int i1 = 0; i1 < 4; ++i1) {
        const size_t j1 = static_cast<size_t>(7 * i1);
        const VertexInformation& info = getVertexInformation(facing, i1);
        const float f8 = bounds[info.xIndex];
        const float f3 = bounds[info.yIndex];
        const float f4 = bounds[info.zIndex];
        faceData[j1] = floatToBits(f8);
        faceData[j1 + 1] = floatToBits(f3);
        faceData[j1 + 2] = floatToBits(f4);
        for (int k = 0; k < 4; ++k) {
            const size_t l = static_cast<size_t>(7 * k);
            const float f5 = bitsToFloat(aint[l]);
            const float f6 = bitsToFloat(aint[l + 1]);
            const float f7 = bitsToFloat(aint[l + 2]);
            if (MathHelper::epsilonEquals(f8, f5) && MathHelper::epsilonEquals(f3, f6) && MathHelper::epsilonEquals(f4, f7)) {
                faceData[j1 + 4] = aint[l + 4];
                faceData[j1 + 4 + 1] = aint[l + 4 + 1];
            }
        }
    }
}

void FaceBakery::lockVertexUv(int vertex, std::array<int32_t, 28>& faceData, EnumFacing facing, const BlockFaceUV& faceUV,
    const TextureAtlasSprite& sprite) const
{
    const size_t i = static_cast<size_t>(7 * vertex);
    float f = bitsToFloat(faceData[i]);
    float f1 = bitsToFloat(faceData[i + 1]);
    float f2 = bitsToFloat(faceData[i + 2]);
    if (f < -0.1f || f >= 1.1f)
        f -= static_cast<float>(MathHelper::floor_float(f));
    if (f1 < -0.1f || f1 >= 1.1f)
        f1 -= static_cast<float>(MathHelper::floor_float(f1));
    if (f2 < -0.1f || f2 >= 1.1f)
        f2 -= static_cast<float>(MathHelper::floor_float(f2));
    float f3;
    float f4;
    faceUv(facing, f, f1, f2, f3, f4);
    const size_t j = static_cast<size_t>(faceUV.func_178345_c(vertex) * 7);
    faceData[j + 4] = floatToBits(sprite.getInterpolatedU(static_cast<double>(f3)));
    faceData[j + 4 + 1] = floatToBits(sprite.getInterpolatedV(static_cast<double>(f4)));
}

} // namespace mc
