// BlockPos, EnumFacing, Vec3 and AxisAlignedBB against the original (tests/reference/MathReference.inc from
// tools/reference/generate_game.ps1), bit for bit.

#include "core/JavaMath.h"
#include "core/StrictMath.h"
#include "util/Vec3.h"

#include "reference/MathReference.inc"

#include <doctest/doctest.h>

#include <bit>
#include <vector>

using namespace mc;

namespace {

double bits(unsigned long long value)
{
    return std::bit_cast<double>(value);
}

unsigned long long raw(double value)
{
    return std::bit_cast<unsigned long long>(value);
}

} // namespace

TEST_CASE("BlockPos packing and hashing")
{
    for (const auto& c : blockPosCases) {
        const BlockPos pos(c.x, c.y, c.z);
        CHECK(pos.toLong() == c.packed);
        CHECK(pos.hashCode() == c.hash);
        CHECK(BlockPos::fromLong(c.packed) == BlockPos(c.ux, c.uy, c.uz));
    }
}

TEST_CASE("BlockPos box iteration runs x, then y, then z")
{
    std::vector<BlockPos> visited;
    for (const BlockPos& pos : BlockPos::getAllInBox(BlockPos(1, 5, -1), BlockPos(0, 4, 0)))
        visited.push_back(pos);
    REQUIRE(visited.size() == 8);
    CHECK(visited.front() == BlockPos(0, 4, -1));
    CHECK(visited[1] == BlockPos(1, 4, -1));
    CHECK(visited[2] == BlockPos(0, 5, -1));
    CHECK(visited.back() == BlockPos(1, 5, 0));
    int single = 0;
    for ([[maybe_unused]] const BlockPos& pos : BlockPos::getAllInBox(BlockPos(3, 3, 3), BlockPos(3, 3, 3)))
        ++single;
    CHECK(single == 1);
}

TEST_CASE("EnumFacing helpers")
{
    for (const auto& c : facingAngleCases)
        CHECK(ordinal(fromAngle(bits(c.angle))) == c.facing);
    for (const auto& c : facingVectorCases)
        CHECK(ordinal(getFacingFromVector(c.x, c.y, c.z)) == c.facing);
    CHECK(getFront(-7) == EnumFacing::UP);
    CHECK(getHorizontal(6) == EnumFacing::NORTH);
    CHECK(getOpposite(EnumFacing::WEST) == EnumFacing::EAST);
    CHECK(rotateAround(EnumFacing::UP, Axis::Z) == EnumFacing::EAST);
    CHECK(facingByName("NoRtH") == EnumFacing::NORTH);
    CHECK_FALSE(facingByName("left"));
    CHECK_THROWS(rotateY(EnumFacing::UP));
}

TEST_CASE("Vec3 normalize and rotations")
{
    for (const auto& c : vec3Cases) {
        const Vec3 v(bits(c.x), bits(c.y), bits(c.z));
        const Vec3 n = v.normalize();
        CHECK(raw(n.xCoord) == c.nx);
        CHECK(raw(n.yCoord) == c.ny);
        CHECK(raw(n.zCoord) == c.nz);
        const Vec3 p = v.rotatePitch(c.angle);
        CHECK(raw(p.xCoord) == c.px);
        CHECK(raw(p.yCoord) == c.py);
        CHECK(raw(p.zCoord) == c.pz);
        const Vec3 y = v.rotateYaw(c.angle);
        CHECK(raw(y.xCoord) == c.yx);
        CHECK(raw(y.yCoord) == c.yy);
        CHECK(raw(y.zCoord) == c.yz);
        CHECK(raw(v.lengthVector()) == c.len);
    }
}

TEST_CASE("AxisAlignedBB intercepts and collision offsets")
{
    for (const auto& c : interceptCases) {
        const AxisAlignedBB box(bits(c.box[0]), bits(c.box[1]), bits(c.box[2]), bits(c.box[3]), bits(c.box[4]), bits(c.box[5]));
        const Vec3 a(bits(c.a[0]), bits(c.a[1]), bits(c.a[2]));
        const Vec3 b(bits(c.b[0]), bits(c.b[1]), bits(c.b[2]));
        const auto hit = box.calculateIntercept(a, b);
        if (c.side < 0) {
            CHECK_FALSE(hit);
        } else {
            REQUIRE(hit);
            CHECK(ordinal(*hit->sideHit) == c.side);
            CHECK(raw(hit->hitVec.xCoord) == c.hit[0]);
            CHECK(raw(hit->hitVec.yCoord) == c.hit[1]);
            CHECK(raw(hit->hitVec.zCoord) == c.hit[2]);
        }
        const AxisAlignedBB other(a.xCoord, a.yCoord, a.zCoord, a.xCoord + 0.6, a.yCoord + 1.8, a.zCoord + 0.6);
        const double move = bits(c.move);
        CHECK(raw(box.calculateXOffset(other, move)) == c.offsets[0]);
        CHECK(raw(box.calculateYOffset(other, move)) == c.offsets[1]);
        CHECK(raw(box.calculateZOffset(other, move)) == c.offsets[2]);
    }
}

TEST_CASE("StrictMath sin and cos match fdlibm and Math")
{
    for (const auto& c : trigCases) {
        const double x = std::bit_cast<double>(c.x);
        CAPTURE(x);
        CHECK(std::bit_cast<unsigned long long>(StrictMath::sin(x)) == c.sin);
        CHECK(std::bit_cast<unsigned long long>(StrictMath::cos(x)) == c.cos);
        CHECK(std::bit_cast<unsigned long long>(java::mathSin(x)) == c.mathSin);
        CHECK(std::bit_cast<unsigned long long>(java::mathCos(x)) == c.mathCos);
    }
}
