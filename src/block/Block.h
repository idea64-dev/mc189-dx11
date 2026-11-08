#pragma once

// Original: net/minecraft/block/Block.java (with Block.SoundType and Block.EnumOffsetType) and
// util/EnumWorldBlockLayer.java.
// Ported so far: everything that doesn't need a world -- construction, the registry, states and metadata,
// physical properties, bounds, render type and layer. World, entity, item and tile entity callbacks are added
// by the stages that bring those systems; the Java methods are named in comments where they belong.
//
// Construction in two steps: Java's Block constructor calls overridable methods (isOpaqueCube, createBlockState)
// that dispatch to the most-derived class, and subclass constructors run afterwards. C++ constructors only take
// the arguments; construct() then runs the Java constructor bodies, base first, with virtual calls reaching the
// final class. Blocks are made through Block::create<T>(...), which does both.

#include "block/material/Material.h"
#include "block/state/BlockState.h"
#include "util/BlockPos.h"
#include "util/Registry.h"
#include "util/ResourceLocation.h"

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Declares getClassName() for a block subclass.
#define MC_BLOCK_CLASS(NAME)     std::string_view getClassName() const override { return #NAME; }

namespace mc {

class CreativeTabs;
class Entity;
class EntityLivingBase;
class EntityPlayer;
class Explosion;
class IBlockAccess;
class Item;
class ItemStack;
class Random;
class TileEntity;
class World;
struct AxisAlignedBB;
struct MovingObjectPosition;
struct Vec3;

enum class EnumWorldBlockLayer { SOLID, CUTOUT_MIPPED, CUTOUT, TRANSLUCENT };

// Java: toString()
constexpr std::string_view getLayerName(EnumWorldBlockLayer layer)
{
    constexpr std::string_view names[4] { "Solid", "Mipped Cutout", "Cutout", "Translucent" };
    return names[static_cast<int>(layer)];
}

class Block {
public:
    class SoundType {
    public:
        enum class Kind { NORMAL, GLASS, LADDER, ANVIL, SLIME };

        SoundType(std::string name, float volume, float frequency, Kind kind = Kind::NORMAL)
            : soundName(std::move(name))
            , volume(volume)
            , frequency(frequency)
            , kind(kind)
        {
        }

        float getVolume() const { return volume; }
        float getFrequency() const { return frequency; }
        std::string getBreakSound() const;
        std::string getStepSound() const;
        std::string getPlaceSound() const;

        const std::string soundName;
        const float volume;
        const float frequency;

    private:
        // The anonymous subclasses of glass, ladder, anvil and slime.
        Kind kind;
    };

    enum class EnumOffsetType { NONE, XZ, XYZ };

    static const SoundType soundTypeStone, soundTypeWood, soundTypeGravel, soundTypeGrass, soundTypePiston, soundTypeMetal,
        soundTypeGlass, soundTypeCloth, soundTypeSand, soundTypeSnow, soundTypeLadder, soundTypeAnvil, SLIME_SOUND;

    static RegistryNamespacedDefaultedByKey<ResourceLocation, Block> blockRegistry;
    static ObjectIntIdentityMap<IBlockState> BLOCK_STATE_IDS;

    // new T(args...) followed by construct(); the block lives for the rest of the program.
    template <class T, class... Args>
    static T* create(Args&&... args)
    {
        auto block = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = block.get();
        static_cast<Block*>(raw)->construct();
        ownedBlocks().push_back(std::move(block));
        return raw;
    }

    Block(const Material& materialIn, const MapColor& mapColorIn);
    explicit Block(const Material& materialIn)
        : Block(materialIn, materialIn.getMaterialMapColor())
    {
    }
    virtual ~Block() = default;

    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    static int getIdFromBlock(const Block* block);
    // Block id plus meta << 12.
    static int getStateId(const IBlockState* state);
    static Block* getBlockById(int id);
    static const IBlockState* getStateById(int id);
    // By registry name, or by numeric id; null when neither matches.
    static Block* getBlockFromName(std::string_view name);
    // The registry name, "null" if unregistered.
    static std::string getNameForBlock(const Block* block);

    bool isFullBlock() const { return fullBlock; }
    int getLightOpacity() const { return lightOpacity; }
    bool isTranslucent() const { return translucent; }
    int getLightValue() const { return lightValue; }
    bool getUseNeighborBrightness() const { return useNeighborBrightness; }
    const Material& getMaterial() const { return blockMaterial; }
    virtual const MapColor& getMapColor(const IBlockState* state) const;
    virtual const IBlockState* getStateFromMeta(int meta) const;
    // Throws std::invalid_argument for blocks with properties that don't override it.
    virtual int getMetaFromState(const IBlockState* state) const;

    // --- world callbacks (Java signatures; the IBlockAccess or World comes first as a pointer) ---
    virtual const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos);
    virtual bool isPassable(IBlockAccess* worldIn, const BlockPos& pos);
    virtual bool isReplaceable(World* worldIn, const BlockPos& pos);
    virtual float getBlockHardness(World* worldIn, const BlockPos& pos);
    virtual int getMixedBrightnessForBlock(IBlockAccess* worldIn, const BlockPos& pos);
    virtual bool shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side);
    virtual bool isBlockSolid(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side);
    virtual AxisAlignedBB getSelectedBoundingBox(World* worldIn, const BlockPos& pos);
    virtual void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity);
    // Java returns null for blocks without collision.
    virtual std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state);
    virtual void randomTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& random);
    virtual void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand);
    virtual void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand);
    virtual void onBlockDestroyedByPlayer(World* worldIn, const BlockPos& pos, const IBlockState* state);
    virtual void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock);
    virtual int tickRate(World* worldIn);
    virtual void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state);
    virtual void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state);
    virtual float getPlayerRelativeBlockHardness(EntityPlayer* playerIn, World* worldIn, const BlockPos& pos);
    void dropBlockAsItem(World* worldIn, const BlockPos& pos, const IBlockState* state, int fortune)
    {
        dropBlockAsItemWithChance(worldIn, pos, state, 1.0f, fortune);
    }
    virtual void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune);
    static void spawnAsEntity(World* worldIn, const BlockPos& pos, ItemStack stack);
    // Java: getExplosionResistance(Entity exploder); no block looks at the exploder.
    virtual float getExplosionResistance() const { return blockResistance / 5.0f; }
    float getExplosionResistance(Entity*) const { return getExplosionResistance(); }
    virtual std::optional<MovingObjectPosition> collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end);
    virtual void onBlockDestroyedByExplosion(World*, const BlockPos&, Explosion*) { }
    virtual bool canReplace(World* worldIn, const BlockPos& pos, EnumFacing side, const ItemStack* stack);
    virtual bool canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side);
    virtual bool canPlaceBlockAt(World* worldIn, const BlockPos& pos);
    virtual bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ);
    virtual void onEntityCollidedWithBlock(World*, const BlockPos&, Entity*) { }
    virtual const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer);
    virtual void onBlockClicked(World*, const BlockPos&, EntityPlayer*) { }
    virtual Vec3 modifyAcceleration(World* worldIn, const BlockPos& pos, Entity* entityIn, const Vec3& motion);
    virtual void setBlockBoundsBasedOnState(IBlockAccess*, const BlockPos&) { }
    virtual int colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass);
    int colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos) { return colorMultiplier(worldIn, pos, 0); }
    virtual int getWeakPower(IBlockAccess*, const BlockPos&, const IBlockState*, EnumFacing) { return 0; }
    virtual void onEntityCollidedWithBlock(World*, const BlockPos&, const IBlockState*, Entity*) { }
    virtual int getStrongPower(IBlockAccess*, const BlockPos&, const IBlockState*, EnumFacing) { return 0; }
    virtual void harvestBlock(World* worldIn, EntityPlayer* player, const BlockPos& pos, const IBlockState* state, TileEntity* te);
    virtual void onBlockPlacedBy(World*, const BlockPos&, const IBlockState*, EntityLivingBase*, const ItemStack&) { }
    virtual bool onBlockEventReceived(World*, const BlockPos&, const IBlockState*, int, int) { return false; }
    virtual void onFallenUpon(World* worldIn, const BlockPos& pos, Entity* entityIn, float fallDistance);
    virtual void onLanded(World* worldIn, Entity* entityIn);
    virtual Item* getItem(World* worldIn, const BlockPos& pos);
    virtual int getDamageValue(World* worldIn, const BlockPos& pos);
    virtual void onBlockHarvested(World*, const BlockPos&, const IBlockState*, EntityPlayer*) { }
    virtual void fillWithRain(World*, const BlockPos&) { }
    virtual bool canDropFromExplosion(Explosion*) { return true; }
    virtual int getComparatorInputOverride(World*, const BlockPos&) { return 0; }
    // Java: ITileEntityProvider.createNewTileEntity -- null for blocks without a tile entity.
    virtual std::shared_ptr<TileEntity> createNewTileEntity(World*, int) { return nullptr; }
    // Java: Block.getBlockFromItem -- the block of an ItemBlock, null otherwise.
    static Block* getBlockFromItem(const Item* item);

    Block* setStepSound(const SoundType& sound);
    Block* setLightOpacity(int opacity);
    Block* setLightLevel(float value);
    Block* setResistance(float resistance);
    Block* setHardness(float hardness);
    Block* setBlockUnbreakable() { return setHardness(-1.0f); }
    Block* setTickRandomly(bool shouldTick);
    Block* setUnlocalizedName(std::string name);
    Block* setCreativeTab(const CreativeTabs* tab);
    Block* disableStats();

    bool isBlockNormalCube() const;
    bool isNormalCube() const;
    virtual bool isVisuallyOpaque() const;
    virtual bool isFullCube() const { return true; }
    virtual int getRenderType() const { return 3; }
    // The hardness field; Java reads it through getBlockHardness(world, pos), which some blocks override.
    float getBlockHardnessValue() const { return blockHardness; }
    float getBlockResistanceValue() const { return blockResistance; }
    bool getTickRandomly() const { return needsRandomTick; }
    bool hasTileEntity() const { return isBlockContainer; }
    virtual bool isOpaqueCube() const { return true; }
    virtual bool isCollidable() const { return true; }
    virtual bool canCollideCheck(const IBlockState* state, bool hitIfLiquid) const;
    virtual int quantityDropped(Random& random) const;
    virtual Item* getItemDropped(const IBlockState* state, Random& rand, int fortune) const;
    virtual int damageDropped(const IBlockState* state) const;
    virtual EnumWorldBlockLayer getBlockLayer() const { return EnumWorldBlockLayer::SOLID; }
    double getBlockBoundsMinX() const { return minX; }
    double getBlockBoundsMaxX() const { return maxX; }
    double getBlockBoundsMinY() const { return minY; }
    double getBlockBoundsMaxY() const { return maxY; }
    double getBlockBoundsMinZ() const { return minZ; }
    double getBlockBoundsMaxZ() const { return maxZ; }
    virtual int getBlockColor() const { return 16777215; }
    virtual int getRenderColor(const IBlockState* state) const;
    virtual bool canProvidePower() const { return false; }
    virtual void setBlockBoundsForItemRender() { }
    // Protected in Java; public here so the registry dump can compare them.
    virtual bool canSilkHarvest() const;
    virtual ItemStack createStackedBlock(const IBlockState* state) const;
    virtual int quantityDroppedWithBonus(int fortune, Random& random) const;
    virtual bool canSpawnInBlock() const;
    std::string getLocalizedName() const;
    virtual std::string getUnlocalizedName() const;
    bool getEnableStats() const { return enableStats; }
    virtual int getMobilityFlag() const;
    virtual float getAmbientOcclusionLightValue() const;
    const CreativeTabs* getCreativeTabToDisplayOn() const { return displayOnCreativeTab; }
    virtual bool isFlowerPot() const { return false; }
    virtual bool requiresUpdates() const { return true; }
    virtual bool isAssociatedBlock(const Block* other) const { return this == other; }
    static bool isEqualTo(const Block* blockIn, const Block* other);
    virtual bool hasComparatorInputOverride() const { return false; }
    virtual const IBlockState* getStateForEntityRender(const IBlockState* state) const { return state; }
    const BlockState& getBlockState() const { return *blockState; }
    const IBlockState* getDefaultState() const { return defaultBlockState; }
    virtual EnumOffsetType getOffsetType() const { return EnumOffsetType::NONE; }
    // The block's creative inventory stacks; one of the item with metadata 0 unless overridden.
    virtual void getSubBlocks(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& list) const;
    std::string toString() const;
    // Java: getClass().getSimpleName(), for the registry dump.
    virtual std::string_view getClassName() const { return "Block"; }

    // Java: Block.registerBlocks(), called from Bootstrap.
    static void registerBlocks();

    float slipperiness = 0.6f;
    float blockParticleGravity = 1.0f;
    const SoundType* stepSound = &soundTypeStone;

protected:
    // The Java constructor body; subclasses call their parent's construct() first.
    virtual void construct();
    virtual std::unique_ptr<BlockState> createBlockState();
    std::unique_ptr<BlockState> makeBlockState(std::vector<const IProperty*> properties)
    {
        return std::make_unique<BlockState>(this, std::move(properties));
    }
    void setDefaultState(const IBlockState* state) { defaultBlockState = state; }
    void setBlockBounds(float minXIn, float minYIn, float minZIn, float maxXIn, float maxYIn, float maxZIn);
    // getSubBlocks helpers: a stack of the item for each metadata, or for each in [from, to).
    static void addSubBlocks(Item* item, std::vector<ItemStack>& list, std::initializer_list<int> metas);
    static void addSubBlocks(Item* item, std::vector<ItemStack>& list, int from, int to);

    bool fullBlock = false;
    int lightOpacity = 0;
    bool translucent = false;
    int lightValue = 0;
    bool useNeighborBrightness = false;
    float blockHardness = 0.0f;
    float blockResistance = 0.0f;
    bool enableStats = true;
    bool needsRandomTick = false;
    bool isBlockContainer = false;
    double minX = 0.0;
    double minY = 0.0;
    double minZ = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    double maxZ = 0.0;
    const Material& blockMaterial;
    const MapColor& blockMapColor;
    std::unique_ptr<BlockState> blockState;

private:
    static void registerBlock(int id, const ResourceLocation& textualID, Block* block);
    static void registerBlock(int id, const std::string& textualID, Block* block);
    static std::vector<std::unique_ptr<Block>>& ownedBlocks();

    const CreativeTabs* displayOnCreativeTab = nullptr;
    const IBlockState* defaultBlockState = nullptr;
    // Java's null: blocks registered without a name show "tile.null".
    std::string unlocalizedName = "null";
};

} // namespace mc
