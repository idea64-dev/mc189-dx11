// Original: net/minecraft/world/gen/feature/WorldGenerator.java, WorldGenAbstractTree.java, WorldGenTrees.java,
// WorldGenShrub.java, WorldGenBigTree.java, WorldGenSwamp.java, WorldGenForest.java, WorldGenTaiga1.java,
// WorldGenTaiga2.java, WorldGenHugeTrees.java, WorldGenMegaPineTree.java, WorldGenMegaJungle.java,
// WorldGenCanopyTree.java and WorldGenSavannaTree.java

#include "world/gen/feature/WorldGenerators.h"

#include "block/AllBlocks.h"
#include "core/Java.h"
#include "core/JavaMath.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/World.h"

#include <cmath>
#include <cstdlib>

namespace mc {

namespace {

const IBlockState* oldLog(BlockPlanks::EnumType type)
{
    return Blocks::log->getDefaultState()->withProperty(BlockOldLog::VARIANT, type);
}

const IBlockState* oldLeaves(BlockPlanks::EnumType type)
{
    return Blocks::leaves->getDefaultState()->withProperty(BlockOldLeaf::VARIANT, type)->withProperty(BlockLeaves::CHECK_DECAY, false);
}

const IBlockState* newLog(BlockPlanks::EnumType type)
{
    return Blocks::log2->getDefaultState()->withProperty(BlockNewLog::VARIANT, type);
}

const IBlockState* newLeaves(BlockPlanks::EnumType type)
{
    return Blocks::leaves2->getDefaultState()->withProperty(BlockNewLeaf::VARIANT, type)->withProperty(BlockLeaves::CHECK_DECAY, false);
}

bool isMaterial(const Block* block, const Material& material)
{
    return &block->getMaterial() == &material;
}

bool airOrLeaves(const Block* block)
{
    return isMaterial(block, Material::air) || isMaterial(block, Material::leaves);
}

} // namespace

void WorldGenerator::setBlockAndNotifyAdequately(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    worldIn->setBlockState(pos, state, doBlockNotify ? 3 : 2);
}

bool WorldGenAbstractTree::func_150523_a(const Block* block)
{
    const Material& material = block->getMaterial();
    return &material == &Material::air || &material == &Material::leaves || block == Blocks::grass || block == Blocks::dirt || block == Blocks::log
        || block == Blocks::log2 || block == Blocks::sapling || block == Blocks::vine;
}

void WorldGenAbstractTree::func_175921_a(World* worldIn, const BlockPos& pos)
{
    if (worldIn->getBlockState(pos)->getBlock() != Blocks::dirt)
        setBlockAndNotifyAdequately(worldIn, pos, Blocks::dirt->getDefaultState());
}

// --- WorldGenTrees ---

WorldGenTrees::WorldGenTrees(bool notify)
    : WorldGenTrees(notify, 4, oldLog(BlockPlanks::EnumType::OAK), oldLeaves(BlockPlanks::EnumType::OAK), false)
{
}

WorldGenTrees::WorldGenTrees(bool notify, int minTreeHeightIn, const IBlockState* woodIn, const IBlockState* leavesIn, bool vinesGrowIn)
    : WorldGenAbstractTree(notify)
    , minTreeHeight(minTreeHeightIn)
    , vinesGrow(vinesGrowIn)
    , metaWood(woodIn)
    , metaLeaves(leavesIn)
{
}

bool WorldGenTrees::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const int i = rand.nextInt(3) + minTreeHeight;
    bool flag = true;
    if (position.getY() < 1 || position.getY() + i + 1 > 256)
        return false;
    for (int j = position.getY(); j <= position.getY() + 1 + i; ++j) {
        int k = 1;
        if (j == position.getY())
            k = 0;
        if (j >= position.getY() + 1 + i - 2)
            k = 2;
        BlockPos mutablePos;
        for (int l = position.getX() - k; l <= position.getX() + k && flag; ++l) {
            for (int i1 = position.getZ() - k; i1 <= position.getZ() + k && flag; ++i1) {
                if (j >= 0 && j < 256) {
                    if (!func_150523_a(worldIn->getBlockState(mutablePos.set(l, j, i1))->getBlock()))
                        flag = false;
                } else {
                    flag = false;
                }
            }
        }
    }
    if (!flag)
        return false;
    const Block* block1 = worldIn->getBlockState(position.down())->getBlock();
    if (!((block1 == Blocks::grass || block1 == Blocks::dirt || block1 == Blocks::farmland) && position.getY() < 256 - i - 1))
        return false;
    func_175921_a(worldIn, position.down());
    const int k2 = 3;
    const int l2 = 0;
    for (int i3 = position.getY() - k2 + i; i3 <= position.getY() + i; ++i3) {
        const int i4 = i3 - (position.getY() + i);
        const int j1 = l2 + 1 - i4 / 2;
        for (int k1 = position.getX() - j1; k1 <= position.getX() + j1; ++k1) {
            const int l1 = k1 - position.getX();
            for (int i2 = position.getZ() - j1; i2 <= position.getZ() + j1; ++i2) {
                const int j2 = i2 - position.getZ();
                if (std::abs(l1) != j1 || std::abs(j2) != j1 || (rand.nextInt(2) != 0 && i4 != 0)) {
                    const BlockPos blockpos(k1, i3, i2);
                    const Block* block = worldIn->getBlockState(blockpos)->getBlock();
                    if (airOrLeaves(block) || isMaterial(block, Material::vine))
                        setBlockAndNotifyAdequately(worldIn, blockpos, metaLeaves);
                }
            }
        }
    }
    for (int j3 = 0; j3 < i; ++j3) {
        const Block* block2 = worldIn->getBlockState(position.up(j3))->getBlock();
        if (airOrLeaves(block2) || isMaterial(block2, Material::vine)) {
            setBlockAndNotifyAdequately(worldIn, position.up(j3), metaWood);
            if (vinesGrow && j3 > 0) {
                if (rand.nextInt(3) > 0 && worldIn->isAirBlock(position.add(-1, j3, 0)))
                    func_181651_a(worldIn, position.add(-1, j3, 0), BlockVine::EAST);
                if (rand.nextInt(3) > 0 && worldIn->isAirBlock(position.add(1, j3, 0)))
                    func_181651_a(worldIn, position.add(1, j3, 0), BlockVine::WEST);
                if (rand.nextInt(3) > 0 && worldIn->isAirBlock(position.add(0, j3, -1)))
                    func_181651_a(worldIn, position.add(0, j3, -1), BlockVine::SOUTH);
                if (rand.nextInt(3) > 0 && worldIn->isAirBlock(position.add(0, j3, 1)))
                    func_181651_a(worldIn, position.add(0, j3, 1), BlockVine::NORTH);
            }
        }
    }
    if (vinesGrow) {
        for (int k3 = position.getY() - 3 + i; k3 <= position.getY() + i; ++k3) {
            const int j4 = k3 - (position.getY() + i);
            const int k4 = 2 - j4 / 2;
            BlockPos mutablePos;
            for (int l4 = position.getX() - k4; l4 <= position.getX() + k4; ++l4) {
                for (int i5 = position.getZ() - k4; i5 <= position.getZ() + k4; ++i5) {
                    mutablePos.set(l4, k3, i5);
                    if (isMaterial(worldIn->getBlockState(mutablePos)->getBlock(), Material::leaves)) {
                        const BlockPos blockpos2 = mutablePos.west();
                        const BlockPos blockpos3 = mutablePos.east();
                        const BlockPos blockpos4 = mutablePos.north();
                        const BlockPos blockpos1 = mutablePos.south();
                        if (rand.nextInt(4) == 0 && isMaterial(worldIn->getBlockState(blockpos2)->getBlock(), Material::air))
                            func_181650_b(worldIn, blockpos2, BlockVine::EAST);
                        if (rand.nextInt(4) == 0 && isMaterial(worldIn->getBlockState(blockpos3)->getBlock(), Material::air))
                            func_181650_b(worldIn, blockpos3, BlockVine::WEST);
                        if (rand.nextInt(4) == 0 && isMaterial(worldIn->getBlockState(blockpos4)->getBlock(), Material::air))
                            func_181650_b(worldIn, blockpos4, BlockVine::SOUTH);
                        if (rand.nextInt(4) == 0 && isMaterial(worldIn->getBlockState(blockpos1)->getBlock(), Material::air))
                            func_181650_b(worldIn, blockpos1, BlockVine::NORTH);
                    }
                }
            }
        }
        if (rand.nextInt(5) == 0 && i > 5) {
            for (int l3 = 0; l3 < 2; ++l3) {
                for (const EnumFacing enumfacing : kPlaneHorizontal) {
                    if (rand.nextInt(4 - l3) == 0) {
                        const EnumFacing enumfacing1 = getOpposite(enumfacing);
                        const int age = rand.nextInt(3);
                        func_181652_a(worldIn, age, position.add(getFrontOffsetX(enumfacing1), i - 5 + l3, getFrontOffsetZ(enumfacing1)), enumfacing);
                    }
                }
            }
        }
    }
    return true;
}

void WorldGenTrees::func_181652_a(World* worldIn, int age, const BlockPos& pos, EnumFacing facing)
{
    setBlockAndNotifyAdequately(worldIn, pos, Blocks::cocoa->getDefaultState()->withProperty(BlockCocoa::AGE, age)->withProperty(BlockCocoa::FACING, facing));
}

void WorldGenTrees::func_181651_a(World* worldIn, const BlockPos& pos, const PropertyBool& side)
{
    setBlockAndNotifyAdequately(worldIn, pos, Blocks::vine->getDefaultState()->withProperty(side, true));
}

void WorldGenTrees::func_181650_b(World* worldIn, BlockPos pos, const PropertyBool& side)
{
    func_181651_a(worldIn, pos, side);
    int i = 4;
    for (pos = pos.down(); isMaterial(worldIn->getBlockState(pos)->getBlock(), Material::air) && i > 0; --i) {
        func_181651_a(worldIn, pos, side);
        pos = pos.down();
    }
}

// --- WorldGenShrub ---

WorldGenShrub::WorldGenShrub(const IBlockState* wood, const IBlockState* leaves)
    : WorldGenTrees(false)
    , leavesMetadata(leaves)
    , woodMetadata(wood)
{
}

bool WorldGenShrub::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    BlockPos position = positionIn;
    const Block* block;
    while ((block = worldIn->getBlockState(position)->getBlock(), airOrLeaves(block)) && position.getY() > 0)
        position = position.down();
    const Block* block1 = worldIn->getBlockState(position)->getBlock();
    if (block1 == Blocks::dirt || block1 == Blocks::grass) {
        position = position.up();
        setBlockAndNotifyAdequately(worldIn, position, woodMetadata);
        for (int i = position.getY(); i <= position.getY() + 2; ++i) {
            const int j = i - position.getY();
            const int k = 2 - j;
            for (int l = position.getX() - k; l <= position.getX() + k; ++l) {
                const int i1 = l - position.getX();
                for (int j1 = position.getZ() - k; j1 <= position.getZ() + k; ++j1) {
                    const int k1 = j1 - position.getZ();
                    if (std::abs(i1) != k || std::abs(k1) != k || rand.nextInt(2) != 0) {
                        const BlockPos blockpos(l, i, j1);
                        if (!worldIn->getBlockState(blockpos)->getBlock()->isFullBlock())
                            setBlockAndNotifyAdequately(worldIn, blockpos, leavesMetadata);
                    }
                }
            }
        }
    }
    return true;
}

// --- WorldGenBigTree ---

void WorldGenBigTree::generateLeafNodeList()
{
    height = static_cast<int>(static_cast<double>(heightLimit) * heightAttenuation);
    if (height >= heightLimit)
        height = heightLimit - 1;
    // Math.pow(x, 2.0) is x * x.
    const double density = leafDensity * static_cast<double>(heightLimit) / 13.0;
    int i = static_cast<int>(1.382 + density * density);
    if (i < 1)
        i = 1;
    const int j = basePos.getY() + height;
    int k = heightLimit - leafDistanceLimit;
    field_175948_j.clear();
    field_175948_j.push_back({ basePos.up(k), j });
    for (; k >= 0; --k) {
        const float f = layerSize(k);
        if (f >= 0.0f) {
            for (int l = 0; l < i; ++l) {
                const double d0 = scaleWidth * static_cast<double>(f) * (static_cast<double>(rand->nextFloat()) + 0.328);
                const double d1 = static_cast<double>(rand->nextFloat() * 2.0f) * 3.141592653589793;
                const double d2 = d0 * java::mathSin(d1) + 0.5;
                const double d3 = d0 * java::mathCos(d1) + 0.5;
                const BlockPos blockpos = basePos.add(d2, static_cast<double>(k - 1), d3);
                const BlockPos blockpos1 = blockpos.up(leafDistanceLimit);
                if (checkBlockLine(blockpos, blockpos1) == -1) {
                    const int i1 = basePos.getX() - blockpos.getX();
                    const int j1 = basePos.getZ() - blockpos.getZ();
                    const double d4 = static_cast<double>(blockpos.getY()) - std::sqrt(static_cast<double>(i1 * i1 + j1 * j1)) * branchSlope;
                    const int k1 = d4 > static_cast<double>(j) ? j : static_cast<int>(d4);
                    const BlockPos blockpos2(basePos.getX(), k1, basePos.getZ());
                    if (checkBlockLine(blockpos2, blockpos) == -1)
                        field_175948_j.push_back({ blockpos, blockpos2.getY() });
                }
            }
        }
    }
}

void WorldGenBigTree::func_181631_a(const BlockPos& pos, float size, const IBlockState* state)
{
    const int i = static_cast<int>(static_cast<double>(size) + 0.618);
    for (int j = -i; j <= i; ++j) {
        for (int k = -i; k <= i; ++k) {
            const double a = static_cast<double>(std::abs(j)) + 0.5;
            const double b = static_cast<double>(std::abs(k)) + 0.5;
            if (a * a + b * b <= static_cast<double>(size * size)) {
                const BlockPos blockpos = pos.add(j, 0, k);
                if (airOrLeaves(world->getBlockState(blockpos)->getBlock()))
                    setBlockAndNotifyAdequately(world, blockpos, state);
            }
        }
    }
}

float WorldGenBigTree::layerSize(int y) const
{
    if (static_cast<float>(y) < static_cast<float>(heightLimit) * 0.3f)
        return -1.0f;
    const float f = static_cast<float>(heightLimit) / 2.0f;
    const float f1 = f - static_cast<float>(y);
    float f2 = MathHelper::sqrt_float(f * f - f1 * f1);
    if (f1 == 0.0f)
        f2 = f;
    else if (std::abs(f1) >= f)
        return 0.0f;
    return f2 * 0.5f;
}

float WorldGenBigTree::leafSize(int y) const
{
    return y >= 0 && y < leafDistanceLimit ? (y != 0 && y != leafDistanceLimit - 1 ? 3.0f : 2.0f) : -1.0f;
}

void WorldGenBigTree::generateLeafNode(const BlockPos& pos)
{
    for (int i = 0; i < leafDistanceLimit; ++i)
        func_181631_a(pos.up(i), leafSize(i), Blocks::leaves->getDefaultState()->withProperty(BlockLeaves::CHECK_DECAY, false));
}

void WorldGenBigTree::func_175937_a(const BlockPos& from, const BlockPos& to, const Block* block)
{
    const BlockPos blockpos = to.add(-from.getX(), -from.getY(), -from.getZ());
    const int i = getGreatestDistance(blockpos);
    const float f = static_cast<float>(blockpos.getX()) / static_cast<float>(i);
    const float f1 = static_cast<float>(blockpos.getY()) / static_cast<float>(i);
    const float f2 = static_cast<float>(blockpos.getZ()) / static_cast<float>(i);
    for (int j = 0; j <= i; ++j) {
        const BlockPos blockpos1 = from.add(static_cast<double>(0.5f + static_cast<float>(j) * f), static_cast<double>(0.5f + static_cast<float>(j) * f1),
            static_cast<double>(0.5f + static_cast<float>(j) * f2));
        const auto axis = static_cast<BlockLog::EnumAxis>(func_175938_b(from, blockpos1));
        setBlockAndNotifyAdequately(world, blockpos1, block->getDefaultState()->withProperty(BlockLog::LOG_AXIS, axis));
    }
}

int WorldGenBigTree::getGreatestDistance(const BlockPos& posIn)
{
    const int i = MathHelper::abs_int(posIn.getX());
    const int j = MathHelper::abs_int(posIn.getY());
    const int k = MathHelper::abs_int(posIn.getZ());
    return k > i && k > j ? k : (j > i ? j : i);
}

int WorldGenBigTree::func_175938_b(const BlockPos& base, const BlockPos& pos)
{
    BlockLog::EnumAxis axis = BlockLog::EnumAxis::Y;
    const int i = std::abs(pos.getX() - base.getX());
    const int j = std::abs(pos.getZ() - base.getZ());
    const int k = std::max(i, j);
    if (k > 0) {
        if (i == k)
            axis = BlockLog::EnumAxis::X;
        else if (j == k)
            axis = BlockLog::EnumAxis::Z;
    }
    return static_cast<int>(axis);
}

void WorldGenBigTree::generateLeaves()
{
    for (const FoliageCoordinates& coordinates : field_175948_j)
        generateLeafNode(coordinates.pos);
}

void WorldGenBigTree::generateTrunk()
{
    const BlockPos blockpos = basePos;
    const BlockPos blockpos1 = basePos.up(height);
    const Block* block = Blocks::log;
    func_175937_a(blockpos, blockpos1, block);
    if (trunkSize == 2) {
        func_175937_a(blockpos.east(), blockpos1.east(), block);
        func_175937_a(blockpos.east().south(), blockpos1.east().south(), block);
        func_175937_a(blockpos.south(), blockpos1.south(), block);
    }
}

void WorldGenBigTree::generateLeafNodeBases()
{
    for (const FoliageCoordinates& coordinates : field_175948_j) {
        const int i = coordinates.field_178000_b;
        const BlockPos blockpos(basePos.getX(), i, basePos.getZ());
        if (!(blockpos == coordinates.pos) && leafNodeNeedsBase(i - basePos.getY()))
            func_175937_a(blockpos, coordinates.pos, Blocks::log);
    }
}

int WorldGenBigTree::checkBlockLine(const BlockPos& posOne, const BlockPos& posTwo)
{
    const BlockPos blockpos = posTwo.add(-posOne.getX(), -posOne.getY(), -posOne.getZ());
    const int i = getGreatestDistance(blockpos);
    // Java divides by zero here when i is 0 (float division: infinity or NaN), then returns -1 unused.
    if (i == 0)
        return -1;
    const float f = static_cast<float>(blockpos.getX()) / static_cast<float>(i);
    const float f1 = static_cast<float>(blockpos.getY()) / static_cast<float>(i);
    const float f2 = static_cast<float>(blockpos.getZ()) / static_cast<float>(i);
    for (int j = 0; j <= i; ++j) {
        const BlockPos blockpos1 = posOne.add(static_cast<double>(0.5f + static_cast<float>(j) * f), static_cast<double>(0.5f + static_cast<float>(j) * f1),
            static_cast<double>(0.5f + static_cast<float>(j) * f2));
        if (!func_150523_a(world->getBlockState(blockpos1)->getBlock()))
            return j;
    }
    return -1;
}

bool WorldGenBigTree::generate(World* worldIn, Random& randIn, const BlockPos& position)
{
    world = worldIn;
    basePos = position;
    rand = std::make_unique<Random>(randIn.nextLong());
    if (heightLimit == 0)
        heightLimit = 5 + rand->nextInt(heightLimitLimit);
    if (!validTreeLocation())
        return false;
    generateLeafNodeList();
    generateLeaves();
    generateTrunk();
    generateLeafNodeBases();
    return true;
}

bool WorldGenBigTree::validTreeLocation()
{
    const Block* block = world->getBlockState(basePos.down())->getBlock();
    if (block != Blocks::dirt && block != Blocks::grass && block != Blocks::farmland)
        return false;
    const int i = checkBlockLine(basePos, basePos.up(heightLimit - 1));
    if (i == -1)
        return true;
    if (i < 6)
        return false;
    heightLimit = i;
    return true;
}

// --- WorldGenSwamp ---

bool WorldGenSwamp::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    BlockPos position = positionIn;
    const IBlockState* const wood = oldLog(BlockPlanks::EnumType::OAK);
    const IBlockState* const leaves = oldLeaves(BlockPlanks::EnumType::OAK);
    int i;
    for (i = rand.nextInt(4) + 5; isMaterial(worldIn->getBlockState(position.down())->getBlock(), Material::water); position = position.down()) {
    }
    bool flag = true;
    if (position.getY() < 1 || position.getY() + i + 1 > 256)
        return false;
    for (int j = position.getY(); j <= position.getY() + 1 + i; ++j) {
        int k = 1;
        if (j == position.getY())
            k = 0;
        if (j >= position.getY() + 1 + i - 2)
            k = 3;
        BlockPos mutablePos;
        for (int l = position.getX() - k; l <= position.getX() + k && flag; ++l) {
            for (int i1 = position.getZ() - k; i1 <= position.getZ() + k && flag; ++i1) {
                if (j >= 0 && j < 256) {
                    const Block* block = worldIn->getBlockState(mutablePos.set(l, j, i1))->getBlock();
                    if (!airOrLeaves(block)) {
                        if (block != Blocks::water && block != Blocks::flowing_water)
                            flag = false;
                        else if (j > position.getY())
                            flag = false;
                    }
                } else {
                    flag = false;
                }
            }
        }
    }
    if (!flag)
        return false;
    const Block* block1 = worldIn->getBlockState(position.down())->getBlock();
    if (!((block1 == Blocks::grass || block1 == Blocks::dirt) && position.getY() < 256 - i - 1))
        return false;
    func_175921_a(worldIn, position.down());
    for (int l1 = position.getY() - 3 + i; l1 <= position.getY() + i; ++l1) {
        const int k2 = l1 - (position.getY() + i);
        const int i3 = 2 - k2 / 2;
        for (int k3 = position.getX() - i3; k3 <= position.getX() + i3; ++k3) {
            const int l3 = k3 - position.getX();
            for (int j1 = position.getZ() - i3; j1 <= position.getZ() + i3; ++j1) {
                const int k1 = j1 - position.getZ();
                if (std::abs(l3) != i3 || std::abs(k1) != i3 || (rand.nextInt(2) != 0 && k2 != 0)) {
                    const BlockPos blockpos(k3, l1, j1);
                    if (!worldIn->getBlockState(blockpos)->getBlock()->isFullBlock())
                        setBlockAndNotifyAdequately(worldIn, blockpos, leaves);
                }
            }
        }
    }
    for (int i2 = 0; i2 < i; ++i2) {
        const Block* block2 = worldIn->getBlockState(position.up(i2))->getBlock();
        if (airOrLeaves(block2) || block2 == Blocks::flowing_water || block2 == Blocks::water)
            setBlockAndNotifyAdequately(worldIn, position.up(i2), wood);
    }
    for (int j2 = position.getY() - 3 + i; j2 <= position.getY() + i; ++j2) {
        const int l2 = j2 - (position.getY() + i);
        const int j3 = 2 - l2 / 2;
        BlockPos mutablePos;
        for (int i4 = position.getX() - j3; i4 <= position.getX() + j3; ++i4) {
            for (int j4 = position.getZ() - j3; j4 <= position.getZ() + j3; ++j4) {
                mutablePos.set(i4, j2, j4);
                if (isMaterial(worldIn->getBlockState(mutablePos)->getBlock(), Material::leaves)) {
                    const BlockPos blockpos3 = mutablePos.west();
                    const BlockPos blockpos4 = mutablePos.east();
                    const BlockPos blockpos1 = mutablePos.north();
                    const BlockPos blockpos2 = mutablePos.south();
                    if (rand.nextInt(4) == 0 && isMaterial(worldIn->getBlockState(blockpos3)->getBlock(), Material::air))
                        func_181647_a(worldIn, blockpos3, BlockVine::EAST);
                    if (rand.nextInt(4) == 0 && isMaterial(worldIn->getBlockState(blockpos4)->getBlock(), Material::air))
                        func_181647_a(worldIn, blockpos4, BlockVine::WEST);
                    if (rand.nextInt(4) == 0 && isMaterial(worldIn->getBlockState(blockpos1)->getBlock(), Material::air))
                        func_181647_a(worldIn, blockpos1, BlockVine::SOUTH);
                    if (rand.nextInt(4) == 0 && isMaterial(worldIn->getBlockState(blockpos2)->getBlock(), Material::air))
                        func_181647_a(worldIn, blockpos2, BlockVine::NORTH);
                }
            }
        }
    }
    return true;
}

void WorldGenSwamp::func_181647_a(World* worldIn, BlockPos pos, const PropertyBool& side)
{
    const IBlockState* iblockstate = Blocks::vine->getDefaultState()->withProperty(side, true);
    setBlockAndNotifyAdequately(worldIn, pos, iblockstate);
    int i = 4;
    for (pos = pos.down(); isMaterial(worldIn->getBlockState(pos)->getBlock(), Material::air) && i > 0; --i) {
        setBlockAndNotifyAdequately(worldIn, pos, iblockstate);
        pos = pos.down();
    }
}

// --- WorldGenForest ---

bool WorldGenForest::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const IBlockState* const wood = oldLog(BlockPlanks::EnumType::BIRCH);
    const IBlockState* const leaves = oldLeaves(BlockPlanks::EnumType::BIRCH);
    int i = rand.nextInt(3) + 5;
    if (useExtraRandomHeight)
        i += rand.nextInt(7);
    bool flag = true;
    if (position.getY() < 1 || position.getY() + i + 1 > 256)
        return false;
    for (int j = position.getY(); j <= position.getY() + 1 + i; ++j) {
        int k = 1;
        if (j == position.getY())
            k = 0;
        if (j >= position.getY() + 1 + i - 2)
            k = 2;
        BlockPos mutablePos;
        for (int l = position.getX() - k; l <= position.getX() + k && flag; ++l) {
            for (int i1 = position.getZ() - k; i1 <= position.getZ() + k && flag; ++i1) {
                if (j >= 0 && j < 256) {
                    if (!func_150523_a(worldIn->getBlockState(mutablePos.set(l, j, i1))->getBlock()))
                        flag = false;
                } else {
                    flag = false;
                }
            }
        }
    }
    if (!flag)
        return false;
    const Block* block1 = worldIn->getBlockState(position.down())->getBlock();
    if (!((block1 == Blocks::grass || block1 == Blocks::dirt || block1 == Blocks::farmland) && position.getY() < 256 - i - 1))
        return false;
    func_175921_a(worldIn, position.down());
    for (int i2 = position.getY() - 3 + i; i2 <= position.getY() + i; ++i2) {
        const int k2 = i2 - (position.getY() + i);
        const int l2 = 1 - k2 / 2;
        for (int i3 = position.getX() - l2; i3 <= position.getX() + l2; ++i3) {
            const int j1 = i3 - position.getX();
            for (int k1 = position.getZ() - l2; k1 <= position.getZ() + l2; ++k1) {
                const int l1 = k1 - position.getZ();
                if (std::abs(j1) != l2 || std::abs(l1) != l2 || (rand.nextInt(2) != 0 && k2 != 0)) {
                    const BlockPos blockpos(i3, i2, k1);
                    if (airOrLeaves(worldIn->getBlockState(blockpos)->getBlock()))
                        setBlockAndNotifyAdequately(worldIn, blockpos, leaves);
                }
            }
        }
    }
    for (int j2 = 0; j2 < i; ++j2) {
        if (airOrLeaves(worldIn->getBlockState(position.up(j2))->getBlock()))
            setBlockAndNotifyAdequately(worldIn, position.up(j2), wood);
    }
    return true;
}

// --- WorldGenTaiga1 ---

bool WorldGenTaiga1::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const IBlockState* const wood = oldLog(BlockPlanks::EnumType::SPRUCE);
    const IBlockState* const leaves = oldLeaves(BlockPlanks::EnumType::SPRUCE);
    const int i = rand.nextInt(5) + 7;
    const int r = rand.nextInt(2);
    const int j = i - r - 3;
    const int k = i - j;
    const int l = 1 + rand.nextInt(k + 1);
    bool flag = true;
    if (position.getY() < 1 || position.getY() + i + 1 > 256)
        return false;
    for (int i1 = position.getY(); i1 <= position.getY() + 1 + i && flag; ++i1) {
        const int j1 = i1 - position.getY() < j ? 0 : l;
        BlockPos mutablePos;
        for (int k1 = position.getX() - j1; k1 <= position.getX() + j1 && flag; ++k1) {
            for (int l1 = position.getZ() - j1; l1 <= position.getZ() + j1 && flag; ++l1) {
                if (i1 >= 0 && i1 < 256) {
                    if (!func_150523_a(worldIn->getBlockState(mutablePos.set(k1, i1, l1))->getBlock()))
                        flag = false;
                } else {
                    flag = false;
                }
            }
        }
    }
    if (!flag)
        return false;
    const Block* block = worldIn->getBlockState(position.down())->getBlock();
    if (!((block == Blocks::grass || block == Blocks::dirt) && position.getY() < 256 - i - 1))
        return false;
    func_175921_a(worldIn, position.down());
    int k2 = 0;
    for (int l2 = position.getY() + i; l2 >= position.getY() + j; --l2) {
        for (int j3 = position.getX() - k2; j3 <= position.getX() + k2; ++j3) {
            const int k3 = j3 - position.getX();
            for (int i2 = position.getZ() - k2; i2 <= position.getZ() + k2; ++i2) {
                const int j2 = i2 - position.getZ();
                if (std::abs(k3) != k2 || std::abs(j2) != k2 || k2 <= 0) {
                    const BlockPos blockpos(j3, l2, i2);
                    if (!worldIn->getBlockState(blockpos)->getBlock()->isFullBlock())
                        setBlockAndNotifyAdequately(worldIn, blockpos, leaves);
                }
            }
        }
        if (k2 >= 1 && l2 == position.getY() + j + 1)
            --k2;
        else if (k2 < l)
            ++k2;
    }
    for (int i3 = 0; i3 < i - 1; ++i3) {
        if (airOrLeaves(worldIn->getBlockState(position.up(i3))->getBlock()))
            setBlockAndNotifyAdequately(worldIn, position.up(i3), wood);
    }
    return true;
}

// --- WorldGenTaiga2 ---

bool WorldGenTaiga2::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const IBlockState* const wood = oldLog(BlockPlanks::EnumType::SPRUCE);
    const IBlockState* const leaves = oldLeaves(BlockPlanks::EnumType::SPRUCE);
    const int i = rand.nextInt(4) + 6;
    const int j = 1 + rand.nextInt(2);
    const int k = i - j;
    const int l = 2 + rand.nextInt(2);
    bool flag = true;
    if (position.getY() < 1 || position.getY() + i + 1 > 256)
        return false;
    for (int i1 = position.getY(); i1 <= position.getY() + 1 + i && flag; ++i1) {
        const int j1 = i1 - position.getY() < j ? 0 : l;
        BlockPos mutablePos;
        for (int k1 = position.getX() - j1; k1 <= position.getX() + j1 && flag; ++k1) {
            for (int l1 = position.getZ() - j1; l1 <= position.getZ() + j1 && flag; ++l1) {
                if (i1 >= 0 && i1 < 256) {
                    if (!airOrLeaves(worldIn->getBlockState(mutablePos.set(k1, i1, l1))->getBlock()))
                        flag = false;
                } else {
                    flag = false;
                }
            }
        }
    }
    if (!flag)
        return false;
    const Block* block1 = worldIn->getBlockState(position.down())->getBlock();
    if (!((block1 == Blocks::grass || block1 == Blocks::dirt || block1 == Blocks::farmland) && position.getY() < 256 - i - 1))
        return false;
    func_175921_a(worldIn, position.down());
    int i3 = rand.nextInt(2);
    int j3 = 1;
    int k3 = 0;
    for (int l3 = 0; l3 <= k; ++l3) {
        const int j4 = position.getY() + i - l3;
        for (int i2 = position.getX() - i3; i2 <= position.getX() + i3; ++i2) {
            const int j2 = i2 - position.getX();
            for (int k2 = position.getZ() - i3; k2 <= position.getZ() + i3; ++k2) {
                const int l2 = k2 - position.getZ();
                if (std::abs(j2) != i3 || std::abs(l2) != i3 || i3 <= 0) {
                    const BlockPos blockpos(i2, j4, k2);
                    if (!worldIn->getBlockState(blockpos)->getBlock()->isFullBlock())
                        setBlockAndNotifyAdequately(worldIn, blockpos, leaves);
                }
            }
        }
        if (i3 >= j3) {
            i3 = k3;
            k3 = 1;
            ++j3;
            if (j3 > l)
                j3 = l;
        } else {
            ++i3;
        }
    }
    const int i4 = rand.nextInt(3);
    for (int k4 = 0; k4 < i - i4; ++k4) {
        if (airOrLeaves(worldIn->getBlockState(position.up(k4))->getBlock()))
            setBlockAndNotifyAdequately(worldIn, position.up(k4), wood);
    }
    return true;
}

// --- WorldGenHugeTrees ---

int WorldGenHugeTrees::func_150533_a(Random& rand)
{
    int i = rand.nextInt(3) + baseHeight;
    if (extraRandomHeight > 1)
        i += rand.nextInt(extraRandomHeight);
    return i;
}

bool WorldGenHugeTrees::func_175926_c(World* worldIn, const BlockPos& pos, int height)
{
    bool flag = true;
    if (pos.getY() < 1 || pos.getY() + height + 1 > 256)
        return false;
    for (int i = 0; i <= 1 + height; ++i) {
        int j = 2;
        if (i == 0)
            j = 1;
        else if (i >= 1 + height - 2)
            j = 2;
        for (int k = -j; k <= j && flag; ++k) {
            for (int l = -j; l <= j && flag; ++l) {
                if (pos.getY() + i < 0 || pos.getY() + i >= 256 || !func_150523_a(worldIn->getBlockState(pos.add(k, i, l))->getBlock()))
                    flag = false;
            }
        }
    }
    return flag;
}

bool WorldGenHugeTrees::func_175927_a(const BlockPos& pos, World* worldIn)
{
    const BlockPos blockpos = pos.down();
    const Block* block = worldIn->getBlockState(blockpos)->getBlock();
    if ((block == Blocks::grass || block == Blocks::dirt) && pos.getY() >= 2) {
        func_175921_a(worldIn, blockpos);
        func_175921_a(worldIn, blockpos.east());
        func_175921_a(worldIn, blockpos.south());
        func_175921_a(worldIn, blockpos.south().east());
        return true;
    }
    return false;
}

bool WorldGenHugeTrees::func_175929_a(World* worldIn, Random&, const BlockPos& pos, int height)
{
    return func_175926_c(worldIn, pos, height) && func_175927_a(pos, worldIn);
}

void WorldGenHugeTrees::func_175925_a(World* worldIn, const BlockPos& pos, int radius)
{
    const int i = radius * radius;
    for (int j = -radius; j <= radius + 1; ++j) {
        for (int k = -radius; k <= radius + 1; ++k) {
            const int l = j - 1;
            const int i1 = k - 1;
            if (j * j + k * k <= i || l * l + i1 * i1 <= i || j * j + i1 * i1 <= i || l * l + k * k <= i) {
                const BlockPos blockpos = pos.add(j, 0, k);
                if (airOrLeaves(worldIn->getBlockState(blockpos)->getBlock()))
                    setBlockAndNotifyAdequately(worldIn, blockpos, leavesMetadata);
            }
        }
    }
}

void WorldGenHugeTrees::func_175928_b(World* worldIn, const BlockPos& pos, int radius)
{
    const int i = radius * radius;
    for (int j = -radius; j <= radius; ++j) {
        for (int k = -radius; k <= radius; ++k) {
            if (j * j + k * k <= i) {
                const BlockPos blockpos = pos.add(j, 0, k);
                if (airOrLeaves(worldIn->getBlockState(blockpos)->getBlock()))
                    setBlockAndNotifyAdequately(worldIn, blockpos, leavesMetadata);
            }
        }
    }
}

// --- WorldGenMegaPineTree ---

WorldGenMegaPineTree::WorldGenMegaPineTree(bool notify, bool useBaseHeightIn)
    : WorldGenHugeTrees(notify, 13, 15, oldLog(BlockPlanks::EnumType::SPRUCE), oldLeaves(BlockPlanks::EnumType::SPRUCE))
    , useBaseHeight(useBaseHeightIn)
{
}

bool WorldGenMegaPineTree::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const int i = func_150533_a(rand);
    if (!func_175929_a(worldIn, rand, position, i))
        return false;
    func_150541_c(worldIn, position.getX(), position.getZ(), position.getY() + i, 0, rand);
    for (int j = 0; j < i; ++j) {
        if (airOrLeaves(worldIn->getBlockState(position.up(j))->getBlock()))
            setBlockAndNotifyAdequately(worldIn, position.up(j), woodMetadata);
        if (j < i - 1) {
            if (airOrLeaves(worldIn->getBlockState(position.add(1, j, 0))->getBlock()))
                setBlockAndNotifyAdequately(worldIn, position.add(1, j, 0), woodMetadata);
            if (airOrLeaves(worldIn->getBlockState(position.add(1, j, 1))->getBlock()))
                setBlockAndNotifyAdequately(worldIn, position.add(1, j, 1), woodMetadata);
            if (airOrLeaves(worldIn->getBlockState(position.add(0, j, 1))->getBlock()))
                setBlockAndNotifyAdequately(worldIn, position.add(0, j, 1), woodMetadata);
        }
    }
    return true;
}

void WorldGenMegaPineTree::func_150541_c(World* worldIn, int x, int z, int y, int radius, Random& rand)
{
    const int i = rand.nextInt(5) + (useBaseHeight ? baseHeight : 3);
    int j = 0;
    for (int k = y - i; k <= y; ++k) {
        const int l = y - k;
        const int i1 = radius + MathHelper::floor_float(static_cast<float>(l) / static_cast<float>(i) * 3.5f);
        func_175925_a(worldIn, BlockPos(x, k, z), i1 + (l > 0 && i1 == j && (k & 1) == 0 ? 1 : 0));
        j = i1;
    }
}

void WorldGenMegaPineTree::func_180711_a(World* worldIn, Random& rand, const BlockPos& pos)
{
    func_175933_b(worldIn, pos.west().north());
    func_175933_b(worldIn, pos.east(2).north());
    func_175933_b(worldIn, pos.west().south(2));
    func_175933_b(worldIn, pos.east(2).south(2));
    for (int i = 0; i < 5; ++i) {
        const int j = rand.nextInt(64);
        const int k = j % 8;
        const int l = j / 8;
        if (k == 0 || k == 7 || l == 0 || l == 7)
            func_175933_b(worldIn, pos.add(-3 + k, 0, -3 + l));
    }
}

void WorldGenMegaPineTree::func_175933_b(World* worldIn, const BlockPos& pos)
{
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            if (std::abs(i) != 2 || std::abs(j) != 2)
                func_175934_c(worldIn, pos.add(i, 0, j));
        }
    }
}

void WorldGenMegaPineTree::func_175934_c(World* worldIn, const BlockPos& pos)
{
    const IBlockState* const podzol = Blocks::dirt->getDefaultState()->withProperty(BlockDirt::VARIANT, BlockDirt::DirtType::PODZOL);
    for (int i = 2; i >= -3; --i) {
        const BlockPos blockpos = pos.up(i);
        const Block* block = worldIn->getBlockState(blockpos)->getBlock();
        if (block == Blocks::grass || block == Blocks::dirt) {
            setBlockAndNotifyAdequately(worldIn, blockpos, podzol);
            break;
        }
        if (!isMaterial(block, Material::air) && i < 0)
            break;
    }
}

// --- WorldGenMegaJungle ---

bool WorldGenMegaJungle::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const int i = func_150533_a(rand);
    if (!func_175929_a(worldIn, rand, position, i))
        return false;
    func_175930_c(worldIn, position.up(i), 2);
    for (int j = position.getY() + i - 2 - rand.nextInt(4); j > position.getY() + i / 2; j -= 2 + rand.nextInt(4)) {
        const float f = rand.nextFloat() * 3.1415927f * 2.0f;
        int k = position.getX() + static_cast<int>(0.5f + MathHelper::cos(f) * 4.0f);
        int l = position.getZ() + static_cast<int>(0.5f + MathHelper::sin(f) * 4.0f);
        for (int i1 = 0; i1 < 5; ++i1) {
            k = position.getX() + static_cast<int>(1.5f + MathHelper::cos(f) * static_cast<float>(i1));
            l = position.getZ() + static_cast<int>(1.5f + MathHelper::sin(f) * static_cast<float>(i1));
            setBlockAndNotifyAdequately(worldIn, BlockPos(k, j - 3 + i1 / 2, l), woodMetadata);
        }
        const int j2 = 1 + rand.nextInt(2);
        const int j1 = j;
        for (int k1 = j - j2; k1 <= j1; ++k1) {
            const int l1 = k1 - j1;
            func_175928_b(worldIn, BlockPos(k, k1, l), 1 - l1);
        }
    }
    for (int i2 = 0; i2 < i; ++i2) {
        const BlockPos blockpos = position.up(i2);
        if (func_150523_a(worldIn->getBlockState(blockpos)->getBlock())) {
            setBlockAndNotifyAdequately(worldIn, blockpos, woodMetadata);
            if (i2 > 0) {
                func_181632_a(worldIn, rand, blockpos.west(), BlockVine::EAST);
                func_181632_a(worldIn, rand, blockpos.north(), BlockVine::SOUTH);
            }
        }
        if (i2 < i - 1) {
            const BlockPos blockpos1 = blockpos.east();
            if (func_150523_a(worldIn->getBlockState(blockpos1)->getBlock())) {
                setBlockAndNotifyAdequately(worldIn, blockpos1, woodMetadata);
                if (i2 > 0) {
                    func_181632_a(worldIn, rand, blockpos1.east(), BlockVine::WEST);
                    func_181632_a(worldIn, rand, blockpos1.north(), BlockVine::SOUTH);
                }
            }
            const BlockPos blockpos2 = blockpos.south().east();
            if (func_150523_a(worldIn->getBlockState(blockpos2)->getBlock())) {
                setBlockAndNotifyAdequately(worldIn, blockpos2, woodMetadata);
                if (i2 > 0) {
                    func_181632_a(worldIn, rand, blockpos2.east(), BlockVine::WEST);
                    func_181632_a(worldIn, rand, blockpos2.south(), BlockVine::NORTH);
                }
            }
            const BlockPos blockpos3 = blockpos.south();
            if (func_150523_a(worldIn->getBlockState(blockpos3)->getBlock())) {
                setBlockAndNotifyAdequately(worldIn, blockpos3, woodMetadata);
                if (i2 > 0) {
                    func_181632_a(worldIn, rand, blockpos3.west(), BlockVine::EAST);
                    func_181632_a(worldIn, rand, blockpos3.south(), BlockVine::NORTH);
                }
            }
        }
    }
    return true;
}

void WorldGenMegaJungle::func_181632_a(World* worldIn, Random& rand, const BlockPos& pos, const PropertyBool& side)
{
    if (rand.nextInt(3) > 0 && worldIn->isAirBlock(pos))
        setBlockAndNotifyAdequately(worldIn, pos, Blocks::vine->getDefaultState()->withProperty(side, true));
}

void WorldGenMegaJungle::func_175930_c(World* worldIn, const BlockPos& pos, int radius)
{
    const int i = 2;
    for (int j = -i; j <= 0; ++j)
        func_175925_a(worldIn, pos.up(j), radius + 1 - j);
}

// --- WorldGenCanopyTree ---

bool WorldGenCanopyTree::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const int a = rand.nextInt(3);
    const int i = a + rand.nextInt(2) + 6;
    const int j = position.getX();
    const int k = position.getY();
    const int l = position.getZ();
    if (k < 1 || k + i + 1 >= 256)
        return false;
    const BlockPos blockpos = position.down();
    const Block* block = worldIn->getBlockState(blockpos)->getBlock();
    if (block != Blocks::grass && block != Blocks::dirt)
        return false;
    if (!func_181638_a(worldIn, position, i))
        return false;
    func_175921_a(worldIn, blockpos);
    func_175921_a(worldIn, blockpos.east());
    func_175921_a(worldIn, blockpos.south());
    func_175921_a(worldIn, blockpos.south().east());
    const EnumFacing enumfacing = randomFacing(Plane::HORIZONTAL, rand);
    const int i1 = i - rand.nextInt(4);
    int j1 = 2 - rand.nextInt(3);
    int k1 = j;
    int l1 = l;
    const int i2 = k + i - 1;
    for (int j2 = 0; j2 < i; ++j2) {
        if (j2 >= i1 && j1 > 0) {
            k1 += getFrontOffsetX(enumfacing);
            l1 += getFrontOffsetZ(enumfacing);
            --j1;
        }
        const int k2 = k + j2;
        const BlockPos blockpos1(k1, k2, l1);
        if (airOrLeaves(worldIn->getBlockState(blockpos1)->getBlock())) {
            func_181639_b(worldIn, blockpos1);
            func_181639_b(worldIn, blockpos1.east());
            func_181639_b(worldIn, blockpos1.south());
            func_181639_b(worldIn, blockpos1.east().south());
        }
    }
    for (int i3 = -2; i3 <= 0; ++i3) {
        for (int l3 = -2; l3 <= 0; ++l3) {
            int k4 = -1;
            func_150526_a(worldIn, k1 + i3, i2 + k4, l1 + l3);
            func_150526_a(worldIn, 1 + k1 - i3, i2 + k4, l1 + l3);
            func_150526_a(worldIn, k1 + i3, i2 + k4, 1 + l1 - l3);
            func_150526_a(worldIn, 1 + k1 - i3, i2 + k4, 1 + l1 - l3);
            if ((i3 > -2 || l3 > -1) && (i3 != -1 || l3 != -2)) {
                k4 = 1;
                func_150526_a(worldIn, k1 + i3, i2 + k4, l1 + l3);
                func_150526_a(worldIn, 1 + k1 - i3, i2 + k4, l1 + l3);
                func_150526_a(worldIn, k1 + i3, i2 + k4, 1 + l1 - l3);
                func_150526_a(worldIn, 1 + k1 - i3, i2 + k4, 1 + l1 - l3);
            }
        }
    }
    if (rand.nextBoolean()) {
        func_150526_a(worldIn, k1, i2 + 2, l1);
        func_150526_a(worldIn, k1 + 1, i2 + 2, l1);
        func_150526_a(worldIn, k1 + 1, i2 + 2, l1 + 1);
        func_150526_a(worldIn, k1, i2 + 2, l1 + 1);
    }
    for (int j3 = -3; j3 <= 4; ++j3) {
        for (int i4 = -3; i4 <= 4; ++i4) {
            if ((j3 != -3 || i4 != -3) && (j3 != -3 || i4 != 4) && (j3 != 4 || i4 != -3) && (j3 != 4 || i4 != 4)
                && (std::abs(j3) < 3 || std::abs(i4) < 3))
                func_150526_a(worldIn, k1 + j3, i2, l1 + i4);
        }
    }
    for (int k3 = -1; k3 <= 2; ++k3) {
        for (int j4 = -1; j4 <= 2; ++j4) {
            if ((k3 < 0 || k3 > 1 || j4 < 0 || j4 > 1) && rand.nextInt(3) <= 0) {
                const int l4 = rand.nextInt(3) + 2;
                for (int i5 = 0; i5 < l4; ++i5)
                    func_181639_b(worldIn, BlockPos(j + k3, i2 - i5 - 1, l + j4));
                for (int j5 = -1; j5 <= 1; ++j5) {
                    for (int l2 = -1; l2 <= 1; ++l2)
                        func_150526_a(worldIn, k1 + k3 + j5, i2, l1 + j4 + l2);
                }
                for (int k5 = -2; k5 <= 2; ++k5) {
                    for (int l5 = -2; l5 <= 2; ++l5) {
                        if (std::abs(k5) != 2 || std::abs(l5) != 2)
                            func_150526_a(worldIn, k1 + k3 + k5, i2 - 1, l1 + j4 + l5);
                    }
                }
            }
        }
    }
    return true;
}

bool WorldGenCanopyTree::func_181638_a(World* worldIn, const BlockPos& pos, int height)
{
    const int i = pos.getX();
    const int j = pos.getY();
    const int k = pos.getZ();
    BlockPos mutablePos;
    for (int l = 0; l <= height + 1; ++l) {
        int i1 = 1;
        if (l == 0)
            i1 = 0;
        if (l >= height - 1)
            i1 = 2;
        for (int j1 = -i1; j1 <= i1; ++j1) {
            for (int k1 = -i1; k1 <= i1; ++k1) {
                if (!func_150523_a(worldIn->getBlockState(mutablePos.set(i + j1, j + l, k + k1))->getBlock()))
                    return false;
            }
        }
    }
    return true;
}

void WorldGenCanopyTree::func_181639_b(World* worldIn, const BlockPos& pos)
{
    if (func_150523_a(worldIn->getBlockState(pos)->getBlock()))
        setBlockAndNotifyAdequately(worldIn, pos, newLog(BlockPlanks::EnumType::DARK_OAK));
}

void WorldGenCanopyTree::func_150526_a(World* worldIn, int x, int y, int z)
{
    const BlockPos blockpos(x, y, z);
    if (isMaterial(worldIn->getBlockState(blockpos)->getBlock(), Material::air))
        setBlockAndNotifyAdequately(worldIn, blockpos, newLeaves(BlockPlanks::EnumType::DARK_OAK));
}

// --- WorldGenSavannaTree ---

bool WorldGenSavannaTree::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const int a = rand.nextInt(3);
    const int i = a + rand.nextInt(3) + 5;
    bool flag = true;
    if (position.getY() < 1 || position.getY() + i + 1 > 256)
        return false;
    for (int j = position.getY(); j <= position.getY() + 1 + i; ++j) {
        int k = 1;
        if (j == position.getY())
            k = 0;
        if (j >= position.getY() + 1 + i - 2)
            k = 2;
        BlockPos mutablePos;
        for (int l = position.getX() - k; l <= position.getX() + k && flag; ++l) {
            for (int i1 = position.getZ() - k; i1 <= position.getZ() + k && flag; ++i1) {
                if (j >= 0 && j < 256) {
                    if (!func_150523_a(worldIn->getBlockState(mutablePos.set(l, j, i1))->getBlock()))
                        flag = false;
                } else {
                    flag = false;
                }
            }
        }
    }
    if (!flag)
        return false;
    const Block* block = worldIn->getBlockState(position.down())->getBlock();
    if (!((block == Blocks::grass || block == Blocks::dirt) && position.getY() < 256 - i - 1))
        return false;
    func_175921_a(worldIn, position.down());
    const EnumFacing enumfacing = randomFacing(Plane::HORIZONTAL, rand);
    const int k2 = i - rand.nextInt(4) - 1;
    int l2 = 3 - rand.nextInt(3);
    int i3 = position.getX();
    int j1 = position.getZ();
    int k1 = 0;
    for (int l1 = 0; l1 < i; ++l1) {
        const int i2 = position.getY() + l1;
        if (l1 >= k2 && l2 > 0) {
            i3 += getFrontOffsetX(enumfacing);
            j1 += getFrontOffsetZ(enumfacing);
            --l2;
        }
        const BlockPos blockpos(i3, i2, j1);
        if (airOrLeaves(worldIn->getBlockState(blockpos)->getBlock())) {
            func_181642_b(worldIn, blockpos);
            k1 = i2;
        }
    }
    BlockPos blockpos2(i3, k1, j1);
    for (int j3 = -3; j3 <= 3; ++j3) {
        for (int i4 = -3; i4 <= 3; ++i4) {
            if (std::abs(j3) != 3 || std::abs(i4) != 3)
                func_175924_b(worldIn, blockpos2.add(j3, 0, i4));
        }
    }
    blockpos2 = blockpos2.up();
    for (int k3 = -1; k3 <= 1; ++k3) {
        for (int j4 = -1; j4 <= 1; ++j4)
            func_175924_b(worldIn, blockpos2.add(k3, 0, j4));
    }
    func_175924_b(worldIn, blockpos2.east(2));
    func_175924_b(worldIn, blockpos2.west(2));
    func_175924_b(worldIn, blockpos2.south(2));
    func_175924_b(worldIn, blockpos2.north(2));
    i3 = position.getX();
    j1 = position.getZ();
    const EnumFacing enumfacing1 = randomFacing(Plane::HORIZONTAL, rand);
    if (enumfacing1 != enumfacing) {
        const int l3 = k2 - rand.nextInt(2) - 1;
        int k4 = 1 + rand.nextInt(3);
        k1 = 0;
        for (int l4 = l3; l4 < i && k4 > 0; --k4) {
            if (l4 >= 1) {
                const int j2 = position.getY() + l4;
                i3 += getFrontOffsetX(enumfacing1);
                j1 += getFrontOffsetZ(enumfacing1);
                const BlockPos blockpos1(i3, j2, j1);
                if (airOrLeaves(worldIn->getBlockState(blockpos1)->getBlock())) {
                    func_181642_b(worldIn, blockpos1);
                    k1 = j2;
                }
            }
            ++l4;
        }
        if (k1 > 0) {
            BlockPos blockpos3(i3, k1, j1);
            for (int i5 = -2; i5 <= 2; ++i5) {
                for (int k5 = -2; k5 <= 2; ++k5) {
                    if (std::abs(i5) != 2 || std::abs(k5) != 2)
                        func_175924_b(worldIn, blockpos3.add(i5, 0, k5));
                }
            }
            blockpos3 = blockpos3.up();
            for (int j5 = -1; j5 <= 1; ++j5) {
                for (int l5 = -1; l5 <= 1; ++l5)
                    func_175924_b(worldIn, blockpos3.add(j5, 0, l5));
            }
        }
    }
    return true;
}

void WorldGenSavannaTree::func_181642_b(World* worldIn, const BlockPos& pos)
{
    setBlockAndNotifyAdequately(worldIn, pos, newLog(BlockPlanks::EnumType::ACACIA));
}

void WorldGenSavannaTree::func_175924_b(World* worldIn, const BlockPos& pos)
{
    if (airOrLeaves(worldIn->getBlockState(pos)->getBlock()))
        setBlockAndNotifyAdequately(worldIn, pos, newLeaves(BlockPlanks::EnumType::ACACIA));
}

} // namespace mc
