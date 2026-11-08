// The world callbacks of the tile entity blocks: BlockSkull.java, BlockSign.java, BlockStandingSign.java,
// BlockWallSign.java, BlockBanner.java, BlockFlowerPot.java, BlockCauldron.java, BlockBrewingStand.java,
// BlockHopper.java, BlockChest.java, BlockEnderChest.java, BlockFurnace.java, BlockDispenser.java,
// BlockDropper.java, BlockJukebox.java, BlockBeacon.java, BlockCommandBlock.java and BlockEnchantmentTable.java.

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"
#include "entity/Entity.h"
#include "init/Items.h"
#include "inventory/Container.h"
#include "item/ItemBlock.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "tileentity/InventoryLargeChest.h"
#include "tileentity/TileEntities.h"
#include "tileentity/TileEntityBanner.h"
#include "tileentity/TileEntityBeacon.h"
#include "tileentity/TileEntityBrewingStand.h"
#include "tileentity/TileEntityHopper.h"
#include "tileentity/TileEntityJukebox.h"
#include "tileentity/TileEntitySign.h"
#include "tileentity/TileEntitySkull.h"
#include "util/MathHelper.h"
#include "world/GameRules.h"

#include <memory>

namespace mc {

namespace {

// Java: InventoryHelper.dropInventoryItems -- spawns the contents as EntityItems with InventoryHelper's own
// Random (no world draws); item entities come with stage 10.
void dropInventoryItems(World*, const BlockPos&, IInventory*)
{
}

// Java: BehaviorDefaultDispenseItem.dispense -- drops one item of the stack in front of the dispenser.
std::shared_ptr<ItemStack> dispenseDefault(World* worldIn, const BlockPos& pos, EnumFacing enumfacing, std::shared_ptr<ItemStack> stack)
{
    // Java: dispenseStack -- splitStack(1) and spawnItem: an EntityItem at the dispense position (stage 10) with
    // rand.nextDouble() and three rand.nextGaussian() for its motion; the draws stay in step.
    [[maybe_unused]] ItemStack itemstack = stack->splitStack(1);
    worldIn->rand.nextDouble();
    worldIn->rand.nextGaussian();
    worldIn->rand.nextGaussian();
    worldIn->rand.nextGaussian();
    // Java: playDispenseSound and spawnDispenseParticles.
    worldIn->playAuxSFX(1000, pos, 0);
    worldIn->playAuxSFX(2000, pos, getFrontOffsetX(enumfacing) + 1 + (getFrontOffsetZ(enumfacing) + 1) * 3);
    return stack;
}

} // namespace

// --- BlockSkull ---

void BlockSkull::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    switch (worldIn->getBlockState(pos)->getValue(FACING)) {
    case EnumFacing::NORTH: setBlockBounds(0.25f, 0.25f, 0.5f, 0.75f, 0.75f, 1.0f); break;
    case EnumFacing::SOUTH: setBlockBounds(0.25f, 0.25f, 0.0f, 0.75f, 0.75f, 0.5f); break;
    case EnumFacing::WEST: setBlockBounds(0.5f, 0.25f, 0.25f, 1.0f, 0.75f, 0.75f); break;
    case EnumFacing::EAST: setBlockBounds(0.0f, 0.25f, 0.25f, 0.5f, 0.75f, 0.75f); break;
    default: setBlockBounds(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f); break;
    }
}

std::optional<AxisAlignedBB> BlockSkull::getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return BlockContainer::getCollisionBoundingBox(worldIn, pos, state);
}

const IBlockState* BlockSkull::onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, placerHorizontalFacing(placer))->withProperty(NODROP, false);
}

std::shared_ptr<TileEntity> BlockSkull::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntitySkull>();
}

int BlockSkull::getDamageValue(World* worldIn, const BlockPos& pos)
{
    const auto* tileentity = dynamic_cast<TileEntitySkull*>(worldIn->getTileEntity(pos));
    return tileentity ? tileentity->getSkullType() : BlockContainer::getDamageValue(worldIn, pos);
}

void BlockSkull::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!worldIn->isRemote) {
        if (!state->getValue(NODROP)) {
            if (auto* tileentityskull = dynamic_cast<TileEntitySkull*>(worldIn->getTileEntity(pos))) {
                ItemStack itemstack(Items::skull, 1, getDamageValue(worldIn, pos));
                if (tileentityskull->getSkullType() == 3 && tileentityskull->getPlayerProfile()) {
                    itemstack.setTagCompound(std::make_shared<NBTTagCompound>());
                    auto nbttagcompound = std::make_shared<NBTTagCompound>();
                    TileEntitySkull::writeGameProfile(*nbttagcompound, *tileentityskull->getPlayerProfile());
                    itemstack.getTagCompound()->setTag("SkullOwner", nbttagcompound);
                }
                spawnAsEntity(worldIn, pos, std::move(itemstack));
            }
        }
        BlockContainer::breakBlock(worldIn, pos, state);
    }
}

bool BlockSkull::canDispenserPlace(World*, const BlockPos&, const ItemStack&)
{
    // Java: a wither skull on the soul sand T of getWitherBasePattern -- stage 10.
    return false;
}

void BlockSkull::checkWitherSpawn(World*, const BlockPos&, TileEntitySkull*)
{
}

// --- BlockSign, BlockStandingSign, BlockWallSign ---

AxisAlignedBB BlockSign::getSelectedBoundingBox(World* worldIn, const BlockPos& pos)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return BlockContainer::getSelectedBoundingBox(worldIn, pos);
}

std::shared_ptr<TileEntity> BlockSign::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntitySign>();
}

bool BlockSign::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState*, EntityPlayer* playerIn, EnumFacing, float, float, float)
{
    if (worldIn->isRemote)
        return true;
    auto* tileentity = dynamic_cast<TileEntitySign*>(worldIn->getTileEntity(pos));
    return tileentity ? tileentity->executeCommand(playerIn) : false;
}

bool BlockSign::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return !hasInvalidNeighbor(worldIn, pos) && BlockContainer::canPlaceBlockAt(worldIn, pos);
}

void BlockStandingSign::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    if (!worldIn->getBlockState(pos.down())->getBlock()->getMaterial().isSolid()) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
    BlockSign::onNeighborBlockChange(worldIn, pos, state, neighborBlock);
}

void BlockWallSign::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const EnumFacing enumfacing = worldIn->getBlockState(pos)->getValue(FACING);
    const float f = 0.28125f;
    const float f1 = 0.78125f;
    const float f2 = 0.0f;
    const float f3 = 1.0f;
    const float f4 = 0.125f;
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    switch (enumfacing) {
    case EnumFacing::NORTH: setBlockBounds(f2, f, 1.0f - f4, f3, f1, 1.0f); break;
    case EnumFacing::SOUTH: setBlockBounds(f2, f, 0.0f, f3, f1, f4); break;
    case EnumFacing::WEST: setBlockBounds(1.0f - f4, f, f2, 1.0f, f1, f3); break;
    case EnumFacing::EAST: setBlockBounds(0.0f, f, f2, f4, f1, f3); break;
    default: break;
    }
}

void BlockWallSign::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    if (!worldIn->getBlockState(pos.offset(getOpposite(enumfacing)))->getBlock()->getMaterial().isSolid()) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
    BlockSign::onNeighborBlockChange(worldIn, pos, state, neighborBlock);
}

// --- BlockBanner ---

AxisAlignedBB BlockBanner::getSelectedBoundingBox(World* worldIn, const BlockPos& pos)
{
    setBlockBoundsBasedOnState(worldIn, pos);
    return BlockContainer::getSelectedBoundingBox(worldIn, pos);
}

std::shared_ptr<TileEntity> BlockBanner::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityBanner>();
}

void BlockBanner::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune)
{
    TileEntity* tileentity = worldIn->getTileEntity(pos);
    if (auto* banner = dynamic_cast<TileEntityBanner*>(tileentity)) {
        ItemStack itemstack(Items::banner, 1, banner->getBaseColor());
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        tileentity->writeToNBT(*nbttagcompound);
        nbttagcompound->removeTag("x");
        nbttagcompound->removeTag("y");
        nbttagcompound->removeTag("z");
        nbttagcompound->removeTag("id");
        itemstack.setTagInfo("BlockEntityTag", nbttagcompound);
        spawnAsEntity(worldIn, pos, std::move(itemstack));
    } else {
        BlockContainer::dropBlockAsItemWithChance(worldIn, pos, state, chance, fortune);
    }
}

bool BlockBanner::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return !hasInvalidNeighbor(worldIn, pos) && BlockContainer::canPlaceBlockAt(worldIn, pos);
}

void BlockBanner::harvestBlock(World* worldIn, EntityPlayer* player, const BlockPos& pos, const IBlockState* state, TileEntity* te)
{
    if (auto* tileentitybanner = dynamic_cast<TileEntityBanner*>(te)) {
        ItemStack itemstack(Items::banner, 1, tileentitybanner->getBaseColor());
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        TileEntityBanner::setBaseColorAndPatterns(*nbttagcompound, tileentitybanner->getBaseColor(), tileentitybanner->getPatterns());
        itemstack.setTagInfo("BlockEntityTag", nbttagcompound);
        spawnAsEntity(worldIn, pos, std::move(itemstack));
    } else {
        BlockContainer::harvestBlock(worldIn, player, pos, state, nullptr);
    }
}

void BlockBanner::BlockBannerHanging::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    const EnumFacing enumfacing = worldIn->getBlockState(pos)->getValue(FACING);
    const float f = 0.0f;
    const float f1 = 0.78125f;
    const float f2 = 0.0f;
    const float f3 = 1.0f;
    const float f4 = 0.125f;
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    switch (enumfacing) {
    case EnumFacing::SOUTH: setBlockBounds(f2, f, 0.0f, f3, f1, f4); break;
    case EnumFacing::WEST: setBlockBounds(1.0f - f4, f, f2, 1.0f, f1, f3); break;
    case EnumFacing::EAST: setBlockBounds(0.0f, f, f2, f4, f1, f3); break;
    default: setBlockBounds(f2, f, 1.0f - f4, f3, f1, 1.0f); break;
    }
}

void BlockBanner::BlockBannerHanging::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    const EnumFacing enumfacing = state->getValue(FACING);
    if (!worldIn->getBlockState(pos.offset(getOpposite(enumfacing)))->getBlock()->getMaterial().isSolid()) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
    BlockBanner::onNeighborBlockChange(worldIn, pos, state, neighborBlock);
}

void BlockBanner::BlockBannerStanding::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    if (!worldIn->getBlockState(pos.down())->getBlock()->getMaterial().isSolid()) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
    BlockBanner::onNeighborBlockChange(worldIn, pos, state, neighborBlock);
}

// --- BlockFlowerPot ---

int BlockFlowerPot::colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass)
{
    if (const auto* tileentity = dynamic_cast<TileEntityFlowerPot*>(worldIn->getTileEntity(pos))) {
        Item* item = tileentity->getFlowerPotItem();
        if (dynamic_cast<ItemBlock*>(item))
            return Block::getBlockFromItem(item)->colorMultiplier(worldIn, pos, renderPass);
    }
    return 16777215;
}

bool BlockFlowerPot::canNotContain(const Block* blockIn, int meta)
{
    return blockIn != Blocks::yellow_flower && blockIn != Blocks::red_flower && blockIn != Blocks::cactus && blockIn != Blocks::brown_mushroom
            && blockIn != Blocks::red_mushroom && blockIn != Blocks::sapling && blockIn != Blocks::deadbush
        ? blockIn == Blocks::tallgrass && meta == static_cast<int>(BlockTallGrass::EnumType::FERN)
        : true;
}

Item* BlockFlowerPot::getItem(World* worldIn, const BlockPos& pos)
{
    TileEntityFlowerPot* tileentityflowerpot = getTileEntity(worldIn, pos);
    return tileentityflowerpot && tileentityflowerpot->getFlowerPotItem() ? tileentityflowerpot->getFlowerPotItem() : Items::flower_pot;
}

int BlockFlowerPot::getDamageValue(World* worldIn, const BlockPos& pos)
{
    TileEntityFlowerPot* tileentityflowerpot = getTileEntity(worldIn, pos);
    return tileentityflowerpot && tileentityflowerpot->getFlowerPotItem() ? tileentityflowerpot->getFlowerPotData() : 0;
}

bool BlockFlowerPot::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    return BlockContainer::canPlaceBlockAt(worldIn, pos) && World::doesBlockHaveSolidTopSurface(worldIn, pos.down());
}

void BlockFlowerPot::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (!World::doesBlockHaveSolidTopSurface(worldIn, pos.down())) {
        dropBlockAsItem(worldIn, pos, state, 0);
        worldIn->setBlockToAir(pos);
    }
}

void BlockFlowerPot::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    TileEntityFlowerPot* tileentityflowerpot = getTileEntity(worldIn, pos);
    if (tileentityflowerpot && tileentityflowerpot->getFlowerPotItem())
        spawnAsEntity(worldIn, pos, ItemStack(tileentityflowerpot->getFlowerPotItem(), 1, tileentityflowerpot->getFlowerPotData()));
    BlockContainer::breakBlock(worldIn, pos, state);
}

TileEntityFlowerPot* BlockFlowerPot::getTileEntity(World* worldIn, const BlockPos& pos)
{
    return dynamic_cast<TileEntityFlowerPot*>(worldIn->getTileEntity(pos));
}

std::shared_ptr<TileEntity> BlockFlowerPot::createNewTileEntity(World*, int meta)
{
    Block* block = nullptr;
    int i = 0;
    switch (meta) {
    case 1:
        block = Blocks::red_flower;
        i = BlockFlower::info(BlockFlower::EnumFlowerType::POPPY).meta;
        break;
    case 2: block = Blocks::yellow_flower; break;
    case 3:
        block = Blocks::sapling;
        i = BlockPlanks::getMetadata(BlockPlanks::EnumType::OAK);
        break;
    case 4:
        block = Blocks::sapling;
        i = BlockPlanks::getMetadata(BlockPlanks::EnumType::SPRUCE);
        break;
    case 5:
        block = Blocks::sapling;
        i = BlockPlanks::getMetadata(BlockPlanks::EnumType::BIRCH);
        break;
    case 6:
        block = Blocks::sapling;
        i = BlockPlanks::getMetadata(BlockPlanks::EnumType::JUNGLE);
        break;
    case 7: block = Blocks::red_mushroom; break;
    case 8: block = Blocks::brown_mushroom; break;
    case 9: block = Blocks::cactus; break;
    case 10: block = Blocks::deadbush; break;
    case 11:
        block = Blocks::tallgrass;
        i = static_cast<int>(BlockTallGrass::EnumType::FERN);
        break;
    case 12:
        block = Blocks::sapling;
        i = BlockPlanks::getMetadata(BlockPlanks::EnumType::ACACIA);
        break;
    case 13:
        block = Blocks::sapling;
        i = BlockPlanks::getMetadata(BlockPlanks::EnumType::DARK_OAK);
        break;
    default: break;
    }
    return std::make_shared<TileEntityFlowerPot>(block ? Item::getItemFromBlock(block) : nullptr, i);
}

const IBlockState* BlockFlowerPot::getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos)
{
    using T = EnumFlowerType;
    T type = T::EMPTY;
    if (const auto* tileentityflowerpot = dynamic_cast<TileEntityFlowerPot*>(worldIn->getTileEntity(pos))) {
        Item* item = tileentityflowerpot->getFlowerPotItem();
        if (dynamic_cast<ItemBlock*>(item)) {
            const int i = tileentityflowerpot->getFlowerPotData();
            const Block* block = Block::getBlockFromItem(item);
            if (block == Blocks::sapling) {
                switch (BlockPlanks::byMetadata(i)) {
                case BlockPlanks::EnumType::OAK: type = T::OAK_SAPLING; break;
                case BlockPlanks::EnumType::SPRUCE: type = T::SPRUCE_SAPLING; break;
                case BlockPlanks::EnumType::BIRCH: type = T::BIRCH_SAPLING; break;
                case BlockPlanks::EnumType::JUNGLE: type = T::JUNGLE_SAPLING; break;
                case BlockPlanks::EnumType::ACACIA: type = T::ACACIA_SAPLING; break;
                case BlockPlanks::EnumType::DARK_OAK: type = T::DARK_OAK_SAPLING; break;
                default: type = T::EMPTY; break;
                }
            } else if (block == Blocks::tallgrass) {
                switch (i) {
                case 0: type = T::DEAD_BUSH; break;
                case 2: type = T::FERN; break;
                default: type = T::EMPTY; break;
                }
            } else if (block == Blocks::yellow_flower) {
                type = T::DANDELION;
            } else if (block == Blocks::red_flower) {
                switch (BlockFlower::getType(BlockFlower::EnumFlowerColor::RED, i)) {
                case BlockFlower::EnumFlowerType::POPPY: type = T::POPPY; break;
                case BlockFlower::EnumFlowerType::BLUE_ORCHID: type = T::BLUE_ORCHID; break;
                case BlockFlower::EnumFlowerType::ALLIUM: type = T::ALLIUM; break;
                case BlockFlower::EnumFlowerType::HOUSTONIA: type = T::HOUSTONIA; break;
                case BlockFlower::EnumFlowerType::RED_TULIP: type = T::RED_TULIP; break;
                case BlockFlower::EnumFlowerType::ORANGE_TULIP: type = T::ORANGE_TULIP; break;
                case BlockFlower::EnumFlowerType::WHITE_TULIP: type = T::WHITE_TULIP; break;
                case BlockFlower::EnumFlowerType::PINK_TULIP: type = T::PINK_TULIP; break;
                case BlockFlower::EnumFlowerType::OXEYE_DAISY: type = T::OXEYE_DAISY; break;
                default: type = T::EMPTY; break;
                }
            } else if (block == Blocks::red_mushroom) {
                type = T::MUSHROOM_RED;
            } else if (block == Blocks::brown_mushroom) {
                type = T::MUSHROOM_BROWN;
            } else if (block == Blocks::deadbush) {
                type = T::DEAD_BUSH;
            } else if (block == Blocks::cactus) {
                type = T::CACTUS;
            }
        }
    }
    return state->withProperty(CONTENTS, type);
}

// --- BlockCauldron ---

void BlockCauldron::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.3125f, 1.0f);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    const float f = 0.125f;
    setBlockBounds(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBounds(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBounds(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
    Block::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBoundsForItemRender();
}

void BlockCauldron::onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn)
{
    const int i = state->getValue(LEVEL);
    const float f = static_cast<float>(pos.getY()) + (6.0f + static_cast<float>(3 * i)) / 16.0f;
    if (!worldIn->isRemote && entityIn->isBurning() && i > 0 && entityIn->getEntityBoundingBox().minY <= f) {
        entityIn->extinguish();
        setWaterLevel(worldIn, pos, state, i - 1);
    }
}

void BlockCauldron::setWaterLevel(World* worldIn, const BlockPos& pos, const IBlockState* state, int level)
{
    worldIn->setBlockState(pos, state->withProperty(LEVEL, MathHelper::clamp_int(level, 0, 3)), 2);
    worldIn->updateComparatorOutputLevel(pos, this);
}

void BlockCauldron::fillWithRain(World* worldIn, const BlockPos& pos)
{
    if (worldIn->rand.nextInt(20) == 1) {
        const IBlockState* iblockstate = worldIn->getBlockState(pos);
        if (iblockstate->getValue(LEVEL) < 3)
            worldIn->setBlockState(pos, iblockstate->cycleProperty(LEVEL), 2);
    }
}

int BlockCauldron::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos)->getValue(LEVEL);
}

// --- BlockBrewingStand ---

std::shared_ptr<TileEntity> BlockBrewingStand::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityBrewingStand>();
}

void BlockBrewingStand::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    setBlockBounds(0.4375f, 0.0f, 0.4375f, 0.5625f, 0.875f, 0.5625f);
    BlockContainer::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBoundsForItemRender();
    BlockContainer::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
}

void BlockBrewingStand::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState*, EntityLivingBase*, const ItemStack& stack)
{
    if (stack.hasDisplayName()) {
        if (auto* tileentity = dynamic_cast<TileEntityBrewingStand*>(worldIn->getTileEntity(pos)))
            tileentity->setName(stack.getDisplayName());
    }
}

void BlockBrewingStand::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random& rand)
{
    const double d0 = static_cast<float>(pos.getX()) + 0.4f + rand.nextFloat() * 0.2f;
    const double d1 = static_cast<float>(pos.getY()) + 0.7f + rand.nextFloat() * 0.3f;
    const double d2 = static_cast<float>(pos.getZ()) + 0.4f + rand.nextFloat() * 0.2f;
    spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0, d1, d2, 0.0, 0.0, 0.0);
}

void BlockBrewingStand::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (auto* tileentity = dynamic_cast<TileEntityBrewingStand*>(worldIn->getTileEntity(pos)))
        dropInventoryItems(worldIn, pos, tileentity);
    BlockContainer::breakBlock(worldIn, pos, state);
}

int BlockBrewingStand::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    return Container::calcRedstone(worldIn->getTileEntity(pos));
}

// --- BlockHopper ---

void BlockHopper::setBlockBoundsBasedOnState(IBlockAccess*, const BlockPos&)
{
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void BlockHopper::addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
    std::vector<AxisAlignedBB>& list, Entity* collidingEntity)
{
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.625f, 1.0f);
    BlockContainer::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    const float f = 0.125f;
    setBlockBounds(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
    BlockContainer::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
    BlockContainer::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBounds(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    BlockContainer::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBounds(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
    BlockContainer::addCollisionBoxesToList(worldIn, pos, state, mask, list, collidingEntity);
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

const IBlockState* BlockHopper::onBlockPlaced(World*, const BlockPos&, EnumFacing facing, float, float, float, int, EntityLivingBase*)
{
    EnumFacing enumfacing = getOpposite(facing);
    if (enumfacing == EnumFacing::UP)
        enumfacing = EnumFacing::DOWN;
    return getDefaultState()->withProperty(FACING, enumfacing)->withProperty(ENABLED, true);
}

std::shared_ptr<TileEntity> BlockHopper::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityHopper>();
}

void BlockHopper::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack)
{
    BlockContainer::onBlockPlacedBy(worldIn, pos, state, placer, stack);
    if (stack.hasDisplayName()) {
        if (auto* tileentity = dynamic_cast<TileEntityHopper*>(worldIn->getTileEntity(pos)))
            tileentity->setCustomName(stack.getDisplayName());
    }
}

void BlockHopper::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    updateState(worldIn, pos, state);
}

void BlockHopper::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    updateState(worldIn, pos, state);
}

void BlockHopper::updateState(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    const bool flag = !worldIn->isBlockPowered(pos);
    if (flag != state->getValue(ENABLED))
        worldIn->setBlockState(pos, state->withProperty(ENABLED, flag), 4);
}

void BlockHopper::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (auto* tileentity = dynamic_cast<TileEntityHopper*>(worldIn->getTileEntity(pos))) {
        dropInventoryItems(worldIn, pos, tileentity);
        worldIn->updateComparatorOutputLevel(pos, this);
    }
    BlockContainer::breakBlock(worldIn, pos, state);
}

int BlockHopper::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    return Container::calcRedstone(worldIn->getTileEntity(pos));
}

// --- BlockChest ---

void BlockChest::setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos)
{
    if (worldIn->getBlockState(pos.north())->getBlock() == this)
        setBlockBounds(0.0625f, 0.0f, 0.0f, 0.9375f, 0.875f, 0.9375f);
    else if (worldIn->getBlockState(pos.south())->getBlock() == this)
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.875f, 1.0f);
    else if (worldIn->getBlockState(pos.west())->getBlock() == this)
        setBlockBounds(0.0f, 0.0f, 0.0625f, 0.9375f, 0.875f, 0.9375f);
    else if (worldIn->getBlockState(pos.east())->getBlock() == this)
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 1.0f, 0.875f, 0.9375f);
    else
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.875f, 0.9375f);
}

void BlockChest::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    checkForSurroundingChests(worldIn, pos, state);
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        const BlockPos blockpos = pos.offset(enumfacing);
        const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
        if (iblockstate->getBlock() == this)
            checkForSurroundingChests(worldIn, blockpos, iblockstate);
    }
}

const IBlockState* BlockChest::onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, placerHorizontalFacing(placer));
}

void BlockChest::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack)
{
    // Java: getHorizontal(floor(placer.rotationYaw * 4 / 360 + 0.5) & 3).getOpposite(); without the placer
    // (stage 8) the placed facing is kept.
    const Entity* entity = placerEntity(placer);
    const EnumFacing enumfacing = entity
        ? getOpposite(getHorizontal(MathHelper::floor_double(static_cast<double>(entity->rotationYaw * 4.0f / 360.0f) + 0.5) & 3))
        : state->getValue(FACING);
    state = state->withProperty(FACING, enumfacing);
    const BlockPos blockpos = pos.north();
    const BlockPos blockpos1 = pos.south();
    const BlockPos blockpos2 = pos.west();
    const BlockPos blockpos3 = pos.east();
    const bool flag = this == worldIn->getBlockState(blockpos)->getBlock();
    const bool flag1 = this == worldIn->getBlockState(blockpos1)->getBlock();
    const bool flag2 = this == worldIn->getBlockState(blockpos2)->getBlock();
    const bool flag3 = this == worldIn->getBlockState(blockpos3)->getBlock();
    if (!flag && !flag1 && !flag2 && !flag3) {
        worldIn->setBlockState(pos, state, 3);
    } else if (getAxis(enumfacing) != Axis::X || (!flag && !flag1)) {
        if (getAxis(enumfacing) == Axis::Z && (flag2 || flag3)) {
            if (flag2)
                worldIn->setBlockState(blockpos2, state, 3);
            else
                worldIn->setBlockState(blockpos3, state, 3);
            worldIn->setBlockState(pos, state, 3);
        }
    } else {
        if (flag)
            worldIn->setBlockState(blockpos, state, 3);
        else
            worldIn->setBlockState(blockpos1, state, 3);
        worldIn->setBlockState(pos, state, 3);
    }
    if (stack.hasDisplayName()) {
        if (auto* tileentity = dynamic_cast<TileEntityChest*>(worldIn->getTileEntity(pos)))
            tileentity->setCustomName(stack.getDisplayName());
    }
}

const IBlockState* BlockChest::checkForSurroundingChests(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (worldIn->isRemote)
        return state;
    const IBlockState* iblockstate = worldIn->getBlockState(pos.north());
    const IBlockState* iblockstate1 = worldIn->getBlockState(pos.south());
    const IBlockState* iblockstate2 = worldIn->getBlockState(pos.west());
    const IBlockState* iblockstate3 = worldIn->getBlockState(pos.east());
    EnumFacing enumfacing = state->getValue(FACING);
    const Block* block = iblockstate->getBlock();
    const Block* block1 = iblockstate1->getBlock();
    const Block* block2 = iblockstate2->getBlock();
    const Block* block3 = iblockstate3->getBlock();
    if (block != this && block1 != this) {
        const bool flag = block->isFullBlock();
        const bool flag1 = block1->isFullBlock();
        if (block2 == this || block3 == this) {
            const BlockPos blockpos1 = block2 == this ? pos.west() : pos.east();
            const IBlockState* iblockstate6 = worldIn->getBlockState(blockpos1.north());
            const IBlockState* iblockstate7 = worldIn->getBlockState(blockpos1.south());
            enumfacing = EnumFacing::SOUTH;
            const EnumFacing enumfacing2 = block2 == this ? iblockstate2->getValue(FACING) : iblockstate3->getValue(FACING);
            if (enumfacing2 == EnumFacing::NORTH)
                enumfacing = EnumFacing::NORTH;
            const Block* block6 = iblockstate6->getBlock();
            const Block* block7 = iblockstate7->getBlock();
            if ((flag || block6->isFullBlock()) && !flag1 && !block7->isFullBlock())
                enumfacing = EnumFacing::SOUTH;
            if ((flag1 || block7->isFullBlock()) && !flag && !block6->isFullBlock())
                enumfacing = EnumFacing::NORTH;
        }
    } else {
        const BlockPos blockpos = block == this ? pos.north() : pos.south();
        const IBlockState* iblockstate4 = worldIn->getBlockState(blockpos.west());
        const IBlockState* iblockstate5 = worldIn->getBlockState(blockpos.east());
        enumfacing = EnumFacing::EAST;
        const EnumFacing enumfacing1 = block == this ? iblockstate->getValue(FACING) : iblockstate1->getValue(FACING);
        if (enumfacing1 == EnumFacing::WEST)
            enumfacing = EnumFacing::WEST;
        const Block* block4 = iblockstate4->getBlock();
        const Block* block5 = iblockstate5->getBlock();
        if ((block2->isFullBlock() || block4->isFullBlock()) && !block3->isFullBlock() && !block5->isFullBlock())
            enumfacing = EnumFacing::EAST;
        if ((block3->isFullBlock() || block5->isFullBlock()) && !block2->isFullBlock() && !block4->isFullBlock())
            enumfacing = EnumFacing::WEST;
    }
    state = state->withProperty(FACING, enumfacing);
    worldIn->setBlockState(pos, state, 3);
    return state;
}

bool BlockChest::canPlaceBlockAt(World* worldIn, const BlockPos& pos)
{
    int i = 0;
    for (const BlockPos& blockpos : { pos.west(), pos.east(), pos.north(), pos.south() }) {
        if (worldIn->getBlockState(blockpos)->getBlock() == this) {
            if (isDoubleChest(worldIn, blockpos))
                return false;
            ++i;
        }
    }
    return i <= 1;
}

bool BlockChest::isDoubleChest(World* worldIn, const BlockPos& pos)
{
    if (worldIn->getBlockState(pos)->getBlock() != this)
        return false;
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        if (worldIn->getBlockState(pos.offset(enumfacing))->getBlock() == this)
            return true;
    }
    return false;
}

void BlockChest::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock)
{
    BlockContainer::onNeighborBlockChange(worldIn, pos, state, neighborBlock);
    if (auto* tileentity = dynamic_cast<TileEntityChest*>(worldIn->getTileEntity(pos)))
        tileentity->updateContainingBlockInfo();
}

void BlockChest::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (auto* tileentity = dynamic_cast<IInventory*>(worldIn->getTileEntity(pos))) {
        dropInventoryItems(worldIn, pos, tileentity);
        worldIn->updateComparatorOutputLevel(pos, this);
    }
    BlockContainer::breakBlock(worldIn, pos, state);
}

std::shared_ptr<IInventory> BlockChest::getLockableContainer(World* worldIn, const BlockPos& pos)
{
    const std::shared_ptr<TileEntity> tileentity = worldIn->getTileEntityShared(pos);
    auto* chest = dynamic_cast<TileEntityChest*>(tileentity.get());
    if (!chest)
        return nullptr;
    std::shared_ptr<IInventory> ilockablecontainer(tileentity, static_cast<IInventory*>(chest));
    if (isBlocked(worldIn, pos))
        return nullptr;
    for (const EnumFacing enumfacing : kPlaneHorizontal) {
        const BlockPos blockpos = pos.offset(enumfacing);
        const Block* block = worldIn->getBlockState(blockpos)->getBlock();
        if (block == this) {
            if (isBlocked(worldIn, blockpos))
                return nullptr;
            const std::shared_ptr<TileEntity> tileentity1 = worldIn->getTileEntityShared(blockpos);
            if (auto* chest1 = dynamic_cast<TileEntityChest*>(tileentity1.get())) {
                std::shared_ptr<IInventory> other(tileentity1, static_cast<IInventory*>(chest1));
                if (enumfacing != EnumFacing::WEST && enumfacing != EnumFacing::NORTH)
                    ilockablecontainer = std::make_shared<InventoryLargeChest>("container.chestDouble", ilockablecontainer, other);
                else
                    ilockablecontainer = std::make_shared<InventoryLargeChest>("container.chestDouble", other, ilockablecontainer);
            }
        }
    }
    return ilockablecontainer;
}

std::shared_ptr<TileEntity> BlockChest::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityChest>();
}

int BlockChest::getWeakPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState*, EnumFacing)
{
    if (!canProvidePower())
        return 0;
    int i = 0;
    if (const auto* tileentity = dynamic_cast<TileEntityChest*>(worldIn->getTileEntity(pos)))
        i = tileentity->numPlayersUsing;
    return MathHelper::clamp_int(i, 0, 15);
}

int BlockChest::getStrongPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side)
{
    return side == EnumFacing::UP ? getWeakPower(worldIn, pos, state, side) : 0;
}

bool BlockChest::isBlocked(World* worldIn, const BlockPos& pos)
{
    return isBelowSolidBlock(worldIn, pos) || isOcelotSittingOnChest(worldIn, pos);
}

bool BlockChest::isBelowSolidBlock(World* worldIn, const BlockPos& pos)
{
    return worldIn->getBlockState(pos.up())->getBlock()->isNormalCube();
}

bool BlockChest::isOcelotSittingOnChest(World*, const BlockPos&)
{
    // Java: a sitting EntityOcelot in the block above -- ocelots come with stage 10.
    return false;
}

int BlockChest::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    return Container::calcRedstoneFromInventory(getLockableContainer(worldIn, pos).get());
}

// --- BlockEnderChest ---

const IBlockState* BlockEnderChest::onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, getOpposite(placerHorizontalFacing(placer)));
}

void BlockEnderChest::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack&)
{
    worldIn->setBlockState(pos, state->withProperty(FACING, getOpposite(placerHorizontalFacing(placer))), 2);
}

std::shared_ptr<TileEntity> BlockEnderChest::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityEnderChest>();
}

void BlockEnderChest::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random& rand)
{
    for (int i = 0; i < 3; ++i) {
        const int j = rand.nextInt(2) * 2 - 1;
        const int k = rand.nextInt(2) * 2 - 1;
        const double d0 = pos.getX() + 0.5 + 0.25 * j;
        const double d1 = static_cast<float>(pos.getY()) + rand.nextFloat();
        const double d2 = pos.getZ() + 0.5 + 0.25 * k;
        const double d3 = rand.nextFloat() * static_cast<float>(j);
        const double d4 = (static_cast<double>(rand.nextFloat()) - 0.5) * 0.125;
        const double d5 = rand.nextFloat() * static_cast<float>(k);
        spawnParticle(worldIn, EnumParticleTypes::PORTAL, d0, d1, d2, d3, d4, d5);
    }
}

// --- BlockFurnace ---

void BlockFurnace::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    setDefaultFacing(worldIn, pos, state);
}

void BlockFurnace::setDefaultFacing(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!worldIn->isRemote) {
        const Block* block = worldIn->getBlockState(pos.north())->getBlock();
        const Block* block1 = worldIn->getBlockState(pos.south())->getBlock();
        const Block* block2 = worldIn->getBlockState(pos.west())->getBlock();
        const Block* block3 = worldIn->getBlockState(pos.east())->getBlock();
        EnumFacing enumfacing = state->getValue(FACING);
        if (enumfacing == EnumFacing::NORTH && block->isFullBlock() && !block1->isFullBlock())
            enumfacing = EnumFacing::SOUTH;
        else if (enumfacing == EnumFacing::SOUTH && block1->isFullBlock() && !block->isFullBlock())
            enumfacing = EnumFacing::NORTH;
        else if (enumfacing == EnumFacing::WEST && block2->isFullBlock() && !block3->isFullBlock())
            enumfacing = EnumFacing::EAST;
        else if (enumfacing == EnumFacing::EAST && block3->isFullBlock() && !block2->isFullBlock())
            enumfacing = EnumFacing::WEST;
        worldIn->setBlockState(pos, state->withProperty(FACING, enumfacing), 2);
    }
}

void BlockFurnace::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    if (isBurning) {
        const EnumFacing enumfacing = state->getValue(FACING);
        const double d0 = pos.getX() + 0.5;
        const double d1 = pos.getY() + rand.nextDouble() * 6.0 / 16.0;
        const double d2 = pos.getZ() + 0.5;
        const double d3 = 0.52;
        const double d4 = rand.nextDouble() * 0.6 - 0.3;
        switch (enumfacing) {
        case EnumFacing::WEST:
            spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0 - d3, d1, d2 + d4, 0.0, 0.0, 0.0);
            spawnParticle(worldIn, EnumParticleTypes::FLAME, d0 - d3, d1, d2 + d4, 0.0, 0.0, 0.0);
            break;
        case EnumFacing::EAST:
            spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0 + d3, d1, d2 + d4, 0.0, 0.0, 0.0);
            spawnParticle(worldIn, EnumParticleTypes::FLAME, d0 + d3, d1, d2 + d4, 0.0, 0.0, 0.0);
            break;
        case EnumFacing::NORTH:
            spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0 + d4, d1, d2 - d3, 0.0, 0.0, 0.0);
            spawnParticle(worldIn, EnumParticleTypes::FLAME, d0 + d4, d1, d2 - d3, 0.0, 0.0, 0.0);
            break;
        case EnumFacing::SOUTH:
            spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0 + d4, d1, d2 + d3, 0.0, 0.0, 0.0);
            spawnParticle(worldIn, EnumParticleTypes::FLAME, d0 + d4, d1, d2 + d3, 0.0, 0.0, 0.0);
            break;
        default: break;
        }
    }
}

void BlockFurnace::setState(bool active, World* worldIn, const BlockPos& pos)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    const std::shared_ptr<TileEntity> tileentity = worldIn->getTileEntityShared(pos);
    keepInventory = true;
    Block* furnace = active ? Blocks::lit_furnace : Blocks::furnace;
    worldIn->setBlockState(pos, furnace->getDefaultState()->withProperty(FACING, iblockstate->getValue(FACING)), 3);
    worldIn->setBlockState(pos, furnace->getDefaultState()->withProperty(FACING, iblockstate->getValue(FACING)), 3);
    keepInventory = false;
    if (tileentity) {
        tileentity->validate();
        worldIn->setTileEntity(pos, tileentity);
    }
}

std::shared_ptr<TileEntity> BlockFurnace::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityFurnace>();
}

const IBlockState* BlockFurnace::onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, getOpposite(placerHorizontalFacing(placer)));
}

void BlockFurnace::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack)
{
    worldIn->setBlockState(pos, state->withProperty(FACING, getOpposite(placerHorizontalFacing(placer))), 2);
    if (stack.hasDisplayName()) {
        if (auto* tileentity = dynamic_cast<TileEntityFurnace*>(worldIn->getTileEntity(pos)))
            tileentity->setCustomInventoryName(stack.getDisplayName());
    }
}

void BlockFurnace::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!keepInventory) {
        if (auto* tileentity = dynamic_cast<TileEntityFurnace*>(worldIn->getTileEntity(pos))) {
            dropInventoryItems(worldIn, pos, tileentity);
            worldIn->updateComparatorOutputLevel(pos, this);
        }
    }
    BlockContainer::breakBlock(worldIn, pos, state);
}

int BlockFurnace::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    return Container::calcRedstone(worldIn->getTileEntity(pos));
}

// --- BlockDispenser, BlockDropper ---

void BlockDispenser::onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    BlockContainer::onBlockAdded(worldIn, pos, state);
    setDefaultDirection(worldIn, pos, state);
}

void BlockDispenser::setDefaultDirection(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (!worldIn->isRemote) {
        EnumFacing enumfacing = state->getValue(FACING);
        const bool flag = worldIn->getBlockState(pos.north())->getBlock()->isFullBlock();
        const bool flag1 = worldIn->getBlockState(pos.south())->getBlock()->isFullBlock();
        if (enumfacing == EnumFacing::NORTH && flag && !flag1) {
            enumfacing = EnumFacing::SOUTH;
        } else if (enumfacing == EnumFacing::SOUTH && flag1 && !flag) {
            enumfacing = EnumFacing::NORTH;
        } else {
            const bool flag2 = worldIn->getBlockState(pos.west())->getBlock()->isFullBlock();
            const bool flag3 = worldIn->getBlockState(pos.east())->getBlock()->isFullBlock();
            if (enumfacing == EnumFacing::WEST && flag2 && !flag3)
                enumfacing = EnumFacing::EAST;
            else if (enumfacing == EnumFacing::EAST && flag3 && !flag2)
                enumfacing = EnumFacing::WEST;
        }
        worldIn->setBlockState(pos, state->withProperty(FACING, enumfacing)->withProperty(TRIGGERED, false), 2);
    }
}

void BlockDispenser::dispense(World* worldIn, const BlockPos& pos)
{
    if (auto* tileentitydispenser = dynamic_cast<TileEntityDispenser*>(worldIn->getTileEntity(pos))) {
        const int i = tileentitydispenser->getDispenseSlot();
        if (i < 0) {
            worldIn->playAuxSFX(1001, pos, 0);
        } else {
            // Java: the item's IBehaviorDispenseItem from dispenseBehaviorRegistry (arrows, eggs, buckets, fire charges,
            // minecarts, boats, TNT, bone meal, ... and BehaviorDefaultDispenseItem for the rest) dispenses the stack --
            // the behaviours register with the entities in stage 10.
        }
    }
}

void BlockDispenser::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    const bool flag = worldIn->isBlockPowered(pos) || worldIn->isBlockPowered(pos.up());
    const bool flag1 = state->getValue(TRIGGERED);
    if (flag && !flag1) {
        worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
        worldIn->setBlockState(pos, state->withProperty(TRIGGERED, true), 4);
    } else if (!flag && flag1) {
        worldIn->setBlockState(pos, state->withProperty(TRIGGERED, false), 4);
    }
}

void BlockDispenser::updateTick(World* worldIn, const BlockPos& pos, const IBlockState*, Random&)
{
    if (!worldIn->isRemote)
        dispense(worldIn, pos);
}

std::shared_ptr<TileEntity> BlockDispenser::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityDispenser>();
}

const IBlockState* BlockDispenser::onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing, float, float, float, int, EntityLivingBase* placer)
{
    return getDefaultState()->withProperty(FACING, BlockPistonBase::getFacingFromEntity(worldIn, pos, placer))->withProperty(TRIGGERED, false);
}

void BlockDispenser::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack)
{
    worldIn->setBlockState(pos, state->withProperty(FACING, BlockPistonBase::getFacingFromEntity(worldIn, pos, placer)), 2);
    if (stack.hasDisplayName()) {
        if (auto* tileentity = dynamic_cast<TileEntityDispenser*>(worldIn->getTileEntity(pos)))
            tileentity->setCustomName(stack.getDisplayName());
    }
}

void BlockDispenser::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    if (auto* tileentity = dynamic_cast<TileEntityDispenser*>(worldIn->getTileEntity(pos))) {
        dropInventoryItems(worldIn, pos, tileentity);
        worldIn->updateComparatorOutputLevel(pos, this);
    }
    BlockContainer::breakBlock(worldIn, pos, state);
}

int BlockDispenser::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    return Container::calcRedstone(worldIn->getTileEntity(pos));
}

std::shared_ptr<TileEntity> BlockDropper::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityDropper>();
}

void BlockDropper::dispense(World* worldIn, const BlockPos& pos)
{
    auto* tileentitydispenser = dynamic_cast<TileEntityDispenser*>(worldIn->getTileEntity(pos));
    if (!tileentitydispenser)
        return;
    const int i = tileentitydispenser->getDispenseSlot();
    if (i < 0) {
        worldIn->playAuxSFX(1001, pos, 0);
        return;
    }
    const std::shared_ptr<ItemStack> itemstack = tileentitydispenser->getStackInSlot(i);
    if (itemstack) {
        const EnumFacing enumfacing = worldIn->getBlockState(pos)->getValue(FACING);
        const BlockPos blockpos = pos.offset(enumfacing);
        const std::shared_ptr<IInventory> iinventory = TileEntityHopper::getInventoryAtPosition(worldIn, blockpos.getX(), blockpos.getY(), blockpos.getZ());
        std::shared_ptr<ItemStack> itemstack1;
        if (!iinventory) {
            // Java: dropBehavior.dispense(blocksourceimpl, itemstack) -- the BehaviorDefaultDispenseItem.
            itemstack1 = dispenseDefault(worldIn, pos, enumfacing, itemstack);
            if (itemstack1 && itemstack1->stackSize <= 0)
                itemstack1 = nullptr;
        } else {
            auto one = std::make_shared<ItemStack>(itemstack->copy().splitStack(1));
            itemstack1 = TileEntityHopper::putStackInInventoryAllSlots(iinventory.get(), one, getOpposite(enumfacing));
            if (!itemstack1) {
                itemstack1 = std::make_shared<ItemStack>(itemstack->copy());
                if (--itemstack1->stackSize <= 0)
                    itemstack1 = nullptr;
            } else {
                itemstack1 = std::make_shared<ItemStack>(itemstack->copy());
            }
        }
        tileentitydispenser->setInventorySlotContents(i, itemstack1);
    }
}

// --- BlockJukebox ---

bool BlockJukebox::onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer*, EnumFacing, float, float, float)
{
    if (state->getValue(HAS_RECORD)) {
        dropRecord(worldIn, pos, state);
        state = state->withProperty(HAS_RECORD, false);
        worldIn->setBlockState(pos, state, 2);
        return true;
    }
    return false;
}

void BlockJukebox::insertRecord(World* worldIn, const BlockPos& pos, const IBlockState* state, const ItemStack& recordStack)
{
    if (!worldIn->isRemote) {
        if (auto* tileentity = dynamic_cast<TileEntityJukebox*>(worldIn->getTileEntity(pos))) {
            tileentity->setRecord(std::make_shared<ItemStack>(recordStack.getItem(), 1, recordStack.getMetadata()));
            worldIn->setBlockState(pos, state->withProperty(HAS_RECORD, true), 2);
        }
    }
}

void BlockJukebox::dropRecord(World* worldIn, const BlockPos& pos, const IBlockState*)
{
    if (!worldIn->isRemote) {
        if (auto* tileentityjukebox = dynamic_cast<TileEntityJukebox*>(worldIn->getTileEntity(pos))) {
            const std::shared_ptr<ItemStack> itemstack = tileentityjukebox->getRecord();
            if (itemstack) {
                worldIn->playAuxSFX(1005, pos, 0);
                worldIn->playRecord(pos, std::string());
                tileentityjukebox->setRecord(nullptr);
                // Java: an EntityItem of the record (setDefaultPickupDelay) at a random spot -- item entities come
                // with stage 10; the draws stay in step.
                worldIn->rand.nextFloat();
                worldIn->rand.nextFloat();
                worldIn->rand.nextFloat();
            }
        }
    }
}

void BlockJukebox::breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state)
{
    dropRecord(worldIn, pos, state);
    BlockContainer::breakBlock(worldIn, pos, state);
}

void BlockJukebox::dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int)
{
    if (!worldIn->isRemote)
        BlockContainer::dropBlockAsItemWithChance(worldIn, pos, state, chance, 0);
}

std::shared_ptr<TileEntity> BlockJukebox::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityJukebox>();
}

int BlockJukebox::getComparatorInputOverride(World* worldIn, const BlockPos& pos)
{
    if (const auto* tileentity = dynamic_cast<TileEntityJukebox*>(worldIn->getTileEntity(pos))) {
        if (const std::shared_ptr<ItemStack>& itemstack = tileentity->getRecord())
            return Item::getIdFromItem(itemstack->getItem()) + 1 - Item::getIdFromItem(Items::record_13);
    }
    return 0;
}

// --- BlockBeacon ---

std::shared_ptr<TileEntity> BlockBeacon::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityBeacon>();
}

void BlockBeacon::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack)
{
    BlockContainer::onBlockPlacedBy(worldIn, pos, state, placer, stack);
    if (stack.hasDisplayName()) {
        if (auto* tileentity = dynamic_cast<TileEntityBeacon*>(worldIn->getTileEntity(pos)))
            tileentity->setName(stack.getDisplayName());
    }
}

void BlockBeacon::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState*, Block*)
{
    if (auto* tileentity = dynamic_cast<TileEntityBeacon*>(worldIn->getTileEntity(pos))) {
        tileentity->updateBeacon();
        worldIn->addBlockEvent(pos, this, 1, 0);
    }
}

void BlockBeacon::updateColorAsync(World* worldIn, const BlockPos& glassPos)
{
    for (int i = glassPos.getY() - 1; i >= 0; --i) {
        const BlockPos blockpos(glassPos.getX(), i, glassPos.getZ());
        if (!worldIn->canSeeSky(blockpos))
            break;
        const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
        if (iblockstate->getBlock() == Blocks::beacon) {
            if (auto* tileentity = dynamic_cast<TileEntityBeacon*>(worldIn->getTileEntity(blockpos))) {
                tileentity->updateBeacon();
                worldIn->addBlockEvent(blockpos, Blocks::beacon, 1, 0);
            }
        }
    }
}

// --- BlockCommandBlock ---

std::shared_ptr<TileEntity> BlockCommandBlock::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityUnported>("Control");
}

void BlockCommandBlock::onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block*)
{
    if (!worldIn->isRemote) {
        const bool flag = worldIn->isBlockPowered(pos);
        const bool flag1 = state->getValue(TRIGGERED);
        if (flag && !flag1) {
            worldIn->setBlockState(pos, state->withProperty(TRIGGERED, true), 4);
            worldIn->scheduleUpdate(pos, this, tickRate(worldIn));
        } else if (!flag && flag1) {
            worldIn->setBlockState(pos, state->withProperty(TRIGGERED, false), 4);
        }
    }
}

void BlockCommandBlock::updateTick(World*, const BlockPos&, const IBlockState*, Random&)
{
}

// --- BlockEnchantmentTable ---

void BlockEnchantmentTable::randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand)
{
    BlockContainer::randomDisplayTick(worldIn, pos, state, rand);
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            if (i > -2 && i < 2 && j == -1)
                j = 2;
            if (rand.nextInt(16) == 0) {
                for (int k = 0; k <= 1; ++k) {
                    const BlockPos blockpos = pos.add(i, k, j);
                    if (worldIn->getBlockState(blockpos)->getBlock() == Blocks::bookshelf) {
                        if (!worldIn->isAirBlock(pos.add(i / 2, 0, j / 2)))
                            break;
                        const float r1 = rand.nextFloat();
                        const float r2 = rand.nextFloat();
                        const float r3 = rand.nextFloat();
                        spawnParticle(worldIn, EnumParticleTypes::ENCHANTMENT_TABLE, pos.getX() + 0.5, pos.getY() + 2.0, pos.getZ() + 0.5,
                            static_cast<double>(static_cast<float>(i) + r1) - 0.5, static_cast<double>(static_cast<float>(k) - r2 - 1.0f),
                            static_cast<double>(static_cast<float>(j) + r3) - 0.5);
                    }
                }
            }
        }
    }
}

std::shared_ptr<TileEntity> BlockEnchantmentTable::createNewTileEntity(World*, int)
{
    return std::make_shared<TileEntityEnchantmentTable>();
}

void BlockEnchantmentTable::onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack)
{
    BlockContainer::onBlockPlacedBy(worldIn, pos, state, placer, stack);
    if (stack.hasDisplayName()) {
        // Java: ((TileEntityEnchantmentTable)tileentity).setCustomName(stack.getDisplayName()) -- TileEntityEnchantmentTable
        // has no setter yet.
    }
}

} // namespace mc
