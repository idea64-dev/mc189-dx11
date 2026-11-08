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
#include "util/Registry.h"
#include "util/ResourceLocation.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Declares getClassName() for a block subclass.
#define MC_BLOCK_CLASS(NAME)     std::string_view getClassName() const override { return #NAME; }

namespace mc {

class CreativeTabs;
class Random;

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
    // Java: getActualState(state, world, pos) -- needs a world (stage 5).

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
    virtual int damageDropped(const IBlockState* state) const;
    virtual float getExplosionResistance() const { return blockResistance / 5.0f; }
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
    // Protected in Java; public here so the registry dump can compare it.
    virtual bool canSilkHarvest() const;
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
