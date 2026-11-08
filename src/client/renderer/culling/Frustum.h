#pragma once

// Original: net/minecraft/client/renderer/culling/ICamera.java, ClippingHelper.java, ClippingHelperImpl.java and
// Frustum.java -- the view frustum planes from the current projection and modelview matrices.

#include <array>

namespace mc {

struct AxisAlignedBB;

class ICamera {
public:
    virtual ~ICamera() = default;
    virtual bool isBoundingBoxInFrustum(const AxisAlignedBB& bb) = 0;
    virtual void setPosition(double x, double y, double z) = 0;
};

class ClippingHelper {
public:
    virtual ~ClippingHelper() = default;

    bool isBoxInFrustum(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) const;

    std::array<std::array<float, 4>, 6> frustum {};
    std::array<float, 16> projectionMatrix {};
    std::array<float, 16> modelviewMatrix {};
    std::array<float, 16> clippingMatrix {};
};

class ClippingHelperImpl : public ClippingHelper {
public:
    // Java: getInstance() -- re-reads the matrices into the shared instance.
    static ClippingHelper& getInstance();
    void init();
};

class Frustum : public ICamera {
public:
    // Java: new Frustum() -- uses ClippingHelperImpl.getInstance(), refreshing it.
    Frustum();
    explicit Frustum(const ClippingHelper& clippingHelperIn);

    void setPosition(double x, double y, double z) override;
    bool isBoxInFrustum(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) const;
    bool isBoundingBoxInFrustum(const AxisAlignedBB& bb) override;

private:
    const ClippingHelper* clippingHelper;
    double xPosition = 0.0;
    double yPosition = 0.0;
    double zPosition = 0.0;
};

} // namespace mc
