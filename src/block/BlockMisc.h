#pragma once

// The remaining blocks: net/minecraft/block/BlockTNT.java, BlockTallGrass.java, BlockTrapDoor.java,
// BlockTripWire.java, BlockTripWireHook.java, BlockVine.java, BlockWall.java, BlockWeb.java, BlockWorkbench.java,
// BlockStoneBrick.java, BlockRedstoneLight.java, BlockRedstoneOre.java and BlockSeaLantern.java

#include "block/BlockBush.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/Colorizer.h"

#include <array>

namespace mc {

class BlockTNT : public Block {
public:
    MC_BLOCK_CLASS(BlockTNT)

    static const PropertyBool EXPLODE;

    BlockTNT()
        : Block(Material::tnt)
    {
    }

    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void onBlockDestroyedByExplosion(World* worldIn, const BlockPos& pos, Explosion* explosionIn) override;
    void onBlockDestroyedByPlayer(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void explode(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* igniter);
    // Java: onBlockActivated lights the TNT with flint and steel or a fire charge -- the player comes with stage 8.
    // Java: onEntityCollidedWithBlock lights it when a burning arrow hits -- arrows come with stage 10.
    bool canDropFromExplosion(Explosion*) override { return false; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(EXPLODE, (meta & 1) > 0); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(EXPLODE) ? 1 : 0; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(EXPLODE, false));
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &EXPLODE }); }
};

class BlockTallGrass : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockTallGrass)

    enum class EnumType { DEAD_BUSH, GRASS, FERN };
    static constexpr std::string_view kTypeNames[3] { "dead_bush", "tall_grass", "fern" };
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 3 ? 0 : meta); }

    static const PropertyEnum<EnumType> TYPE;

    BlockTallGrass()
        : BlockBush(Material::vine)
    {
    }

    int getBlockColor() const override { return ColorizerGrass::getGrassColor(0.5, 1.0); }
    int getRenderColor(const IBlockState* state) const override
    {
        if (state->getBlock() != this)
            return BlockBush::getRenderColor(state);
        return state->getValue(TYPE) == EnumType::DEAD_BUSH ? 16777215 : ColorizerGrass::getGrassColor(0.5, 1.0);
    }
    int quantityDroppedWithBonus(int fortune, Random& random) const override { return 1 + random.nextInt(fortune * 2 + 1); }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(TYPE, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return static_cast<int>(state->getValue(TYPE)); }
    EnumOffsetType getOffsetType() const override { return EnumOffsetType::XYZ; }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, 1, 3); }
    Item* getItemDropped(const IBlockState*, Random& rand, int) const override;
    bool canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool isReplaceable(World*, const BlockPos&) override { return true; }
    int colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass) override;
    // Java: harvestBlock drops the grass itself for shears -- the player comes with stage 8.
    int getDamageValue(World* worldIn, const BlockPos& pos) override;
    // Java: IGrowable
    bool canGrow(World*, const BlockPos&, const IBlockState* state, bool) { return state->getValue(TYPE) != EnumType::DEAD_BUSH; }
    bool canUseBonemeal(World*, Random&, const BlockPos&, const IBlockState*) { return true; }
    void grow(World* worldIn, Random& rand, const BlockPos& pos, const IBlockState* state);

protected:
    void construct() override
    {
        BlockBush::construct();
        setDefaultState(blockState->getBaseState()->withProperty(TYPE, EnumType::DEAD_BUSH));
        const float f = 0.4f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.8f, 0.5f + f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &TYPE }); }
};

class BlockTrapDoor : public Block {
public:
    MC_BLOCK_CLASS(BlockTrapDoor)

    enum class DoorHalf { TOP, BOTTOM };
    static const PropertyDirection FACING;
    static const PropertyBool OPEN;
    static const PropertyEnum<DoorHalf> HALF;

    explicit BlockTrapDoor(const Material& materialIn)
        : Block(materialIn)
    {
    }

    static EnumFacing getFacing(int meta)
    {
        switch (meta & 3) {
        case 0: return EnumFacing::NORTH;
        case 1: return EnumFacing::SOUTH;
        case 2: return EnumFacing::WEST;
        default: return EnumFacing::EAST;
        }
    }
    static int getMetaForFacing(EnumFacing facing)
    {
        switch (facing) {
        case EnumFacing::NORTH: return 0;
        case EnumFacing::SOUTH: return 1;
        case EnumFacing::WEST: return 2;
        default: return 3;
        }
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool isPassable(IBlockAccess* worldIn, const BlockPos& pos) override;
    AxisAlignedBB getSelectedBoundingBox(World* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.40625f, 0.0f, 1.0f, 0.59375f, 1.0f); }
    void setBounds(const IBlockState* state);
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    std::optional<MovingObjectPosition> collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    bool canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side) override;
    static bool isValidSupportBlock(const Block* blockIn);
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getFacing(meta))->withProperty(OPEN, (meta & 4) != 0)->withProperty(HALF, (meta & 8) == 0 ? DoorHalf::BOTTOM : DoorHalf::TOP);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getMetaForFacing(state->getValue(FACING));
        if (state->getValue(OPEN))
            i |= 4;
        if (state->getValue(HALF) == DoorHalf::TOP)
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(OPEN, false)->withProperty(HALF, DoorHalf::BOTTOM));
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &OPEN, &HALF }); }
};

class BlockTripWire : public Block {
public:
    MC_BLOCK_CLASS(BlockTripWire)

    static const PropertyBool POWERED, SUSPENDED, ATTACHED, DISARMED, NORTH, EAST, SOUTH, WEST;

    BlockTripWire()
        : Block(Material::circuits)
    {
    }

    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }
    Item* getItem(World*, const BlockPos&) override { return Items::string; }
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    // Java: onBlockHarvested disarms the wire (flag 4) when the player holds shears -- the player comes with stage 8.
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn) override;
    void randomTick(World*, const BlockPos&, const IBlockState*, Random&) override { }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    static bool isConnectedTo(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing direction);
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(POWERED, (meta & 1) > 0)->withProperty(SUSPENDED, (meta & 2) > 0)->withProperty(ATTACHED, (meta & 4) > 0)->withProperty(DISARMED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        if (state->getValue(POWERED))
            i |= 1;
        if (state->getValue(SUSPENDED))
            i |= 2;
        if (state->getValue(ATTACHED))
            i |= 4;
        if (state->getValue(DISARMED))
            i |= 8;
        return i;
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::string; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(POWERED, false)->withProperty(SUSPENDED, false)->withProperty(ATTACHED, false)->withProperty(DISARMED, false)->withProperty(NORTH, false)->withProperty(EAST, false)->withProperty(SOUTH, false)->withProperty(WEST, false));
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.15625f, 1.0f);
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &POWERED, &SUSPENDED, &ATTACHED, &DISARMED, &NORTH, &EAST, &WEST, &SOUTH }); }

private:
    void notifyHook(World* worldIn, const BlockPos& pos, const IBlockState* state);
    void updateState(World* worldIn, const BlockPos& pos);
};

class BlockTripWireHook : public Block {
public:
    MC_BLOCK_CLASS(BlockTripWireHook)

    static const PropertyDirection FACING;
    static const PropertyBool POWERED, ATTACHED, SUSPENDED;

    BlockTripWireHook()
        : Block(Material::circuits)
    {
    }

    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    // Java: func_176260_a -- re-evaluates the hook, the wire and the opposite hook; p_176260_7_ may be null.
    void func_176260_a(World* worldIn, const BlockPos& pos, const IBlockState* hookState, bool p_176260_4_, bool p_176260_5_, int p_176260_6_,
        const IBlockState* p_176260_7_);
    void randomTick(World*, const BlockPos&, const IBlockState*, Random&) override { }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    int getWeakPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing) override { return state->getValue(POWERED) ? 15 : 0; }
    int getStrongPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing side) override
    {
        return !state->getValue(POWERED) ? 0 : (state->getValue(FACING) == side ? 15 : 0);
    }
    bool canProvidePower() const override { return true; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT_MIPPED; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getHorizontal(meta & 3))->withProperty(POWERED, (meta & 8) > 0)->withProperty(ATTACHED, (meta & 4) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        if (state->getValue(POWERED))
            i |= 8;
        if (state->getValue(ATTACHED))
            i |= 4;
        return i;
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(POWERED, false)->withProperty(ATTACHED, false)->withProperty(SUSPENDED, false));
        setCreativeTab(&CreativeTabs::tabRedstone);
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &POWERED, &ATTACHED, &SUSPENDED }); }

private:
    void func_180694_a(World* worldIn, const BlockPos& pos, bool p_180694_3_, bool p_180694_4_, bool p_180694_5_, bool p_180694_6_);
    void func_176262_b(World* worldIn, const BlockPos& pos, EnumFacing facing);
    bool checkForDrop(World* worldIn, const BlockPos& pos, const IBlockState* state);
};

class BlockVine : public Block {
public:
    bool canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side) override;
    static bool canPlaceOn(const Block* blockIn);
    MC_BLOCK_CLASS(BlockVine)

    static const PropertyBool UP, NORTH, EAST, SOUTH, WEST;
    static const std::array<const PropertyBool*, 5> ALL_FACES;

    BlockVine()
        : Block(Material::vine)
    {
    }

    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f); }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    int getBlockColor() const override { return ColorizerFoliage::getFoliageColorBasic(); }
    int getRenderColor(const IBlockState*) const override { return ColorizerFoliage::getFoliageColorBasic(); }
    int quantityDropped(Random&) const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(SOUTH, (meta & 1) > 0)->withProperty(WEST, (meta & 2) > 0)->withProperty(NORTH, (meta & 4) > 0)->withProperty(EAST, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        if (state->getValue(SOUTH))
            i |= 1;
        if (state->getValue(WEST))
            i |= 2;
        if (state->getValue(NORTH))
            i |= 4;
        if (state->getValue(EAST))
            i |= 8;
        return i;
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }
    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    bool isReplaceable(World*, const BlockPos&) override { return true; }
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    int colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    // Java: harvestBlock drops the vine itself for shears -- the player comes with stage 8.
    static const PropertyBool& getPropertyFor(EnumFacing side);
    static int getNumGrownFaces(const IBlockState* state);

private:
    bool recheckGrownSides(World* worldIn, const BlockPos& pos, const IBlockState* state);

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(UP, false)->withProperty(NORTH, false)->withProperty(EAST, false)->withProperty(SOUTH, false)->withProperty(WEST, false));
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &UP, &NORTH, &EAST, &SOUTH, &WEST }); }
};

class BlockWall : public Block {
public:
    MC_BLOCK_CLASS(BlockWall)

    enum class EnumType { NORMAL, MOSSY };
    struct EnumTypeInfo {
        std::string_view name;
        std::string_view unlocalizedName;
    };
    static constexpr EnumTypeInfo kTypes[2] { { "cobblestone", "normal" }, { "mossy_cobblestone", "mossy" } };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static const PropertyBool UP, NORTH, EAST, SOUTH, WEST;
    static const PropertyEnum<EnumType> VARIANT;
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 2 ? 0 : meta); }

    explicit BlockWall(Block* modelBlockIn)
        : Block(modelBlockIn->getMaterial())
        , modelBlock(modelBlockIn)
    {
    }

    bool isFullCube() const override { return false; }
    bool isPassable(IBlockAccess*, const BlockPos&) override { return false; }
    bool isOpaqueCube() const override { return false; }
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool canConnectTo(IBlockAccess* worldIn, const BlockPos& pos);
    int damageDropped(const IBlockState* state) const override { return static_cast<int>(state->getValue(VARIANT)); }
    bool shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side) override;
    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return static_cast<int>(state->getValue(VARIANT)); }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, 0, 2); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(UP, false)->withProperty(NORTH, false)->withProperty(EAST, false)->withProperty(SOUTH, false)->withProperty(WEST, false)->withProperty(VARIANT, EnumType::NORMAL));
        setHardness(modelBlock->getBlockHardnessValue());
        setResistance(modelBlock->getBlockResistanceValue() / 3.0f);
        setStepSound(*modelBlock->stepSound);
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &UP, &NORTH, &EAST, &WEST, &SOUTH, &VARIANT }); }

private:
    Block* modelBlock;
};

class BlockWeb : public Block {
public:
    MC_BLOCK_CLASS(BlockWeb)

    BlockWeb()
        : Block(Material::web)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canSilkHarvest() const override { return true; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::string; }
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn) override;

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
};

class BlockWorkbench : public Block {
public:
    MC_BLOCK_CLASS(BlockWorkbench)

    BlockWorkbench()
        : Block(Material::wood)
    {
    }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
};

class BlockStoneBrick : public Block {
public:
    MC_BLOCK_CLASS(BlockStoneBrick)

    enum class EnumType { DEFAULT, MOSSY, CRACKED, CHISELED };
    struct EnumTypeInfo {
        int meta;
        std::string_view name;
        std::string_view unlocalizedName;
    };
    static constexpr EnumTypeInfo kTypes[4] {
        { 0, "stonebrick", "default" }, { 1, "mossy_stonebrick", "mossy" }, { 2, "cracked_stonebrick", "cracked" }, { 3, "chiseled_stonebrick", "chiseled" },
    };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 4 ? 0 : meta); }

    static const PropertyEnum<EnumType> VARIANT;
    static constexpr int DEFAULT_META = 0;
    static constexpr int MOSSY_META = 1;
    static constexpr int CRACKED_META = 2;
    static constexpr int CHISELED_META = 3;

    BlockStoneBrick()
        : Block(Material::rock)
    {
    }

    int damageDropped(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, 0, 4); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, EnumType::DEFAULT));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT }); }
};

class BlockRedstoneLight : public Block {
public:
    MC_BLOCK_CLASS(BlockRedstoneLight)

    explicit BlockRedstoneLight(bool isOnIn)
        : Block(Material::redstoneLight)
        , isOn(isOnIn)
    {
    }
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(Blocks::redstone_lamp); }
    Item* getItem(World*, const BlockPos&) override { return Item::getItemFromBlock(Blocks::redstone_lamp); }
    ItemStack createStackedBlock(const IBlockState*) const override { return ItemStack(Blocks::redstone_lamp); }

protected:
    void construct() override
    {
        Block::construct();
        if (isOn)
            setLightLevel(1.0f);
    }

private:
    const bool isOn;
};

class BlockRedstoneOre : public Block {
public:
    MC_BLOCK_CLASS(BlockRedstoneOre)

    explicit BlockRedstoneOre(bool isOnIn)
        : Block(Material::rock)
        , isOn(isOnIn)
    {
    }

    int tickRate(World*) override { return 30; }
    void onBlockClicked(World* worldIn, const BlockPos& pos, EntityPlayer* playerIn) override;
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, Entity* entityIn) override;
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    int quantityDroppedWithBonus(int fortune, Random& random) const override
    {
        // Java evaluates left to right; C++ leaves the order of + unspecified.
        const int base = quantityDropped(random);
        return base + random.nextInt(fortune + 1);
    }
    int quantityDropped(Random& random) const override { return 4 + random.nextInt(2); }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::redstone; }
    ItemStack createStackedBlock(const IBlockState*) const override { return ItemStack(Blocks::redstone_ore); }

protected:
    void construct() override
    {
        Block::construct();
        if (isOn)
            setTickRandomly(true);
    }

private:
    void activate(World* worldIn, const BlockPos& pos);
    void spawnParticles(World* worldIn, const BlockPos& pos);

    const bool isOn;
};

class BlockSeaLantern : public Block {
public:
    MC_BLOCK_CLASS(BlockSeaLantern)

    explicit BlockSeaLantern(const Material& materialIn)
        : Block(materialIn)
    {
    }

    int quantityDropped(Random& random) const override { return 2 + random.nextInt(2); }
    int quantityDroppedWithBonus(int fortune, Random& random) const override
    {
        const int base = quantityDropped(random);
        return MathHelper::clamp_int(base + random.nextInt(fortune + 1), 1, 5);
    }
    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::quartzColor; }
    bool canSilkHarvest() const override { return true; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::prismarine_crystals; }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

MC_ENUM_TRAITS(BlockTallGrass::EnumType, 3, BlockTallGrass::kTypeNames[static_cast<int>(value)])
MC_ENUM_TRAITS(BlockTrapDoor::DoorHalf, 2, value == BlockTrapDoor::DoorHalf::TOP ? "top" : "bottom")
MC_ENUM_TRAITS(BlockWall::EnumType, 2, BlockWall::info(value).name)
MC_ENUM_TRAITS(BlockStoneBrick::EnumType, 4, BlockStoneBrick::info(value).name)

inline const PropertyBool BlockTNT::EXPLODE("explode");
inline const PropertyEnum<BlockTallGrass::EnumType> BlockTallGrass::TYPE("type");
inline const PropertyDirection BlockTrapDoor::FACING("facing", isHorizontalFacing);
inline const PropertyBool BlockTrapDoor::OPEN("open");
inline const PropertyEnum<BlockTrapDoor::DoorHalf> BlockTrapDoor::HALF("half");
inline const PropertyBool BlockTripWire::POWERED("powered");
inline const PropertyBool BlockTripWire::SUSPENDED("suspended");
inline const PropertyBool BlockTripWire::ATTACHED("attached");
inline const PropertyBool BlockTripWire::DISARMED("disarmed");
inline const PropertyBool BlockTripWire::NORTH("north");
inline const PropertyBool BlockTripWire::EAST("east");
inline const PropertyBool BlockTripWire::SOUTH("south");
inline const PropertyBool BlockTripWire::WEST("west");
inline const PropertyDirection BlockTripWireHook::FACING("facing", isHorizontalFacing);
inline const PropertyBool BlockTripWireHook::POWERED("powered");
inline const PropertyBool BlockTripWireHook::ATTACHED("attached");
inline const PropertyBool BlockTripWireHook::SUSPENDED("suspended");
inline const PropertyBool BlockVine::UP("up");
inline const PropertyBool BlockVine::NORTH("north");
inline const PropertyBool BlockVine::EAST("east");
inline const PropertyBool BlockVine::SOUTH("south");
inline const PropertyBool BlockVine::WEST("west");
inline const std::array<const PropertyBool*, 5> BlockVine::ALL_FACES { &UP, &NORTH, &SOUTH, &WEST, &EAST };
inline const PropertyBool BlockWall::UP("up");
inline const PropertyBool BlockWall::NORTH("north");
inline const PropertyBool BlockWall::EAST("east");
inline const PropertyBool BlockWall::SOUTH("south");
inline const PropertyBool BlockWall::WEST("west");
inline const PropertyEnum<BlockWall::EnumType> BlockWall::VARIANT("variant");
inline const PropertyEnum<BlockStoneBrick::EnumType> BlockStoneBrick::VARIANT("variant");

} // namespace mc
