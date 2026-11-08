#include "client/renderer/chunk/VisGraph.h"

#include <array>
#include <deque>

namespace mc {

void SetVisibility::setManyVisible(FacingSet facings)
{
    for (EnumFacing enumfacing : kFacings) {
        if (!(facings & facingBit(enumfacing)))
            continue;
        for (EnumFacing enumfacing1 : kFacings) {
            if (facings & facingBit(enumfacing1))
                setVisible(enumfacing, enumfacing1, true);
        }
    }
}

void SetVisibility::setVisible(EnumFacing facing, EnumFacing facing2, bool visible)
{
    bits[static_cast<size_t>(ordinal(facing) + ordinal(facing2) * 6)] = visible;
    bits[static_cast<size_t>(ordinal(facing2) + ordinal(facing) * 6)] = visible;
}

void SetVisibility::setAllVisible(bool visible)
{
    if (visible)
        bits.set();
    else
        bits.reset();
}

namespace {

// Java: field_178613_e -- the 1352 positions on the faces of the section, x outermost.
const std::array<int, 1352>& faceIndices()
{
    static const std::array<int, 1352> indices = [] {
        std::array<int, 1352> result {};
        int k = 0;
        for (int l = 0; l < 16; ++l) {
            for (int i1 = 0; i1 < 16; ++i1) {
                for (int j1 = 0; j1 < 16; ++j1) {
                    if (l == 0 || l == 15 || i1 == 0 || i1 == 15 || j1 == 0 || j1 == 15)
                        result[static_cast<size_t>(k++)] = l << 0 | i1 << 8 | j1 << 4;
                }
            }
        }
        return result;
    }();
    return indices;
}

} // namespace

void VisGraph::setOpaque(const BlockPos& pos)
{
    opaque[static_cast<size_t>(getIndex(pos))] = true;
    --empty;
}

SetVisibility VisGraph::computeVisibility()
{
    SetVisibility setvisibility;
    if (4096 - empty < 256) {
        setvisibility.setAllVisible(true);
    } else if (empty == 0) {
        setvisibility.setAllVisible(false);
    } else {
        for (int i : faceIndices()) {
            if (!opaque[static_cast<size_t>(i)])
                setvisibility.setManyVisible(floodFill(i));
        }
    }
    return setvisibility;
}

SetVisibility::FacingSet VisGraph::getVisibleFacings(const BlockPos& pos)
{
    return floodFill(getIndex(pos));
}

SetVisibility::FacingSet VisGraph::floodFill(int index)
{
    SetVisibility::FacingSet set = 0;
    std::deque<int> queue;
    queue.push_back(index);
    opaque[static_cast<size_t>(index)] = true;

    while (!queue.empty()) {
        const int i = queue.front();
        queue.pop_front();
        addEdges(i, set);
        for (EnumFacing enumfacing : kFacings) {
            const int j = getNeighborIndexAtFace(i, enumfacing);
            if (j >= 0 && !opaque[static_cast<size_t>(j)]) {
                opaque[static_cast<size_t>(j)] = true;
                queue.push_back(j);
            }
        }
    }
    return set;
}

void VisGraph::addEdges(int index, SetVisibility::FacingSet& set)
{
    const int i = index >> 0 & 15;
    if (i == 0)
        set |= facingBit(EnumFacing::WEST);
    else if (i == 15)
        set |= facingBit(EnumFacing::EAST);

    const int j = index >> 8 & 15;
    if (j == 0)
        set |= facingBit(EnumFacing::DOWN);
    else if (j == 15)
        set |= facingBit(EnumFacing::UP);

    const int k = index >> 4 & 15;
    if (k == 0)
        set |= facingBit(EnumFacing::NORTH);
    else if (k == 15)
        set |= facingBit(EnumFacing::SOUTH);
}

int VisGraph::getNeighborIndexAtFace(int index, EnumFacing facing)
{
    switch (facing) {
    case EnumFacing::DOWN: return (index >> 8 & 15) == 0 ? -1 : index - 256;
    case EnumFacing::UP: return (index >> 8 & 15) == 15 ? -1 : index + 256;
    case EnumFacing::NORTH: return (index >> 4 & 15) == 0 ? -1 : index - 16;
    case EnumFacing::SOUTH: return (index >> 4 & 15) == 15 ? -1 : index + 16;
    case EnumFacing::WEST: return (index >> 0 & 15) == 0 ? -1 : index - 1;
    case EnumFacing::EAST: return (index >> 0 & 15) == 15 ? -1 : index + 1;
    }
    return -1;
}

} // namespace mc
