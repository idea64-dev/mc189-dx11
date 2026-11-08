#include "world/gen/structure/StructureMineshaftPieces.h"

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/EnumDyeColor.h"
#include "item/ItemMisc.h"
#include "nbt/NBTTagCompound.h"
#include "tileentity/TileEntities.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/gen/structure/MapGenMineshaft.h"
#include "world/gen/structure/MapGenStructure.h"

#include <cstdlib>

namespace mc {

namespace StructureMineshaftPieces {

namespace {

const std::vector<WeightedRandomChestContent>& chestContent()
{
    static const std::vector<WeightedRandomChestContent> list {
        { Items::iron_ingot, 0, 1, 5, 10 },
        { Items::gold_ingot, 0, 1, 3, 5 },
        { Items::redstone, 0, 4, 9, 5 },
        { Items::dye, getDyeDamage(EnumDyeColor::BLUE), 4, 9, 5 },
        { Items::diamond, 0, 1, 2, 3 },
        { Items::coal, 0, 3, 8, 10 },
        { Items::bread, 0, 1, 3, 15 },
        { Items::iron_pickaxe, 0, 1, 1, 1 },
        { Item::getItemFromBlock(Blocks::rail), 0, 4, 8, 1 },
        { Items::melon_seeds, 0, 2, 4, 10 },
        { Items::pumpkin_seeds, 0, 2, 4, 10 },
        { Items::saddle, 0, 1, 1, 3 },
        { Items::iron_horse_armor, 0, 1, 1, 1 },
    };
    return list;
}

std::shared_ptr<StructureComponent> func_175892_a(const StructureComponentList& listIn, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    const int i = rand.nextInt(100);
    if (i >= 80) {
        if (const auto bb = Cross::func_175813_a(listIn, rand, x, y, z, facing))
            return std::make_shared<Cross>(type, rand, *bb, facing);
    } else if (i >= 70) {
        if (const auto bb = Stairs::func_175812_a(listIn, rand, x, y, z, facing))
            return std::make_shared<Stairs>(type, rand, *bb, facing);
    } else {
        if (const auto bb = Corridor::func_175814_a(listIn, rand, x, y, z, facing))
            return std::make_shared<Corridor>(type, rand, *bb, facing);
    }
    return nullptr;
}

std::shared_ptr<StructureComponent> func_175890_b(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand, int x, int y, int z,
    EnumFacing facing, int type)
{
    if (type > 8)
        return nullptr;
    if (std::abs(x - componentIn->getBoundingBox().minX) <= 80 && std::abs(z - componentIn->getBoundingBox().minZ) <= 80) {
        std::shared_ptr<StructureComponent> structurecomponent = func_175892_a(listIn, rand, x, y, z, facing, type + 1);
        if (structurecomponent) {
            listIn.push_back(structurecomponent);
            structurecomponent->buildComponent(componentIn, listIn, rand);
        }
        return structurecomponent;
    }
    return nullptr;
}

// minY - 1 + rand.nextInt(3), drawn before the piece.
int nearY(Random& rand, int minY)
{
    return minY - 1 + rand.nextInt(3);
}

} // namespace

void registerStructurePieces()
{
    MapGenStructureIO::registerStructureComponent("MSCorridor", [] { return std::make_shared<Corridor>(); });
    MapGenStructureIO::registerStructureComponent("MSCrossing", [] { return std::make_shared<Cross>(); });
    MapGenStructureIO::registerStructureComponent("MSRoom", [] { return std::make_shared<Room>(); });
    MapGenStructureIO::registerStructureComponent("MSStairs", [] { return std::make_shared<Stairs>(); });
}

// --- Corridor ---

Corridor::Corridor(int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing)
    : StructureComponent(type)
{
    coordBaseMode = facing;
    boundingBox = structurebb;
    hasRails = rand.nextInt(3) == 0;
    hasSpiders = !hasRails && rand.nextInt(23) == 0;
    if (!isMode(EnumFacing::NORTH) && !isMode(EnumFacing::SOUTH))
        sectionCount = structurebb.getXSize() / 5;
    else
        sectionCount = structurebb.getZSize() / 5;
}

void Corridor::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    tagCompound.setBoolean("hr", hasRails);
    tagCompound.setBoolean("sc", hasSpiders);
    tagCompound.setBoolean("hps", spawnerPlaced);
    tagCompound.setInteger("Num", sectionCount);
}

void Corridor::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    hasRails = tagCompound.getBoolean("hr");
    hasSpiders = tagCompound.getBoolean("sc");
    spawnerPlaced = tagCompound.getBoolean("hps");
    sectionCount = tagCompound.getInteger("Num");
}

std::optional<StructureBoundingBox> Corridor::func_175814_a(const StructureComponentList& listIn, Random& rand, int x, int y, int z, EnumFacing facing)
{
    StructureBoundingBox structureboundingbox(x, y, z, x, y + 2, z);
    int i;
    for (i = rand.nextInt(3) + 2; i > 0; --i) {
        const int j = i * 5;
        switch (facing) {
        case EnumFacing::NORTH:
            structureboundingbox.maxX = x + 2;
            structureboundingbox.minZ = z - (j - 1);
            break;
        case EnumFacing::SOUTH:
            structureboundingbox.maxX = x + 2;
            structureboundingbox.maxZ = z + (j - 1);
            break;
        case EnumFacing::WEST:
            structureboundingbox.minX = x - (j - 1);
            structureboundingbox.maxZ = z + 2;
            break;
        case EnumFacing::EAST:
            structureboundingbox.maxX = x + (j - 1);
            structureboundingbox.maxZ = z + 2;
            break;
        default:
            break;
        }
        if (!findIntersecting(listIn, structureboundingbox))
            break;
    }
    if (i > 0)
        return structureboundingbox;
    return std::nullopt;
}

void Corridor::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    const int i = getComponentType();
    const int j = rand.nextInt(4);
    const StructureBoundingBox& bb = boundingBox;
    if (coordBaseMode) {
        switch (*coordBaseMode) {
        case EnumFacing::NORTH:
            if (j <= 1) {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.minX, y, bb.minZ - 1, *coordBaseMode, i);
            } else if (j == 2) {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.minX - 1, y, bb.minZ, EnumFacing::WEST, i);
            } else {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.maxX + 1, y, bb.minZ, EnumFacing::EAST, i);
            }
            break;
        case EnumFacing::SOUTH:
            if (j <= 1) {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.minX, y, bb.maxZ + 1, *coordBaseMode, i);
            } else if (j == 2) {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.minX - 1, y, bb.maxZ - 3, EnumFacing::WEST, i);
            } else {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.maxX + 1, y, bb.maxZ - 3, EnumFacing::EAST, i);
            }
            break;
        case EnumFacing::WEST:
            if (j <= 1) {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.minX - 1, y, bb.minZ, *coordBaseMode, i);
            } else if (j == 2) {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.minX, y, bb.minZ - 1, EnumFacing::NORTH, i);
            } else {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.minX, y, bb.maxZ + 1, EnumFacing::SOUTH, i);
            }
            break;
        case EnumFacing::EAST:
            if (j <= 1) {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.maxX + 1, y, bb.minZ, *coordBaseMode, i);
            } else if (j == 2) {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.maxX - 3, y, bb.minZ - 1, EnumFacing::NORTH, i);
            } else {
                const int y = nearY(rand, bb.minY);
                func_175890_b(componentIn, listIn, rand, bb.maxX - 3, y, bb.maxZ + 1, EnumFacing::SOUTH, i);
            }
            break;
        default:
            break;
        }
    }
    if (i < 8) {
        if (!isMode(EnumFacing::NORTH) && !isMode(EnumFacing::SOUTH)) {
            for (int i1 = boundingBox.minX + 3; i1 + 3 <= boundingBox.maxX; i1 += 5) {
                const int j1 = rand.nextInt(5);
                if (j1 == 0)
                    func_175890_b(componentIn, listIn, rand, i1, boundingBox.minY, boundingBox.minZ - 1, EnumFacing::NORTH, i + 1);
                else if (j1 == 1)
                    func_175890_b(componentIn, listIn, rand, i1, boundingBox.minY, boundingBox.maxZ + 1, EnumFacing::SOUTH, i + 1);
            }
        } else {
            for (int k = boundingBox.minZ + 3; k + 3 <= boundingBox.maxZ; k += 5) {
                const int l = rand.nextInt(5);
                if (l == 0)
                    func_175890_b(componentIn, listIn, rand, boundingBox.minX - 1, boundingBox.minY, k, EnumFacing::WEST, i + 1);
                else if (l == 1)
                    func_175890_b(componentIn, listIn, rand, boundingBox.maxX + 1, boundingBox.minY, k, EnumFacing::EAST, i + 1);
            }
        }
    }
}

bool Corridor::generateMinecartChest(World* worldIn, const StructureBoundingBox& boundingBoxIn, Random& rand, int x, int y, int z,
    const std::vector<WeightedRandomChestContent>& listIn, int max)
{
    const BlockPos blockpos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    if (boundingBoxIn.isVecInside(blockpos) && &worldIn->getBlockState(blockpos)->getBlock()->getMaterial() == &Material::air) {
        const int i = rand.nextBoolean() ? 1 : 0;
        worldIn->setBlockState(blockpos, Blocks::rail->getStateFromMeta(getMetadataWithOffset(Blocks::rail, i)), 2);
        // Java: an EntityMinecartChest at the rail, filled and spawned -- entities come with stage 10. A detached
        // 27-slot chest takes the loot, so the draws stay the same.
        TileEntityChest minecartInventory;
        WeightedRandomChestContent::generateChestContents(rand, listIn, minecartInventory, max);
        return true;
    }
    return false;
}

bool Corridor::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& sbb)
{
    if (isLiquidInStructureBoundingBox(worldIn, sbb))
        return false;
    const IBlockState* const air = Blocks::air->getDefaultState();
    const IBlockState* const web = Blocks::web->getDefaultState();
    const IBlockState* const planks = Blocks::planks->getDefaultState();
    const IBlockState* const fence = Blocks::oak_fence->getDefaultState();
    const IBlockState* const torch = Blocks::torch->getStateFromMeta(getIndex(EnumFacing::UP));
    const int i1 = sectionCount * 5 - 1;
    fillWithBlocks(worldIn, sbb, 0, 0, 0, 2, 1, i1, air, air, false);
    func_175805_a(worldIn, sbb, randomIn, 0.8f, 0, 2, 0, 2, 2, i1, air, air, false);
    if (hasSpiders)
        func_175805_a(worldIn, sbb, randomIn, 0.6f, 0, 0, 0, 2, 1, i1, web, air, false);
    for (int j1 = 0; j1 < sectionCount; ++j1) {
        const int k1 = 2 + j1 * 5;
        fillWithBlocks(worldIn, sbb, 0, 0, k1, 0, 1, k1, fence, air, false);
        fillWithBlocks(worldIn, sbb, 2, 0, k1, 2, 1, k1, fence, air, false);
        if (randomIn.nextInt(4) == 0) {
            fillWithBlocks(worldIn, sbb, 0, 2, k1, 0, 2, k1, planks, air, false);
            fillWithBlocks(worldIn, sbb, 2, 2, k1, 2, 2, k1, planks, air, false);
        } else {
            fillWithBlocks(worldIn, sbb, 0, 2, k1, 2, 2, k1, planks, air, false);
        }
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.1f, 0, 2, k1 - 1, web);
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.1f, 2, 2, k1 - 1, web);
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.1f, 0, 2, k1 + 1, web);
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.1f, 2, 2, k1 + 1, web);
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.05f, 0, 2, k1 - 2, web);
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.05f, 2, 2, k1 - 2, web);
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.05f, 0, 2, k1 + 2, web);
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.05f, 2, 2, k1 + 2, web);
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.05f, 1, 2, k1 - 1, torch);
        randomlyPlaceBlock(worldIn, sbb, randomIn, 0.05f, 1, 2, k1 + 1, torch);
        if (randomIn.nextInt(100) == 0) {
            const auto list = WeightedRandomChestContent::func_177629_a(chestContent(), { Items::enchanted_book->getRandom(randomIn) });
            const int max = 3 + randomIn.nextInt(4);
            generateMinecartChest(worldIn, sbb, randomIn, 2, 0, k1 - 1, list, max);
        }
        if (randomIn.nextInt(100) == 0) {
            const auto list = WeightedRandomChestContent::func_177629_a(chestContent(), { Items::enchanted_book->getRandom(randomIn) });
            const int max = 3 + randomIn.nextInt(4);
            generateMinecartChest(worldIn, sbb, randomIn, 0, 0, k1 + 1, list, max);
        }
        if (hasSpiders && !spawnerPlaced) {
            const int l1 = getYWithOffset(0);
            int i2 = k1 - 1 + randomIn.nextInt(3);
            const int j2 = getXWithOffset(1, i2);
            i2 = getZWithOffset(1, i2);
            const BlockPos blockpos(j2, l1, i2);
            if (sbb.isVecInside(blockpos)) {
                spawnerPlaced = true;
                worldIn->setBlockState(blockpos, Blocks::mob_spawner->getDefaultState(), 2);
                if (auto* spawner = dynamic_cast<TileEntityMobSpawner*>(worldIn->getTileEntity(blockpos)))
                    spawner->getSpawnerBaseLogic().setEntityName("CaveSpider");
            }
        }
    }
    for (int k2 = 0; k2 <= 2; ++k2) {
        for (int i3 = 0; i3 <= i1; ++i3) {
            const int j3 = -1;
            if (&getBlockStateFromPos(worldIn, k2, j3, i3, sbb)->getBlock()->getMaterial() == &Material::air) {
                const int k3 = -1;
                setBlockState(worldIn, planks, k2, k3, i3, sbb);
            }
        }
    }
    if (hasRails) {
        for (int l2 = 0; l2 <= i1; ++l2) {
            const IBlockState* iblockstate = getBlockStateFromPos(worldIn, 1, -1, l2, sbb);
            if (&iblockstate->getBlock()->getMaterial() != &Material::air && iblockstate->getBlock()->isFullBlock())
                randomlyPlaceBlock(worldIn, sbb, randomIn, 0.7f, 1, 0, l2, Blocks::rail->getStateFromMeta(getMetadataWithOffset(Blocks::rail, 0)));
        }
    }
    return true;
}

// --- Cross ---

Cross::Cross(int type, Random&, const StructureBoundingBox& structurebb, EnumFacing facing)
    : StructureComponent(type)
    , corridorDirection(facing)
    , isMultipleFloors(structurebb.getYSize() > 3)
{
    boundingBox = structurebb;
}

void Cross::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    tagCompound.setBoolean("tf", isMultipleFloors);
    tagCompound.setInteger("D", getHorizontalIndex(corridorDirection));
}

void Cross::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    isMultipleFloors = tagCompound.getBoolean("tf");
    corridorDirection = getHorizontal(tagCompound.getInteger("D"));
}

std::optional<StructureBoundingBox> Cross::func_175813_a(const StructureComponentList& listIn, Random& rand, int x, int y, int z, EnumFacing facing)
{
    StructureBoundingBox structureboundingbox(x, y, z, x, y + 2, z);
    if (rand.nextInt(4) == 0)
        structureboundingbox.maxY += 4;
    switch (facing) {
    case EnumFacing::NORTH:
        structureboundingbox.minX = x - 1;
        structureboundingbox.maxX = x + 3;
        structureboundingbox.minZ = z - 4;
        break;
    case EnumFacing::SOUTH:
        structureboundingbox.minX = x - 1;
        structureboundingbox.maxX = x + 3;
        structureboundingbox.maxZ = z + 4;
        break;
    case EnumFacing::WEST:
        structureboundingbox.minX = x - 4;
        structureboundingbox.minZ = z - 1;
        structureboundingbox.maxZ = z + 3;
        break;
    case EnumFacing::EAST:
        structureboundingbox.maxX = x + 4;
        structureboundingbox.minZ = z - 1;
        structureboundingbox.maxZ = z + 3;
        break;
    default:
        break;
    }
    if (findIntersecting(listIn, structureboundingbox))
        return std::nullopt;
    return structureboundingbox;
}

void Cross::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    const int i = getComponentType();
    const StructureBoundingBox& bb = boundingBox;
    switch (corridorDirection) {
    case EnumFacing::NORTH:
        func_175890_b(componentIn, listIn, rand, bb.minX + 1, bb.minY, bb.minZ - 1, EnumFacing::NORTH, i);
        func_175890_b(componentIn, listIn, rand, bb.minX - 1, bb.minY, bb.minZ + 1, EnumFacing::WEST, i);
        func_175890_b(componentIn, listIn, rand, bb.maxX + 1, bb.minY, bb.minZ + 1, EnumFacing::EAST, i);
        break;
    case EnumFacing::SOUTH:
        func_175890_b(componentIn, listIn, rand, bb.minX + 1, bb.minY, bb.maxZ + 1, EnumFacing::SOUTH, i);
        func_175890_b(componentIn, listIn, rand, bb.minX - 1, bb.minY, bb.minZ + 1, EnumFacing::WEST, i);
        func_175890_b(componentIn, listIn, rand, bb.maxX + 1, bb.minY, bb.minZ + 1, EnumFacing::EAST, i);
        break;
    case EnumFacing::WEST:
        func_175890_b(componentIn, listIn, rand, bb.minX + 1, bb.minY, bb.minZ - 1, EnumFacing::NORTH, i);
        func_175890_b(componentIn, listIn, rand, bb.minX + 1, bb.minY, bb.maxZ + 1, EnumFacing::SOUTH, i);
        func_175890_b(componentIn, listIn, rand, bb.minX - 1, bb.minY, bb.minZ + 1, EnumFacing::WEST, i);
        break;
    case EnumFacing::EAST:
        func_175890_b(componentIn, listIn, rand, bb.minX + 1, bb.minY, bb.minZ - 1, EnumFacing::NORTH, i);
        func_175890_b(componentIn, listIn, rand, bb.minX + 1, bb.minY, bb.maxZ + 1, EnumFacing::SOUTH, i);
        func_175890_b(componentIn, listIn, rand, bb.maxX + 1, bb.minY, bb.minZ + 1, EnumFacing::EAST, i);
        break;
    default:
        break;
    }
    if (isMultipleFloors) {
        if (rand.nextBoolean())
            func_175890_b(componentIn, listIn, rand, bb.minX + 1, bb.minY + 3 + 1, bb.minZ - 1, EnumFacing::NORTH, i);
        if (rand.nextBoolean())
            func_175890_b(componentIn, listIn, rand, bb.minX - 1, bb.minY + 3 + 1, bb.minZ + 1, EnumFacing::WEST, i);
        if (rand.nextBoolean())
            func_175890_b(componentIn, listIn, rand, bb.maxX + 1, bb.minY + 3 + 1, bb.minZ + 1, EnumFacing::EAST, i);
        if (rand.nextBoolean())
            func_175890_b(componentIn, listIn, rand, bb.minX + 1, bb.minY + 3 + 1, bb.maxZ + 1, EnumFacing::SOUTH, i);
    }
}

bool Cross::addComponentParts(World* worldIn, Random&, const StructureBoundingBox& sbb)
{
    if (isLiquidInStructureBoundingBox(worldIn, sbb))
        return false;
    const IBlockState* const air = Blocks::air->getDefaultState();
    const IBlockState* const planks = Blocks::planks->getDefaultState();
    const StructureBoundingBox bb = boundingBox;
    if (isMultipleFloors) {
        fillWithBlocks(worldIn, sbb, bb.minX + 1, bb.minY, bb.minZ, bb.maxX - 1, bb.minY + 3 - 1, bb.maxZ, air, air, false);
        fillWithBlocks(worldIn, sbb, bb.minX, bb.minY, bb.minZ + 1, bb.maxX, bb.minY + 3 - 1, bb.maxZ - 1, air, air, false);
        fillWithBlocks(worldIn, sbb, bb.minX + 1, bb.maxY - 2, bb.minZ, bb.maxX - 1, bb.maxY, bb.maxZ, air, air, false);
        fillWithBlocks(worldIn, sbb, bb.minX, bb.maxY - 2, bb.minZ + 1, bb.maxX, bb.maxY, bb.maxZ - 1, air, air, false);
        fillWithBlocks(worldIn, sbb, bb.minX + 1, bb.minY + 3, bb.minZ + 1, bb.maxX - 1, bb.minY + 3, bb.maxZ - 1, air, air, false);
    } else {
        fillWithBlocks(worldIn, sbb, bb.minX + 1, bb.minY, bb.minZ, bb.maxX - 1, bb.maxY, bb.maxZ, air, air, false);
        fillWithBlocks(worldIn, sbb, bb.minX, bb.minY, bb.minZ + 1, bb.maxX, bb.maxY, bb.maxZ - 1, air, air, false);
    }
    fillWithBlocks(worldIn, sbb, bb.minX + 1, bb.minY, bb.minZ + 1, bb.minX + 1, bb.maxY, bb.minZ + 1, planks, air, false);
    fillWithBlocks(worldIn, sbb, bb.minX + 1, bb.minY, bb.maxZ - 1, bb.minX + 1, bb.maxY, bb.maxZ - 1, planks, air, false);
    fillWithBlocks(worldIn, sbb, bb.maxX - 1, bb.minY, bb.minZ + 1, bb.maxX - 1, bb.maxY, bb.minZ + 1, planks, air, false);
    fillWithBlocks(worldIn, sbb, bb.maxX - 1, bb.minY, bb.maxZ - 1, bb.maxX - 1, bb.maxY, bb.maxZ - 1, planks, air, false);
    for (int i = bb.minX; i <= bb.maxX; ++i) {
        for (int j = bb.minZ; j <= bb.maxZ; ++j) {
            if (&getBlockStateFromPos(worldIn, i, bb.minY - 1, j, sbb)->getBlock()->getMaterial() == &Material::air)
                setBlockState(worldIn, planks, i, bb.minY - 1, j, sbb);
        }
    }
    return true;
}

// --- Room ---

Room::Room(int type, Random& rand, int x, int z)
    : StructureComponent(type)
{
    const int maxX = x + 7 + rand.nextInt(6);
    const int maxY = 54 + rand.nextInt(6);
    const int maxZ = z + 7 + rand.nextInt(6);
    boundingBox = StructureBoundingBox(x, 50, z, maxX, maxY, maxZ);
}

void Room::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    const int i = getComponentType();
    int j = boundingBox.getYSize() - 3 - 1;
    if (j <= 0)
        j = 1;
    int k;
    for (k = 0; k < boundingBox.getXSize(); k = k + 4) {
        k = k + rand.nextInt(boundingBox.getXSize());
        if (k + 3 > boundingBox.getXSize())
            break;
        const int y = boundingBox.minY + rand.nextInt(j) + 1;
        if (const auto structurecomponent = func_175890_b(componentIn, listIn, rand, boundingBox.minX + k, y, boundingBox.minZ - 1, EnumFacing::NORTH, i)) {
            const StructureBoundingBox& sbb = structurecomponent->getBoundingBox();
            roomsLinkedToTheRoom.emplace_back(sbb.minX, sbb.minY, boundingBox.minZ, sbb.maxX, sbb.maxY, boundingBox.minZ + 1);
        }
    }
    for (k = 0; k < boundingBox.getXSize(); k = k + 4) {
        k = k + rand.nextInt(boundingBox.getXSize());
        if (k + 3 > boundingBox.getXSize())
            break;
        const int y = boundingBox.minY + rand.nextInt(j) + 1;
        if (const auto structurecomponent1 = func_175890_b(componentIn, listIn, rand, boundingBox.minX + k, y, boundingBox.maxZ + 1, EnumFacing::SOUTH, i)) {
            const StructureBoundingBox& sbb = structurecomponent1->getBoundingBox();
            roomsLinkedToTheRoom.emplace_back(sbb.minX, sbb.minY, boundingBox.maxZ - 1, sbb.maxX, sbb.maxY, boundingBox.maxZ);
        }
    }
    for (k = 0; k < boundingBox.getZSize(); k = k + 4) {
        k = k + rand.nextInt(boundingBox.getZSize());
        if (k + 3 > boundingBox.getZSize())
            break;
        const int y = boundingBox.minY + rand.nextInt(j) + 1;
        if (const auto structurecomponent2 = func_175890_b(componentIn, listIn, rand, boundingBox.minX - 1, y, boundingBox.minZ + k, EnumFacing::WEST, i)) {
            const StructureBoundingBox& sbb = structurecomponent2->getBoundingBox();
            roomsLinkedToTheRoom.emplace_back(boundingBox.minX, sbb.minY, sbb.minZ, boundingBox.minX + 1, sbb.maxY, sbb.maxZ);
        }
    }
    for (k = 0; k < boundingBox.getZSize(); k = k + 4) {
        k = k + rand.nextInt(boundingBox.getZSize());
        if (k + 3 > boundingBox.getZSize())
            break;
        const int y = boundingBox.minY + rand.nextInt(j) + 1;
        if (const auto structurecomponent3 = func_175890_b(componentIn, listIn, rand, boundingBox.maxX + 1, y, boundingBox.minZ + k, EnumFacing::EAST, i)) {
            const StructureBoundingBox& sbb = structurecomponent3->getBoundingBox();
            roomsLinkedToTheRoom.emplace_back(boundingBox.maxX - 1, sbb.minY, sbb.minZ, boundingBox.maxX, sbb.maxY, sbb.maxZ);
        }
    }
}

bool Room::addComponentParts(World* worldIn, Random&, const StructureBoundingBox& sbb)
{
    if (isLiquidInStructureBoundingBox(worldIn, sbb))
        return false;
    const IBlockState* const air = Blocks::air->getDefaultState();
    const StructureBoundingBox bb = boundingBox;
    fillWithBlocks(worldIn, sbb, bb.minX, bb.minY, bb.minZ, bb.maxX, bb.minY, bb.maxZ, Blocks::dirt->getDefaultState(), air, true);
    fillWithBlocks(worldIn, sbb, bb.minX, bb.minY + 1, bb.minZ, bb.maxX, std::min(bb.minY + 3, bb.maxY), bb.maxZ, air, air, false);
    for (const StructureBoundingBox& structureboundingbox : roomsLinkedToTheRoom)
        fillWithBlocks(worldIn, sbb, structureboundingbox.minX, structureboundingbox.maxY - 2, structureboundingbox.minZ, structureboundingbox.maxX,
            structureboundingbox.maxY, structureboundingbox.maxZ, air, air, false);
    randomlyRareFillWithBlocks(worldIn, sbb, bb.minX, bb.minY + 4, bb.minZ, bb.maxX, bb.maxY, bb.maxZ, air, false);
    return true;
}

void Room::func_181138_a(int x, int y, int z)
{
    StructureComponent::func_181138_a(x, y, z);
    for (StructureBoundingBox& structureboundingbox : roomsLinkedToTheRoom)
        structureboundingbox.offset(x, y, z);
}

void Room::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (const StructureBoundingBox& structureboundingbox : roomsLinkedToTheRoom)
        nbttaglist->appendTag(structureboundingbox.toNBTTagIntArray());
    tagCompound.setTag("Entrances", nbttaglist);
}

void Room::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    const auto nbttaglist = tagCompound.getTagList("Entrances", 11);
    for (int i = 0; i < nbttaglist->tagCount(); ++i)
        roomsLinkedToTheRoom.emplace_back(nbttaglist->getIntArrayAt(i));
}

// --- Stairs ---

Stairs::Stairs(int type, Random&, const StructureBoundingBox& structurebb, EnumFacing facing)
    : StructureComponent(type)
{
    coordBaseMode = facing;
    boundingBox = structurebb;
}

std::optional<StructureBoundingBox> Stairs::func_175812_a(const StructureComponentList& listIn, Random&, int x, int y, int z, EnumFacing facing)
{
    StructureBoundingBox structureboundingbox(x, y - 5, z, x, y + 2, z);
    switch (facing) {
    case EnumFacing::NORTH:
        structureboundingbox.maxX = x + 2;
        structureboundingbox.minZ = z - 8;
        break;
    case EnumFacing::SOUTH:
        structureboundingbox.maxX = x + 2;
        structureboundingbox.maxZ = z + 8;
        break;
    case EnumFacing::WEST:
        structureboundingbox.minX = x - 8;
        structureboundingbox.maxZ = z + 2;
        break;
    case EnumFacing::EAST:
        structureboundingbox.maxX = x + 8;
        structureboundingbox.maxZ = z + 2;
        break;
    default:
        break;
    }
    if (findIntersecting(listIn, structureboundingbox))
        return std::nullopt;
    return structureboundingbox;
}

void Stairs::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    const int i = getComponentType();
    if (!coordBaseMode)
        return;
    switch (*coordBaseMode) {
    case EnumFacing::NORTH:
        func_175890_b(componentIn, listIn, rand, boundingBox.minX, boundingBox.minY, boundingBox.minZ - 1, EnumFacing::NORTH, i);
        break;
    case EnumFacing::SOUTH:
        func_175890_b(componentIn, listIn, rand, boundingBox.minX, boundingBox.minY, boundingBox.maxZ + 1, EnumFacing::SOUTH, i);
        break;
    case EnumFacing::WEST:
        func_175890_b(componentIn, listIn, rand, boundingBox.minX - 1, boundingBox.minY, boundingBox.minZ, EnumFacing::WEST, i);
        break;
    case EnumFacing::EAST:
        func_175890_b(componentIn, listIn, rand, boundingBox.maxX + 1, boundingBox.minY, boundingBox.minZ, EnumFacing::EAST, i);
        break;
    default:
        break;
    }
}

bool Stairs::addComponentParts(World* worldIn, Random&, const StructureBoundingBox& sbb)
{
    if (isLiquidInStructureBoundingBox(worldIn, sbb))
        return false;
    const IBlockState* const air = Blocks::air->getDefaultState();
    fillWithBlocks(worldIn, sbb, 0, 5, 0, 2, 7, 1, air, air, false);
    fillWithBlocks(worldIn, sbb, 0, 0, 7, 2, 2, 8, air, air, false);
    for (int i = 0; i < 5; ++i)
        fillWithBlocks(worldIn, sbb, 0, 5 - i - (i < 4 ? 1 : 0), 2 + i, 2, 7 - i, 2 + i, air, air, false);
    return true;
}

} // namespace StructureMineshaftPieces

// --- MapGenMineshaft ---

MapGenMineshaft::MapGenMineshaft(const FlatGeneratorInfo::Options& options)
{
    for (const auto& [key, value] : options) {
        if (key == "chance")
            field_82673_e = MathHelper::parseDoubleWithDefault(value, field_82673_e);
    }
}

bool MapGenMineshaft::canSpawnStructureAtCoords(int chunkX, int chunkZ)
{
    return rand.nextDouble() < field_82673_e && rand.nextInt(80) < std::max(std::abs(chunkX), std::abs(chunkZ));
}

std::shared_ptr<StructureStart> MapGenMineshaft::getStructureStart(int chunkX, int chunkZ)
{
    return std::make_shared<StructureMineshaftStart>(worldObj, rand, chunkX, chunkZ);
}

StructureMineshaftStart::StructureMineshaftStart(World* worldIn, Random& rand, int chunkX, int chunkZ)
    : StructureStart(chunkX, chunkZ)
{
    auto room = std::make_shared<StructureMineshaftPieces::Room>(0, rand, (chunkX << 4) + 2, (chunkZ << 4) + 2);
    components.push_back(room);
    room->buildComponent(room.get(), components, rand);
    updateBoundingBox();
    markAvailableHeight(worldIn, rand, 10);
}

} // namespace mc
