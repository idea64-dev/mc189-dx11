#include "item/ItemBlock.h"

#include "block/BlockDoublePlant.h"
#include "block/BlockLeaves.h"
#include "block/BlockLiquid.h"
#include "block/BlockSign.h"
#include "block/BlockSlab.h"
#include "block/BlockSnow.h"
#include "client/resources/Locale.h"
#include "creativetab/CreativeTabs.h"
#include "entity/player/EntityPlayer.h"
#include "init/Blocks.h"
#include "item/EnumDyeColor.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "server/MinecraftServer.h"
#include "server/management/ServerConfigurationManager.h"
#include "tileentity/TileEntity.h"
#include "tileentity/TileEntityBanner.h"
#include "util/MathHelper.h"
#include "world/Colorizer.h"
#include "world/World.h"
#include "world/WorldServer.h"

namespace mc {

// --- ItemBlock ---

std::string ItemBlock::getUnlocalizedName(const ItemStack&) const
{
    return block->getUnlocalizedName();
}

std::string ItemBlock::getUnlocalizedName() const
{
    return block->getUnlocalizedName();
}

const CreativeTabs* ItemBlock::getCreativeTab() const
{
    return block->getCreativeTabToDisplayOn();
}

void ItemBlock::getSubItems(Item* item, const CreativeTabs* tab, std::vector<ItemStack>& subItems) const
{
    block->getSubBlocks(item, tab, subItems);
}

// --- ItemMultiTexture ---

ItemMultiTexture::ItemMultiTexture(Block* blockIn, Block* block2, std::vector<std::string> namesByMeta)
    : ItemMultiTexture(blockIn, block2, [names = std::move(namesByMeta)](const ItemStack& stack) {
        int i = stack.getMetadata();
        if (i < 0 || i >= static_cast<int>(names.size()))
            i = 0;
        return names[static_cast<size_t>(i)];
    })
{
}

std::string ItemMultiTexture::getUnlocalizedName(const ItemStack& stack) const
{
    return ItemBlock::getUnlocalizedName() + "." + nameFunction(stack);
}

// --- ItemColored ---

int ItemColored::getColorFromItemStack(const ItemStack& stack, int) const
{
    return coloredBlock->getRenderColor(coloredBlock->getStateFromMeta(stack.getMetadata()));
}

std::string ItemColored::getUnlocalizedName(const ItemStack& stack) const
{
    if (!hasSubtypeNames)
        return ItemBlock::getUnlocalizedName(stack);
    const int i = stack.getMetadata();
    return i >= 0 && i < static_cast<int>(subtypeNames.size()) ? ItemBlock::getUnlocalizedName(stack) + "." + subtypeNames[static_cast<size_t>(i)]
                                                               : ItemBlock::getUnlocalizedName(stack);
}

// --- ItemCloth ---

std::string ItemCloth::getUnlocalizedName(const ItemStack& stack) const
{
    return ItemBlock::getUnlocalizedName() + "." + std::string(info(dyeColorByMetadata(stack.getMetadata())).unlocalizedName);
}

// --- ItemSlab ---

std::string ItemSlab::getUnlocalizedName(const ItemStack& stack) const
{
    return singleSlab->getUnlocalizedName(stack.getMetadata());
}

// --- ItemLeaves ---

ItemLeaves::ItemLeaves(BlockLeaves* blockIn)
    : ItemBlock(blockIn)
    , leaves(blockIn)
{
    setMaxDamage(0);
    setHasSubtypes(true);
}

int ItemLeaves::getColorFromItemStack(const ItemStack& stack, int) const
{
    return leaves->getRenderColor(leaves->getStateFromMeta(stack.getMetadata()));
}

std::string ItemLeaves::getUnlocalizedName(const ItemStack& stack) const
{
    return ItemBlock::getUnlocalizedName() + "." + std::string(BlockPlanks::info(leaves->getWoodType(stack.getMetadata())).unlocalizedName);
}

// --- ItemLilyPad ---

int ItemLilyPad::getColorFromItemStack(const ItemStack& stack, int) const
{
    return Blocks::waterlily->getRenderColor(Blocks::waterlily->getStateFromMeta(stack.getMetadata()));
}

// --- ItemDoublePlant ---

int ItemDoublePlant::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    const BlockDoublePlant::EnumPlantType type = BlockDoublePlant::byMetadata(stack.getMetadata());
    return type != BlockDoublePlant::EnumPlantType::GRASS && type != BlockDoublePlant::EnumPlantType::FERN ? ItemMultiTexture::getColorFromItemStack(stack, renderPass)
                                                                                                           : ColorizerGrass::getGrassColor(0.5, 1.0);
}

// --- ItemBanner ---

namespace {

EnumDyeColor getBaseColor(const ItemStack& stack)
{
    const auto& tag = stack.getTagCompound();
    const auto nbttagcompound = tag && tag->hasKey("BlockEntityTag", 10) ? tag->getCompoundTag("BlockEntityTag") : nullptr;
    if (nbttagcompound && nbttagcompound->hasKey("Base"))
        return dyeColorByDyeDamage(nbttagcompound->getInteger("Base"));
    return dyeColorByDyeDamage(stack.getMetadata());
}

} // namespace

ItemBanner::ItemBanner()
    : ItemBlock(Blocks::standing_banner)
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabDecorations);
    setHasSubtypes(true);
    setMaxDamage(0);
}

std::string ItemBanner::getItemStackDisplayName(const ItemStack& stack) const
{
    return StatCollector::translateToLocal("item.banner." + std::string(info(getBaseColor(stack)).unlocalizedName) + ".name");
}

int ItemBanner::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    if (renderPass == 0)
        return 16777215;
    return info(getBaseColor(stack)).mapColor->colorValue;
}

void ItemBanner::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (const EnumDyeColor color : enumValues<EnumDyeColor, 16>()) {
        // TileEntityBanner.setBaseColorAndPatterns(tag, dyeDamage, null) sets Base only.
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        nbttagcompound->setInteger("Base", getDyeDamage(color));
        auto nbttagcompound1 = std::make_shared<NBTTagCompound>();
        nbttagcompound1->setTag("BlockEntityTag", nbttagcompound);
        ItemStack itemstack(item, 1, getDyeDamage(color));
        itemstack.setTagCompound(nbttagcompound1);
        subItems.push_back(std::move(itemstack));
    }
}

const CreativeTabs* ItemBanner::getCreativeTab() const
{
    return &CreativeTabs::tabDecorations;
}

namespace {

// The block's place sound at the middle of pos (Java repeats this in each placing item).
void playPlaceSound(World* worldIn, const BlockPos& pos, const Block* block)
{
    const Block::SoundType& sound = *block->stepSound;
    worldIn->playSoundEffect(static_cast<double>(static_cast<float>(pos.getX()) + 0.5f), static_cast<double>(static_cast<float>(pos.getY()) + 0.5f),
        static_cast<double>(static_cast<float>(pos.getZ()) + 0.5f), sound.getPlaceSound(), (sound.getVolume() + 1.0f) / 2.0f, sound.getFrequency() * 0.8f);
}

} // namespace

bool ItemBlock::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing side, float hitX, float hitY, float hitZ)
{
    BlockPos pos = posIn;
    Block* clicked = worldIn->getBlockState(pos)->getBlock();

    if (!clicked->isReplaceable(worldIn, pos))
        pos = pos.offset(side);

    if (stack.stackSize == 0)
        return false;
    if (!playerIn->canPlayerEdit(pos, side, &stack))
        return false;
    if (!worldIn->canBlockBePlaced(block, pos, false, side, nullptr, &stack))
        return false;

    const int i = getMetadata(stack.getMetadata());
    const IBlockState* iblockstate1 = block->onBlockPlaced(worldIn, pos, side, hitX, hitY, hitZ, i, playerIn);

    if (worldIn->setBlockState(pos, iblockstate1, 3)) {
        iblockstate1 = worldIn->getBlockState(pos);
        if (iblockstate1->getBlock() == block) {
            setTileEntityNBT(worldIn, playerIn, pos, stack);
            block->onBlockPlacedBy(worldIn, pos, iblockstate1, playerIn, stack);
        }
        playPlaceSound(worldIn, pos, block);
        --stack.stackSize;
    }

    return true;
}

bool ItemBlock::setTileEntityNBT(World* worldIn, EntityPlayer* player, const BlockPos& pos, ItemStack& stack)
{
    // Java: MinecraftServer.getServer() is null on a client without an integrated server, and nothing is copied
    // then. Here the server is the one of a WorldServer; a client world goes on (singleplayer).
    auto* worldServer = dynamic_cast<WorldServer*>(worldIn);

    if (stack.hasTagCompound() && stack.getTagCompound()->hasKey("BlockEntityTag", 10)) {
        if (TileEntity* tileentity = worldIn->getTileEntity(pos)) {
            if (!worldIn->isRemote && tileentity->func_183000_F() && worldServer
                && !worldServer->getMinecraftServer()->getConfigurationManager()->canSendCommands(player->getGameProfile()))
                return false;

            NBTTagCompound nbttagcompound;
            // Java copies the compound before writing into it, so the comparison is against an empty one.
            const std::shared_ptr<NBTBase> nbttagcompound1 = nbttagcompound.copy();
            tileentity->writeToNBT(nbttagcompound);
            const auto nbttagcompound2 = stack.getTagCompound()->getCompoundTag("BlockEntityTag");
            nbttagcompound.merge(*nbttagcompound2);
            nbttagcompound.setInteger("x", pos.getX());
            nbttagcompound.setInteger("y", pos.getY());
            nbttagcompound.setInteger("z", pos.getZ());

            if (!nbttagcompound.equals(*nbttagcompound1)) {
                tileentity->readFromNBT(nbttagcompound);
                tileentity->markDirty();
                return true;
            }
        }
    }

    return false;
}

bool ItemBlock::canPlaceBlockOnSide(World* worldIn, const BlockPos& posIn, EnumFacing sideIn, EntityPlayer*, ItemStack* stack)
{
    BlockPos pos = posIn;
    EnumFacing side = sideIn;
    Block* clicked = worldIn->getBlockState(pos)->getBlock();

    if (clicked == Blocks::snow_layer)
        side = EnumFacing::UP;
    else if (!clicked->isReplaceable(worldIn, pos))
        pos = pos.offset(side);

    return worldIn->canBlockBePlaced(block, pos, false, side, nullptr, stack);
}

int ItemSlab::getVariant(const ItemStack& stack) const
{
    // Java: BlockStoneSlab / BlockStoneSlabNew / BlockWoodSlab.getVariant -- the variant of metadata & 7.
    return singleSlab->getStateFromMeta(stack.getMetadata() & 7)->getRawValue(singleSlab->getVariantProperty());
}

bool ItemSlab::placeDouble(ItemStack& stack, World* worldIn, const BlockPos& pos, int variant)
{
    const IBlockState* iblockstate1 = doubleSlab->getDefaultState()->withRawValue(singleSlab->getVariantProperty(), variant);
    const std::optional<AxisAlignedBB> box = doubleSlab->getCollisionBoundingBox(worldIn, pos, iblockstate1);
    // Java: checkNoEntityCollision(null) is true.
    if ((!box || worldIn->checkNoEntityCollision(*box)) && worldIn->setBlockState(pos, iblockstate1, 3)) {
        playPlaceSound(worldIn, pos, doubleSlab);
        --stack.stackSize;
    }
    return true;
}

bool ItemSlab::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY, float hitZ)
{
    if (stack.stackSize == 0)
        return false;
    if (!playerIn->canPlayerEdit(pos.offset(side), side, &stack))
        return false;

    const int object = getVariant(stack);
    const IBlockState* iblockstate = worldIn->getBlockState(pos);

    if (iblockstate->getBlock() == singleSlab) {
        const int comparable = iblockstate->getRawValue(singleSlab->getVariantProperty());
        const BlockSlab::EnumBlockHalf half = iblockstate->getValue(BlockSlab::HALF);
        if (((side == EnumFacing::UP && half == BlockSlab::EnumBlockHalf::BOTTOM) || (side == EnumFacing::DOWN && half == BlockSlab::EnumBlockHalf::TOP))
            && comparable == object)
            return placeDouble(stack, worldIn, pos, comparable);
    }

    return tryPlace(stack, worldIn, pos.offset(side), object) ? true : ItemBlock::onItemUse(stack, playerIn, worldIn, pos, side, hitX, hitY, hitZ);
}

bool ItemSlab::canPlaceBlockOnSide(World* worldIn, const BlockPos& posIn, EnumFacing side, EntityPlayer* player, ItemStack* stack)
{
    const BlockPos blockpos = posIn;
    const IProperty& iproperty = singleSlab->getVariantProperty();
    const int object = getVariant(*stack);
    const IBlockState* iblockstate = worldIn->getBlockState(posIn);

    if (iblockstate->getBlock() == singleSlab) {
        const bool flag = iblockstate->getValue(BlockSlab::HALF) == BlockSlab::EnumBlockHalf::TOP;
        if (((side == EnumFacing::UP && !flag) || (side == EnumFacing::DOWN && flag)) && object == iblockstate->getRawValue(iproperty))
            return true;
    }

    const BlockPos pos = posIn.offset(side);
    const IBlockState* iblockstate1 = worldIn->getBlockState(pos);
    return iblockstate1->getBlock() == singleSlab && object == iblockstate1->getRawValue(iproperty)
        ? true
        : ItemBlock::canPlaceBlockOnSide(worldIn, blockpos, side, player, stack);
}

bool ItemSlab::tryPlace(ItemStack& stack, World* worldIn, const BlockPos& pos, int variantInStack)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() == singleSlab) {
        const int comparable = iblockstate->getRawValue(singleSlab->getVariantProperty());
        if (comparable == variantInStack)
            return placeDouble(stack, worldIn, pos, comparable);
    }
    return false;
}

bool ItemSnow::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY, float hitZ)
{
    if (stack.stackSize == 0)
        return false;
    if (!playerIn->canPlayerEdit(pos, side, &stack))
        return false;

    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    Block* clicked = iblockstate->getBlock();
    BlockPos blockpos = pos;

    if ((side != EnumFacing::UP || clicked != block) && !clicked->isReplaceable(worldIn, pos)) {
        blockpos = pos.offset(side);
        iblockstate = worldIn->getBlockState(blockpos);
        clicked = iblockstate->getBlock();
    }

    if (clicked == block) {
        const int i = iblockstate->getValue(BlockSnow::LAYERS);
        if (i <= 7) {
            const IBlockState* iblockstate1 = iblockstate->withProperty(BlockSnow::LAYERS, i + 1);
            const std::optional<AxisAlignedBB> axisalignedbb = block->getCollisionBoundingBox(worldIn, blockpos, iblockstate1);
            if (axisalignedbb && worldIn->checkNoEntityCollision(*axisalignedbb) && worldIn->setBlockState(blockpos, iblockstate1, 2)) {
                playPlaceSound(worldIn, blockpos, block);
                --stack.stackSize;
                return true;
            }
        }
    }

    return ItemBlock::onItemUse(stack, playerIn, worldIn, blockpos, side, hitX, hitY, hitZ);
}

std::shared_ptr<ItemStack> ItemLilyPad::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    const std::optional<MovingObjectPosition> movingobjectposition = getMovingObjectPositionFromPlayer(worldIn, playerIn, true);
    if (!movingobjectposition)
        return itemStackIn;

    if (movingobjectposition->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK) {
        const BlockPos blockpos = *movingobjectposition->getBlockPos();
        const EnumFacing sideHit = *movingobjectposition->sideHit;
        if (!worldIn->isBlockModifiable(playerIn, blockpos))
            return itemStackIn;
        if (!playerIn->canPlayerEdit(blockpos.offset(sideHit), sideHit, itemStackIn.get()))
            return itemStackIn;

        const BlockPos blockpos1 = blockpos.up();
        const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
        if (&iblockstate->getBlock()->getMaterial() == &Material::water && iblockstate->getValue(BlockLiquid::LEVEL) == 0 && worldIn->isAirBlock(blockpos1)) {
            worldIn->setBlockState(blockpos1, Blocks::waterlily->getDefaultState());
            if (!playerIn->capabilities.isCreativeMode)
                --itemStackIn->stackSize;
            // Java: triggerAchievement(StatList.objectUseStats[...]) -- stage 12.
        }
    }

    return itemStackIn;
}

bool ItemBanner::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing side, float, float, float)
{
    if (side == EnumFacing::DOWN)
        return false;
    if (!worldIn->getBlockState(posIn)->getBlock()->getMaterial().isSolid())
        return false;

    const BlockPos pos = posIn.offset(side);
    if (!playerIn->canPlayerEdit(pos, side, &stack))
        return false;
    if (!Blocks::standing_banner->canPlaceBlockAt(worldIn, pos))
        return false;
    if (worldIn->isRemote)
        return true;

    if (side == EnumFacing::UP) {
        const int i = MathHelper::floor_double(static_cast<double>((playerIn->rotationYaw + 180.0f) * 16.0f / 360.0f) + 0.5) & 15;
        worldIn->setBlockState(pos, Blocks::standing_banner->getDefaultState()->withProperty(BlockStandingSign::ROTATION, i), 3);
    } else {
        worldIn->setBlockState(pos, Blocks::wall_banner->getDefaultState()->withProperty(BlockWallSign::FACING, side), 3);
    }

    --stack.stackSize;
    if (auto* tileentity = dynamic_cast<TileEntityBanner*>(worldIn->getTileEntity(pos)))
        tileentity->setItemValues(stack);
    return true;
}

} // namespace mc
