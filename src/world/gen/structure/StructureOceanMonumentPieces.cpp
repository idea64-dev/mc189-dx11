#include "world/gen/structure/StructureOceanMonumentPieces.h"

#include "block/AllBlocks.h"
#include "block/material/Material.h"
#include "core/Java.h"
#include "init/Blocks.h"
#include "nbt/NBTTagCompound.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/WorldChunkManager.h"
#include "world/gen/structure/MapGenStructure.h"
#include "world/gen/structure/StructureOceanMonument.h"

#include <algorithm>
#include <utility>

namespace mc {

namespace StructureOceanMonumentPieces {

namespace {

// Java: MonumentRoomFitHelper and its implementations, tried in this order for each free room.
struct MonumentRoomFitHelper {
    bool (*func_175969_a)(const RoomDefinition* room);
    std::shared_ptr<Piece> (*func_175968_a)(EnumFacing facing, RoomDefinition* room, Random& rand);
};

constexpr int EAST_INDEX = getIndex(EnumFacing::EAST);
constexpr int NORTH_INDEX = getIndex(EnumFacing::NORTH);
constexpr int SOUTH_INDEX = getIndex(EnumFacing::SOUTH);
constexpr int WEST_INDEX = getIndex(EnumFacing::WEST);
constexpr int UP_INDEX = getIndex(EnumFacing::UP);

const MonumentRoomFitHelper fitHelpers[] {
    // XYDoubleRoomFitHelper
    { [](const RoomDefinition* room) {
         if (room->field_175966_c[EAST_INDEX] && !room->field_175965_b[EAST_INDEX]->field_175963_d && room->field_175966_c[UP_INDEX]
             && !room->field_175965_b[UP_INDEX]->field_175963_d) {
             const RoomDefinition* east = room->field_175965_b[EAST_INDEX];
             return east->field_175966_c[UP_INDEX] && !east->field_175965_b[UP_INDEX]->field_175963_d;
         }
         return false;
     },
        [](EnumFacing facing, RoomDefinition* room, Random& rand) -> std::shared_ptr<Piece> {
            room->field_175963_d = true;
            room->field_175965_b[EAST_INDEX]->field_175963_d = true;
            room->field_175965_b[UP_INDEX]->field_175963_d = true;
            room->field_175965_b[EAST_INDEX]->field_175965_b[UP_INDEX]->field_175963_d = true;
            return std::make_shared<DoubleXYRoom>(facing, room, rand);
        } },
    // YZDoubleRoomFitHelper
    { [](const RoomDefinition* room) {
         if (room->field_175966_c[NORTH_INDEX] && !room->field_175965_b[NORTH_INDEX]->field_175963_d && room->field_175966_c[UP_INDEX]
             && !room->field_175965_b[UP_INDEX]->field_175963_d) {
             const RoomDefinition* north = room->field_175965_b[NORTH_INDEX];
             return north->field_175966_c[UP_INDEX] && !north->field_175965_b[UP_INDEX]->field_175963_d;
         }
         return false;
     },
        [](EnumFacing facing, RoomDefinition* room, Random& rand) -> std::shared_ptr<Piece> {
            room->field_175963_d = true;
            room->field_175965_b[NORTH_INDEX]->field_175963_d = true;
            room->field_175965_b[UP_INDEX]->field_175963_d = true;
            room->field_175965_b[NORTH_INDEX]->field_175965_b[UP_INDEX]->field_175963_d = true;
            return std::make_shared<DoubleYZRoom>(facing, room, rand);
        } },
    // ZDoubleRoomFitHelper
    { [](const RoomDefinition* room) { return room->field_175966_c[NORTH_INDEX] && !room->field_175965_b[NORTH_INDEX]->field_175963_d; },
        [](EnumFacing facing, RoomDefinition* room, Random& rand) -> std::shared_ptr<Piece> {
            RoomDefinition* roomdefinition = room;
            if (!room->field_175966_c[NORTH_INDEX] || room->field_175965_b[NORTH_INDEX]->field_175963_d)
                roomdefinition = room->field_175965_b[SOUTH_INDEX];
            roomdefinition->field_175963_d = true;
            roomdefinition->field_175965_b[NORTH_INDEX]->field_175963_d = true;
            return std::make_shared<DoubleZRoom>(facing, roomdefinition, rand);
        } },
    // XDoubleRoomFitHelper
    { [](const RoomDefinition* room) { return room->field_175966_c[EAST_INDEX] && !room->field_175965_b[EAST_INDEX]->field_175963_d; },
        [](EnumFacing facing, RoomDefinition* room, Random& rand) -> std::shared_ptr<Piece> {
            room->field_175963_d = true;
            room->field_175965_b[EAST_INDEX]->field_175963_d = true;
            return std::make_shared<DoubleXRoom>(facing, room, rand);
        } },
    // YDoubleRoomFitHelper
    { [](const RoomDefinition* room) { return room->field_175966_c[UP_INDEX] && !room->field_175965_b[UP_INDEX]->field_175963_d; },
        [](EnumFacing facing, RoomDefinition* room, Random& rand) -> std::shared_ptr<Piece> {
            room->field_175963_d = true;
            room->field_175965_b[UP_INDEX]->field_175963_d = true;
            return std::make_shared<DoubleYRoom>(facing, room, rand);
        } },
    // FitSimpleRoomTopHelper
    { [](const RoomDefinition* room) {
         return !room->field_175966_c[WEST_INDEX] && !room->field_175966_c[EAST_INDEX] && !room->field_175966_c[NORTH_INDEX] && !room->field_175966_c[SOUTH_INDEX]
             && !room->field_175966_c[UP_INDEX];
     },
        [](EnumFacing facing, RoomDefinition* room, Random& rand) -> std::shared_ptr<Piece> {
            room->field_175963_d = true;
            return std::make_shared<SimpleTopRoom>(facing, room, rand);
        } },
    // FitSimpleRoomHelper
    { [](const RoomDefinition*) { return true; },
        [](EnumFacing facing, RoomDefinition* room, Random& rand) -> std::shared_ptr<Piece> {
            room->field_175963_d = true;
            return std::make_shared<SimpleRoom>(facing, room, rand);
        } },
};

// Java: Collections.shuffle(list, rnd) for a random-access list.
template <class T>
void javaShuffle(std::vector<T>& list, Random& rnd)
{
    for (int i = static_cast<int>(list.size()); i > 1; --i)
        std::swap(list[static_cast<size_t>(i - 1)], list[static_cast<size_t>(rnd.nextInt(i))]);
}

} // namespace

void registerOceanMonumentPieces()
{
    MapGenStructureIO::registerStructureComponent("OMB", [] { return std::make_shared<MonumentBuilding>(); });
    MapGenStructureIO::registerStructureComponent("OMCR", [] { return std::make_shared<MonumentCoreRoom>(); });
    MapGenStructureIO::registerStructureComponent("OMDXR", [] { return std::make_shared<DoubleXRoom>(); });
    MapGenStructureIO::registerStructureComponent("OMDXYR", [] { return std::make_shared<DoubleXYRoom>(); });
    MapGenStructureIO::registerStructureComponent("OMDYR", [] { return std::make_shared<DoubleYRoom>(); });
    MapGenStructureIO::registerStructureComponent("OMDYZR", [] { return std::make_shared<DoubleYZRoom>(); });
    MapGenStructureIO::registerStructureComponent("OMDZR", [] { return std::make_shared<DoubleZRoom>(); });
    MapGenStructureIO::registerStructureComponent("OMEntry", [] { return std::make_shared<EntryRoom>(); });
    MapGenStructureIO::registerStructureComponent("OMPenthouse", [] { return std::make_shared<Penthouse>(); });
    MapGenStructureIO::registerStructureComponent("OMSimple", [] { return std::make_shared<SimpleRoom>(); });
    MapGenStructureIO::registerStructureComponent("OMSimpleT", [] { return std::make_shared<SimpleTopRoom>(); });
}

// --- RoomDefinition ---

void RoomDefinition::func_175957_a(EnumFacing facing, RoomDefinition* other)
{
    field_175965_b[static_cast<size_t>(getIndex(facing))] = other;
    other->field_175965_b[static_cast<size_t>(getIndex(getOpposite(facing)))] = this;
}

void RoomDefinition::func_175958_a()
{
    for (size_t i = 0; i < 6; ++i)
        field_175966_c[i] = field_175965_b[i] != nullptr;
}

bool RoomDefinition::func_175959_a(int mark)
{
    if (field_175964_e)
        return true;
    field_175962_f = mark;
    for (size_t i = 0; i < 6; ++i) {
        if (field_175965_b[i] && field_175966_c[i] && field_175965_b[i]->field_175962_f != mark && field_175965_b[i]->func_175959_a(mark))
            return true;
    }
    return false;
}

int RoomDefinition::func_175960_c() const
{
    int i = 0;
    for (size_t j = 0; j < 6; ++j) {
        if (field_175966_c[j])
            ++i;
    }
    return i;
}

// --- Piece ---

const IBlockState* Piece::field_175828_a()
{
    return Blocks::prismarine->getStateFromMeta(BlockPrismarine::ROUGH_META);
}

const IBlockState* Piece::field_175826_b()
{
    return Blocks::prismarine->getStateFromMeta(BlockPrismarine::BRICKS_META);
}

const IBlockState* Piece::field_175827_c()
{
    return Blocks::prismarine->getStateFromMeta(BlockPrismarine::DARK_META);
}

const IBlockState* Piece::field_175825_e()
{
    return Blocks::sea_lantern->getDefaultState();
}

const IBlockState* Piece::field_175822_f()
{
    return Blocks::water->getDefaultState();
}

Piece::Piece(EnumFacing facing, const StructureBoundingBox& structurebb)
    : StructureComponent(1)
{
    coordBaseMode = facing;
    boundingBox = structurebb;
}

Piece::Piece(int type, EnumFacing facing, RoomDefinition* room, int sizeX, int sizeY, int sizeZ)
    : StructureComponent(type)
{
    coordBaseMode = facing;
    field_175830_k = room;
    const int i = room->field_175967_a;
    const int j = i % 5;
    const int k = i / 5 % 5;
    const int l = i / 25;
    if (facing != EnumFacing::NORTH && facing != EnumFacing::SOUTH)
        boundingBox = StructureBoundingBox(0, 0, 0, sizeZ * 8 - 1, sizeY * 4 - 1, sizeX * 8 - 1);
    else
        boundingBox = StructureBoundingBox(0, 0, 0, sizeX * 8 - 1, sizeY * 4 - 1, sizeZ * 8 - 1);
    switch (facing) {
    case EnumFacing::NORTH:
        boundingBox.offset(j * 8, l * 4, -(k + sizeZ) * 8 + 1);
        break;
    case EnumFacing::SOUTH:
        boundingBox.offset(j * 8, l * 4, k * 8);
        break;
    case EnumFacing::WEST:
        boundingBox.offset(-(k + sizeZ) * 8 + 1, l * 4, j * 8);
        break;
    default:
        boundingBox.offset(k * 8, l * 4, j * 8);
        break;
    }
}

void Piece::func_181655_a(World* worldIn, const StructureBoundingBox& structurebb, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool skipAir)
{
    for (int i = minY; i <= maxY; ++i) {
        for (int j = minX; j <= maxX; ++j) {
            for (int k = minZ; k <= maxZ; ++k) {
                if (!skipAir || !isMaterial(getBlockStateFromPos(worldIn, j, i, k, structurebb), Material::air)) {
                    if (getYWithOffset(i) >= worldIn->getSeaLevel())
                        setBlockState(worldIn, Blocks::air->getDefaultState(), j, i, k, structurebb);
                    else
                        setBlockState(worldIn, field_175822_f(), j, i, k, structurebb);
                }
            }
        }
    }
}

void Piece::func_175821_a(World* worldIn, const StructureBoundingBox& structurebb, int x, int z, bool hasOpening)
{
    if (hasOpening) {
        fillWithBlocks(worldIn, structurebb, x + 0, 0, z + 0, x + 2, 0, z + 8 - 1, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, structurebb, x + 5, 0, z + 0, x + 8 - 1, 0, z + 8 - 1, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, structurebb, x + 3, 0, z + 0, x + 4, 0, z + 2, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, structurebb, x + 3, 0, z + 5, x + 4, 0, z + 8 - 1, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, structurebb, x + 3, 0, z + 2, x + 4, 0, z + 2, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structurebb, x + 3, 0, z + 5, x + 4, 0, z + 5, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structurebb, x + 2, 0, z + 3, x + 2, 0, z + 4, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structurebb, x + 5, 0, z + 3, x + 5, 0, z + 4, field_175826_b(), field_175826_b(), false);
    } else {
        fillWithBlocks(worldIn, structurebb, x + 0, 0, z + 0, x + 8 - 1, 0, z + 8 - 1, field_175828_a(), field_175828_a(), false);
    }
}

void Piece::func_175819_a(World* worldIn, const StructureBoundingBox& structurebb, int minX, int minY, int minZ, int maxX, int maxY, int maxZ,
    const IBlockState* state)
{
    for (int i = minY; i <= maxY; ++i) {
        for (int j = minX; j <= maxX; ++j) {
            for (int k = minZ; k <= maxZ; ++k) {
                if (getBlockStateFromPos(worldIn, j, i, k, structurebb) == field_175822_f())
                    setBlockState(worldIn, state, j, i, k, structurebb);
            }
        }
    }
}

bool Piece::func_175818_a(const StructureBoundingBox& structurebb, int minX, int minZ, int maxX, int maxZ)
{
    const int i = getXWithOffset(minX, minZ);
    const int j = getZWithOffset(minX, minZ);
    const int k = getXWithOffset(maxX, maxZ);
    const int l = getZWithOffset(maxX, maxZ);
    return structurebb.intersectsWith(std::min(i, k), std::min(j, l), std::max(i, k), std::max(j, l));
}

bool Piece::func_175817_a(World*, const StructureBoundingBox& structurebb, int x, int y, int z)
{
    const int i = getXWithOffset(x, z);
    const int j = getYWithOffset(y);
    const int k = getZWithOffset(x, z);
    if (structurebb.isVecInside(BlockPos(i, j, k))) {
        // Java: an elder EntityGuardian at (i + 0.5, j, k + 0.5) -- entities come with stage 10. Its setup only
        // draws from the entity's own random, not the world's.
        return true;
    }
    return false;
}

// --- the rooms ---

#define MC_ROOM_CTOR(NAME, X, Y, Z)                                                                                                                 \
    NAME::NAME(EnumFacing facing, RoomDefinition* room, Random&)                                                                                   \
        : Piece(1, facing, room, X, Y, Z)                                                                                                           \
    {                                                                                                                                               \
    }

MC_ROOM_CTOR(DoubleXRoom, 2, 1, 1)
MC_ROOM_CTOR(DoubleXYRoom, 2, 2, 1)
MC_ROOM_CTOR(DoubleYRoom, 1, 2, 1)
MC_ROOM_CTOR(DoubleYZRoom, 1, 2, 2)
MC_ROOM_CTOR(DoubleZRoom, 1, 1, 2)
MC_ROOM_CTOR(MonumentCoreRoom, 2, 2, 2)
MC_ROOM_CTOR(SimpleTopRoom, 1, 1, 1)

#undef MC_ROOM_CTOR

SimpleRoom::SimpleRoom(EnumFacing facing, RoomDefinition* room, Random& rand)
    : Piece(1, facing, room, 1, 1, 1)
{
    field_175833_o = rand.nextInt(3);
}

EntryRoom::EntryRoom(EnumFacing facing, RoomDefinition* room)
    : Piece(1, facing, room, 1, 1, 1)
{
}

Penthouse::Penthouse(EnumFacing facing, const StructureBoundingBox& structurebb)
    : Piece(facing, structurebb)
{
}

WingRoom::WingRoom(EnumFacing facing, const StructureBoundingBox& structurebb, int index)
    : Piece(facing, structurebb)
{
    field_175834_o = index & 1;
}

// --- MonumentBuilding ---

MonumentBuilding::MonumentBuilding(Random& rand, int x, int z, EnumFacing facing)
    : Piece(0)
{
    coordBaseMode = facing;
    // Java switches on the facing but builds the same box either way.
    boundingBox = StructureBoundingBox(x, 39, z, x + 58 - 1, 61, z + 58 - 1);
    const std::vector<RoomDefinition*> list = func_175836_a(rand);
    field_175845_o->field_175963_d = true;
    field_175843_q.push_back(std::make_shared<EntryRoom>(facing, field_175845_o));
    field_175843_q.push_back(std::make_shared<MonumentCoreRoom>(facing, field_175844_p, rand));
    for (RoomDefinition* roomdefinition : list) {
        if (!roomdefinition->field_175963_d && !roomdefinition->func_175961_b()) {
            for (const MonumentRoomFitHelper& helper : fitHelpers) {
                if (helper.func_175969_a(roomdefinition)) {
                    field_175843_q.push_back(helper.func_175968_a(facing, roomdefinition, rand));
                    break;
                }
            }
        }
    }
    const int j = boundingBox.minY;
    const int k = getXWithOffset(9, 22);
    const int l = getZWithOffset(9, 22);
    for (const auto& piece : field_175843_q)
        piece->func_181138_a(k, j, l);
    const StructureBoundingBox structureboundingbox1 = StructureBoundingBox::func_175899_a(getXWithOffset(1, 1), getYWithOffset(1), getZWithOffset(1, 1),
        getXWithOffset(23, 21), getYWithOffset(8), getZWithOffset(23, 21));
    const StructureBoundingBox structureboundingbox2 = StructureBoundingBox::func_175899_a(getXWithOffset(34, 1), getYWithOffset(1), getZWithOffset(34, 1),
        getXWithOffset(56, 21), getYWithOffset(8), getZWithOffset(56, 21));
    const StructureBoundingBox structureboundingbox = StructureBoundingBox::func_175899_a(getXWithOffset(22, 22), getYWithOffset(13), getZWithOffset(22, 22),
        getXWithOffset(35, 35), getYWithOffset(17), getZWithOffset(35, 35));
    int i = rand.nextInt();
    field_175843_q.push_back(std::make_shared<WingRoom>(facing, structureboundingbox1, i++));
    field_175843_q.push_back(std::make_shared<WingRoom>(facing, structureboundingbox2, i++));
    field_175843_q.push_back(std::make_shared<Penthouse>(facing, structureboundingbox));
}

std::vector<RoomDefinition*> MonumentBuilding::func_175836_a(Random& rand)
{
    std::array<RoomDefinition*, 75> rooms {};
    const auto makeRoom = [this](int index) {
        roomStorage.push_back(std::make_unique<RoomDefinition>(index));
        return roomStorage.back().get();
    };
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 4; ++j) {
            const int l = func_175820_a(i, 0, j);
            rooms[static_cast<size_t>(l)] = makeRoom(l);
        }
    }
    for (int i2 = 0; i2 < 5; ++i2) {
        for (int l2 = 0; l2 < 4; ++l2) {
            const int j4 = func_175820_a(i2, 1, l2);
            rooms[static_cast<size_t>(j4)] = makeRoom(j4);
        }
    }
    for (int j2 = 1; j2 < 4; ++j2) {
        for (int i3 = 0; i3 < 2; ++i3) {
            const int k4 = func_175820_a(j2, 2, i3);
            rooms[static_cast<size_t>(k4)] = makeRoom(k4);
        }
    }
    field_175845_o = rooms[field_175823_g];
    for (int k2 = 0; k2 < 5; ++k2) {
        for (int j3 = 0; j3 < 5; ++j3) {
            for (int i4 = 0; i4 < 3; ++i4) {
                const int l4 = func_175820_a(k2, i4, j3);
                if (rooms[static_cast<size_t>(l4)]) {
                    // Java: EnumFacing.values()
                    for (int f = 0; f < 6; ++f) {
                        const EnumFacing enumfacing = getFront(f);
                        const int i1 = k2 + getFrontOffsetX(enumfacing);
                        const int j1 = i4 + getFrontOffsetY(enumfacing);
                        const int k1 = j3 + getFrontOffsetZ(enumfacing);
                        if (i1 >= 0 && i1 < 5 && k1 >= 0 && k1 < 5 && j1 >= 0 && j1 < 3) {
                            const int l1 = func_175820_a(i1, j1, k1);
                            if (rooms[static_cast<size_t>(l1)]) {
                                if (k1 != j3)
                                    rooms[static_cast<size_t>(l4)]->func_175957_a(getOpposite(enumfacing), rooms[static_cast<size_t>(l1)]);
                                else
                                    rooms[static_cast<size_t>(l4)]->func_175957_a(enumfacing, rooms[static_cast<size_t>(l1)]);
                            }
                        }
                    }
                }
            }
        }
    }
    RoomDefinition* roomdefinition = makeRoom(1003);
    rooms[field_175831_h]->func_175957_a(EnumFacing::UP, roomdefinition);
    RoomDefinition* roomdefinition1 = makeRoom(1001);
    rooms[field_175832_i]->func_175957_a(EnumFacing::SOUTH, roomdefinition1);
    RoomDefinition* roomdefinition2 = makeRoom(1002);
    rooms[field_175829_j]->func_175957_a(EnumFacing::SOUTH, roomdefinition2);
    roomdefinition->field_175963_d = true;
    roomdefinition1->field_175963_d = true;
    roomdefinition2->field_175963_d = true;
    field_175845_o->field_175964_e = true;
    field_175844_p = rooms[static_cast<size_t>(func_175820_a(rand.nextInt(4), 0, 2))];
    field_175844_p->field_175963_d = true;
    field_175844_p->field_175965_b[EAST_INDEX]->field_175963_d = true;
    field_175844_p->field_175965_b[NORTH_INDEX]->field_175963_d = true;
    field_175844_p->field_175965_b[EAST_INDEX]->field_175965_b[NORTH_INDEX]->field_175963_d = true;
    field_175844_p->field_175965_b[UP_INDEX]->field_175963_d = true;
    field_175844_p->field_175965_b[EAST_INDEX]->field_175965_b[UP_INDEX]->field_175963_d = true;
    field_175844_p->field_175965_b[NORTH_INDEX]->field_175965_b[UP_INDEX]->field_175963_d = true;
    field_175844_p->field_175965_b[EAST_INDEX]->field_175965_b[NORTH_INDEX]->field_175965_b[UP_INDEX]->field_175963_d = true;
    std::vector<RoomDefinition*> list;
    for (RoomDefinition* roomdefinition4 : rooms) {
        if (roomdefinition4) {
            roomdefinition4->func_175958_a();
            list.push_back(roomdefinition4);
        }
    }
    roomdefinition->func_175958_a();
    javaShuffle(list, rand);
    int i5 = 1;
    for (RoomDefinition* roomdefinition3 : list) {
        int j5 = 0;
        int k5 = 0;
        while (j5 < 2 && k5 < 5) {
            ++k5;
            const int l5 = rand.nextInt(6);
            if (roomdefinition3->field_175966_c[static_cast<size_t>(l5)]) {
                const int i6 = getIndex(getOpposite(getFront(l5)));
                roomdefinition3->field_175966_c[static_cast<size_t>(l5)] = false;
                roomdefinition3->field_175965_b[static_cast<size_t>(l5)]->field_175966_c[static_cast<size_t>(i6)] = false;
                // Java: func_175959_a(i5++) && neighbour.func_175959_a(i5++), the second only when the first holds.
                bool connected = roomdefinition3->func_175959_a(i5++);
                if (connected)
                    connected = roomdefinition3->field_175965_b[static_cast<size_t>(l5)]->func_175959_a(i5++);
                if (connected) {
                    ++j5;
                } else {
                    roomdefinition3->field_175966_c[static_cast<size_t>(l5)] = true;
                    roomdefinition3->field_175965_b[static_cast<size_t>(l5)]->field_175966_c[static_cast<size_t>(i6)] = true;
                }
            }
        }
    }
    list.push_back(roomdefinition);
    list.push_back(roomdefinition1);
    list.push_back(roomdefinition2);
    return list;
}

bool DoubleXRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    RoomDefinition* roomdefinition = field_175830_k->field_175965_b[getIndex(EnumFacing::EAST)];
    RoomDefinition* roomdefinition1 = field_175830_k;
    if (field_175830_k->field_175967_a / 25 > 0)
    {
        func_175821_a(worldIn, structureBoundingBoxIn, 8, 0, roomdefinition->field_175966_c[getIndex(EnumFacing::DOWN)]);
        func_175821_a(worldIn, structureBoundingBoxIn, 0, 0, roomdefinition1->field_175966_c[getIndex(EnumFacing::DOWN)]);
    }
    if (roomdefinition1->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 1, 4, 1, 7, 4, 6, field_175828_a());
    }
    if (roomdefinition->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 8, 4, 1, 14, 4, 6, field_175828_a());
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 0, 0, 3, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 15, 3, 0, 15, 3, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 0, 15, 3, 0, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 7, 14, 3, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 2, 7, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 15, 2, 0, 15, 2, 7, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 15, 2, 0, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 7, 14, 2, 7, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 0, 1, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 15, 1, 0, 15, 1, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 0, 15, 1, 0, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 7, 14, 1, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 0, 10, 1, 4, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 2, 0, 9, 2, 3, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 3, 0, 10, 3, 4, field_175826_b(), field_175826_b(), false);
    setBlockState(worldIn, field_175825_e(), 6, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, field_175825_e(), 9, 2, 3, structureBoundingBoxIn);
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 0, 4, 2, 0, false);
    }
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 7, 4, 2, 7, false);
    }
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 1, 3, 0, 2, 4, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 11, 1, 0, 12, 2, 0, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 11, 1, 7, 12, 2, 7, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 15, 1, 3, 15, 2, 4, false);
    }
    return true;
}

bool DoubleXYRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    RoomDefinition* roomdefinition = field_175830_k->field_175965_b[getIndex(EnumFacing::EAST)];
    RoomDefinition* roomdefinition1 = field_175830_k;
    RoomDefinition* roomdefinition2 = roomdefinition1->field_175965_b[getIndex(EnumFacing::UP)];
    RoomDefinition* roomdefinition3 = roomdefinition->field_175965_b[getIndex(EnumFacing::UP)];
    if (field_175830_k->field_175967_a / 25 > 0)
    {
        func_175821_a(worldIn, structureBoundingBoxIn, 8, 0, roomdefinition->field_175966_c[getIndex(EnumFacing::DOWN)]);
        func_175821_a(worldIn, structureBoundingBoxIn, 0, 0, roomdefinition1->field_175966_c[getIndex(EnumFacing::DOWN)]);
    }
    if (roomdefinition2->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 1, 8, 1, 7, 8, 6, field_175828_a());
    }
    if (roomdefinition3->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 8, 8, 1, 14, 8, 6, field_175828_a());
    }
    for (int i = 1; i <= 7; ++i)
    {
        const IBlockState* iblockstate = field_175826_b();
        if (i == 2 || i == 6)
        {
            iblockstate = field_175828_a();
        }
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, i, 0, 0, i, 7, iblockstate, iblockstate, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 15, i, 0, 15, i, 7, iblockstate, iblockstate, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, i, 0, 15, i, 0, iblockstate, iblockstate, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, i, 7, 14, i, 7, iblockstate, iblockstate, false);
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 3, 2, 7, 4, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 2, 4, 7, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 5, 4, 7, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 13, 1, 3, 13, 7, 4, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 1, 2, 12, 7, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 11, 1, 5, 12, 7, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 3, 5, 3, 4, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 1, 3, 10, 3, 4, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 7, 2, 10, 7, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 5, 2, 5, 7, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 5, 2, 10, 7, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 5, 5, 5, 7, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 5, 5, 10, 7, 5, field_175826_b(), field_175826_b(), false);
    setBlockState(worldIn, field_175826_b(), 6, 6, 2, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 9, 6, 2, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 6, 6, 5, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 9, 6, 5, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 4, 3, 6, 4, 4, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 4, 3, 10, 4, 4, field_175826_b(), field_175826_b(), false);
    setBlockState(worldIn, field_175825_e(), 5, 4, 2, structureBoundingBoxIn);
    setBlockState(worldIn, field_175825_e(), 5, 4, 5, structureBoundingBoxIn);
    setBlockState(worldIn, field_175825_e(), 10, 4, 2, structureBoundingBoxIn);
    setBlockState(worldIn, field_175825_e(), 10, 4, 5, structureBoundingBoxIn);
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 0, 4, 2, 0, false);
    }
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 7, 4, 2, 7, false);
    }
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 1, 3, 0, 2, 4, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 11, 1, 0, 12, 2, 0, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 11, 1, 7, 12, 2, 7, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 15, 1, 3, 15, 2, 4, false);
    }
    if (roomdefinition2->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 5, 0, 4, 6, 0, false);
    }
    if (roomdefinition2->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 5, 7, 4, 6, 7, false);
    }
    if (roomdefinition2->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 5, 3, 0, 6, 4, false);
    }
    if (roomdefinition3->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 11, 5, 0, 12, 6, 0, false);
    }
    if (roomdefinition3->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 11, 5, 7, 12, 6, 7, false);
    }
    if (roomdefinition3->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 15, 5, 3, 15, 6, 4, false);
    }
    return true;
}

bool DoubleYRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_175830_k->field_175967_a / 25 > 0)
    {
        func_175821_a(worldIn, structureBoundingBoxIn, 0, 0, field_175830_k->field_175966_c[getIndex(EnumFacing::DOWN)]);
    }
    RoomDefinition* roomdefinition = field_175830_k->field_175965_b[getIndex(EnumFacing::UP)];
    if (roomdefinition->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 1, 8, 1, 6, 8, 6, field_175828_a());
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 0, 0, 4, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 4, 0, 7, 4, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 0, 6, 4, 0, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 7, 6, 4, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 4, 1, 2, 4, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 2, 1, 4, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 4, 1, 5, 4, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 4, 2, 6, 4, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 4, 5, 2, 4, 6, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 5, 1, 4, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 4, 5, 5, 4, 6, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 4, 5, 6, 4, 5, field_175826_b(), field_175826_b(), false);
    RoomDefinition* roomdefinition1 = field_175830_k;
    for (int i = 1; i <= 5; i += 4)
    {
        int j = 0;
        if (roomdefinition1->field_175966_c[getIndex(EnumFacing::SOUTH)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 2, i, j, 2, i + 2, j, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 5, i, j, 5, i + 2, j, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, i + 2, j, 4, i + 2, j, field_175826_b(), field_175826_b(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, i, j, 7, i + 2, j, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, i + 1, j, 7, i + 1, j, field_175828_a(), field_175828_a(), false);
        }
        j = 7;
        if (roomdefinition1->field_175966_c[getIndex(EnumFacing::NORTH)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 2, i, j, 2, i + 2, j, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 5, i, j, 5, i + 2, j, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, i + 2, j, 4, i + 2, j, field_175826_b(), field_175826_b(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, i, j, 7, i + 2, j, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, i + 1, j, 7, i + 1, j, field_175828_a(), field_175828_a(), false);
        }
        int k = 0;
        if (roomdefinition1->field_175966_c[getIndex(EnumFacing::WEST)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i, 2, k, i + 2, 2, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i, 5, k, i + 2, 5, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i + 2, 3, k, i + 2, 4, field_175826_b(), field_175826_b(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i, 0, k, i + 2, 7, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i + 1, 0, k, i + 1, 7, field_175828_a(), field_175828_a(), false);
        }
        k = 7;
        if (roomdefinition1->field_175966_c[getIndex(EnumFacing::EAST)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i, 2, k, i + 2, 2, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i, 5, k, i + 2, 5, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i + 2, 3, k, i + 2, 4, field_175826_b(), field_175826_b(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i, 0, k, i + 2, 7, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, k, i + 1, 0, k, i + 1, 7, field_175828_a(), field_175828_a(), false);
        }
        roomdefinition1 = roomdefinition;
    }
    return true;
}

bool DoubleYZRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    RoomDefinition* roomdefinition = field_175830_k->field_175965_b[getIndex(EnumFacing::NORTH)];
    RoomDefinition* roomdefinition1 = field_175830_k;
    RoomDefinition* roomdefinition2 = roomdefinition->field_175965_b[getIndex(EnumFacing::UP)];
    RoomDefinition* roomdefinition3 = roomdefinition1->field_175965_b[getIndex(EnumFacing::UP)];
    if (field_175830_k->field_175967_a / 25 > 0)
    {
        func_175821_a(worldIn, structureBoundingBoxIn, 0, 8, roomdefinition->field_175966_c[getIndex(EnumFacing::DOWN)]);
        func_175821_a(worldIn, structureBoundingBoxIn, 0, 0, roomdefinition1->field_175966_c[getIndex(EnumFacing::DOWN)]);
    }
    if (roomdefinition3->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 1, 8, 1, 6, 8, 7, field_175828_a());
    }
    if (roomdefinition2->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 1, 8, 8, 6, 8, 14, field_175828_a());
    }
    for (int i = 1; i <= 7; ++i)
    {
        const IBlockState* iblockstate = field_175826_b();
        if (i == 2 || i == 6)
        {
            iblockstate = field_175828_a();
        }
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, i, 0, 0, i, 15, iblockstate, iblockstate, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, i, 0, 7, i, 15, iblockstate, iblockstate, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, i, 0, 6, i, 0, iblockstate, iblockstate, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, i, 15, 6, i, 15, iblockstate, iblockstate, false);
    }
    for (int j = 1; j <= 7; ++j)
    {
        const IBlockState* iblockstate1 = field_175827_c();
        if (j == 2 || j == 6)
        {
            iblockstate1 = field_175825_e();
        }
        fillWithBlocks(worldIn, structureBoundingBoxIn, 3, j, 7, 4, j, 8, iblockstate1, iblockstate1, false);
    }
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 0, 4, 2, 0, false);
    }
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 7, 1, 3, 7, 2, 4, false);
    }
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 1, 3, 0, 2, 4, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 15, 4, 2, 15, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 1, 11, 0, 2, 12, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 7, 1, 11, 7, 2, 12, false);
    }
    if (roomdefinition3->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 5, 0, 4, 6, 0, false);
    }
    if (roomdefinition3->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 7, 5, 3, 7, 6, 4, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 4, 2, 6, 4, 5, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 2, 6, 3, 2, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 5, 6, 3, 5, field_175826_b(), field_175826_b(), false);
    }
    if (roomdefinition3->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 5, 3, 0, 6, 4, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 2, 2, 4, 5, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 2, 1, 3, 2, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 5, 1, 3, 5, field_175826_b(), field_175826_b(), false);
    }
    if (roomdefinition2->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 5, 15, 4, 6, 15, false);
    }
    if (roomdefinition2->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 5, 11, 0, 6, 12, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 10, 2, 4, 13, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 10, 1, 3, 10, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 13, 1, 3, 13, field_175826_b(), field_175826_b(), false);
    }
    if (roomdefinition2->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 7, 5, 11, 7, 6, 12, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 4, 10, 6, 4, 13, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 10, 6, 3, 10, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 13, 6, 3, 13, field_175826_b(), field_175826_b(), false);
    }
    return true;
}

bool DoubleZRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    RoomDefinition* roomdefinition = field_175830_k->field_175965_b[getIndex(EnumFacing::NORTH)];
    RoomDefinition* roomdefinition1 = field_175830_k;
    if (field_175830_k->field_175967_a / 25 > 0)
    {
        func_175821_a(worldIn, structureBoundingBoxIn, 0, 8, roomdefinition->field_175966_c[getIndex(EnumFacing::DOWN)]);
        func_175821_a(worldIn, structureBoundingBoxIn, 0, 0, roomdefinition1->field_175966_c[getIndex(EnumFacing::DOWN)]);
    }
    if (roomdefinition1->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 1, 4, 1, 6, 4, 7, field_175828_a());
    }
    if (roomdefinition->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 1, 4, 8, 6, 4, 14, field_175828_a());
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 0, 0, 3, 15, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 3, 0, 7, 3, 15, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 0, 7, 3, 0, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 15, 6, 3, 15, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 2, 15, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 0, 7, 2, 15, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 7, 2, 0, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 15, 6, 2, 15, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 0, 1, 15, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 0, 7, 1, 15, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 0, 7, 1, 0, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 15, 6, 1, 15, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 1, 1, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 1, 6, 1, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 1, 1, 3, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 3, 1, 6, 3, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 13, 1, 1, 14, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 13, 6, 1, 14, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 13, 1, 3, 14, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 3, 13, 6, 3, 14, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 6, 2, 3, 6, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 6, 5, 3, 6, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 9, 2, 3, 9, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 9, 5, 3, 9, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 2, 6, 4, 2, 6, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 2, 9, 4, 2, 9, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 2, 7, 2, 2, 8, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 7, 5, 2, 8, field_175826_b(), field_175826_b(), false);
    setBlockState(worldIn, field_175825_e(), 2, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, field_175825_e(), 5, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, field_175825_e(), 2, 2, 10, structureBoundingBoxIn);
    setBlockState(worldIn, field_175825_e(), 5, 2, 10, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 2, 3, 5, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 5, 3, 5, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 2, 3, 10, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 5, 3, 10, structureBoundingBoxIn);
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 0, 4, 2, 0, false);
    }
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 7, 1, 3, 7, 2, 4, false);
    }
    if (roomdefinition1->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 1, 3, 0, 2, 4, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 15, 4, 2, 15, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 1, 11, 0, 2, 12, false);
    }
    if (roomdefinition->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 7, 1, 11, 7, 2, 12, false);
    }
    return true;
}

bool EntryRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 0, 2, 3, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 3, 0, 7, 3, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 1, 2, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 2, 0, 7, 2, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 0, 1, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 0, 7, 1, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 7, 7, 3, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 0, 2, 3, 0, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 0, 6, 3, 0, field_175826_b(), field_175826_b(), false);
    if (field_175830_k->field_175966_c[getIndex(EnumFacing::NORTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 7, 4, 2, 7, false);
    }
    if (field_175830_k->field_175966_c[getIndex(EnumFacing::WEST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 0, 1, 3, 1, 2, 4, false);
    }
    if (field_175830_k->field_175966_c[getIndex(EnumFacing::EAST)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 6, 1, 3, 7, 2, 4, false);
    }
    return true;
}

bool MonumentBuilding::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    int i = std::max(worldIn->getSeaLevel(), 64) - boundingBox.minY;
    func_181655_a(worldIn, structureBoundingBoxIn, 0, 0, 0, 58, i, 58, false);
    func_175840_a(false, 0, worldIn, randomIn, structureBoundingBoxIn);
    func_175840_a(true, 33, worldIn, randomIn, structureBoundingBoxIn);
    func_175839_b(worldIn, randomIn, structureBoundingBoxIn);
    func_175837_c(worldIn, randomIn, structureBoundingBoxIn);
    func_175841_d(worldIn, randomIn, structureBoundingBoxIn);
    func_175835_e(worldIn, randomIn, structureBoundingBoxIn);
    func_175842_f(worldIn, randomIn, structureBoundingBoxIn);
    func_175838_g(worldIn, randomIn, structureBoundingBoxIn);
    for (int j = 0; j < 7; ++j)
    {
        int k = 0;
        while (k < 7)
        {
            if (k == 0 && j == 3)
            {
                k = 6;
            }
            int l = j * 9;
            int i1 = k * 9;
            for (int j1 = 0; j1 < 4; ++j1)
            {
                for (int k1 = 0; k1 < 4; ++k1)
                {
                    setBlockState(worldIn, field_175826_b(), l + j1, 0, i1 + k1, structureBoundingBoxIn);
                    replaceAirAndLiquidDownwards(worldIn, field_175826_b(), l + j1, -1, i1 + k1, structureBoundingBoxIn);
                }
            }
            if (j != 0 && j != 6)
            {
                k += 6;
            }
            else
            {
                ++k;
            }
        }
    }
    for (int l1 = 0; l1 < 5; ++l1)
    {
        func_181655_a(worldIn, structureBoundingBoxIn, -1 - l1, 0 + l1 * 2, -1 - l1, -1 - l1, 23, 58 + l1, false);
        func_181655_a(worldIn, structureBoundingBoxIn, 58 + l1, 0 + l1 * 2, -1 - l1, 58 + l1, 23, 58 + l1, false);
        func_181655_a(worldIn, structureBoundingBoxIn, 0 - l1, 0 + l1 * 2, -1 - l1, 57 + l1, 23, -1 - l1, false);
        func_181655_a(worldIn, structureBoundingBoxIn, 0 - l1, 0 + l1 * 2, 58 + l1, 57 + l1, 23, 58 + l1, false);
    }
    for (const auto& piece : field_175843_q)
    {
        if (piece->getBoundingBox().intersectsWith(structureBoundingBoxIn))
        {
            piece->addComponentParts(worldIn, randomIn, structureBoundingBoxIn);
        }
    }
    return true;
}

void MonumentBuilding::func_175840_a(bool p_175840_1_, int p_175840_2_, World* worldIn, Random& p_175840_4_, const StructureBoundingBox& p_175840_5_)
{
    int i = 24;
    if (func_175818_a(p_175840_5_, p_175840_2_, 0, p_175840_2_ + 23, 20))
    {
        fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + 0, 0, 0, p_175840_2_ + 24, 0, 20, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175840_5_, p_175840_2_ + 0, 1, 0, p_175840_2_ + 24, 10, 20, false);
        for (int j = 0; j < 4; ++j)
        {
            fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + j, j + 1, j, p_175840_2_ + j, j + 1, 20, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + j + 7, j + 5, j + 7, p_175840_2_ + j + 7, j + 5, 20, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + 17 - j, j + 5, j + 7, p_175840_2_ + 17 - j, j + 5, 20, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + 24 - j, j + 1, j, p_175840_2_ + 24 - j, j + 1, 20, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + j + 1, j + 1, j, p_175840_2_ + 23 - j, j + 1, j, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + j + 8, j + 5, j + 7, p_175840_2_ + 16 - j, j + 5, j + 7, field_175826_b(), field_175826_b(), false);
        }
        fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + 4, 4, 4, p_175840_2_ + 6, 4, 20, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + 7, 4, 4, p_175840_2_ + 17, 4, 6, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + 18, 4, 4, p_175840_2_ + 20, 4, 20, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + 11, 8, 11, p_175840_2_ + 13, 8, 20, field_175828_a(), field_175828_a(), false);
        setBlockState(worldIn, field_175824_d(), p_175840_2_ + 12, 9, 12, p_175840_5_);
        setBlockState(worldIn, field_175824_d(), p_175840_2_ + 12, 9, 15, p_175840_5_);
        setBlockState(worldIn, field_175824_d(), p_175840_2_ + 12, 9, 18, p_175840_5_);
        int j1 = p_175840_1_ ? p_175840_2_ + 19 : p_175840_2_ + 5;
        int k = p_175840_1_ ? p_175840_2_ + 5 : p_175840_2_ + 19;
        for (int l = 20; l >= 5; l -= 3)
        {
            setBlockState(worldIn, field_175824_d(), j1, 5, l, p_175840_5_);
        }
        for (int k1 = 19; k1 >= 7; k1 -= 3)
        {
            setBlockState(worldIn, field_175824_d(), k, 5, k1, p_175840_5_);
        }
        for (int l1 = 0; l1 < 4; ++l1)
        {
            int i1 = p_175840_1_ ? p_175840_2_ + (24 - (17 - l1 * 3)) : p_175840_2_ + 17 - l1 * 3;
            setBlockState(worldIn, field_175824_d(), i1, 5, 5, p_175840_5_);
        }
        setBlockState(worldIn, field_175824_d(), k, 5, 5, p_175840_5_);
        fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + 11, 1, 12, p_175840_2_ + 13, 7, 12, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175840_5_, p_175840_2_ + 12, 1, 11, p_175840_2_ + 12, 7, 13, field_175828_a(), field_175828_a(), false);
    }
}

void MonumentBuilding::func_175839_b(World* worldIn, Random& p_175839_2_, const StructureBoundingBox& p_175839_3_)
{
    if (func_175818_a(p_175839_3_, 22, 5, 35, 17))
    {
        func_181655_a(worldIn, p_175839_3_, 25, 0, 0, 32, 8, 20, false);
        for (int i = 0; i < 4; ++i)
        {
            fillWithBlocks(worldIn, p_175839_3_, 24, 2, 5 + i * 4, 24, 4, 5 + i * 4, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175839_3_, 22, 4, 5 + i * 4, 23, 4, 5 + i * 4, field_175826_b(), field_175826_b(), false);
            setBlockState(worldIn, field_175826_b(), 25, 5, 5 + i * 4, p_175839_3_);
            setBlockState(worldIn, field_175826_b(), 26, 6, 5 + i * 4, p_175839_3_);
            setBlockState(worldIn, field_175825_e(), 26, 5, 5 + i * 4, p_175839_3_);
            fillWithBlocks(worldIn, p_175839_3_, 33, 2, 5 + i * 4, 33, 4, 5 + i * 4, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175839_3_, 34, 4, 5 + i * 4, 35, 4, 5 + i * 4, field_175826_b(), field_175826_b(), false);
            setBlockState(worldIn, field_175826_b(), 32, 5, 5 + i * 4, p_175839_3_);
            setBlockState(worldIn, field_175826_b(), 31, 6, 5 + i * 4, p_175839_3_);
            setBlockState(worldIn, field_175825_e(), 31, 5, 5 + i * 4, p_175839_3_);
            fillWithBlocks(worldIn, p_175839_3_, 27, 6, 5 + i * 4, 30, 6, 5 + i * 4, field_175828_a(), field_175828_a(), false);
        }
    }
}

void MonumentBuilding::func_175837_c(World* worldIn, Random& p_175837_2_, const StructureBoundingBox& p_175837_3_)
{
    if (func_175818_a(p_175837_3_, 15, 20, 42, 21))
    {
        fillWithBlocks(worldIn, p_175837_3_, 15, 0, 21, 42, 0, 21, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175837_3_, 26, 1, 21, 31, 3, 21, false);
        fillWithBlocks(worldIn, p_175837_3_, 21, 12, 21, 36, 12, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 17, 11, 21, 40, 11, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 16, 10, 21, 41, 10, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 15, 7, 21, 42, 9, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 16, 6, 21, 41, 6, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 17, 5, 21, 40, 5, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 21, 4, 21, 36, 4, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 22, 3, 21, 26, 3, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 31, 3, 21, 35, 3, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 23, 2, 21, 25, 2, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 32, 2, 21, 34, 2, 21, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175837_3_, 28, 4, 20, 29, 4, 21, field_175826_b(), field_175826_b(), false);
        setBlockState(worldIn, field_175826_b(), 27, 3, 21, p_175837_3_);
        setBlockState(worldIn, field_175826_b(), 30, 3, 21, p_175837_3_);
        setBlockState(worldIn, field_175826_b(), 26, 2, 21, p_175837_3_);
        setBlockState(worldIn, field_175826_b(), 31, 2, 21, p_175837_3_);
        setBlockState(worldIn, field_175826_b(), 25, 1, 21, p_175837_3_);
        setBlockState(worldIn, field_175826_b(), 32, 1, 21, p_175837_3_);
        for (int i = 0; i < 7; ++i)
        {
            setBlockState(worldIn, field_175827_c(), 28 - i, 6 + i, 21, p_175837_3_);
            setBlockState(worldIn, field_175827_c(), 29 + i, 6 + i, 21, p_175837_3_);
        }
        for (int j = 0; j < 4; ++j)
        {
            setBlockState(worldIn, field_175827_c(), 28 - j, 9 + j, 21, p_175837_3_);
            setBlockState(worldIn, field_175827_c(), 29 + j, 9 + j, 21, p_175837_3_);
        }
        setBlockState(worldIn, field_175827_c(), 28, 12, 21, p_175837_3_);
        setBlockState(worldIn, field_175827_c(), 29, 12, 21, p_175837_3_);
        for (int k = 0; k < 3; ++k)
        {
            setBlockState(worldIn, field_175827_c(), 22 - k * 2, 8, 21, p_175837_3_);
            setBlockState(worldIn, field_175827_c(), 22 - k * 2, 9, 21, p_175837_3_);
            setBlockState(worldIn, field_175827_c(), 35 + k * 2, 8, 21, p_175837_3_);
            setBlockState(worldIn, field_175827_c(), 35 + k * 2, 9, 21, p_175837_3_);
        }
        func_181655_a(worldIn, p_175837_3_, 15, 13, 21, 42, 15, 21, false);
        func_181655_a(worldIn, p_175837_3_, 15, 1, 21, 15, 6, 21, false);
        func_181655_a(worldIn, p_175837_3_, 16, 1, 21, 16, 5, 21, false);
        func_181655_a(worldIn, p_175837_3_, 17, 1, 21, 20, 4, 21, false);
        func_181655_a(worldIn, p_175837_3_, 21, 1, 21, 21, 3, 21, false);
        func_181655_a(worldIn, p_175837_3_, 22, 1, 21, 22, 2, 21, false);
        func_181655_a(worldIn, p_175837_3_, 23, 1, 21, 24, 1, 21, false);
        func_181655_a(worldIn, p_175837_3_, 42, 1, 21, 42, 6, 21, false);
        func_181655_a(worldIn, p_175837_3_, 41, 1, 21, 41, 5, 21, false);
        func_181655_a(worldIn, p_175837_3_, 37, 1, 21, 40, 4, 21, false);
        func_181655_a(worldIn, p_175837_3_, 36, 1, 21, 36, 3, 21, false);
        func_181655_a(worldIn, p_175837_3_, 33, 1, 21, 34, 1, 21, false);
        func_181655_a(worldIn, p_175837_3_, 35, 1, 21, 35, 2, 21, false);
    }
}

void MonumentBuilding::func_175841_d(World* worldIn, Random& p_175841_2_, const StructureBoundingBox& p_175841_3_)
{
    if (func_175818_a(p_175841_3_, 21, 21, 36, 36))
    {
        fillWithBlocks(worldIn, p_175841_3_, 21, 0, 22, 36, 0, 36, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175841_3_, 21, 1, 22, 36, 23, 36, false);
        for (int i = 0; i < 4; ++i)
        {
            fillWithBlocks(worldIn, p_175841_3_, 21 + i, 13 + i, 21 + i, 36 - i, 13 + i, 21 + i, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175841_3_, 21 + i, 13 + i, 36 - i, 36 - i, 13 + i, 36 - i, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175841_3_, 21 + i, 13 + i, 22 + i, 21 + i, 13 + i, 35 - i, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, p_175841_3_, 36 - i, 13 + i, 22 + i, 36 - i, 13 + i, 35 - i, field_175826_b(), field_175826_b(), false);
        }
        fillWithBlocks(worldIn, p_175841_3_, 25, 16, 25, 32, 16, 32, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175841_3_, 25, 17, 25, 25, 19, 25, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, p_175841_3_, 32, 17, 25, 32, 19, 25, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, p_175841_3_, 25, 17, 32, 25, 19, 32, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, p_175841_3_, 32, 17, 32, 32, 19, 32, field_175826_b(), field_175826_b(), false);
        setBlockState(worldIn, field_175826_b(), 26, 20, 26, p_175841_3_);
        setBlockState(worldIn, field_175826_b(), 27, 21, 27, p_175841_3_);
        setBlockState(worldIn, field_175825_e(), 27, 20, 27, p_175841_3_);
        setBlockState(worldIn, field_175826_b(), 26, 20, 31, p_175841_3_);
        setBlockState(worldIn, field_175826_b(), 27, 21, 30, p_175841_3_);
        setBlockState(worldIn, field_175825_e(), 27, 20, 30, p_175841_3_);
        setBlockState(worldIn, field_175826_b(), 31, 20, 31, p_175841_3_);
        setBlockState(worldIn, field_175826_b(), 30, 21, 30, p_175841_3_);
        setBlockState(worldIn, field_175825_e(), 30, 20, 30, p_175841_3_);
        setBlockState(worldIn, field_175826_b(), 31, 20, 26, p_175841_3_);
        setBlockState(worldIn, field_175826_b(), 30, 21, 27, p_175841_3_);
        setBlockState(worldIn, field_175825_e(), 30, 20, 27, p_175841_3_);
        fillWithBlocks(worldIn, p_175841_3_, 28, 21, 27, 29, 21, 27, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175841_3_, 27, 21, 28, 27, 21, 29, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175841_3_, 28, 21, 30, 29, 21, 30, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175841_3_, 30, 21, 28, 30, 21, 29, field_175828_a(), field_175828_a(), false);
    }
}

void MonumentBuilding::func_175835_e(World* worldIn, Random& p_175835_2_, const StructureBoundingBox& p_175835_3_)
{
    if (func_175818_a(p_175835_3_, 0, 21, 6, 58))
    {
        fillWithBlocks(worldIn, p_175835_3_, 0, 0, 21, 6, 0, 57, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175835_3_, 0, 1, 21, 6, 7, 57, false);
        fillWithBlocks(worldIn, p_175835_3_, 4, 4, 21, 6, 4, 53, field_175828_a(), field_175828_a(), false);
        for (int i = 0; i < 4; ++i)
        {
            fillWithBlocks(worldIn, p_175835_3_, i, i + 1, 21, i, i + 1, 57 - i, field_175826_b(), field_175826_b(), false);
        }
        for (int j = 23; j < 53; j += 3)
        {
            setBlockState(worldIn, field_175824_d(), 5, 5, j, p_175835_3_);
        }
        setBlockState(worldIn, field_175824_d(), 5, 5, 52, p_175835_3_);
        for (int k = 0; k < 4; ++k)
        {
            fillWithBlocks(worldIn, p_175835_3_, k, k + 1, 21, k, k + 1, 57 - k, field_175826_b(), field_175826_b(), false);
        }
        fillWithBlocks(worldIn, p_175835_3_, 4, 1, 52, 6, 3, 52, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175835_3_, 5, 1, 51, 5, 3, 53, field_175828_a(), field_175828_a(), false);
    }
    if (func_175818_a(p_175835_3_, 51, 21, 58, 58))
    {
        fillWithBlocks(worldIn, p_175835_3_, 51, 0, 21, 57, 0, 57, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175835_3_, 51, 1, 21, 57, 7, 57, false);
        fillWithBlocks(worldIn, p_175835_3_, 51, 4, 21, 53, 4, 53, field_175828_a(), field_175828_a(), false);
        for (int l = 0; l < 4; ++l)
        {
            fillWithBlocks(worldIn, p_175835_3_, 57 - l, l + 1, 21, 57 - l, l + 1, 57 - l, field_175826_b(), field_175826_b(), false);
        }
        for (int i1 = 23; i1 < 53; i1 += 3)
        {
            setBlockState(worldIn, field_175824_d(), 52, 5, i1, p_175835_3_);
        }
        setBlockState(worldIn, field_175824_d(), 52, 5, 52, p_175835_3_);
        fillWithBlocks(worldIn, p_175835_3_, 51, 1, 52, 53, 3, 52, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175835_3_, 52, 1, 51, 52, 3, 53, field_175828_a(), field_175828_a(), false);
    }
    if (func_175818_a(p_175835_3_, 0, 51, 57, 57))
    {
        fillWithBlocks(worldIn, p_175835_3_, 7, 0, 51, 50, 0, 57, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175835_3_, 7, 1, 51, 50, 10, 57, false);
        for (int j1 = 0; j1 < 4; ++j1)
        {
            fillWithBlocks(worldIn, p_175835_3_, j1 + 1, j1 + 1, 57 - j1, 56 - j1, j1 + 1, 57 - j1, field_175826_b(), field_175826_b(), false);
        }
    }
}

void MonumentBuilding::func_175842_f(World* worldIn, Random& p_175842_2_, const StructureBoundingBox& p_175842_3_)
{
    if (func_175818_a(p_175842_3_, 7, 21, 13, 50))
    {
        fillWithBlocks(worldIn, p_175842_3_, 7, 0, 21, 13, 0, 50, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175842_3_, 7, 1, 21, 13, 10, 50, false);
        fillWithBlocks(worldIn, p_175842_3_, 11, 8, 21, 13, 8, 53, field_175828_a(), field_175828_a(), false);
        for (int i = 0; i < 4; ++i)
        {
            fillWithBlocks(worldIn, p_175842_3_, i + 7, i + 5, 21, i + 7, i + 5, 54, field_175826_b(), field_175826_b(), false);
        }
        for (int j = 21; j <= 45; j += 3)
        {
            setBlockState(worldIn, field_175824_d(), 12, 9, j, p_175842_3_);
        }
    }
    if (func_175818_a(p_175842_3_, 44, 21, 50, 54))
    {
        fillWithBlocks(worldIn, p_175842_3_, 44, 0, 21, 50, 0, 50, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175842_3_, 44, 1, 21, 50, 10, 50, false);
        fillWithBlocks(worldIn, p_175842_3_, 44, 8, 21, 46, 8, 53, field_175828_a(), field_175828_a(), false);
        for (int k = 0; k < 4; ++k)
        {
            fillWithBlocks(worldIn, p_175842_3_, 50 - k, k + 5, 21, 50 - k, k + 5, 54, field_175826_b(), field_175826_b(), false);
        }
        for (int l = 21; l <= 45; l += 3)
        {
            setBlockState(worldIn, field_175824_d(), 45, 9, l, p_175842_3_);
        }
    }
    if (func_175818_a(p_175842_3_, 8, 44, 49, 54))
    {
        fillWithBlocks(worldIn, p_175842_3_, 14, 0, 44, 43, 0, 50, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175842_3_, 14, 1, 44, 43, 10, 50, false);
        for (int i1 = 12; i1 <= 45; i1 += 3)
        {
            setBlockState(worldIn, field_175824_d(), i1, 9, 45, p_175842_3_);
            setBlockState(worldIn, field_175824_d(), i1, 9, 52, p_175842_3_);
            if (i1 == 12 || i1 == 18 || i1 == 24 || i1 == 33 || i1 == 39 || i1 == 45)
            {
                setBlockState(worldIn, field_175824_d(), i1, 9, 47, p_175842_3_);
                setBlockState(worldIn, field_175824_d(), i1, 9, 50, p_175842_3_);
                setBlockState(worldIn, field_175824_d(), i1, 10, 45, p_175842_3_);
                setBlockState(worldIn, field_175824_d(), i1, 10, 46, p_175842_3_);
                setBlockState(worldIn, field_175824_d(), i1, 10, 51, p_175842_3_);
                setBlockState(worldIn, field_175824_d(), i1, 10, 52, p_175842_3_);
                setBlockState(worldIn, field_175824_d(), i1, 11, 47, p_175842_3_);
                setBlockState(worldIn, field_175824_d(), i1, 11, 50, p_175842_3_);
                setBlockState(worldIn, field_175824_d(), i1, 12, 48, p_175842_3_);
                setBlockState(worldIn, field_175824_d(), i1, 12, 49, p_175842_3_);
            }
        }
        for (int j1 = 0; j1 < 3; ++j1)
        {
            fillWithBlocks(worldIn, p_175842_3_, 8 + j1, 5 + j1, 54, 49 - j1, 5 + j1, 54, field_175828_a(), field_175828_a(), false);
        }
        fillWithBlocks(worldIn, p_175842_3_, 11, 8, 54, 46, 8, 54, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, p_175842_3_, 14, 8, 44, 43, 8, 53, field_175828_a(), field_175828_a(), false);
    }
}

void MonumentBuilding::func_175838_g(World* worldIn, Random& p_175838_2_, const StructureBoundingBox& p_175838_3_)
{
    if (func_175818_a(p_175838_3_, 14, 21, 20, 43))
    {
        fillWithBlocks(worldIn, p_175838_3_, 14, 0, 21, 20, 0, 43, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175838_3_, 14, 1, 22, 20, 14, 43, false);
        fillWithBlocks(worldIn, p_175838_3_, 18, 12, 22, 20, 12, 39, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175838_3_, 18, 12, 21, 20, 12, 21, field_175826_b(), field_175826_b(), false);
        for (int i = 0; i < 4; ++i)
        {
            fillWithBlocks(worldIn, p_175838_3_, i + 14, i + 9, 21, i + 14, i + 9, 43 - i, field_175826_b(), field_175826_b(), false);
        }
        for (int j = 23; j <= 39; j += 3)
        {
            setBlockState(worldIn, field_175824_d(), 19, 13, j, p_175838_3_);
        }
    }
    if (func_175818_a(p_175838_3_, 37, 21, 43, 43))
    {
        fillWithBlocks(worldIn, p_175838_3_, 37, 0, 21, 43, 0, 43, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175838_3_, 37, 1, 22, 43, 14, 43, false);
        fillWithBlocks(worldIn, p_175838_3_, 37, 12, 22, 39, 12, 39, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, p_175838_3_, 37, 12, 21, 39, 12, 21, field_175826_b(), field_175826_b(), false);
        for (int k = 0; k < 4; ++k)
        {
            fillWithBlocks(worldIn, p_175838_3_, 43 - k, k + 9, 21, 43 - k, k + 9, 43 - k, field_175826_b(), field_175826_b(), false);
        }
        for (int l = 23; l <= 39; l += 3)
        {
            setBlockState(worldIn, field_175824_d(), 38, 13, l, p_175838_3_);
        }
    }
    if (func_175818_a(p_175838_3_, 15, 37, 42, 43))
    {
        fillWithBlocks(worldIn, p_175838_3_, 21, 0, 37, 36, 0, 43, field_175828_a(), field_175828_a(), false);
        func_181655_a(worldIn, p_175838_3_, 21, 1, 37, 36, 14, 43, false);
        fillWithBlocks(worldIn, p_175838_3_, 21, 12, 37, 36, 12, 39, field_175828_a(), field_175828_a(), false);
        for (int i1 = 0; i1 < 4; ++i1)
        {
            fillWithBlocks(worldIn, p_175838_3_, 15 + i1, i1 + 9, 43 - i1, 42 - i1, i1 + 9, 43 - i1, field_175826_b(), field_175826_b(), false);
        }
        for (int j1 = 21; j1 <= 36; j1 += 3)
        {
            setBlockState(worldIn, field_175824_d(), j1, 13, 38, p_175838_3_);
        }
    }
}

bool MonumentCoreRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    func_175819_a(worldIn, structureBoundingBoxIn, 1, 8, 0, 14, 8, 14, field_175828_a());
    int i = 7;
    const IBlockState* iblockstate = field_175826_b();
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, i, 0, 0, i, 15, iblockstate, iblockstate, false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 15, i, 0, 15, i, 15, iblockstate, iblockstate, false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, i, 0, 15, i, 0, iblockstate, iblockstate, false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, i, 15, 14, i, 15, iblockstate, iblockstate, false);
    for (i = 1; i <= 6; ++i)
    {
        iblockstate = field_175826_b();
        if (i == 2 || i == 6)
        {
            iblockstate = field_175828_a();
        }
        for (int j = 0; j <= 15; j += 15)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, j, i, 0, j, i, 1, iblockstate, iblockstate, false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, j, i, 6, j, i, 9, iblockstate, iblockstate, false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, j, i, 14, j, i, 15, iblockstate, iblockstate, false);
        }
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, i, 0, 1, i, 0, iblockstate, iblockstate, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, i, 0, 9, i, 0, iblockstate, iblockstate, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 14, i, 0, 14, i, 0, iblockstate, iblockstate, false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, i, 15, 14, i, 15, iblockstate, iblockstate, false);
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 3, 6, 9, 6, 9, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 4, 7, 8, 5, 8, Blocks::gold_block->getDefaultState(), Blocks::gold_block->getDefaultState(), false);
    for (i = 3; i <= 6; i += 3)
    {
        for (int k = 6; k <= 9; k += 3)
        {
            setBlockState(worldIn, field_175825_e(), k, i, 6, structureBoundingBoxIn);
            setBlockState(worldIn, field_175825_e(), k, i, 9, structureBoundingBoxIn);
        }
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 6, 5, 2, 6, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 9, 5, 2, 9, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 1, 6, 10, 2, 6, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 1, 9, 10, 2, 9, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 5, 6, 2, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 1, 5, 9, 2, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 10, 6, 2, 10, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 1, 10, 9, 2, 10, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 5, 5, 6, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 10, 5, 6, 10, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 2, 5, 10, 6, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 2, 10, 10, 6, 10, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 7, 1, 5, 7, 6, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 7, 1, 10, 7, 6, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 7, 9, 5, 7, 14, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 7, 9, 10, 7, 14, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 7, 5, 6, 7, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 7, 10, 6, 7, 10, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 7, 5, 14, 7, 5, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 7, 10, 14, 7, 10, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 2, 2, 1, 3, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 2, 3, 1, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 13, 1, 2, 13, 1, 3, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 1, 2, 12, 1, 2, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 12, 2, 1, 13, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 13, 3, 1, 13, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 13, 1, 12, 13, 1, 13, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 1, 13, 12, 1, 13, field_175826_b(), field_175826_b(), false);
    return true;
}

bool Penthouse::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, -1, 2, 11, -1, 11, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, -1, 0, 1, -1, 11, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, -1, 0, 13, -1, 11, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, -1, 0, 11, -1, 1, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, -1, 12, 11, -1, 13, field_175828_a(), field_175828_a(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 0, 0, 13, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 13, 0, 0, 13, 0, 13, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 0, 12, 0, 0, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 13, 12, 0, 13, field_175826_b(), field_175826_b(), false);
    for (int i = 2; i <= 11; i += 3)
    {
        setBlockState(worldIn, field_175825_e(), 0, 0, i, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 13, 0, i, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), i, 0, 0, structureBoundingBoxIn);
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 0, 3, 4, 0, 9, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 0, 3, 11, 0, 9, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 0, 9, 9, 0, 11, field_175826_b(), field_175826_b(), false);
    setBlockState(worldIn, field_175826_b(), 5, 0, 8, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 8, 0, 8, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 10, 0, 10, structureBoundingBoxIn);
    setBlockState(worldIn, field_175826_b(), 3, 0, 10, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 0, 3, 3, 0, 7, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 0, 3, 10, 0, 7, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 0, 10, 7, 0, 10, field_175827_c(), field_175827_c(), false);
    int l = 3;
    for (int j = 0; j < 2; ++j)
    {
        for (int k = 2; k <= 8; k += 3)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, l, 0, k, l, 2, k, field_175826_b(), field_175826_b(), false);
        }
        l = 10;
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 0, 10, 5, 2, 10, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 0, 10, 8, 2, 10, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, -1, 7, 7, -1, 8, field_175827_c(), field_175827_c(), false);
    func_181655_a(worldIn, structureBoundingBoxIn, 6, -1, 3, 7, -1, 4, false);
    func_175817_a(worldIn, structureBoundingBoxIn, 6, 1, 6);
    return true;
}

bool SimpleRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_175830_k->field_175967_a / 25 > 0)
    {
        func_175821_a(worldIn, structureBoundingBoxIn, 0, 0, field_175830_k->field_175966_c[getIndex(EnumFacing::DOWN)]);
    }
    if (field_175830_k->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 1, 4, 1, 6, 4, 6, field_175828_a());
    }
    bool flag = field_175833_o != 0 && randomIn.nextBoolean() && !field_175830_k->field_175966_c[getIndex(EnumFacing::DOWN)] && !field_175830_k->field_175966_c[getIndex(EnumFacing::UP)] && field_175830_k->func_175960_c() > 1;
    if (field_175833_o == 0)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 2, 1, 2, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 0, 2, 3, 2, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 2, 2, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 2, 2, 0, field_175828_a(), field_175828_a(), false);
        setBlockState(worldIn, field_175825_e(), 1, 2, 1, structureBoundingBoxIn);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 0, 7, 1, 2, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 3, 0, 7, 3, 2, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 0, 7, 2, 2, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 0, 6, 2, 0, field_175828_a(), field_175828_a(), false);
        setBlockState(worldIn, field_175825_e(), 6, 2, 1, structureBoundingBoxIn);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 5, 2, 1, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 5, 2, 3, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 5, 0, 2, 7, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 7, 2, 2, 7, field_175828_a(), field_175828_a(), false);
        setBlockState(worldIn, field_175825_e(), 1, 2, 6, structureBoundingBoxIn);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 5, 7, 1, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 3, 5, 7, 3, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 5, 7, 2, 7, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 7, 6, 2, 7, field_175828_a(), field_175828_a(), false);
        setBlockState(worldIn, field_175825_e(), 6, 2, 6, structureBoundingBoxIn);
        if (field_175830_k->field_175966_c[getIndex(EnumFacing::SOUTH)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 3, 0, 4, 3, 0, field_175826_b(), field_175826_b(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 3, 0, 4, 3, 1, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 2, 0, 4, 2, 0, field_175828_a(), field_175828_a(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 0, 4, 1, 1, field_175826_b(), field_175826_b(), false);
        }
        if (field_175830_k->field_175966_c[getIndex(EnumFacing::NORTH)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 3, 7, 4, 3, 7, field_175826_b(), field_175826_b(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 3, 6, 4, 3, 7, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 2, 7, 4, 2, 7, field_175828_a(), field_175828_a(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 6, 4, 1, 7, field_175826_b(), field_175826_b(), false);
        }
        if (field_175830_k->field_175966_c[getIndex(EnumFacing::WEST)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 3, 0, 3, 4, field_175826_b(), field_175826_b(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 3, 1, 3, 4, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 3, 0, 2, 4, field_175828_a(), field_175828_a(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 3, 1, 1, 4, field_175826_b(), field_175826_b(), false);
        }
        if (field_175830_k->field_175966_c[getIndex(EnumFacing::EAST)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 3, 3, 7, 3, 4, field_175826_b(), field_175826_b(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 3, 3, 7, 3, 4, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 3, 7, 2, 4, field_175828_a(), field_175828_a(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 3, 7, 1, 4, field_175826_b(), field_175826_b(), false);
        }
    }
    else if (field_175833_o == 1)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 2, 2, 3, 2, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 5, 2, 3, 5, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 5, 5, 3, 5, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 2, 5, 3, 2, field_175826_b(), field_175826_b(), false);
        setBlockState(worldIn, field_175825_e(), 2, 2, 2, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 2, 2, 5, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 5, 2, 5, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 5, 2, 2, structureBoundingBoxIn);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 1, 3, 0, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 1, 0, 3, 1, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 7, 1, 3, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 6, 0, 3, 6, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 7, 7, 3, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 6, 7, 3, 6, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 0, 7, 3, 0, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 1, 7, 3, 1, field_175826_b(), field_175826_b(), false);
        setBlockState(worldIn, field_175828_a(), 1, 2, 0, structureBoundingBoxIn);
        setBlockState(worldIn, field_175828_a(), 0, 2, 1, structureBoundingBoxIn);
        setBlockState(worldIn, field_175828_a(), 1, 2, 7, structureBoundingBoxIn);
        setBlockState(worldIn, field_175828_a(), 0, 2, 6, structureBoundingBoxIn);
        setBlockState(worldIn, field_175828_a(), 6, 2, 7, structureBoundingBoxIn);
        setBlockState(worldIn, field_175828_a(), 7, 2, 6, structureBoundingBoxIn);
        setBlockState(worldIn, field_175828_a(), 6, 2, 0, structureBoundingBoxIn);
        setBlockState(worldIn, field_175828_a(), 7, 2, 1, structureBoundingBoxIn);
        if (!field_175830_k->field_175966_c[getIndex(EnumFacing::SOUTH)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 0, 6, 3, 0, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 6, 2, 0, field_175828_a(), field_175828_a(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 0, 6, 1, 0, field_175826_b(), field_175826_b(), false);
        }
        if (!field_175830_k->field_175966_c[getIndex(EnumFacing::NORTH)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 7, 6, 3, 7, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 7, 6, 2, 7, field_175828_a(), field_175828_a(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 7, 6, 1, 7, field_175826_b(), field_175826_b(), false);
        }
        if (!field_175830_k->field_175966_c[getIndex(EnumFacing::WEST)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 1, 0, 3, 6, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 1, 0, 2, 6, field_175828_a(), field_175828_a(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 1, 0, 1, 6, field_175826_b(), field_175826_b(), false);
        }
        if (!field_175830_k->field_175966_c[getIndex(EnumFacing::EAST)])
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 3, 1, 7, 3, 6, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 1, 7, 2, 6, field_175828_a(), field_175828_a(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 1, 7, 1, 6, field_175826_b(), field_175826_b(), false);
        }
    }
    else if (field_175833_o == 2)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 0, 1, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 0, 7, 1, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 0, 6, 1, 0, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 7, 6, 1, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 2, 7, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 0, 7, 2, 7, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 6, 2, 0, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 7, 6, 2, 7, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 0, 0, 3, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 3, 0, 7, 3, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 0, 6, 3, 0, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 7, 6, 3, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 3, 0, 2, 4, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 3, 7, 2, 4, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 0, 4, 2, 0, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 7, 4, 2, 7, field_175827_c(), field_175827_c(), false);
        if (field_175830_k->field_175966_c[getIndex(EnumFacing::SOUTH)])
        {
            func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 0, 4, 2, 0, false);
        }
        if (field_175830_k->field_175966_c[getIndex(EnumFacing::NORTH)])
        {
            func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 7, 4, 2, 7, false);
        }
        if (field_175830_k->field_175966_c[getIndex(EnumFacing::WEST)])
        {
            func_181655_a(worldIn, structureBoundingBoxIn, 0, 1, 3, 0, 2, 4, false);
        }
        if (field_175830_k->field_175966_c[getIndex(EnumFacing::EAST)])
        {
            func_181655_a(worldIn, structureBoundingBoxIn, 7, 1, 3, 7, 2, 4, false);
        }
    }
    if (flag)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 3, 4, 1, 4, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 2, 3, 4, 2, 4, field_175828_a(), field_175828_a(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 3, 3, 4, 3, 4, field_175826_b(), field_175826_b(), false);
    }
    return true;
}

bool SimpleTopRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_175830_k->field_175967_a / 25 > 0)
    {
        func_175821_a(worldIn, structureBoundingBoxIn, 0, 0, field_175830_k->field_175966_c[getIndex(EnumFacing::DOWN)]);
    }
    if (field_175830_k->field_175965_b[getIndex(EnumFacing::UP)] == nullptr)
    {
        func_175819_a(worldIn, structureBoundingBoxIn, 1, 4, 1, 6, 4, 6, field_175828_a());
    }
    for (int i = 1; i <= 6; ++i)
    {
        for (int j = 1; j <= 6; ++j)
        {
            if (randomIn.nextInt(3) != 0)
            {
                int k = 2 + (randomIn.nextInt(4) == 0 ? 0 : 1);
                fillWithBlocks(worldIn, structureBoundingBoxIn, i, k, j, i, 3, j, Blocks::sponge->getStateFromMeta(1), Blocks::sponge->getStateFromMeta(1), false);
            }
        }
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 0, 1, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 0, 7, 1, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 0, 6, 1, 0, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 7, 6, 1, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 2, 7, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 0, 7, 2, 7, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 6, 2, 0, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 7, 6, 2, 7, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 0, 0, 3, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 3, 0, 7, 3, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 0, 6, 3, 0, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 7, 6, 3, 7, field_175826_b(), field_175826_b(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 3, 0, 2, 4, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 3, 7, 2, 4, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 0, 4, 2, 0, field_175827_c(), field_175827_c(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 7, 4, 2, 7, field_175827_c(), field_175827_c(), false);
    if (field_175830_k->field_175966_c[getIndex(EnumFacing::SOUTH)])
    {
        func_181655_a(worldIn, structureBoundingBoxIn, 3, 1, 0, 4, 2, 0, false);
    }
    return true;
}

bool WingRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_175834_o == 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 10 - i, 3 - i, 20 - i, 12 + i, 3 - i, 20, field_175826_b(), field_175826_b(), false);
        }
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 0, 6, 15, 0, 16, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 0, 6, 6, 3, 20, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 16, 0, 6, 16, 3, 20, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 7, 7, 1, 20, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 15, 1, 7, 15, 1, 20, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 1, 6, 9, 3, 6, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 13, 1, 6, 15, 3, 6, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 1, 7, 9, 1, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 13, 1, 7, 14, 1, 7, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 0, 5, 13, 0, 5, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 0, 7, 12, 0, 7, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 0, 10, 8, 0, 12, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 14, 0, 10, 14, 0, 12, field_175827_c(), field_175827_c(), false);
        for (int i1 = 18; i1 >= 7; i1 -= 3)
        {
            setBlockState(worldIn, field_175825_e(), 6, 3, i1, structureBoundingBoxIn);
            setBlockState(worldIn, field_175825_e(), 16, 3, i1, structureBoundingBoxIn);
        }
        setBlockState(worldIn, field_175825_e(), 10, 0, 10, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 12, 0, 10, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 10, 0, 12, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 12, 0, 12, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 8, 3, 6, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 14, 3, 6, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 4, 2, 4, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 4, 1, 4, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 4, 0, 4, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 18, 2, 4, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 18, 1, 4, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 18, 0, 4, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 4, 2, 18, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 4, 1, 18, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 4, 0, 18, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 18, 2, 18, structureBoundingBoxIn);
        setBlockState(worldIn, field_175825_e(), 18, 1, 18, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 18, 0, 18, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 9, 7, 20, structureBoundingBoxIn);
        setBlockState(worldIn, field_175826_b(), 13, 7, 20, structureBoundingBoxIn);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 0, 21, 7, 4, 21, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 15, 0, 21, 16, 4, 21, field_175826_b(), field_175826_b(), false);
        func_175817_a(worldIn, structureBoundingBoxIn, 11, 2, 16);
    }
    else if (field_175834_o == 1)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 3, 18, 13, 3, 20, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 0, 18, 9, 2, 18, field_175826_b(), field_175826_b(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 13, 0, 18, 13, 2, 18, field_175826_b(), field_175826_b(), false);
        int j1 = 9;
        int j = 20;
        int k = 5;
        for (int l = 0; l < 2; ++l)
        {
            setBlockState(worldIn, field_175826_b(), j1, k + 1, j, structureBoundingBoxIn);
            setBlockState(worldIn, field_175825_e(), j1, k, j, structureBoundingBoxIn);
            setBlockState(worldIn, field_175826_b(), j1, k - 1, j, structureBoundingBoxIn);
            j1 = 13;
        }
        fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 3, 7, 15, 3, 14, field_175826_b(), field_175826_b(), false);
        j1 = 10;
        for (int k1 = 0; k1 < 2; ++k1)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, j1, 0, 10, j1, 6, 10, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, j1, 0, 12, j1, 6, 12, field_175826_b(), field_175826_b(), false);
            setBlockState(worldIn, field_175825_e(), j1, 0, 10, structureBoundingBoxIn);
            setBlockState(worldIn, field_175825_e(), j1, 0, 12, structureBoundingBoxIn);
            setBlockState(worldIn, field_175825_e(), j1, 4, 10, structureBoundingBoxIn);
            setBlockState(worldIn, field_175825_e(), j1, 4, 12, structureBoundingBoxIn);
            j1 = 12;
        }
        j1 = 8;
        for (int l1 = 0; l1 < 2; ++l1)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, j1, 0, 7, j1, 2, 7, field_175826_b(), field_175826_b(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, j1, 0, 14, j1, 2, 14, field_175826_b(), field_175826_b(), false);
            j1 = 14;
        }
        fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 3, 8, 8, 3, 13, field_175827_c(), field_175827_c(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 14, 3, 8, 14, 3, 13, field_175827_c(), field_175827_c(), false);
        func_175817_a(worldIn, structureBoundingBoxIn, 11, 5, 13);
    }
    return true;
}

} // namespace StructureOceanMonumentPieces

// --- StructureOceanMonument ---

const std::vector<BiomeGenBase*>& StructureOceanMonument::field_175802_d()
{
    static const std::vector<BiomeGenBase*> list { BiomeGenBase::ocean, BiomeGenBase::deepOcean, BiomeGenBase::river, BiomeGenBase::frozenOcean,
        BiomeGenBase::frozenRiver };
    return list;
}

const std::vector<BiomeGenBase::SpawnListEntry>& StructureOceanMonument::getScatteredFeatureSpawnList()
{
    static const std::vector<BiomeGenBase::SpawnListEntry> list { { "Guardian", 1, 2, 4 } };
    return list;
}

StructureOceanMonument::StructureOceanMonument(const FlatGeneratorInfo::Options& options)
    : StructureOceanMonument()
{
    for (const auto& [key, value] : options) {
        if (key == "spacing")
            field_175800_f = MathHelper::parseIntWithDefaultAndMax(value, field_175800_f, 1);
        else if (key == "separation")
            field_175801_g = MathHelper::parseIntWithDefaultAndMax(value, field_175801_g, 1);
    }
}

bool StructureOceanMonument::canSpawnStructureAtCoords(int chunkX, int chunkZ)
{
    const int i = chunkX;
    const int j = chunkZ;
    if (chunkX < 0)
        chunkX -= field_175800_f - 1;
    if (chunkZ < 0)
        chunkZ -= field_175800_f - 1;
    int k = chunkX / field_175800_f;
    int l = chunkZ / field_175800_f;
    Random& random = worldObj->setRandomSeed(k, l, 10387313);
    k = k * field_175800_f;
    l = l * field_175800_f;
    const int k1 = random.nextInt(field_175800_f - field_175801_g);
    const int k2 = random.nextInt(field_175800_f - field_175801_g);
    k = k + (k1 + k2) / 2;
    const int l1 = random.nextInt(field_175800_f - field_175801_g);
    const int l2 = random.nextInt(field_175800_f - field_175801_g);
    l = l + (l1 + l2) / 2;
    if (i == k && j == l) {
        if (worldObj->getWorldChunkManager()->getBiomeGenerator(BlockPos(i * 16 + 8, 64, j * 16 + 8), nullptr) != BiomeGenBase::deepOcean)
            return false;
        return worldObj->getWorldChunkManager()->areBiomesViable(i * 16 + 8, j * 16 + 8, 29, field_175802_d());
    }
    return false;
}

std::shared_ptr<StructureStart> StructureOceanMonument::getStructureStart(int chunkX, int chunkZ)
{
    return std::make_shared<StartMonument>(worldObj, rand, chunkX, chunkZ);
}

StructureOceanMonument::StartMonument::StartMonument(World* worldIn, Random& rand, int chunkX, int chunkZ)
    : StructureStart(chunkX, chunkZ)
{
    func_175789_b(worldIn, rand, chunkX, chunkZ);
}

void StructureOceanMonument::StartMonument::func_175789_b(World* worldIn, Random& rand, int chunkX, int chunkZ)
{
    rand.setSeed(worldIn->getSeed());
    const int64_t i = rand.nextLong();
    const int64_t j = rand.nextLong();
    const int64_t k = java::mul(static_cast<int64_t>(chunkX), i);
    const int64_t l = java::mul(static_cast<int64_t>(chunkZ), j);
    rand.setSeed(k ^ l ^ worldIn->getSeed());
    const int i1 = chunkX * 16 + 8 - 29;
    const int j1 = chunkZ * 16 + 8 - 29;
    const EnumFacing enumfacing = randomFacing(Plane::HORIZONTAL, rand);
    components.push_back(std::make_shared<StructureOceanMonumentPieces::MonumentBuilding>(rand, i1, j1, enumfacing));
    updateBoundingBox();
    field_175790_d = true;
}

void StructureOceanMonument::StartMonument::generateStructure(World* worldIn, Random& rand, const StructureBoundingBox& structurebb)
{
    if (!field_175790_d) {
        components.clear();
        func_175789_b(worldIn, rand, getChunkPosX(), getChunkPosZ());
    }
    StructureStart::generateStructure(worldIn, rand, structurebb);
}

bool StructureOceanMonument::StartMonument::func_175788_a(const ChunkCoordIntPair& pair)
{
    return field_175791_c.contains(pair) ? false : StructureStart::func_175788_a(pair);
}

void StructureOceanMonument::StartMonument::func_175787_b(const ChunkCoordIntPair& pair)
{
    StructureStart::func_175787_b(pair);
    field_175791_c.put(pair, true);
}

void StructureOceanMonument::StartMonument::writeToNBT(NBTTagCompound& tagCompound) const
{
    StructureStart::writeToNBT(tagCompound);
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (const auto& [chunkcoordintpair, unused] : field_175791_c) {
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        nbttagcompound->setInteger("X", chunkcoordintpair.chunkXPos);
        nbttagcompound->setInteger("Z", chunkcoordintpair.chunkZPos);
        nbttaglist->appendTag(nbttagcompound);
    }
    tagCompound.setTag("Processed", nbttaglist);
}

void StructureOceanMonument::StartMonument::readFromNBT(const NBTTagCompound& tagCompound)
{
    StructureStart::readFromNBT(tagCompound);
    if (tagCompound.hasKey("Processed", 9)) {
        const auto nbttaglist = tagCompound.getTagList("Processed", 10);
        for (int i = 0; i < nbttaglist->tagCount(); ++i) {
            const auto nbttagcompound = nbttaglist->getCompoundTagAt(i);
            field_175791_c.put(ChunkCoordIntPair(nbttagcompound->getInteger("X"), nbttagcompound->getInteger("Z")), true);
        }
    }
}

} // namespace mc
