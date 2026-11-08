#pragma once

// Original: net/minecraft/block/BlockPistonBase.java, BlockPistonExtension.java and BlockPistonMoving.java

#include "block/BlockContainer.h"

#include <optional>
#include <stdexcept>
#include <vector>

namespace mc {

class TileEntityPiston;

class BlockPistonBase : public Block {
public:
    MC_BLOCK_CLASS(BlockPistonBase)

    static const PropertyDirection FACING;
    static const PropertyBool EXTENDED;

    explicit BlockPistonBase(bool isStickyIn)
        : Block(Material::piston)
        , isStickyArg(isStickyIn)
    {
    }

    // Java returns null for 6 and 7.
    static std::optional<EnumFacing> getFacing(int meta)
    {
        const int i = meta & 7;
        return i > 5 ? std::nullopt : std::optional<EnumFacing>(getFront(i));
    }

    bool isOpaqueCube() const override { return false; }
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    bool onBlockEventReceived(World* worldIn, const BlockPos& pos, const IBlockState* state, int eventID, int eventParam) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f); }
    void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool isFullCube() const override { return false; }
    static EnumFacing getFacingFromEntity(World* worldIn, const BlockPos& clickedBlock, EntityLivingBase* entityIn);
    static bool canPush(Block* blockIn, World* worldIn, const BlockPos& pos, EnumFacing direction, bool allowDestroy);
    const IBlockState* getStateForEntityRender(const IBlockState*) const override { return getDefaultState()->withProperty(FACING, EnumFacing::UP); }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return withFacing(getDefaultState(), getFacing(meta))->withProperty(EXTENDED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getIndex(state->getValue(FACING));
        if (state->getValue(EXTENDED))
            i |= 8;
        return i;
    }

    // withProperty(FACING, null) throws IllegalArgumentException in Java.
    static const IBlockState* withFacing(const IBlockState* state, std::optional<EnumFacing> facing)
    {
        if (!facing)
            throw std::invalid_argument("Cannot set property facing to null");
        return state->withProperty(FACING, *facing);
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(EXTENDED, false));
        isSticky = isStickyArg;
        setStepSound(soundTypePiston);
        setHardness(0.5f);
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &EXTENDED }); }

private:
    void checkForMove(World* worldIn, const BlockPos& pos, const IBlockState* state);
    bool shouldBeExtended(World* worldIn, const BlockPos& pos, EnumFacing facing);
    bool doMove(World* worldIn, const BlockPos& pos, EnumFacing direction, bool extending);

    bool isSticky = false;
    bool isStickyArg;
};

class BlockPistonExtension : public Block {
public:
    MC_BLOCK_CLASS(BlockPistonExtension)

    enum class EnumPistonType { DEFAULT, STICKY };
    static const PropertyDirection FACING;
    static const PropertyEnum<EnumPistonType> TYPE;
    static const PropertyBool SHORT;

    BlockPistonExtension()
        : Block(Material::piston)
    {
    }

    static std::optional<EnumFacing> getFacing(int meta) { return BlockPistonBase::getFacing(meta); }

    // Java: onBlockHarvested removes the piston behind the head for creative players -- the player comes with stage 8.
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canPlaceBlockAt(World*, const BlockPos&) override { return false; }
    bool canPlaceBlockOnSide(World*, const BlockPos&, EnumFacing) override { return false; }
    int quantityDropped(Random&) const override { return 0; }
    void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void applyHeadBounds(const IBlockState* state);
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    bool shouldSideBeRendered(IBlockAccess*, const BlockPos&, EnumFacing) override { return true; }
    Item* getItem(World* worldIn, const BlockPos& pos) override;
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const auto facing = getFacing(meta);
        if (!facing)
            throw std::invalid_argument("Cannot set property facing to null");
        return getDefaultState()->withProperty(FACING, *facing)->withProperty(TYPE, (meta & 8) > 0 ? EnumPistonType::STICKY : EnumPistonType::DEFAULT);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getIndex(state->getValue(FACING));
        if (state->getValue(TYPE) == EnumPistonType::STICKY)
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(TYPE, EnumPistonType::DEFAULT)->withProperty(SHORT, false));
        setStepSound(soundTypePiston);
        setHardness(0.5f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &TYPE, &SHORT }); }

private:
    void applyCoreBounds(const IBlockState* state);
};

class BlockPistonMoving : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockPistonMoving)

    static inline const PropertyDirection& FACING = BlockPistonExtension::FACING;
    static inline const PropertyEnum<BlockPistonExtension::EnumPistonType>& TYPE = BlockPistonExtension::TYPE;

    BlockPistonMoving()
        : BlockContainer(Material::piston)
    {
    }

    std::shared_ptr<TileEntity> createNewTileEntity(World*, int) override { return nullptr; }
    static std::shared_ptr<TileEntity> newTileEntity(const IBlockState* state, EnumFacing facing, bool extending, bool renderHead);
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool canPlaceBlockAt(World*, const BlockPos&) override { return false; }
    bool canPlaceBlockOnSide(World*, const BlockPos&, EnumFacing) override { return false; }
    void onBlockDestroyedByPlayer(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    std::optional<MovingObjectPosition> collisionRayTrace(World*, const BlockPos&, const Vec3&, const Vec3&) override { return std::nullopt; }
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* extendingBlock, float progress, EnumFacing direction);
    Item* getItem(World*, const BlockPos&) override { return nullptr; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const auto facing = BlockPistonExtension::getFacing(meta);
        if (!facing)
            throw std::invalid_argument("Cannot set property facing to null");
        return getDefaultState()->withProperty(FACING, *facing)->withProperty(TYPE, (meta & 8) > 0 ? BlockPistonExtension::EnumPistonType::STICKY : BlockPistonExtension::EnumPistonType::DEFAULT);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getIndex(state->getValue(FACING));
        if (state->getValue(TYPE) == BlockPistonExtension::EnumPistonType::STICKY)
            i |= 8;
        return i;
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(TYPE, BlockPistonExtension::EnumPistonType::DEFAULT));
        setHardness(-1.0f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &TYPE }); }

private:
    static TileEntityPiston* getTileEntity(IBlockAccess* worldIn, const BlockPos& pos);
};

MC_ENUM_TRAITS(BlockPistonExtension::EnumPistonType, 2, value == BlockPistonExtension::EnumPistonType::DEFAULT ? "normal" : "sticky")

inline const PropertyDirection BlockPistonBase::FACING("facing");
inline const PropertyBool BlockPistonBase::EXTENDED("extended");
inline const PropertyDirection BlockPistonExtension::FACING("facing");
inline const PropertyEnum<BlockPistonExtension::EnumPistonType> BlockPistonExtension::TYPE("type");
inline const PropertyBool BlockPistonExtension::SHORT("short");

} // namespace mc
