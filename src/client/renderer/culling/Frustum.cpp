#include "client/renderer/culling/Frustum.h"

#include "client/renderer/DxStateManager.h"
#include "util/MathHelper.h"
#include "util/Vec3.h"

namespace mc {

namespace {

double dot(const std::array<float, 4>& plane, double x, double y, double z)
{
    return static_cast<double>(plane[0]) * x + static_cast<double>(plane[1]) * y + static_cast<double>(plane[2]) * z + static_cast<double>(plane[3]);
}

void normalize(std::array<float, 4>& plane)
{
    const float f = MathHelper::sqrt_float(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
    plane[0] /= f;
    plane[1] /= f;
    plane[2] /= f;
    plane[3] /= f;
}

} // namespace

bool ClippingHelper::isBoxInFrustum(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) const
{
    // Java tests all eight corners; the corner furthest along the plane normal gives the largest of those eight
    // sums (rounding is monotonic, term by term), so it alone decides whether all are <= 0.
    for (const std::array<float, 4>& plane : frustum) {
        if (dot(plane, plane[0] > 0.0f ? maxX : minX, plane[1] > 0.0f ? maxY : minY, plane[2] > 0.0f ? maxZ : minZ) <= 0.0)
            return false;
    }
    return true;
}

ClippingHelper& ClippingHelperImpl::getInstance()
{
    static ClippingHelperImpl instance;
    instance.init();
    return instance;
}

void ClippingHelperImpl::init()
{
    DxStateManager::getFloat(2983, projectionMatrix.data());
    DxStateManager::getFloat(2982, modelviewMatrix.data());
    const auto& p = projectionMatrix;
    const auto& m = modelviewMatrix;
    auto& c = clippingMatrix;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            c[static_cast<size_t>(row * 4 + col)] = m[static_cast<size_t>(row * 4 + 0)] * p[static_cast<size_t>(0 + col)]
                + m[static_cast<size_t>(row * 4 + 1)] * p[static_cast<size_t>(4 + col)] + m[static_cast<size_t>(row * 4 + 2)] * p[static_cast<size_t>(8 + col)]
                + m[static_cast<size_t>(row * 4 + 3)] * p[static_cast<size_t>(12 + col)];
        }
    }

    // Planes: right, left, bottom, top, far, near -- column 3 minus/plus columns 0, 1, 2.
    const int columns[6] { 0, 0, 1, 1, 2, 2 };
    const float signs[6] { -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f };
    for (int i = 0; i < 6; ++i) {
        std::array<float, 4>& plane = frustum[static_cast<size_t>(i)];
        const int col = columns[i];
        for (int k = 0; k < 4; ++k) {
            const float w = c[static_cast<size_t>(k * 4 + 3)];
            const float v = c[static_cast<size_t>(k * 4 + col)];
            plane[static_cast<size_t>(k)] = signs[i] < 0.0f ? w - v : w + v;
        }
        normalize(plane);
    }
}

Frustum::Frustum()
    : clippingHelper(&ClippingHelperImpl::getInstance())
{
}

Frustum::Frustum(const ClippingHelper& clippingHelperIn)
    : clippingHelper(&clippingHelperIn)
{
}

void Frustum::setPosition(double x, double y, double z)
{
    xPosition = x;
    yPosition = y;
    zPosition = z;
}

bool Frustum::isBoxInFrustum(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) const
{
    return clippingHelper->isBoxInFrustum(minX - xPosition, minY - yPosition, minZ - zPosition, maxX - xPosition, maxY - yPosition, maxZ - zPosition);
}

bool Frustum::isBoundingBoxInFrustum(const AxisAlignedBB& bb)
{
    return isBoxInFrustum(bb.minX, bb.minY, bb.minZ, bb.maxX, bb.maxY, bb.maxZ);
}

} // namespace mc
