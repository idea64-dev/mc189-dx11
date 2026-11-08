#include "world/gen/structure/StructureComponent.h"

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "item/ItemMisc.h"
#include "nbt/NBTTagCompound.h"
#include "tileentity/TileEntities.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/gen/structure/MapGenStructure.h"

#include <algorithm>

namespace mc {

// --- StructureBoundingBox ---

StructureBoundingBox StructureBoundingBox::getComponentToAddBoundingBox(int x, int y, int z, int xMin, int yMin, int zMin, int xMax, int yMax, int zMax,
    EnumFacing facing)
{
    switch (facing) {
    case EnumFacing::NORTH:
        return StructureBoundingBox(x + xMin, y + yMin, z - zMax + 1 + zMin, x + xMax - 1 + xMin, y + yMax - 1 + yMin, z + zMin);
    case EnumFacing::SOUTH:
        return StructureBoundingBox(x + xMin, y + yMin, z + zMin, x + xMax - 1 + xMin, y + yMax - 1 + yMin, z + zMax - 1 + zMin);
    case EnumFacing::WEST:
        return StructureBoundingBox(x - zMax + 1 + zMin, y + yMin, z + xMin, x + zMin, y + yMax - 1 + yMin, z + xMax - 1 + xMin);
    case EnumFacing::EAST:
        return StructureBoundingBox(x + zMin, y + yMin, z + xMin, x + zMax - 1 + zMin, y + yMax - 1 + yMin, z + xMax - 1 + xMin);
    default:
        return StructureBoundingBox(x + xMin, y + yMin, z + zMin, x + xMax - 1 + xMin, y + yMax - 1 + yMin, z + zMax - 1 + zMin);
    }
}

std::string StructureBoundingBox::toString() const
{
    return "StructureBoundingBox{x0=" + std::to_string(minX) + ", y0=" + std::to_string(minY) + ", z0=" + std::to_string(minZ) + ", x1=" + std::to_string(maxX)
        + ", y1=" + std::to_string(maxY) + ", z1=" + std::to_string(maxZ) + "}";
}

std::shared_ptr<NBTTagIntArray> StructureBoundingBox::toNBTTagIntArray() const
{
    return std::make_shared<NBTTagIntArray>(std::vector<int32_t> { minX, minY, minZ, maxX, maxY, maxZ });
}

// --- StructureComponent ---

bool StructureComponent::isMaterial(const IBlockState* state, const Material& material)
{
    return &state->getBlock()->getMaterial() == &material;
}

StructureComponent::BlockSelector::BlockSelector() = default;

const IBlockState* StructureComponent::BlockSelector::getBlockState() const
{
    return blockstate ? blockstate : Blocks::air->getDefaultState();
}

std::shared_ptr<NBTTagCompound> StructureComponent::createStructureBaseNBT() const
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    nbttagcompound->setString("id", std::string(getStructureName()));
    nbttagcompound->setTag("BB", boundingBox.toNBTTagIntArray());
    nbttagcompound->setInteger("O", coordBaseMode ? getHorizontalIndex(*coordBaseMode) : -1);
    nbttagcompound->setInteger("GD", componentType);
    writeStructureToNBT(*nbttagcompound);
    return nbttagcompound;
}

void StructureComponent::readStructureBaseNBT(World*, const NBTTagCompound& tagCompound)
{
    if (tagCompound.hasKey("BB"))
        boundingBox = StructureBoundingBox(tagCompound.getIntArray("BB"));
    const int i = tagCompound.getInteger("O");
    if (i == -1)
        coordBaseMode.reset();
    else
        coordBaseMode = getHorizontal(i);
    componentType = tagCompound.getInteger("GD");
    readStructureFromNBT(tagCompound);
}

void StructureComponent::buildComponent(StructureComponent*, StructureComponentList&, Random&)
{
}

std::shared_ptr<StructureComponent> StructureComponent::findIntersecting(const StructureComponentList& listIn, const StructureBoundingBox& boundingboxIn)
{
    for (const auto& structurecomponent : listIn) {
        if (structurecomponent && structurecomponent->getBoundingBox().intersectsWith(boundingboxIn))
            return structurecomponent;
    }
    return nullptr;
}

bool StructureComponent::isLiquidInStructureBoundingBox(World* worldIn, const StructureBoundingBox& boundingboxIn) const
{
    const int i = std::max(boundingBox.minX - 1, boundingboxIn.minX);
    const int j = std::max(boundingBox.minY - 1, boundingboxIn.minY);
    const int k = std::max(boundingBox.minZ - 1, boundingboxIn.minZ);
    const int l = std::min(boundingBox.maxX + 1, boundingboxIn.maxX);
    const int i1 = std::min(boundingBox.maxY + 1, boundingboxIn.maxY);
    const int j1 = std::min(boundingBox.maxZ + 1, boundingboxIn.maxZ);
    BlockPos pos;
    const auto liquid = [&](int x, int y, int z) { return worldIn->getBlockState(pos.set(x, y, z))->getBlock()->getMaterial().isLiquid(); };
    for (int k1 = i; k1 <= l; ++k1) {
        for (int l1 = k; l1 <= j1; ++l1) {
            if (liquid(k1, j, l1))
                return true;
            if (liquid(k1, i1, l1))
                return true;
        }
    }
    for (int i2 = i; i2 <= l; ++i2) {
        for (int k2 = j; k2 <= i1; ++k2) {
            if (liquid(i2, k2, k))
                return true;
            if (liquid(i2, k2, j1))
                return true;
        }
    }
    for (int j2 = k; j2 <= j1; ++j2) {
        for (int l2 = j; l2 <= i1; ++l2) {
            if (liquid(i, l2, j2))
                return true;
            if (liquid(l, l2, j2))
                return true;
        }
    }
    return false;
}

int StructureComponent::getXWithOffset(int x, int z) const
{
    if (!coordBaseMode)
        return x;
    switch (*coordBaseMode) {
    case EnumFacing::NORTH:
    case EnumFacing::SOUTH:
        return boundingBox.minX + x;
    case EnumFacing::WEST:
        return boundingBox.maxX - z;
    case EnumFacing::EAST:
        return boundingBox.minX + z;
    default:
        return x;
    }
}

int StructureComponent::getYWithOffset(int y) const
{
    return !coordBaseMode ? y : y + boundingBox.minY;
}

int StructureComponent::getZWithOffset(int x, int z) const
{
    if (!coordBaseMode)
        return z;
    switch (*coordBaseMode) {
    case EnumFacing::NORTH:
        return boundingBox.maxZ - z;
    case EnumFacing::SOUTH:
        return boundingBox.minZ + z;
    case EnumFacing::WEST:
    case EnumFacing::EAST:
        return boundingBox.minZ + x;
    default:
        return z;
    }
}

int StructureComponent::getMetadataWithOffset(const Block* blockIn, int meta) const
{
    const auto mode = coordBaseMode;
    const auto is = [&](EnumFacing f) { return mode && *mode == f; };
    constexpr int N = getIndex(EnumFacing::NORTH);
    constexpr int S = getIndex(EnumFacing::SOUTH);
    constexpr int W = getIndex(EnumFacing::WEST);
    constexpr int E = getIndex(EnumFacing::EAST);
    if (blockIn == Blocks::rail) {
        if (is(EnumFacing::WEST) || is(EnumFacing::EAST)) {
            if (meta == 1)
                return 0;
            return 1;
        }
    } else if (dynamic_cast<const BlockDoor*>(blockIn)) {
        if (is(EnumFacing::SOUTH)) {
            if (meta == 0)
                return 2;
            if (meta == 2)
                return 0;
        } else {
            if (is(EnumFacing::WEST))
                return (meta + 1) & 3;
            if (is(EnumFacing::EAST))
                return (meta + 3) & 3;
        }
    } else if (blockIn != Blocks::stone_stairs && blockIn != Blocks::oak_stairs && blockIn != Blocks::nether_brick_stairs && blockIn != Blocks::stone_brick_stairs
        && blockIn != Blocks::sandstone_stairs) {
        if (blockIn == Blocks::ladder) {
            if (is(EnumFacing::SOUTH)) {
                if (meta == N)
                    return S;
                if (meta == S)
                    return N;
            } else if (is(EnumFacing::WEST)) {
                if (meta == N)
                    return W;
                if (meta == S)
                    return E;
                if (meta == W)
                    return N;
                if (meta == E)
                    return S;
            } else if (is(EnumFacing::EAST)) {
                if (meta == N)
                    return E;
                if (meta == S)
                    return W;
                if (meta == W)
                    return N;
                if (meta == E)
                    return S;
            }
        } else if (blockIn == Blocks::stone_button) {
            if (is(EnumFacing::SOUTH)) {
                if (meta == 3)
                    return 4;
                if (meta == 4)
                    return 3;
            } else if (is(EnumFacing::WEST)) {
                if (meta == 3)
                    return 1;
                if (meta == 4)
                    return 2;
                if (meta == 2)
                    return 3;
                if (meta == 1)
                    return 4;
            } else if (is(EnumFacing::EAST)) {
                if (meta == 3)
                    return 2;
                if (meta == 4)
                    return 1;
                if (meta == 2)
                    return 3;
                if (meta == 1)
                    return 4;
            }
        } else if (blockIn != Blocks::tripwire_hook && !dynamic_cast<const BlockDirectional*>(blockIn)) {
            if (blockIn == Blocks::piston || blockIn == Blocks::sticky_piston || blockIn == Blocks::lever || blockIn == Blocks::dispenser) {
                if (is(EnumFacing::SOUTH)) {
                    if (meta == N || meta == S)
                        return getIndex(getOpposite(getFront(meta)));
                } else if (is(EnumFacing::WEST)) {
                    if (meta == N)
                        return W;
                    if (meta == S)
                        return E;
                    if (meta == W)
                        return N;
                    if (meta == E)
                        return S;
                } else if (is(EnumFacing::EAST)) {
                    if (meta == N)
                        return E;
                    if (meta == S)
                        return W;
                    if (meta == W)
                        return N;
                    if (meta == E)
                        return S;
                }
            }
        } else {
            const EnumFacing enumfacing = getHorizontal(meta);
            if (is(EnumFacing::SOUTH)) {
                if (enumfacing == EnumFacing::SOUTH || enumfacing == EnumFacing::NORTH)
                    return getHorizontalIndex(getOpposite(enumfacing));
            } else if (is(EnumFacing::WEST)) {
                if (enumfacing == EnumFacing::NORTH)
                    return getHorizontalIndex(EnumFacing::WEST);
                if (enumfacing == EnumFacing::SOUTH)
                    return getHorizontalIndex(EnumFacing::EAST);
                if (enumfacing == EnumFacing::WEST)
                    return getHorizontalIndex(EnumFacing::NORTH);
                if (enumfacing == EnumFacing::EAST)
                    return getHorizontalIndex(EnumFacing::SOUTH);
            } else if (is(EnumFacing::EAST)) {
                if (enumfacing == EnumFacing::NORTH)
                    return getHorizontalIndex(EnumFacing::EAST);
                if (enumfacing == EnumFacing::SOUTH)
                    return getHorizontalIndex(EnumFacing::WEST);
                if (enumfacing == EnumFacing::WEST)
                    return getHorizontalIndex(EnumFacing::NORTH);
                if (enumfacing == EnumFacing::EAST)
                    return getHorizontalIndex(EnumFacing::SOUTH);
            }
        }
    } else if (is(EnumFacing::SOUTH)) {
        if (meta == 2)
            return 3;
        if (meta == 3)
            return 2;
    } else if (is(EnumFacing::WEST)) {
        if (meta == 0)
            return 2;
        if (meta == 1)
            return 3;
        if (meta == 2)
            return 0;
        if (meta == 3)
            return 1;
    } else if (is(EnumFacing::EAST)) {
        if (meta == 0)
            return 2;
        if (meta == 1)
            return 3;
        if (meta == 2)
            return 1;
        if (meta == 3)
            return 0;
    }
    return meta;
}

void StructureComponent::setBlockState(World* worldIn, const IBlockState* blockstateIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn)
{
    const BlockPos blockpos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    if (boundingboxIn.isVecInside(blockpos))
        worldIn->setBlockState(blockpos, blockstateIn, 2);
}

const IBlockState* StructureComponent::getBlockStateFromPos(World* worldIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn) const
{
    const BlockPos blockpos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    return !boundingboxIn.isVecInside(blockpos) ? Blocks::air->getDefaultState() : worldIn->getBlockState(blockpos);
}

void StructureComponent::fillWithAir(World* worldIn, const StructureBoundingBox& structurebb, int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
{
    for (int i = minY; i <= maxY; ++i) {
        for (int j = minX; j <= maxX; ++j) {
            for (int k = minZ; k <= maxZ; ++k)
                setBlockState(worldIn, Blocks::air->getDefaultState(), j, i, k, structurebb);
        }
    }
}

void StructureComponent::fillWithBlocks(World* worldIn, const StructureBoundingBox& boundingboxIn, int xMin, int yMin, int zMin, int xMax, int yMax, int zMax,
    const IBlockState* boundaryBlockState, const IBlockState* insideBlockState, bool existingOnly)
{
    for (int i = yMin; i <= yMax; ++i) {
        for (int j = xMin; j <= xMax; ++j) {
            for (int k = zMin; k <= zMax; ++k) {
                if (!existingOnly || &getBlockStateFromPos(worldIn, j, i, k, boundingboxIn)->getBlock()->getMaterial() != &Material::air) {
                    if (i != yMin && i != yMax && j != xMin && j != xMax && k != zMin && k != zMax)
                        setBlockState(worldIn, insideBlockState, j, i, k, boundingboxIn);
                    else
                        setBlockState(worldIn, boundaryBlockState, j, i, k, boundingboxIn);
                }
            }
        }
    }
}

void StructureComponent::fillWithRandomizedBlocks(World* worldIn, const StructureBoundingBox& boundingboxIn, int minX, int minY, int minZ, int maxX, int maxY,
    int maxZ, bool alwaysReplace, Random& rand, BlockSelector& blockselector)
{
    for (int i = minY; i <= maxY; ++i) {
        for (int j = minX; j <= maxX; ++j) {
            for (int k = minZ; k <= maxZ; ++k) {
                if (!alwaysReplace || &getBlockStateFromPos(worldIn, j, i, k, boundingboxIn)->getBlock()->getMaterial() != &Material::air) {
                    blockselector.selectBlocks(rand, j, i, k, i == minY || i == maxY || j == minX || j == maxX || k == minZ || k == maxZ);
                    setBlockState(worldIn, blockselector.getBlockState(), j, i, k, boundingboxIn);
                }
            }
        }
    }
}

void StructureComponent::func_175805_a(World* worldIn, const StructureBoundingBox& boundingboxIn, Random& rand, float chance, int minX, int minY, int minZ,
    int maxX, int maxY, int maxZ, const IBlockState* blockstate1, const IBlockState* blockstate2, bool existingOnly)
{
    for (int i = minY; i <= maxY; ++i) {
        for (int j = minX; j <= maxX; ++j) {
            for (int k = minZ; k <= maxZ; ++k) {
                if (rand.nextFloat() <= chance
                    && (!existingOnly || &getBlockStateFromPos(worldIn, j, i, k, boundingboxIn)->getBlock()->getMaterial() != &Material::air)) {
                    if (i != minY && i != maxY && j != minX && j != maxX && k != minZ && k != maxZ)
                        setBlockState(worldIn, blockstate2, j, i, k, boundingboxIn);
                    else
                        setBlockState(worldIn, blockstate1, j, i, k, boundingboxIn);
                }
            }
        }
    }
}

void StructureComponent::randomlyPlaceBlock(World* worldIn, const StructureBoundingBox& boundingboxIn, Random& rand, float chance, int x, int y, int z,
    const IBlockState* blockstateIn)
{
    if (rand.nextFloat() < chance)
        setBlockState(worldIn, blockstateIn, x, y, z, boundingboxIn);
}

void StructureComponent::randomlyRareFillWithBlocks(World* worldIn, const StructureBoundingBox& boundingboxIn, int minX, int minY, int minZ, int maxX, int maxY,
    int maxZ, const IBlockState* blockstateIn, bool existingOnly)
{
    const float f = static_cast<float>(maxX - minX + 1);
    const float f1 = static_cast<float>(maxY - minY + 1);
    const float f2 = static_cast<float>(maxZ - minZ + 1);
    const float f3 = static_cast<float>(minX) + f / 2.0f;
    const float f4 = static_cast<float>(minZ) + f2 / 2.0f;
    for (int i = minY; i <= maxY; ++i) {
        const float f5 = static_cast<float>(i - minY) / f1;
        for (int j = minX; j <= maxX; ++j) {
            const float f6 = (static_cast<float>(j) - f3) / (f * 0.5f);
            for (int k = minZ; k <= maxZ; ++k) {
                const float f7 = (static_cast<float>(k) - f4) / (f2 * 0.5f);
                if (!existingOnly || &getBlockStateFromPos(worldIn, j, i, k, boundingboxIn)->getBlock()->getMaterial() != &Material::air) {
                    const float f8 = f6 * f6 + f5 * f5 + f7 * f7;
                    if (f8 <= 1.05f)
                        setBlockState(worldIn, blockstateIn, j, i, k, boundingboxIn);
                }
            }
        }
    }
}

void StructureComponent::clearCurrentPositionBlocksUpwards(World* worldIn, int x, int y, int z, const StructureBoundingBox& structurebb)
{
    BlockPos blockpos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    if (structurebb.isVecInside(blockpos)) {
        while (!worldIn->isAirBlock(blockpos) && blockpos.getY() < 255) {
            worldIn->setBlockState(blockpos, Blocks::air->getDefaultState(), 2);
            blockpos = blockpos.up();
        }
    }
}

void StructureComponent::replaceAirAndLiquidDownwards(World* worldIn, const IBlockState* blockstateIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn)
{
    const int i = getXWithOffset(x, z);
    int j = getYWithOffset(y);
    const int k = getZWithOffset(x, z);
    if (boundingboxIn.isVecInside(BlockPos(i, j, k))) {
        while ((worldIn->isAirBlock(BlockPos(i, j, k)) || worldIn->getBlockState(BlockPos(i, j, k))->getBlock()->getMaterial().isLiquid()) && j > 1) {
            worldIn->setBlockState(BlockPos(i, j, k), blockstateIn, 2);
            --j;
        }
    }
}

bool StructureComponent::generateChestContents(World* worldIn, const StructureBoundingBox& boundingBoxIn, Random& rand, int x, int y, int z,
    const std::vector<WeightedRandomChestContent>& listIn, int max)
{
    const BlockPos blockpos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    if (boundingBoxIn.isVecInside(blockpos) && worldIn->getBlockState(blockpos)->getBlock() != Blocks::chest) {
        const IBlockState* iblockstate = Blocks::chest->getDefaultState();
        worldIn->setBlockState(blockpos, Blocks::chest->correctFacing(worldIn, blockpos, iblockstate), 2);
        if (auto* chest = dynamic_cast<TileEntityChest*>(worldIn->getTileEntity(blockpos)))
            WeightedRandomChestContent::generateChestContents(rand, listIn, *chest, max);
        return true;
    }
    return false;
}

bool StructureComponent::generateDispenserContents(World* worldIn, const StructureBoundingBox& boundingBoxIn, Random& rand, int x, int y, int z, int meta,
    const std::vector<WeightedRandomChestContent>& listIn, int max)
{
    const BlockPos blockpos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    if (boundingBoxIn.isVecInside(blockpos) && worldIn->getBlockState(blockpos)->getBlock() != Blocks::dispenser) {
        worldIn->setBlockState(blockpos, Blocks::dispenser->getStateFromMeta(getMetadataWithOffset(Blocks::dispenser, meta)), 2);
        if (auto* dispenser = dynamic_cast<TileEntityDispenser*>(worldIn->getTileEntity(blockpos)))
            WeightedRandomChestContent::generateDispenserContents(rand, listIn, *dispenser, max);
        return true;
    }
    return false;
}

void StructureComponent::placeDoorCurrentPosition(World* worldIn, const StructureBoundingBox& boundingBoxIn, Random&, int x, int y, int z, EnumFacing facing)
{
    const BlockPos blockpos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    if (boundingBoxIn.isVecInside(blockpos))
        ItemDoor::placeDoor(worldIn, blockpos, rotateYCCW(facing), Blocks::oak_door);
}

// --- StructureStart ---

void StructureStart::generateStructure(World* worldIn, Random& rand, const StructureBoundingBox& structurebb)
{
    for (auto it = components.begin(); it != components.end();) {
        StructureComponent* structurecomponent = it->get();
        if (structurecomponent->getBoundingBox().intersectsWith(structurebb) && !structurecomponent->addComponentParts(worldIn, rand, structurebb))
            it = components.erase(it);
        else
            ++it;
    }
}

void StructureStart::updateBoundingBox()
{
    boundingBox = StructureBoundingBox::getNewBoundingBox();
    for (const auto& structurecomponent : components)
        boundingBox.expandTo(structurecomponent->getBoundingBox());
}

std::shared_ptr<NBTTagCompound> StructureStart::writeStructureComponentsToNBT(int chunkX, int chunkZ) const
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    nbttagcompound->setString("id", std::string(getStructureName()));
    nbttagcompound->setInteger("ChunkX", chunkX);
    nbttagcompound->setInteger("ChunkZ", chunkZ);
    nbttagcompound->setTag("BB", boundingBox.toNBTTagIntArray());
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (const auto& structurecomponent : components)
        nbttaglist->appendTag(structurecomponent->createStructureBaseNBT());
    nbttagcompound->setTag("Children", nbttaglist);
    writeToNBT(*nbttagcompound);
    return nbttagcompound;
}

void StructureStart::readStructureComponentsFromNBT(World* worldIn, const NBTTagCompound& tagCompound)
{
    chunkPosX = tagCompound.getInteger("ChunkX");
    chunkPosZ = tagCompound.getInteger("ChunkZ");
    if (tagCompound.hasKey("BB"))
        boundingBox = StructureBoundingBox(tagCompound.getIntArray("BB"));
    const auto nbttaglist = tagCompound.getTagList("Children", 10);
    for (int i = 0; i < nbttaglist->tagCount(); ++i) {
        // Java adds null for unknown pieces, which would throw later; they are skipped here.
        if (auto component = MapGenStructureIO::getStructureComponent(*nbttaglist->getCompoundTagAt(i), worldIn))
            components.push_back(std::move(component));
    }
    readFromNBT(tagCompound);
}

void StructureStart::markAvailableHeight(World* worldIn, Random& rand, int minDepth)
{
    const int i = worldIn->getSeaLevel() - minDepth;
    int j = boundingBox.getYSize() + 1;
    if (j < i)
        j += rand.nextInt(i - j);
    const int k = j - boundingBox.maxY;
    boundingBox.offset(0, k, 0);
    for (const auto& structurecomponent : components)
        structurecomponent->func_181138_a(0, k, 0);
}

void StructureStart::setRandomHeight(World*, Random& rand, int minY, int maxY)
{
    const int i = maxY - minY + 1 - boundingBox.getYSize();
    int j = 1;
    if (i > 1)
        j = minY + rand.nextInt(i);
    else
        j = minY;
    const int k = j - boundingBox.minY;
    boundingBox.offset(0, k, 0);
    for (const auto& structurecomponent : components)
        structurecomponent->func_181138_a(0, k, 0);
}

} // namespace mc
