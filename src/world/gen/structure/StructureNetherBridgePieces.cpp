#include "world/gen/structure/StructureNetherBridgePieces.h"

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/ItemArmor.h"
#include "item/Item.h"
#include "nbt/NBTTagCompound.h"
#include "tileentity/TileEntities.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/gen/structure/MapGenNetherBridge.h"
#include "world/gen/structure/MapGenStructure.h"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace mc {

namespace StructureNetherBridgePieces {

namespace {

std::shared_ptr<Piece> stairsFinder(StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return Stairs::func_175872_a(list, rand, x, y, z, type, facing);
}

std::shared_ptr<Piece> throneFinder(StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return Throne::func_175874_a(list, rand, x, y, z, type, facing);
}

std::array<PieceWeight, 6> primaryComponents {
    PieceWeight(&Straight::func_175882_a, 30, 0, true),
    PieceWeight(&Crossing3::func_175885_a, 10, 4),
    PieceWeight(&Crossing::func_175873_a, 10, 4),
    PieceWeight(&stairsFinder, 10, 3),
    PieceWeight(&throneFinder, 5, 2),
    PieceWeight(&Entrance::func_175881_a, 5, 1),
};

std::array<PieceWeight, 7> secondaryComponents {
    PieceWeight(&Corridor5::func_175877_a, 25, 0, true),
    PieceWeight(&Crossing2::func_175878_a, 15, 5),
    PieceWeight(&Corridor2::func_175876_a, 5, 10),
    PieceWeight(&Corridor::func_175879_a, 5, 10),
    PieceWeight(&Corridor3::func_175883_a, 10, 3, true),
    PieceWeight(&Corridor4::func_175880_a, 7, 2),
    PieceWeight(&NetherStalkRoom::func_175875_a, 5, 2),
};

// Java: Piece.field_111019_a
const std::vector<WeightedRandomChestContent>& field_111019_a()
{
    static const std::vector<WeightedRandomChestContent> list {
        { Items::diamond, 0, 1, 3, 5 },
        { Items::iron_ingot, 0, 1, 5, 5 },
        { Items::gold_ingot, 0, 1, 3, 15 },
        { Items::golden_sword, 0, 1, 1, 5 },
        { Items::golden_chestplate, 0, 1, 1, 5 },
        { Items::flint_and_steel, 0, 1, 1, 5 },
        { Items::nether_wart, 0, 3, 7, 5 },
        { Items::saddle, 0, 1, 1, 10 },
        { Items::golden_horse_armor, 0, 1, 1, 8 },
        { Items::iron_horse_armor, 0, 1, 1, 5 },
        { Items::diamond_horse_armor, 0, 1, 1, 3 },
        { Item::getItemFromBlock(Blocks::obsidian), 0, 2, 4, 2 },
    };
    return list;
}

} // namespace

void registerNetherFortressPieces()
{
    MapGenStructureIO::registerStructureComponent("NeBCr", [] { return std::make_shared<Crossing3>(); });
    MapGenStructureIO::registerStructureComponent("NeBEF", [] { return std::make_shared<End>(); });
    MapGenStructureIO::registerStructureComponent("NeBS", [] { return std::make_shared<Straight>(); });
    MapGenStructureIO::registerStructureComponent("NeCCS", [] { return std::make_shared<Corridor3>(); });
    MapGenStructureIO::registerStructureComponent("NeCTB", [] { return std::make_shared<Corridor4>(); });
    MapGenStructureIO::registerStructureComponent("NeCE", [] { return std::make_shared<Entrance>(); });
    MapGenStructureIO::registerStructureComponent("NeSCSC", [] { return std::make_shared<Crossing2>(); });
    MapGenStructureIO::registerStructureComponent("NeSCLT", [] { return std::make_shared<Corridor>(); });
    MapGenStructureIO::registerStructureComponent("NeSC", [] { return std::make_shared<Corridor5>(); });
    MapGenStructureIO::registerStructureComponent("NeSCRT", [] { return std::make_shared<Corridor2>(); });
    MapGenStructureIO::registerStructureComponent("NeCSR", [] { return std::make_shared<NetherStalkRoom>(); });
    MapGenStructureIO::registerStructureComponent("NeMT", [] { return std::make_shared<Throne>(); });
    MapGenStructureIO::registerStructureComponent("NeRC", [] { return std::make_shared<Crossing>(); });
    MapGenStructureIO::registerStructureComponent("NeSR", [] { return std::make_shared<Stairs>(); });
    MapGenStructureIO::registerStructureComponent("NeStart", [] { return std::make_shared<Start>(); });
}

// --- Piece ---

int Piece::getTotalWeight(const std::vector<PieceWeight*>& weights) const
{
    bool flag = false;
    int i = 0;
    for (const PieceWeight* weight : weights) {
        if (weight->field_78824_d > 0 && weight->field_78827_c < weight->field_78824_d)
            flag = true;
        i += weight->field_78826_b;
    }
    return flag ? i : -1;
}

std::shared_ptr<Piece> Piece::func_175871_a(Start* start, std::vector<PieceWeight*>& weights, StructureComponentList& list, Random& rand, int x, int y, int z,
    EnumFacing facing, int type)
{
    const int i = getTotalWeight(weights);
    const bool flag = i > 0 && type <= 30;
    int j = 0;
    while (j < 5 && flag) {
        ++j;
        int k = rand.nextInt(i);
        for (PieceWeight* weight : weights) {
            k -= weight->field_78826_b;
            if (k < 0) {
                if (!weight->func_78822_a(type) || (weight == start->theNetherBridgePieceWeight && !weight->field_78825_e))
                    break;
                // Java: func_175887_b dispatches on the piece class.
                std::shared_ptr<Piece> piece = weight->weightClass(list, rand, x, y, z, facing, type);
                if (piece) {
                    ++weight->field_78827_c;
                    start->theNetherBridgePieceWeight = weight;
                    if (!weight->func_78823_a())
                        std::erase(weights, weight);
                    return piece;
                }
            }
        }
    }
    return End::func_175884_a(list, rand, x, y, z, facing, type);
}

std::shared_ptr<StructureComponent> Piece::func_175870_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing,
    int type, bool secondary)
{
    if (std::abs(x - start->getBoundingBox().minX) <= 112 && std::abs(z - start->getBoundingBox().minZ) <= 112) {
        std::vector<PieceWeight*>& weights = secondary ? start->secondaryWeights : start->primaryWeights;
        std::shared_ptr<StructureComponent> structurecomponent = func_175871_a(start, weights, list, rand, x, y, z, facing, type + 1);
        if (structurecomponent) {
            list.push_back(structurecomponent);
            start->field_74967_d.push_back(structurecomponent);
        }
        return structurecomponent;
    }
    // Java returns this end piece without adding it to the structure.
    return End::func_175884_a(list, rand, x, y, z, facing, type);
}

std::shared_ptr<StructureComponent> Piece::getNextComponentNormal(Start* start, StructureComponentList& list, Random& rand, int x, int y, bool secondary)
{
    if (!coordBaseMode)
        return nullptr;
    switch (*coordBaseMode) {
    case EnumFacing::NORTH:
        return func_175870_a(start, list, rand, boundingBox.minX + x, boundingBox.minY + y, boundingBox.minZ - 1, *coordBaseMode, getComponentType(), secondary);
    case EnumFacing::SOUTH:
        return func_175870_a(start, list, rand, boundingBox.minX + x, boundingBox.minY + y, boundingBox.maxZ + 1, *coordBaseMode, getComponentType(), secondary);
    case EnumFacing::WEST:
        return func_175870_a(start, list, rand, boundingBox.minX - 1, boundingBox.minY + y, boundingBox.minZ + x, *coordBaseMode, getComponentType(), secondary);
    case EnumFacing::EAST:
        return func_175870_a(start, list, rand, boundingBox.maxX + 1, boundingBox.minY + y, boundingBox.minZ + x, *coordBaseMode, getComponentType(), secondary);
    default:
        return nullptr;
    }
}

std::shared_ptr<StructureComponent> Piece::getNextComponentX(Start* start, StructureComponentList& list, Random& rand, int y, int z, bool secondary)
{
    if (!coordBaseMode)
        return nullptr;
    switch (*coordBaseMode) {
    case EnumFacing::NORTH:
    case EnumFacing::SOUTH:
        return func_175870_a(start, list, rand, boundingBox.minX - 1, boundingBox.minY + y, boundingBox.minZ + z, EnumFacing::WEST, getComponentType(), secondary);
    case EnumFacing::WEST:
    case EnumFacing::EAST:
        return func_175870_a(start, list, rand, boundingBox.minX + z, boundingBox.minY + y, boundingBox.minZ - 1, EnumFacing::NORTH, getComponentType(), secondary);
    default:
        return nullptr;
    }
}

std::shared_ptr<StructureComponent> Piece::getNextComponentZ(Start* start, StructureComponentList& list, Random& rand, int y, int z, bool secondary)
{
    if (!coordBaseMode)
        return nullptr;
    switch (*coordBaseMode) {
    case EnumFacing::NORTH:
    case EnumFacing::SOUTH:
        return func_175870_a(start, list, rand, boundingBox.maxX + 1, boundingBox.minY + y, boundingBox.minZ + z, EnumFacing::EAST, getComponentType(), secondary);
    case EnumFacing::WEST:
    case EnumFacing::EAST:
        return func_175870_a(start, list, rand, boundingBox.minX + z, boundingBox.minY + y, boundingBox.maxZ + 1, EnumFacing::SOUTH, getComponentType(), secondary);
    default:
        return nullptr;
    }
}

// --- Start ---

Start::Start(Random& rand, int x, int z)
    : Crossing3(rand, x, z)
{
    for (PieceWeight& weight : primaryComponents) {
        weight.field_78827_c = 0;
        primaryWeights.push_back(&weight);
    }
    for (PieceWeight& weight : secondaryComponents) {
        weight.field_78827_c = 0;
        secondaryWeights.push_back(&weight);
    }
}

// --- the pieces ---

Corridor::Corridor(int p_i45615_1_, Random& p_i45615_2_, const StructureBoundingBox& p_i45615_3_, EnumFacing p_i45615_4_)
    : Piece(p_i45615_1_)
{
    coordBaseMode = p_i45615_4_;
    boundingBox = p_i45615_3_;
    field_111021_b = p_i45615_2_.nextInt(3) == 0;
}

void Corridor::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Piece::readStructureFromNBT(tagCompound);
    field_111021_b = tagCompound.getBoolean("Chest");
}

void Corridor::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Piece::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Chest", field_111021_b);
}

void Corridor::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentX(static_cast<Start*>(componentIn), listIn, rand, 0, 1, true);
}

std::shared_ptr<Piece> Corridor::func_175879_a(StructureComponentList& p_175879_0_, Random& p_175879_1_, int p_175879_2_, int p_175879_3_, int p_175879_4_, EnumFacing p_175879_5_, int p_175879_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175879_2_, p_175879_3_, p_175879_4_, -1, 0, 0, 5, 7, 5, p_175879_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175879_0_, structureboundingbox) == nullptr ? std::make_shared<Corridor>(p_175879_6_, p_175879_1_, structureboundingbox, p_175879_5_) : nullptr;
}

bool Corridor::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 1, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 4, 5, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 2, 0, 4, 5, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 3, 1, 4, 4, 1, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 3, 3, 4, 4, 3, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 5, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 4, 3, 5, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 4, 1, 4, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 3, 4, 3, 4, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    if (field_111021_b && structureBoundingBoxIn.isVecInside(BlockPos(getXWithOffset(3, 3), getYWithOffset(2), getZWithOffset(3, 3))))
    {
        field_111021_b = false;
        generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 3, 2, 3, field_111019_a(), 2 + randomIn.nextInt(4));
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 6, 0, 4, 6, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int i = 0; i <= 4; ++i)
    {
        for (int j = 0; j <= 4; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, j, structureBoundingBoxIn);
        }
    }
    return true;
}

Corridor2::Corridor2(int p_i45613_1_, Random& p_i45613_2_, const StructureBoundingBox& p_i45613_3_, EnumFacing p_i45613_4_)
    : Piece(p_i45613_1_)
{
    coordBaseMode = p_i45613_4_;
    boundingBox = p_i45613_3_;
    field_111020_b = p_i45613_2_.nextInt(3) == 0;
}

void Corridor2::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Piece::readStructureFromNBT(tagCompound);
    field_111020_b = tagCompound.getBoolean("Chest");
}

void Corridor2::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Piece::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Chest", field_111020_b);
}

void Corridor2::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentZ(static_cast<Start*>(componentIn), listIn, rand, 0, 1, true);
}

std::shared_ptr<Piece> Corridor2::func_175876_a(StructureComponentList& p_175876_0_, Random& p_175876_1_, int p_175876_2_, int p_175876_3_, int p_175876_4_, EnumFacing p_175876_5_, int p_175876_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175876_2_, p_175876_3_, p_175876_4_, -1, 0, 0, 5, 7, 5, p_175876_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175876_0_, structureboundingbox) == nullptr ? std::make_shared<Corridor2>(p_175876_6_, p_175876_1_, structureboundingbox, p_175876_5_) : nullptr;
}

bool Corridor2::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 1, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 4, 5, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 5, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 1, 0, 4, 1, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 3, 0, 4, 3, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 2, 0, 4, 5, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 4, 4, 5, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 4, 1, 4, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 3, 4, 3, 4, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    if (field_111020_b && structureBoundingBoxIn.isVecInside(BlockPos(getXWithOffset(1, 3), getYWithOffset(2), getZWithOffset(1, 3))))
    {
        field_111020_b = false;
        generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 1, 2, 3, field_111019_a(), 2 + randomIn.nextInt(4));
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 6, 0, 4, 6, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int i = 0; i <= 4; ++i)
    {
        for (int j = 0; j <= 4; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, j, structureBoundingBoxIn);
        }
    }
    return true;
}

Corridor3::Corridor3(int p_i45619_1_, Random& p_i45619_2_, const StructureBoundingBox& p_i45619_3_, EnumFacing p_i45619_4_)
    : Piece(p_i45619_1_)
{
    coordBaseMode = p_i45619_4_;
    boundingBox = p_i45619_3_;
}

void Corridor3::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Start*>(componentIn), listIn, rand, 1, 0, true);
}

std::shared_ptr<Piece> Corridor3::func_175883_a(StructureComponentList& p_175883_0_, Random& p_175883_1_, int p_175883_2_, int p_175883_3_, int p_175883_4_, EnumFacing p_175883_5_, int p_175883_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175883_2_, p_175883_3_, p_175883_4_, -1, -7, 0, 5, 14, 10, p_175883_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175883_0_, structureboundingbox) == nullptr ? std::make_shared<Corridor3>(p_175883_6_, p_175883_1_, structureboundingbox, p_175883_5_) : nullptr;
}

bool Corridor3::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    int i = getMetadataWithOffset(Blocks::nether_brick_stairs, 2);
    for (int j = 0; j <= 9; ++j)
    {
        int k = std::max(1, 7 - j);
        int l = std::min(std::max(k + 5, 14 - j), 13);
        int i1 = j;
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, j, 4, k, j, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, k + 1, j, 3, l - 1, j, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        if (j <= 6)
        {
            setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(i), 1, k + 1, j, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(i), 2, k + 1, j, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(i), 3, k + 1, j, structureBoundingBoxIn);
        }
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, l, j, 4, l, j, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, k + 1, j, 0, l - 1, j, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 4, k + 1, j, 4, l - 1, j, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
        if ((j & 1) == 0)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, k + 2, j, 0, k + 3, j, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 4, k + 2, j, 4, k + 3, j, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        }
        for (int j1 = 0; j1 <= 4; ++j1)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), j1, -1, i1, structureBoundingBoxIn);
        }
    }
    return true;
}

Corridor4::Corridor4(int p_i45618_1_, Random& p_i45618_2_, const StructureBoundingBox& p_i45618_3_, EnumFacing p_i45618_4_)
    : Piece(p_i45618_1_)
{
    coordBaseMode = p_i45618_4_;
    boundingBox = p_i45618_3_;
}

void Corridor4::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    int i = 1;
    if (isMode(EnumFacing::WEST) || isMode(EnumFacing::NORTH))
    {
        i = 5;
    }
    getNextComponentX(static_cast<Start*>(componentIn), listIn, rand, 0, i, rand.nextInt(8) > 0);
    getNextComponentZ(static_cast<Start*>(componentIn), listIn, rand, 0, i, rand.nextInt(8) > 0);
}

std::shared_ptr<Piece> Corridor4::func_175880_a(StructureComponentList& p_175880_0_, Random& p_175880_1_, int p_175880_2_, int p_175880_3_, int p_175880_4_, EnumFacing p_175880_5_, int p_175880_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175880_2_, p_175880_3_, p_175880_4_, -3, 0, 0, 9, 7, 9, p_175880_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175880_0_, structureboundingbox) == nullptr ? std::make_shared<Corridor4>(p_175880_6_, p_175880_1_, structureboundingbox, p_175880_5_) : nullptr;
}

bool Corridor4::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 8, 1, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 8, 5, 8, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 6, 0, 8, 6, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 2, 5, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 2, 0, 8, 5, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 0, 1, 4, 0, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 3, 0, 7, 4, 0, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 4, 8, 2, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 4, 2, 2, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 4, 7, 2, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 8, 8, 3, 8, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 6, 0, 3, 7, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 3, 6, 8, 3, 7, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 4, 0, 5, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 3, 4, 8, 5, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 5, 2, 5, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 3, 5, 7, 5, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 5, 1, 5, 5, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 4, 5, 7, 5, 5, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    for (int i = 0; i <= 5; ++i)
    {
        for (int j = 0; j <= 8; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), j, -1, i, structureBoundingBoxIn);
        }
    }
    return true;
}

Corridor5::Corridor5(int p_i45614_1_, Random& p_i45614_2_, const StructureBoundingBox& p_i45614_3_, EnumFacing p_i45614_4_)
    : Piece(p_i45614_1_)
{
    coordBaseMode = p_i45614_4_;
    boundingBox = p_i45614_3_;
}

void Corridor5::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Start*>(componentIn), listIn, rand, 1, 0, true);
}

std::shared_ptr<Piece> Corridor5::func_175877_a(StructureComponentList& p_175877_0_, Random& p_175877_1_, int p_175877_2_, int p_175877_3_, int p_175877_4_, EnumFacing p_175877_5_, int p_175877_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175877_2_, p_175877_3_, p_175877_4_, -1, 0, 0, 5, 7, 5, p_175877_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175877_0_, structureboundingbox) == nullptr ? std::make_shared<Corridor5>(p_175877_6_, p_175877_1_, structureboundingbox, p_175877_5_) : nullptr;
}

bool Corridor5::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 1, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 4, 5, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 5, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 2, 0, 4, 5, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 1, 0, 4, 1, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 3, 0, 4, 3, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 3, 1, 4, 4, 1, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 3, 3, 4, 4, 3, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 6, 0, 4, 6, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int i = 0; i <= 4; ++i)
    {
        for (int j = 0; j <= 4; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, j, structureBoundingBoxIn);
        }
    }
    return true;
}

Crossing::Crossing(int p_i45610_1_, Random& p_i45610_2_, const StructureBoundingBox& p_i45610_3_, EnumFacing p_i45610_4_)
    : Piece(p_i45610_1_)
{
    coordBaseMode = p_i45610_4_;
    boundingBox = p_i45610_3_;
}

void Crossing::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Start*>(componentIn), listIn, rand, 2, 0, false);
    getNextComponentX(static_cast<Start*>(componentIn), listIn, rand, 0, 2, false);
    getNextComponentZ(static_cast<Start*>(componentIn), listIn, rand, 0, 2, false);
}

std::shared_ptr<Piece> Crossing::func_175873_a(StructureComponentList& p_175873_0_, Random& p_175873_1_, int p_175873_2_, int p_175873_3_, int p_175873_4_, EnumFacing p_175873_5_, int p_175873_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175873_2_, p_175873_3_, p_175873_4_, -2, 0, 0, 7, 9, 7, p_175873_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175873_0_, structureboundingbox) == nullptr ? std::make_shared<Crossing>(p_175873_6_, p_175873_1_, structureboundingbox, p_175873_5_) : nullptr;
}

bool Crossing::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 6, 1, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 6, 7, 6, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 1, 6, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 6, 1, 6, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 0, 6, 6, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 6, 6, 6, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 6, 1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 5, 0, 6, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 2, 0, 6, 6, 1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 2, 5, 6, 6, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 6, 0, 4, 6, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 0, 4, 5, 0, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 6, 6, 4, 6, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 6, 4, 5, 6, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 6, 2, 0, 6, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 2, 0, 5, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 6, 2, 6, 6, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 5, 2, 6, 5, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    for (int i = 0; i <= 6; ++i)
    {
        for (int j = 0; j <= 6; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, j, structureBoundingBoxIn);
        }
    }
    return true;
}

Crossing2::Crossing2(int p_i45616_1_, Random& p_i45616_2_, const StructureBoundingBox& p_i45616_3_, EnumFacing p_i45616_4_)
    : Piece(p_i45616_1_)
{
    coordBaseMode = p_i45616_4_;
    boundingBox = p_i45616_3_;
}

void Crossing2::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Start*>(componentIn), listIn, rand, 1, 0, true);
    getNextComponentX(static_cast<Start*>(componentIn), listIn, rand, 0, 1, true);
    getNextComponentZ(static_cast<Start*>(componentIn), listIn, rand, 0, 1, true);
}

std::shared_ptr<Piece> Crossing2::func_175878_a(StructureComponentList& p_175878_0_, Random& p_175878_1_, int p_175878_2_, int p_175878_3_, int p_175878_4_, EnumFacing p_175878_5_, int p_175878_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175878_2_, p_175878_3_, p_175878_4_, -1, 0, 0, 5, 7, 5, p_175878_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175878_0_, structureboundingbox) == nullptr ? std::make_shared<Crossing2>(p_175878_6_, p_175878_1_, structureboundingbox, p_175878_5_) : nullptr;
}

bool Crossing2::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 1, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 4, 5, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 5, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 2, 0, 4, 5, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 4, 0, 5, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 2, 4, 4, 5, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 6, 0, 4, 6, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int i = 0; i <= 4; ++i)
    {
        for (int j = 0; j <= 4; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, j, structureBoundingBoxIn);
        }
    }
    return true;
}

Crossing3::Crossing3(int p_i45622_1_, Random& p_i45622_2_, const StructureBoundingBox& p_i45622_3_, EnumFacing p_i45622_4_)
    : Piece(p_i45622_1_)
{
    coordBaseMode = p_i45622_4_;
    boundingBox = p_i45622_3_;
}

Crossing3::Crossing3(Random& p_i2042_1_, int p_i2042_2_, int p_i2042_3_)
    : Piece(0)
{
    coordBaseMode = randomFacing(Plane::HORIZONTAL, p_i2042_1_);
    // Java switches on the facing but builds the same box either way.
    boundingBox = StructureBoundingBox(p_i2042_2_, 64, p_i2042_3_, p_i2042_2_ + 19 - 1, 73, p_i2042_3_ + 19 - 1);
}

void Crossing3::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Start*>(componentIn), listIn, rand, 8, 3, false);
    getNextComponentX(static_cast<Start*>(componentIn), listIn, rand, 3, 8, false);
    getNextComponentZ(static_cast<Start*>(componentIn), listIn, rand, 3, 8, false);
}

std::shared_ptr<Piece> Crossing3::func_175885_a(StructureComponentList& p_175885_0_, Random& p_175885_1_, int p_175885_2_, int p_175885_3_, int p_175885_4_, EnumFacing p_175885_5_, int p_175885_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175885_2_, p_175885_3_, p_175885_4_, -8, -3, 0, 19, 10, 19, p_175885_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175885_0_, structureboundingbox) == nullptr ? std::make_shared<Crossing3>(p_175885_6_, p_175885_1_, structureboundingbox, p_175885_5_) : nullptr;
}

bool Crossing3::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 3, 0, 11, 4, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 7, 18, 4, 11, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 5, 0, 10, 7, 18, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 8, 18, 7, 10, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 5, 0, 7, 5, 7, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 5, 11, 7, 5, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 5, 0, 11, 5, 7, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 5, 11, 11, 5, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 7, 7, 5, 7, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 5, 7, 18, 5, 7, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 11, 7, 5, 11, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 5, 11, 18, 5, 11, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 0, 11, 2, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 13, 11, 2, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 0, 0, 11, 1, 3, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 0, 15, 11, 1, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int i = 7; i <= 11; ++i)
    {
        for (int j = 0; j <= 2; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, j, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, 18 - j, structureBoundingBoxIn);
        }
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 7, 5, 2, 11, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 13, 2, 7, 18, 2, 11, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 7, 3, 1, 11, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 15, 0, 7, 18, 1, 11, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int k = 0; k <= 2; ++k)
    {
        for (int l = 7; l <= 11; ++l)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), k, -1, l, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), 18 - k, -1, l, structureBoundingBoxIn);
        }
    }
    return true;
}

End::End(int p_i45621_1_, Random& p_i45621_2_, const StructureBoundingBox& p_i45621_3_, EnumFacing p_i45621_4_)
    : Piece(p_i45621_1_)
{
    coordBaseMode = p_i45621_4_;
    boundingBox = p_i45621_3_;
    fillSeed = p_i45621_2_.nextInt();
}

std::shared_ptr<Piece> End::func_175884_a(StructureComponentList& p_175884_0_, Random& p_175884_1_, int p_175884_2_, int p_175884_3_, int p_175884_4_, EnumFacing p_175884_5_, int p_175884_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175884_2_, p_175884_3_, p_175884_4_, -1, -3, 0, 5, 10, 8, p_175884_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175884_0_, structureboundingbox) == nullptr ? std::make_shared<End>(p_175884_6_, p_175884_1_, structureboundingbox, p_175884_5_) : nullptr;
}

void End::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Piece::readStructureFromNBT(tagCompound);
    fillSeed = tagCompound.getInteger("Seed");
}

void End::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Piece::writeStructureToNBT(tagCompound);
    tagCompound.setInteger("Seed", fillSeed);
}

bool End::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    Random random(static_cast<int64_t>(fillSeed));
    for (int i = 0; i <= 4; ++i)
    {
        for (int j = 3; j <= 4; ++j)
        {
            int k = random.nextInt(8);
            fillWithBlocks(worldIn, structureBoundingBoxIn, i, j, 0, i, j, k, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
        }
    }
    int l = random.nextInt(8);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 0, 0, 5, l, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    l = random.nextInt(8);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 5, 0, 4, 5, l, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (l = 0; l <= 4; ++l)
    {
        int i1 = random.nextInt(5);
        fillWithBlocks(worldIn, structureBoundingBoxIn, l, 2, 0, l, 2, i1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    }
    for (l = 0; l <= 4; ++l)
    {
        for (int j1 = 0; j1 <= 1; ++j1)
        {
            int k1 = random.nextInt(3);
            fillWithBlocks(worldIn, structureBoundingBoxIn, l, j1, 0, l, j1, k1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
        }
    }
    return true;
}

Entrance::Entrance(int p_i45617_1_, Random& p_i45617_2_, const StructureBoundingBox& p_i45617_3_, EnumFacing p_i45617_4_)
    : Piece(p_i45617_1_)
{
    coordBaseMode = p_i45617_4_;
    boundingBox = p_i45617_3_;
}

void Entrance::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Start*>(componentIn), listIn, rand, 5, 3, true);
}

std::shared_ptr<Piece> Entrance::func_175881_a(StructureComponentList& p_175881_0_, Random& p_175881_1_, int p_175881_2_, int p_175881_3_, int p_175881_4_, EnumFacing p_175881_5_, int p_175881_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175881_2_, p_175881_3_, p_175881_4_, -5, -3, 0, 13, 14, 13, p_175881_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175881_0_, structureboundingbox) == nullptr ? std::make_shared<Entrance>(p_175881_6_, p_175881_1_, structureboundingbox, p_175881_5_) : nullptr;
}

bool Entrance::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 0, 12, 4, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 0, 12, 13, 12, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 0, 1, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 5, 0, 12, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 11, 4, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 5, 11, 10, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 9, 11, 7, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 0, 4, 12, 1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 5, 0, 10, 12, 1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 9, 0, 7, 12, 1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 11, 2, 10, 12, 10, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 8, 0, 7, 8, 0, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    for (int i = 1; i <= 11; i += 2)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, i, 10, 0, i, 11, 0, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, i, 10, 12, i, 11, 12, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 10, i, 0, 11, i, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 10, i, 12, 11, i, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), i, 13, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), i, 13, 12, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), 0, 13, i, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), 12, 13, i, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), i + 1, 13, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), i + 1, 13, 12, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 0, 13, i + 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 12, 13, i + 1, structureBoundingBoxIn);
    }
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 0, 13, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 0, 13, 12, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 0, 13, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 12, 13, 0, structureBoundingBoxIn);
    for (int k = 3; k <= 9; k += 2)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 7, k, 1, 8, k, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 7, k, 11, 8, k, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 2, 0, 8, 2, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 4, 12, 2, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 0, 0, 8, 1, 3, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 0, 9, 8, 1, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 4, 3, 1, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 0, 4, 12, 1, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int l = 4; l <= 8; ++l)
    {
        for (int j = 0; j <= 2; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), l, -1, j, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), l, -1, 12 - j, structureBoundingBoxIn);
        }
    }
    for (int i1 = 0; i1 <= 2; ++i1)
    {
        for (int j1 = 4; j1 <= 8; ++j1)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i1, -1, j1, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), 12 - i1, -1, j1, structureBoundingBoxIn);
        }
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 5, 5, 7, 5, 7, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 6, 6, 4, 6, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), 6, 0, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::flowing_lava->getDefaultState(), 6, 5, 6, structureBoundingBoxIn);
    const BlockPos blockpos(getXWithOffset(6, 6), getYWithOffset(5), getZWithOffset(6, 6));
    if (structureBoundingBoxIn.isVecInside(blockpos))
    {
        worldIn->forceBlockUpdateTick(Blocks::flowing_lava, blockpos, randomIn);
    }
    return true;
}

NetherStalkRoom::NetherStalkRoom(int p_i45612_1_, Random& p_i45612_2_, const StructureBoundingBox& p_i45612_3_, EnumFacing p_i45612_4_)
    : Piece(p_i45612_1_)
{
    coordBaseMode = p_i45612_4_;
    boundingBox = p_i45612_3_;
}

void NetherStalkRoom::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Start*>(componentIn), listIn, rand, 5, 3, true);
    getNextComponentNormal(static_cast<Start*>(componentIn), listIn, rand, 5, 11, true);
}

std::shared_ptr<Piece> NetherStalkRoom::func_175875_a(StructureComponentList& p_175875_0_, Random& p_175875_1_, int p_175875_2_, int p_175875_3_, int p_175875_4_, EnumFacing p_175875_5_, int p_175875_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175875_2_, p_175875_3_, p_175875_4_, -5, -3, 0, 13, 14, 13, p_175875_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175875_0_, structureboundingbox) == nullptr ? std::make_shared<NetherStalkRoom>(p_175875_6_, p_175875_1_, structureboundingbox, p_175875_5_) : nullptr;
}

bool NetherStalkRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 0, 12, 4, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 0, 12, 13, 12, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 0, 1, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 5, 0, 12, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 11, 4, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 5, 11, 10, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 9, 11, 7, 12, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 0, 4, 12, 1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 5, 0, 10, 12, 1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 9, 0, 7, 12, 1, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 11, 2, 10, 12, 10, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int i = 1; i <= 11; i += 2)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, i, 10, 0, i, 11, 0, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, i, 10, 12, i, 11, 12, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 10, i, 0, 11, i, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 10, i, 12, 11, i, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), i, 13, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), i, 13, 12, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), 0, 13, i, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), 12, 13, i, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), i + 1, 13, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), i + 1, 13, 12, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 0, 13, i + 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 12, 13, i + 1, structureBoundingBoxIn);
    }
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 0, 13, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 0, 13, 12, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 0, 13, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 12, 13, 0, structureBoundingBoxIn);
    for (int j1 = 3; j1 <= 9; j1 += 2)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 7, j1, 1, 8, j1, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 7, j1, 11, 8, j1, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    }
    int k1 = getMetadataWithOffset(Blocks::nether_brick_stairs, 3);
    for (int j = 0; j <= 6; ++j)
    {
        int k = j + 4;
        for (int l = 5; l <= 7; ++l)
        {
            setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(k1), l, 5 + j, k, structureBoundingBoxIn);
        }
        if (k >= 5 && k <= 8)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 5, k, 7, j + 4, k, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
        }
        else if (k >= 9 && k <= 10)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 8, k, 7, j + 4, k, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
        }
        if (j >= 1)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 6 + j, k, 7, 9 + j, k, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
    }
    for (int l1 = 5; l1 <= 7; ++l1)
    {
        setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(k1), l1, 12, 11, structureBoundingBoxIn);
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 6, 7, 5, 7, 7, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 6, 7, 7, 7, 7, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 13, 12, 7, 13, 12, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 2, 3, 5, 3, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 9, 3, 5, 10, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 4, 2, 5, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 5, 2, 10, 5, 3, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 5, 9, 10, 5, 10, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 5, 4, 10, 5, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    int i2 = getMetadataWithOffset(Blocks::nether_brick_stairs, 0);
    int j2 = getMetadataWithOffset(Blocks::nether_brick_stairs, 1);
    setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(j2), 4, 5, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(j2), 4, 5, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(j2), 4, 5, 9, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(j2), 4, 5, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(i2), 8, 5, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(i2), 8, 5, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(i2), 8, 5, 9, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_stairs->getStateFromMeta(i2), 8, 5, 10, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 4, 4, 4, 4, 8, Blocks::soul_sand->getDefaultState(), Blocks::soul_sand->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 4, 4, 9, 4, 8, Blocks::soul_sand->getDefaultState(), Blocks::soul_sand->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 5, 4, 4, 5, 8, Blocks::nether_wart->getDefaultState(), Blocks::nether_wart->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 5, 4, 9, 5, 8, Blocks::nether_wart->getDefaultState(), Blocks::nether_wart->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 2, 0, 8, 2, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 4, 12, 2, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 0, 0, 8, 1, 3, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 0, 9, 8, 1, 12, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 4, 3, 1, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 0, 4, 12, 1, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int k2 = 4; k2 <= 8; ++k2)
    {
        for (int i1 = 0; i1 <= 2; ++i1)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), k2, -1, i1, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), k2, -1, 12 - i1, structureBoundingBoxIn);
        }
    }
    for (int l2 = 0; l2 <= 2; ++l2)
    {
        for (int i3 = 4; i3 <= 8; ++i3)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), l2, -1, i3, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), 12 - l2, -1, i3, structureBoundingBoxIn);
        }
    }
    return true;
}

Stairs::Stairs(int p_i45609_1_, Random& p_i45609_2_, const StructureBoundingBox& p_i45609_3_, EnumFacing p_i45609_4_)
    : Piece(p_i45609_1_)
{
    coordBaseMode = p_i45609_4_;
    boundingBox = p_i45609_3_;
}

void Stairs::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentZ(static_cast<Start*>(componentIn), listIn, rand, 6, 2, false);
}

std::shared_ptr<Piece> Stairs::func_175872_a(StructureComponentList& p_175872_0_, Random& p_175872_1_, int p_175872_2_, int p_175872_3_, int p_175872_4_, int p_175872_5_, EnumFacing p_175872_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175872_2_, p_175872_3_, p_175872_4_, -2, 0, 0, 7, 11, 7, p_175872_6_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175872_0_, structureboundingbox) == nullptr ? std::make_shared<Stairs>(p_175872_5_, p_175872_1_, structureboundingbox, p_175872_6_) : nullptr;
}

bool Stairs::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 6, 1, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 6, 10, 6, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 1, 8, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 0, 6, 8, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 1, 0, 8, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 2, 1, 6, 8, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 6, 5, 8, 6, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 2, 0, 5, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 3, 2, 6, 5, 2, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 3, 4, 6, 5, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    setBlockState(worldIn, Blocks::nether_brick->getDefaultState(), 5, 2, 5, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 2, 5, 4, 3, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 2, 5, 3, 4, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 2, 5, 2, 5, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 5, 1, 6, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 7, 1, 5, 7, 4, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 8, 2, 6, 8, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 6, 0, 4, 8, 0, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 5, 0, 4, 5, 0, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    for (int i = 0; i <= 6; ++i)
    {
        for (int j = 0; j <= 6; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, j, structureBoundingBoxIn);
        }
    }
    return true;
}

Straight::Straight(int p_i45620_1_, Random& p_i45620_2_, const StructureBoundingBox& p_i45620_3_, EnumFacing p_i45620_4_)
    : Piece(p_i45620_1_)
{
    coordBaseMode = p_i45620_4_;
    boundingBox = p_i45620_3_;
}

void Straight::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Start*>(componentIn), listIn, rand, 1, 3, false);
}

std::shared_ptr<Piece> Straight::func_175882_a(StructureComponentList& p_175882_0_, Random& p_175882_1_, int p_175882_2_, int p_175882_3_, int p_175882_4_, EnumFacing p_175882_5_, int p_175882_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175882_2_, p_175882_3_, p_175882_4_, -1, -3, 0, 5, 10, 19, p_175882_5_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175882_0_, structureboundingbox) == nullptr ? std::make_shared<Straight>(p_175882_6_, p_175882_1_, structureboundingbox, p_175882_5_) : nullptr;
}

bool Straight::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 0, 4, 4, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 5, 0, 3, 7, 18, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 0, 0, 5, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 5, 0, 4, 5, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 4, 2, 5, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 13, 4, 2, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 1, 3, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 15, 4, 1, 18, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    for (int i = 0; i <= 4; ++i)
    {
        for (int j = 0; j <= 2; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, j, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, 18 - j, structureBoundingBoxIn);
        }
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 1, 0, 4, 1, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 4, 0, 4, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 14, 0, 4, 14, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 17, 0, 4, 17, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 1, 4, 4, 1, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 3, 4, 4, 4, 4, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 3, 14, 4, 4, 14, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 17, 4, 4, 17, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    return true;
}

Throne::Throne(int p_i45611_1_, Random& p_i45611_2_, const StructureBoundingBox& p_i45611_3_, EnumFacing p_i45611_4_)
    : Piece(p_i45611_1_)
{
    coordBaseMode = p_i45611_4_;
    boundingBox = p_i45611_3_;
}

void Throne::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Piece::readStructureFromNBT(tagCompound);
    hasSpawner = tagCompound.getBoolean("Mob");
}

void Throne::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Piece::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Mob", hasSpawner);
}

std::shared_ptr<Piece> Throne::func_175874_a(StructureComponentList& p_175874_0_, Random& p_175874_1_, int p_175874_2_, int p_175874_3_, int p_175874_4_, int p_175874_5_, EnumFacing p_175874_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175874_2_, p_175874_3_, p_175874_4_, -2, 0, 0, 7, 8, 9, p_175874_6_);
    return isAboveGround(structureboundingbox) && findIntersecting(p_175874_0_, structureboundingbox) == nullptr ? std::make_shared<Throne>(p_175874_5_, p_175874_1_, structureboundingbox, p_175874_6_) : nullptr;
}

bool Throne::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 6, 7, 7, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 0, 5, 1, 7, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 1, 5, 2, 7, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 2, 5, 3, 7, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 3, 5, 4, 7, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 1, 4, 2, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 0, 5, 4, 2, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 5, 2, 1, 5, 3, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 5, 2, 5, 5, 3, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 3, 0, 5, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 5, 3, 6, 5, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 5, 8, 5, 5, 8, Blocks::nether_brick->getDefaultState(), Blocks::nether_brick->getDefaultState(), false);
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 1, 6, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::nether_brick_fence->getDefaultState(), 5, 6, 3, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 6, 3, 0, 6, 8, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 6, 3, 6, 6, 8, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 6, 8, 5, 7, 8, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 8, 8, 4, 8, 8, Blocks::nether_brick_fence->getDefaultState(), Blocks::nether_brick_fence->getDefaultState(), false);
    if (!hasSpawner)
    {
        const BlockPos blockpos(getXWithOffset(3, 5), getYWithOffset(5), getZWithOffset(3, 5));
        if (structureBoundingBoxIn.isVecInside(blockpos))
        {
            hasSpawner = true;
            worldIn->setBlockState(blockpos, Blocks::mob_spawner->getDefaultState(), 2);
            if (auto* spawner = dynamic_cast<TileEntityMobSpawner*>(worldIn->getTileEntity(blockpos)))
                spawner->getSpawnerBaseLogic().setEntityName("Blaze");
        }
    }
    for (int i = 0; i <= 6; ++i)
    {
        for (int j = 0; j <= 6; ++j)
        {
            replaceAirAndLiquidDownwards(worldIn, Blocks::nether_brick->getDefaultState(), i, -1, j, structureBoundingBoxIn);
        }
    }
    return true;
}


} // namespace StructureNetherBridgePieces

// --- MapGenNetherBridge ---

MapGenNetherBridge::MapGenNetherBridge()
    : spawnList { { "Blaze", 10, 2, 3 }, { "PigZombie", 5, 4, 4 }, { "Skeleton", 10, 4, 4 }, { "LavaSlime", 3, 4, 4 } }
{
}

bool MapGenNetherBridge::canSpawnStructureAtCoords(int chunkX, int chunkZ)
{
    const int i = chunkX >> 4;
    const int j = chunkZ >> 4;
    rand.setSeed(static_cast<int64_t>(i ^ (j << 4)) ^ worldObj->getSeed());
    rand.nextInt();
    if (rand.nextInt(3) != 0)
        return false;
    if (chunkX != (i << 4) + 4 + rand.nextInt(8))
        return false;
    return chunkZ == (j << 4) + 4 + rand.nextInt(8);
}

std::shared_ptr<StructureStart> MapGenNetherBridge::getStructureStart(int chunkX, int chunkZ)
{
    return std::make_shared<Start>(worldObj, rand, chunkX, chunkZ);
}

MapGenNetherBridge::Start::Start(World* worldIn, Random& rand, int chunkX, int chunkZ)
    : StructureStart(chunkX, chunkZ)
{
    auto start = std::make_shared<StructureNetherBridgePieces::Start>(rand, (chunkX << 4) + 2, (chunkZ << 4) + 2);
    components.push_back(start);
    start->buildComponent(start.get(), components, rand);
    StructureComponentList& list = start->field_74967_d;
    while (!list.empty()) {
        const int i = rand.nextInt(static_cast<int>(list.size()));
        const std::shared_ptr<StructureComponent> structurecomponent = list[static_cast<size_t>(i)];
        list.erase(list.begin() + i);
        structurecomponent->buildComponent(start.get(), components, rand);
    }
    updateBoundingBox();
    setRandomHeight(worldIn, rand, 48, 70);
}

} // namespace mc
