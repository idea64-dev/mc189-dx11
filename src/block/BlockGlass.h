#pragma once

// Original: net/minecraft/block/BlockGlass.java, BlockStainedGlass.java, BlockIce.java, BlockPortal.java and
// BlockSlime.java -- the BlockBreakable subclasses.

#include "block/BlockBreakable.h"
#include "item/EnumDyeColor.h"

namespace mc {

class BlockGlass : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockGlass)

    BlockGlass(const Material& materialIn, bool ignoreSimilarityIn)
        : BlockBreakable(materialIn, ignoreSimilarityIn)
    {
    }

    int quantityDropped(Random&) const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    bool isFullCube() const override { return false; }
    bool canSilkHarvest() const override { return true; }

protected:
    void construct() override
    {
        BlockBreakable::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockStainedGlass : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockStainedGlass)

    static const PropertyEnum<EnumDyeColor> COLOR;

    explicit BlockStainedGlass(const Material& materialIn)
        : BlockBreakable(materialIn, false)
    {
    }

    int damageDropped(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }
    const MapColor& getMapColor(const IBlockState* state) const override { return *info(state->getValue(COLOR)).mapColor; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }
    int quantityDropped(Random&) const override { return 0; }
    bool canSilkHarvest() const override { return true; }
    bool isFullCube() const override { return false; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(COLOR, dyeColorByMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, 0, 16); }
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;

protected:
    void construct() override
    {
        BlockBreakable::construct();
        setDefaultState(blockState->getBaseState()->withProperty(COLOR, EnumDyeColor::WHITE));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &COLOR }); }
};

class BlockIce : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockIce)

    BlockIce()
        : BlockBreakable(Material::ice, false)
    {
    }

    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }
    int quantityDropped(Random&) const override { return 0; }
    int getMobilityFlag() const override { return 0; }
    void harvestBlock(World* worldIn, EntityPlayer* player, const BlockPos& pos, const IBlockState* state, TileEntity* te) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;

protected:
    void construct() override
    {
        BlockBreakable::construct();
        slipperiness = 0.98f;
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockPortal : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockPortal)

    static const PropertyEnum<Axis> AXIS;

    BlockPortal()
        : BlockBreakable(Material::portal, false)
    {
    }

    static int getMetaForAxis(Axis axis) { return axis == Axis::X ? 1 : (axis == Axis::Z ? 2 : 0); }

    bool isFullCube() const override { return false; }
    int quantityDropped(Random&) const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(AXIS, (meta & 3) == 2 ? Axis::Z : Axis::X); }
    int getMetaFromState(const IBlockState* state) const override { return getMetaForAxis(state->getValue(AXIS)); }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    // Java: func_176548_d -- lights a portal in an obsidian frame around pos; false when there is none.
    bool func_176548_d(World* worldIn, const BlockPos& pos);
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    bool shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side) override;
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn) override;
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    Item* getItem(World*, const BlockPos&) override { return nullptr; }
    // Java: func_181089_f -- the BlockPattern helper for teleporting; comes with the dimension travel (stage 10).

    // Java: BlockPortal.Size -- the frame around a position along an axis.
    class Size {
    public:
        Size(World* worldIn, BlockPos pos, Axis axisIn);
        int func_181100_a() const { return field_150862_g; }
        int func_181101_b() const { return field_150868_h; }
        bool func_150860_b() const;
        void func_150859_c();

        int field_150864_e = 0;
        int field_150862_g = 0;
        int field_150868_h = 0;

    private:
        int func_180120_a(const BlockPos& pos, EnumFacing facing);
        int func_150858_a();
        static bool func_150857_a(const Block* block);

        World* world;
        Axis axis;
        EnumFacing field_150866_c;
        EnumFacing field_150863_d;
        std::optional<BlockPos> field_150861_f;
    };

protected:
    void construct() override
    {
        BlockBreakable::construct();
        setDefaultState(blockState->getBaseState()->withProperty(AXIS, Axis::X));
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AXIS }); }
};

class BlockSlime : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockSlime)

    BlockSlime()
        : BlockBreakable(Material::clay, false, MapColor::grassColor)
    {
    }

    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }
    void onFallenUpon(World* worldIn, const BlockPos& pos, Entity* entityIn, float fallDistance) override;
    void onLanded(World* worldIn, Entity* entityIn) override;
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, Entity* entityIn) override;

protected:
    void construct() override
    {
        BlockBreakable::construct();
        setCreativeTab(&CreativeTabs::tabDecorations);
        slipperiness = 0.8f;
    }
};

inline const PropertyEnum<EnumDyeColor> BlockStainedGlass::COLOR("color");
inline const PropertyEnum<Axis> BlockPortal::AXIS("axis", { Axis::X, Axis::Z });

} // namespace mc
