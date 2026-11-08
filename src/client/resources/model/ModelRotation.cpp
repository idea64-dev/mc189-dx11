#include "client/resources/model/ModelRotation.h"

#include "util/MathHelper.h"

namespace mc {

const ModelRotation ModelRotation::X0_Y0(0, 0, 0);
const ModelRotation ModelRotation::X0_Y90(1, 0, 90);
const ModelRotation ModelRotation::X0_Y180(2, 0, 180);
const ModelRotation ModelRotation::X0_Y270(3, 0, 270);
const ModelRotation ModelRotation::X90_Y0(4, 90, 0);
const ModelRotation ModelRotation::X90_Y90(5, 90, 90);
const ModelRotation ModelRotation::X90_Y180(6, 90, 180);
const ModelRotation ModelRotation::X90_Y270(7, 90, 270);
const ModelRotation ModelRotation::X180_Y0(8, 180, 0);
const ModelRotation ModelRotation::X180_Y90(9, 180, 90);
const ModelRotation ModelRotation::X180_Y180(10, 180, 180);
const ModelRotation ModelRotation::X180_Y270(11, 180, 270);
const ModelRotation ModelRotation::X270_Y0(12, 270, 0);
const ModelRotation ModelRotation::X270_Y90(13, 270, 90);
const ModelRotation ModelRotation::X270_Y180(14, 270, 180);
const ModelRotation ModelRotation::X270_Y270(15, 270, 270);

namespace {

int combineXY(int x, int y)
{
    return x * 360 + y;
}

} // namespace

ModelRotation::ModelRotation(int ordinalIn, int x, int y)
    : ordinalIndex(ordinalIn)
    , combinedXY(combineXY(x, y))
    , quartersX(MathHelper::abs_int(x / 90))
    , quartersY(MathHelper::abs_int(y / 90))
{
    const lwjgl::Matrix4f mx = lwjgl::Matrix4f::rotate(static_cast<float>(-x) * 0.017453292f, lwjgl::Vector3f(1.0f, 0.0f, 0.0f), lwjgl::Matrix4f());
    const lwjgl::Matrix4f my = lwjgl::Matrix4f::rotate(static_cast<float>(-y) * 0.017453292f, lwjgl::Vector3f(0.0f, 1.0f, 0.0f), lwjgl::Matrix4f());
    matrix4d = lwjgl::Matrix4f::mul(my, mx);
}

EnumFacing ModelRotation::rotateFace(EnumFacing facing) const
{
    EnumFacing enumfacing = facing;
    for (int i = 0; i < quartersX; ++i)
        enumfacing = rotateAround(enumfacing, Axis::X);
    if (getAxis(enumfacing) != Axis::Y) {
        for (int j = 0; j < quartersY; ++j)
            enumfacing = rotateAround(enumfacing, Axis::Y);
    }
    return enumfacing;
}

int ModelRotation::rotateVertex(EnumFacing facing, int vertexIndex) const
{
    int i = vertexIndex;
    if (getAxis(facing) == Axis::X)
        i = (vertexIndex + quartersX) % 4;
    EnumFacing enumfacing = facing;
    for (int j = 0; j < quartersX; ++j)
        enumfacing = rotateAround(enumfacing, Axis::X);
    if (getAxis(enumfacing) == Axis::Y)
        i = (i + quartersY) % 4;
    return i;
}

const ModelRotation* ModelRotation::getModelRotation(int x, int y)
{
    static const ModelRotation* const all[16] { &X0_Y0, &X0_Y90, &X0_Y180, &X0_Y270, &X90_Y0, &X90_Y90, &X90_Y180, &X90_Y270, &X180_Y0, &X180_Y90,
        &X180_Y180, &X180_Y270, &X270_Y0, &X270_Y90, &X270_Y180, &X270_Y270 };
    const int combined = combineXY(MathHelper::normalizeAngle(x, 360), MathHelper::normalizeAngle(y, 360));
    for (const ModelRotation* rotation : all) {
        if (rotation->combinedXY == combined)
            return rotation;
    }
    return nullptr;
}

} // namespace mc
