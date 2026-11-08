#include "block/Block.h"

#include "client/resources/Locale.h"
#include "core/Java.h"
#include "core/JavaString.h"
#include "creativetab/CreativeTabs.h"
#include "item/ItemStack.h"
#include "util/Random.h"

namespace mc {

// --- SoundType ---

std::string Block::SoundType::getBreakSound() const
{
    switch (kind) {
    case Kind::GLASS: return "dig.glass";
    case Kind::LADDER: return "dig.wood";
    case Kind::ANVIL: return "dig.stone";
    case Kind::SLIME: return "mob.slime.big";
    default: return "dig." + soundName;
    }
}

std::string Block::SoundType::getStepSound() const
{
    return kind == Kind::SLIME ? "mob.slime.small" : "step." + soundName;
}

std::string Block::SoundType::getPlaceSound() const
{
    switch (kind) {
    case Kind::GLASS: return "step.stone";
    case Kind::ANVIL: return "random.anvil_land";
    case Kind::SLIME: return "mob.slime.big";
    default: return getBreakSound();
    }
}

const Block::SoundType Block::soundTypeStone("stone", 1.0f, 1.0f);
const Block::SoundType Block::soundTypeWood("wood", 1.0f, 1.0f);
const Block::SoundType Block::soundTypeGravel("gravel", 1.0f, 1.0f);
const Block::SoundType Block::soundTypeGrass("grass", 1.0f, 1.0f);
const Block::SoundType Block::soundTypePiston("stone", 1.0f, 1.0f);
const Block::SoundType Block::soundTypeMetal("stone", 1.0f, 1.5f);
const Block::SoundType Block::soundTypeGlass("stone", 1.0f, 1.0f, SoundType::Kind::GLASS);
const Block::SoundType Block::soundTypeCloth("cloth", 1.0f, 1.0f);
const Block::SoundType Block::soundTypeSand("sand", 1.0f, 1.0f);
const Block::SoundType Block::soundTypeSnow("snow", 1.0f, 1.0f);
const Block::SoundType Block::soundTypeLadder("ladder", 1.0f, 1.0f, SoundType::Kind::LADDER);
const Block::SoundType Block::soundTypeAnvil("anvil", 0.3f, 1.0f, SoundType::Kind::ANVIL);
const Block::SoundType Block::SLIME_SOUND("slime", 1.0f, 1.0f, SoundType::Kind::SLIME);

// --- Registry ---

RegistryNamespacedDefaultedByKey<ResourceLocation, Block> Block::blockRegistry(ResourceLocation("air"));
ObjectIntIdentityMap<IBlockState> Block::BLOCK_STATE_IDS;

std::vector<std::unique_ptr<Block>>& Block::ownedBlocks()
{
    static std::vector<std::unique_ptr<Block>> blocks;
    return blocks;
}

int Block::getIdFromBlock(const Block* block)
{
    return blockRegistry.getIDForObject(block);
}

int Block::getStateId(const IBlockState* state)
{
    const Block* block = state->getBlock();
    return getIdFromBlock(block) + (block->getMetaFromState(state) << 12);
}

Block* Block::getBlockById(int id)
{
    return blockRegistry.getObjectById(id);
}

const IBlockState* Block::getStateById(int id)
{
    const int i = id & 4095;
    const int j = (id >> 12) & 15;
    return getBlockById(i)->getStateFromMeta(j);
}

Block* Block::getBlockFromName(std::string_view name)
{
    const ResourceLocation location(name);
    if (blockRegistry.containsKey(location))
        return blockRegistry.getObject(location);
    try {
        return blockRegistry.getObjectById(java::parseInt(name));
    } catch (const std::invalid_argument&) {
        return nullptr;
    }
}

std::string Block::getNameForBlock(const Block* block)
{
    const auto name = blockRegistry.getNameForObject(block);
    return name ? name->toString() : "null";
}

void Block::registerBlock(int id, const ResourceLocation& textualID, Block* block)
{
    blockRegistry.registerObject(id, textualID, block);
}

void Block::registerBlock(int id, const std::string& textualID, Block* block)
{
    registerBlock(id, ResourceLocation(textualID), block);
}

// --- Block ---

Block::Block(const Material& materialIn, const MapColor& mapColorIn)
    : blockMaterial(materialIn)
    , blockMapColor(mapColorIn)
{
}

void Block::construct()
{
    enableStats = true;
    stepSound = &soundTypeStone;
    blockParticleGravity = 1.0f;
    slipperiness = 0.6f;
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    fullBlock = isOpaqueCube();
    lightOpacity = isOpaqueCube() ? 255 : 0;
    translucent = !blockMaterial.blocksLight();
    blockState = createBlockState();
    setDefaultState(blockState->getBaseState());
}

std::unique_ptr<BlockState> Block::createBlockState()
{
    return makeBlockState({});
}

const MapColor& Block::getMapColor(const IBlockState* /*state*/) const
{
    return blockMapColor;
}

const IBlockState* Block::getStateFromMeta(int /*meta*/) const
{
    return getDefaultState();
}

int Block::getMetaFromState(const IBlockState* state) const
{
    if (state != nullptr && !state->getPropertyNames().empty())
        throw std::invalid_argument("Don't know how to convert " + state->toString() + " back into data...");
    return 0;
}

Block* Block::setStepSound(const SoundType& sound)
{
    stepSound = &sound;
    return this;
}

Block* Block::setLightOpacity(int opacity)
{
    lightOpacity = opacity;
    return this;
}

Block* Block::setLightLevel(float value)
{
    lightValue = java::f2i(15.0f * value);
    return this;
}

Block* Block::setResistance(float resistance)
{
    blockResistance = resistance * 3.0f;
    return this;
}

Block* Block::setHardness(float hardness)
{
    blockHardness = hardness;
    if (blockResistance < hardness * 5.0f)
        blockResistance = hardness * 5.0f;
    return this;
}

Block* Block::setTickRandomly(bool shouldTick)
{
    needsRandomTick = shouldTick;
    return this;
}

Block* Block::setUnlocalizedName(std::string name)
{
    unlocalizedName = std::move(name);
    return this;
}

Block* Block::setCreativeTab(const CreativeTabs* tab)
{
    displayOnCreativeTab = tab;
    return this;
}

Block* Block::disableStats()
{
    enableStats = false;
    return this;
}

bool Block::isBlockNormalCube() const
{
    return blockMaterial.blocksMovement() && isFullCube();
}

bool Block::isNormalCube() const
{
    return blockMaterial.isOpaque() && isFullCube() && !canProvidePower();
}

bool Block::isVisuallyOpaque() const
{
    return blockMaterial.blocksMovement() && isFullCube();
}

void Block::setBlockBounds(float minXIn, float minYIn, float minZIn, float maxXIn, float maxYIn, float maxZIn)
{
    minX = minXIn;
    minY = minYIn;
    minZ = minZIn;
    maxX = maxXIn;
    maxY = maxYIn;
    maxZ = maxZIn;
}

bool Block::canCollideCheck(const IBlockState* /*state*/, bool /*hitIfLiquid*/) const
{
    return isCollidable();
}

int Block::quantityDropped(Random& /*random*/) const
{
    return 1;
}

int Block::damageDropped(const IBlockState* /*state*/) const
{
    return 0;
}

int Block::getRenderColor(const IBlockState* /*state*/) const
{
    return 16777215;
}

bool Block::canSilkHarvest() const
{
    return isFullCube() && !isBlockContainer;
}

int Block::quantityDroppedWithBonus(int /*fortune*/, Random& random) const
{
    return quantityDropped(random);
}

bool Block::canSpawnInBlock() const
{
    return !blockMaterial.isSolid() && !blockMaterial.isLiquid();
}

std::string Block::getLocalizedName() const
{
    return StatCollector::translateToLocal(getUnlocalizedName() + ".name");
}

std::string Block::getUnlocalizedName() const
{
    return "tile." + unlocalizedName;
}

int Block::getMobilityFlag() const
{
    return blockMaterial.getMaterialMobility();
}

float Block::getAmbientOcclusionLightValue() const
{
    return isBlockNormalCube() ? 0.2f : 1.0f;
}

bool Block::isEqualTo(const Block* blockIn, const Block* other)
{
    return blockIn != nullptr && other != nullptr && (blockIn == other || blockIn->isAssociatedBlock(other));
}

std::string Block::toString() const
{
    return "Block{" + getNameForBlock(this) + "}";
}

void Block::getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const
{
    list.emplace_back(item, 1, 0);
}

void Block::addSubBlocks(Item* item, std::vector<ItemStack>& list, std::initializer_list<int> metas)
{
    for (const int meta : metas)
        list.emplace_back(item, 1, meta);
}

void Block::addSubBlocks(Item* item, std::vector<ItemStack>& list, int from, int to)
{
    for (int meta = from; meta < to; ++meta)
        list.emplace_back(item, 1, meta);
}

} // namespace mc
