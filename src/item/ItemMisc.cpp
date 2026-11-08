#include "item/ItemMisc.h"

#include "block/AllBlocks.h"
#include "client/resources/Locale.h"
#include "core/JavaString.h"
#include "creativetab/CreativeTabs.h"
#include "enchantment/Enchantment.h"
#include "enchantment/EnchantmentHelper.h"
#include "entity/EntityItemHooks.h"
#include "entity/EntityList.h"
#include "entity/player/EntityPlayer.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/EnumDyeColor.h"
#include "item/ItemBlock.h"
#include "item/ItemPotion.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "tileentity/TileEntities.h"
#include "tileentity/TileEntitySign.h"
#include "tileentity/TileEntitySkull.h"
#include "util/EnumParticleTypes.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/chunk/IChunkProvider.h"

#include <unordered_map>

namespace mc {

ItemBow::ItemBow()
{
    maxStackSize = 1;
    setMaxDamage(384);
    setCreativeTab(&CreativeTabs::tabCombat);
}

ItemFishingRod::ItemFishingRod()
{
    setMaxDamage(64);
    setMaxStackSize(1);
    setCreativeTab(&CreativeTabs::tabTools);
}

ItemFlintAndSteel::ItemFlintAndSteel()
{
    maxStackSize = 1;
    setMaxDamage(64);
    setCreativeTab(&CreativeTabs::tabTools);
}

ItemCarrotOnAStick::ItemCarrotOnAStick()
{
    setCreativeTab(&CreativeTabs::tabTransport);
    setMaxStackSize(1);
    setMaxDamage(25);
}

ItemDoor::ItemDoor(Block* blockIn)
    : block(blockIn)
{
    setCreativeTab(&CreativeTabs::tabRedstone);
}

void ItemDoor::placeDoor(World* worldIn, const BlockPos& pos, EnumFacing facing, Block* door)
{
    const BlockPos blockpos = pos.offset(rotateY(facing));
    const BlockPos blockpos1 = pos.offset(rotateYCCW(facing));
    const int i = (worldIn->getBlockState(blockpos1)->getBlock()->isNormalCube() ? 1 : 0) + (worldIn->getBlockState(blockpos1.up())->getBlock()->isNormalCube() ? 1 : 0);
    const int j = (worldIn->getBlockState(blockpos)->getBlock()->isNormalCube() ? 1 : 0) + (worldIn->getBlockState(blockpos.up())->getBlock()->isNormalCube() ? 1 : 0);
    const bool flag = worldIn->getBlockState(blockpos1)->getBlock() == door || worldIn->getBlockState(blockpos1.up())->getBlock() == door;
    const bool flag1 = worldIn->getBlockState(blockpos)->getBlock() == door || worldIn->getBlockState(blockpos.up())->getBlock() == door;
    bool flag2 = false;
    if ((flag && !flag1) || j > i)
        flag2 = true;
    const BlockPos blockpos2 = pos.up();
    const IBlockState* iblockstate = door->getDefaultState()
                                         ->withProperty(BlockDoor::FACING, facing)
                                         ->withProperty(BlockDoor::HINGE, flag2 ? BlockDoor::EnumHingePosition::RIGHT : BlockDoor::EnumHingePosition::LEFT);
    worldIn->setBlockState(pos, iblockstate->withProperty(BlockDoor::HALF, BlockDoor::EnumDoorHalf::LOWER), 2);
    worldIn->setBlockState(blockpos2, iblockstate->withProperty(BlockDoor::HALF, BlockDoor::EnumDoorHalf::UPPER), 2);
    worldIn->notifyNeighborsOfStateChange(pos, door);
    worldIn->notifyNeighborsOfStateChange(blockpos2, door);
}

ItemMinecart::ItemMinecart(EnumMinecartType type)
    : minecartType(type)
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabTransport);
}

ItemBucket::ItemBucket(Block* containedBlock)
    : isFull(containedBlock)
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemBucketMilk::ItemBucketMilk()
{
    setMaxStackSize(1);
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemHangingEntity::ItemHangingEntity(HangingType type)
    : hangingEntityClass(type)
{
    setCreativeTab(&CreativeTabs::tabDecorations);
}

ItemSign::ItemSign()
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabDecorations);
}

ItemBed::ItemBed()
{
    setCreativeTab(&CreativeTabs::tabDecorations);
}

ItemSaddle::ItemSaddle()
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabTransport);
}

ItemRedstone::ItemRedstone()
{
    setCreativeTab(&CreativeTabs::tabRedstone);
}

ItemSnowball::ItemSnowball()
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemBoat::ItemBoat()
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabTransport);
}

bool ItemBook::isItemTool(const ItemStack& stack) const
{
    return stack.stackSize == 1;
}

ItemEgg::ItemEgg()
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabMaterials);
}

ItemEmptyMap::ItemEmptyMap()
{
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemEnderPearl::ItemEnderPearl()
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemEnderEye::ItemEnderEye()
{
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemGlassBottle::ItemGlassBottle()
{
    setCreativeTab(&CreativeTabs::tabBrewing);
}

ItemFireball::ItemFireball()
{
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemLead::ItemLead()
{
    setCreativeTab(&CreativeTabs::tabTools);
}

ItemNameTag::ItemNameTag()
{
    setCreativeTab(&CreativeTabs::tabTools);
}

ItemArmorStand::ItemArmorStand()
{
    setCreativeTab(&CreativeTabs::tabDecorations);
}

// --- ItemMonsterPlacer ---

ItemMonsterPlacer::ItemMonsterPlacer()
{
    setHasSubtypes(true);
    setCreativeTab(&CreativeTabs::tabMisc);
}

std::string ItemMonsterPlacer::getItemStackDisplayName(const ItemStack& stack) const
{
    std::string s = java::trim(StatCollector::translateToLocal(getUnlocalizedName() + ".name"));
    if (const auto s1 = EntityList::getStringFromID(stack.getMetadata()))
        s += " " + StatCollector::translateToLocal("entity." + *s1 + ".name");
    return s;
}

int ItemMonsterPlacer::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    const EntityList::EntityEggInfo* egg = EntityList::getEggInfo(stack.getMetadata());
    return egg ? (renderPass == 0 ? egg->primaryColor : egg->secondaryColor) : 16777215;
}

void ItemMonsterPlacer::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (const EntityList::EntityEggInfo& egg : EntityList::entityEggs())
        subItems.emplace_back(item, 1, egg.spawnedID);
}

// --- ItemSkull ---

ItemSkull::ItemSkull()
{
    setCreativeTab(&CreativeTabs::tabDecorations);
    setMaxDamage(0);
    setHasSubtypes(true);
}

void ItemSkull::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (int i = 0; i < 5; ++i)
        subItems.emplace_back(item, 1, i);
}

std::string ItemSkull::getUnlocalizedName(const ItemStack& stack) const
{
    int i = stack.getMetadata();
    if (i < 0 || i >= 5)
        i = 0;
    return Item::getUnlocalizedName() + "." + skullTypes[i];
}

std::string ItemSkull::getItemStackDisplayName(const ItemStack& stack) const
{
    if (stack.getMetadata() == 3 && stack.hasTagCompound()) {
        const auto& tag = stack.getTagCompound();
        if (tag->hasKey("SkullOwner", 8))
            return StatCollector::translateToLocalFormatted("item.skull.player.name", tag->getString("SkullOwner"));
        if (tag->hasKey("SkullOwner", 10)) {
            const auto nbttagcompound = tag->getCompoundTag("SkullOwner");
            if (nbttagcompound->hasKey("Name", 8))
                return StatCollector::translateToLocalFormatted("item.skull.player.name", nbttagcompound->getString("Name"));
        }
    }
    return Item::getItemStackDisplayName(stack);
}

bool ItemSkull::updateItemStackNBT(NBTTagCompound& nbt) const
{
    if (nbt.hasKey("SkullOwner", 8) && !nbt.getString("SkullOwner").empty()) {
        // TileEntitySkull.updateGameprofile without a running server returns the profile unchanged (no id, the
        // name only), and NBTUtil.writeGameProfile then writes just the name. With the integrated server
        // (stage 9) the profile cache fills in id and textures.
        auto profile = std::make_shared<NBTTagCompound>();
        profile->setString("Name", nbt.getString("SkullOwner"));
        nbt.setTag("SkullOwner", profile);
        return true;
    }
    return false;
}

// --- ItemFireworkCharge ---

int ItemFireworkCharge::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    if (renderPass != 1)
        return Item::getColorFromItemStack(stack, renderPass);
    const auto nbtbase = std::dynamic_pointer_cast<NBTTagIntArray>(getExplosionTag(stack, "Colors"));
    if (!nbtbase)
        return 9079434;
    const std::vector<int32_t>& aint = nbtbase->getIntArray();
    if (aint.size() == 1)
        return aint[0];
    // Java divides by the length -- an empty array throws ArithmeticException there.
    int i = 0;
    int j = 0;
    int k = 0;
    for (const int l : aint) {
        i += (l & 16711680) >> 16;
        j += (l & 65280) >> 8;
        k += (l & 255) >> 0;
    }
    const int n = static_cast<int>(aint.size());
    i = i / n;
    j = j / n;
    k = k / n;
    return i << 16 | j << 8 | k;
}

std::shared_ptr<NBTBase> ItemFireworkCharge::getExplosionTag(const ItemStack& stack, const std::string& key)
{
    if (stack.hasTagCompound())
        return stack.getTagCompound()->getCompoundTag("Explosion")->getTag(key);
    return nullptr;
}

// --- ItemEnchantedBook ---

EnumRarity ItemEnchantedBook::getRarity(const ItemStack& stack) const
{
    return getEnchantments(stack)->tagCount() > 0 ? EnumRarity::UNCOMMON : Item::getRarity(stack);
}

std::shared_ptr<NBTTagList> ItemEnchantedBook::getEnchantments(const ItemStack& stack) const
{
    const auto& nbttagcompound = stack.getTagCompound();
    if (nbttagcompound && nbttagcompound->hasKey("StoredEnchantments", 9))
        return std::static_pointer_cast<NBTTagList>(nbttagcompound->getTag("StoredEnchantments"));
    return std::make_shared<NBTTagList>();
}

void ItemEnchantedBook::addEnchantment(ItemStack& stack, const EnchantmentData& enchantment) const
{
    const auto nbttaglist = getEnchantments(stack);
    bool flag = true;
    for (int i = 0; i < nbttaglist->tagCount(); ++i) {
        const auto nbttagcompound = nbttaglist->getCompoundTagAt(i);
        if (nbttagcompound->getShort("id") == enchantment.enchantmentobj->effectId) {
            if (nbttagcompound->getShort("lvl") < enchantment.enchantmentLevel)
                nbttagcompound->setShort("lvl", static_cast<int16_t>(enchantment.enchantmentLevel));
            flag = false;
            break;
        }
    }
    if (flag) {
        auto nbttagcompound1 = std::make_shared<NBTTagCompound>();
        nbttagcompound1->setShort("id", static_cast<int16_t>(enchantment.enchantmentobj->effectId));
        nbttagcompound1->setShort("lvl", static_cast<int16_t>(enchantment.enchantmentLevel));
        nbttaglist->appendTag(nbttagcompound1);
    }
    if (!stack.hasTagCompound())
        stack.setTagCompound(std::make_shared<NBTTagCompound>());
    stack.getTagCompound()->setTag("StoredEnchantments", nbttaglist);
}

ItemStack ItemEnchantedBook::getEnchantedItemStack(const EnchantmentData& data) const
{
    ItemStack itemstack(const_cast<ItemEnchantedBook*>(this));
    addEnchantment(itemstack, data);
    return itemstack;
}

void ItemEnchantedBook::getAll(const Enchantment* enchantment, std::vector<ItemStack>& list) const
{
    for (int i = enchantment->getMinLevel(); i <= enchantment->getMaxLevel(); ++i)
        list.push_back(getEnchantedItemStack(EnchantmentData(const_cast<Enchantment*>(enchantment), i)));
}

WeightedRandomChestContent ItemEnchantedBook::getRandom(Random& rand) const
{
    return getRandom(rand, 1, 1, 1);
}

WeightedRandomChestContent ItemEnchantedBook::getRandom(Random& rand, int minChance, int maxChance, int weight) const
{
    auto itemstack = std::make_shared<ItemStack>(Items::book, 1, 0);
    EnchantmentHelper::addRandomEnchantment(rand, *itemstack, 30);
    return WeightedRandomChestContent(itemstack, minChance, maxChance, weight);
}

// --- ItemDye ---

ItemDye::ItemDye()
{
    setHasSubtypes(true);
    setMaxDamage(0);
    setCreativeTab(&CreativeTabs::tabMaterials);
}

std::string ItemDye::getUnlocalizedName(const ItemStack& stack) const
{
    return Item::getUnlocalizedName() + "." + std::string(info(dyeColorByDyeDamage(stack.getMetadata())).unlocalizedName);
}

void ItemDye::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (int i = 0; i < 16; ++i)
        subItems.emplace_back(item, 1, i);
}

// --- ItemCoal ---

ItemCoal::ItemCoal()
{
    setHasSubtypes(true);
    setMaxDamage(0);
    setCreativeTab(&CreativeTabs::tabMaterials);
}

std::string ItemCoal::getUnlocalizedName(const ItemStack& stack) const
{
    return stack.getMetadata() == 1 ? "item.charcoal" : "item.coal";
}

void ItemCoal::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    subItems.emplace_back(item, 1, 0);
    subItems.emplace_back(item, 1, 1);
}

// --- ItemRecord ---

namespace {

std::unordered_map<std::string, ItemRecord*>& records()
{
    static std::unordered_map<std::string, ItemRecord*> map;
    return map;
}

} // namespace

ItemRecord::ItemRecord(std::string name)
    : recordName(std::move(name))
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabMisc);
    records()["records." + recordName] = this;
}

std::string ItemRecord::getRecordNameLocal() const
{
    return StatCollector::translateToLocal("item.record." + recordName + ".desc");
}

ItemRecord* ItemRecord::getRecord(const std::string& name)
{
    const auto it = records().find(name);
    return it == records().end() ? nullptr : it->second;
}

// --- ItemExpBottle ---

ItemExpBottle::ItemExpBottle()
{
    setCreativeTab(&CreativeTabs::tabMisc);
}

// --- Books ---

bool ItemWritableBook::isNBTValid(const NBTTagCompound* nbt)
{
    if (!nbt || !nbt->hasKey("pages", 9))
        return false;
    const auto nbttaglist = nbt->getTagList("pages", 8);
    for (int i = 0; i < nbttaglist->tagCount(); ++i) {
        if (java::utf16Length(nbttaglist->getStringTagAt(i)) > 32767)
            return false;
    }
    return true;
}

bool ItemEditableBook::validBookTagContents(const NBTTagCompound* nbt)
{
    if (!ItemWritableBook::isNBTValid(nbt) || !nbt->hasKey("title", 8))
        return false;
    const std::string s = nbt->getString("title");
    return java::utf16Length(s) <= 32 ? nbt->hasKey("author", 8) : false;
}

int ItemEditableBook::getGeneration(const ItemStack& book)
{
    return book.getTagCompound()->getInteger("generation");
}

std::string ItemEditableBook::getItemStackDisplayName(const ItemStack& stack) const
{
    if (stack.hasTagCompound()) {
        const std::string s = stack.getTagCompound()->getString("title");
        if (!s.empty())
            return s;
    }
    return Item::getItemStackDisplayName(stack);
}

namespace {

// Java: worldIn.playSoundAtEntity(playerIn, "random.bow", 0.5F, 0.4F / (itemRand.nextFloat() * 0.4F + 0.8F)) --
// the throw sound of the throwable items.
void playThrowSound(World* worldIn, EntityPlayer* playerIn)
{
    worldIn->playSoundAtEntity(playerIn, "random.bow", 0.5f, 0.4f / (Item::itemRand().nextFloat() * 0.4f + 0.8f));
}

// Java: player.inventory.addItemStackToInventory(stack), dropping it when the inventory is full.
void giveOrDrop(EntityPlayer* player, Item* item)
{
    if (!player->inventory.addItemStackToInventory(std::make_shared<ItemStack>(item)))
        player->dropPlayerItemWithRandomChoice(std::make_shared<ItemStack>(item, 1, 0), false);
}

void spawnParticle(World* worldIn, EnumParticleTypes type, double x, double y, double z, double xOffset, double yOffset, double zOffset)
{
    worldIn->spawnParticle(getParticleID(type), false, x, y, z, xOffset, yOffset, zOffset);
}

// Java: Math.random() -- the process-wide generator, used for effects only.
double mathRandom()
{
    static Random random;
    return random.nextDouble();
}

} // namespace

std::shared_ptr<ItemStack> ItemBow::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World*, EntityPlayer* playerIn)
{
    if (playerIn->capabilities.isCreativeMode || playerIn->inventory.hasItem(Items::arrow))
        playerIn->setItemInUse(itemStackIn, getMaxItemUseDuration(*itemStackIn));
    return itemStackIn;
}

void ItemBow::onPlayerStoppedUsing(ItemStack& stack, World* worldIn, EntityPlayer* playerIn, int timeLeft)
{
    const bool flag = playerIn->capabilities.isCreativeMode || EnchantmentHelper::getEnchantmentLevel(Enchantment::infinity->effectId, &stack) > 0;

    if (flag || playerIn->inventory.hasItem(Items::arrow)) {
        const int i = getMaxItemUseDuration(stack) - timeLeft;
        float f = static_cast<float>(i) / 20.0f;
        f = (f * f + f * 2.0f) / 3.0f;

        if (static_cast<double>(f) < 0.1)
            return;

        if (f > 1.0f)
            f = 1.0f;

        // Java: new EntityArrow(worldIn, playerIn, f * 2.0F), critical at full draw, with the power, punch and
        // flame enchantments -- the arrow entity, stage 10.
        damageItem(stack, 1, playerIn);
        worldIn->playSoundAtEntity(playerIn, "random.bow", 1.0f, 1.0f / (itemRand().nextFloat() * 0.4f + 1.2f) + f * 0.5f);

        if (!flag)
            playerIn->inventory.consumeInventoryItem(Items::arrow);
        // Java: triggerAchievement(StatList.objectUseStats[...]) -- stage 12.
    }
}

std::shared_ptr<ItemStack> ItemFishingRod::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    // Java: with a hook out, reels it in (fishEntity.handleHookRetraction) -- the hook entity, stage 10; so there
    // is never one out here.
    playThrowSound(worldIn, playerIn);
    // Java: spawns an EntityFishHook (server side) -- stage 10.
    playerIn->swingItem();
    return itemStackIn;
}

bool ItemFlintAndSteel::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing side, float, float, float)
{
    const BlockPos pos = posIn.offset(side);
    if (!playerIn->canPlayerEdit(pos, side, &stack))
        return false;

    if (&worldIn->getBlockState(pos)->getBlock()->getMaterial() == &Material::air) {
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "fire.ignite", 1.0f, itemRand().nextFloat() * 0.4f + 0.8f);
        worldIn->setBlockState(pos, Blocks::fire->getDefaultState());
    }

    damageItem(stack, 1, playerIn);
    return true;
}

bool ItemReed::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing sideIn, float hitX, float hitY, float hitZ)
{
    BlockPos pos = posIn;
    EnumFacing side = sideIn;
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    Block* clicked = iblockstate->getBlock();

    if (clicked == Blocks::snow_layer && iblockstate->getValue(BlockSnow::LAYERS) < 1)
        side = EnumFacing::UP;
    else if (!clicked->isReplaceable(worldIn, pos))
        pos = pos.offset(side);

    if (!playerIn->canPlayerEdit(pos, side, &stack))
        return false;
    if (stack.stackSize == 0)
        return false;

    if (worldIn->canBlockBePlaced(block, pos, false, side, nullptr, &stack)) {
        const IBlockState* iblockstate1 = block->onBlockPlaced(worldIn, pos, side, hitX, hitY, hitZ, 0, playerIn);
        if (worldIn->setBlockState(pos, iblockstate1, 3)) {
            iblockstate1 = worldIn->getBlockState(pos);
            if (iblockstate1->getBlock() == block) {
                ItemBlock::setTileEntityNBT(worldIn, playerIn, pos, stack);
                iblockstate1->getBlock()->onBlockPlacedBy(worldIn, pos, iblockstate1, playerIn, stack);
            }
            const Block::SoundType& sound = *block->stepSound;
            worldIn->playSoundEffect(static_cast<double>(static_cast<float>(pos.getX()) + 0.5f), static_cast<double>(static_cast<float>(pos.getY()) + 0.5f),
                static_cast<double>(static_cast<float>(pos.getZ()) + 0.5f), sound.getPlaceSound(), (sound.getVolume() + 1.0f) / 2.0f,
                sound.getFrequency() * 0.8f);
            --stack.stackSize;
            return true;
        }
    }

    return false;
}

bool ItemDoor::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing side, float, float, float)
{
    if (side != EnumFacing::UP)
        return false;

    BlockPos pos = posIn;
    if (!worldIn->getBlockState(pos)->getBlock()->isReplaceable(worldIn, pos))
        pos = pos.offset(side);

    if (!playerIn->canPlayerEdit(pos, side, &stack))
        return false;
    if (!block->canPlaceBlockAt(worldIn, pos))
        return false;

    placeDoor(worldIn, pos, fromAngle(static_cast<double>(playerIn->rotationYaw)), block);
    --stack.stackSize;
    return true;
}

bool ItemMinecart::onItemUse(ItemStack& stack, EntityPlayer*, World* worldIn, const BlockPos& pos, EnumFacing, float, float, float)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (!BlockRailBase::isRailBlock(iblockstate))
        return false;

    // Java: spawns EntityMinecart.getMinecart(...) on the rail (half a block higher on a slope), named after the
    // stack -- the minecart entities, stage 10.
    --stack.stackSize;
    return true;
}

std::shared_ptr<ItemStack> ItemBucket::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    const bool flag = isFull == Blocks::air;
    const std::optional<MovingObjectPosition> movingobjectposition = getMovingObjectPositionFromPlayer(worldIn, playerIn, flag);
    if (!movingobjectposition)
        return itemStackIn;

    if (movingobjectposition->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK) {
        const BlockPos blockpos = *movingobjectposition->getBlockPos();
        const EnumFacing sideHit = *movingobjectposition->sideHit;

        if (!worldIn->isBlockModifiable(playerIn, blockpos))
            return itemStackIn;

        if (flag) {
            if (!playerIn->canPlayerEdit(blockpos.offset(sideHit), sideHit, itemStackIn.get()))
                return itemStackIn;

            const IBlockState* iblockstate = worldIn->getBlockState(blockpos);
            const Material& material = iblockstate->getBlock()->getMaterial();

            if (&material == &Material::water && iblockstate->getValue(BlockLiquid::LEVEL) == 0) {
                worldIn->setBlockToAir(blockpos);
                // Java: triggerAchievement(StatList.objectUseStats[...]) -- stage 12.
                return fillBucket(itemStackIn, playerIn, Items::water_bucket);
            }

            if (&material == &Material::lava && iblockstate->getValue(BlockLiquid::LEVEL) == 0) {
                worldIn->setBlockToAir(blockpos);
                return fillBucket(itemStackIn, playerIn, Items::lava_bucket);
            }
        } else {
            if (isFull == Blocks::air)
                return std::make_shared<ItemStack>(Items::bucket);

            const BlockPos blockpos1 = blockpos.offset(sideHit);
            if (!playerIn->canPlayerEdit(blockpos1, sideHit, itemStackIn.get()))
                return itemStackIn;

            if (tryPlaceContainedLiquid(worldIn, blockpos1) && !playerIn->capabilities.isCreativeMode)
                return std::make_shared<ItemStack>(Items::bucket);
        }
    }

    return itemStackIn;
}

std::shared_ptr<ItemStack> ItemBucket::fillBucket(const std::shared_ptr<ItemStack>& emptyBuckets, EntityPlayer* player, Item* fullBucket)
{
    if (player->capabilities.isCreativeMode)
        return emptyBuckets;
    if (--emptyBuckets->stackSize <= 0)
        return std::make_shared<ItemStack>(fullBucket);
    giveOrDrop(player, fullBucket);
    return emptyBuckets;
}

bool ItemBucket::tryPlaceContainedLiquid(World* worldIn, const BlockPos& pos)
{
    if (isFull == Blocks::air)
        return false;

    const Material& material = worldIn->getBlockState(pos)->getBlock()->getMaterial();
    const bool flag = !material.isSolid();

    if (!worldIn->isAirBlock(pos) && !flag)
        return false;

    if (worldIn->provider->doesWaterVaporize() && isFull == Blocks::flowing_water) {
        const int i = pos.getX();
        const int j = pos.getY();
        const int k = pos.getZ();
        const float r1 = worldIn->rand.nextFloat();
        const float r2 = worldIn->rand.nextFloat();
        worldIn->playSoundEffect(static_cast<double>(static_cast<float>(i) + 0.5f), static_cast<double>(static_cast<float>(j) + 0.5f),
            static_cast<double>(static_cast<float>(k) + 0.5f), "random.fizz", 0.5f, 2.6f + (r1 - r2) * 0.8f);
        for (int l = 0; l < 8; ++l) {
            const double x = i + mathRandom();
            const double y = j + mathRandom();
            const double z = k + mathRandom();
            spawnParticle(worldIn, EnumParticleTypes::SMOKE_LARGE, x, y, z, 0.0, 0.0, 0.0);
        }
    } else {
        if (!worldIn->isRemote && flag && !material.isLiquid())
            worldIn->destroyBlock(pos, true);
        worldIn->setBlockState(pos, isFull->getDefaultState(), 3);
    }

    return true;
}

std::shared_ptr<ItemStack> ItemBucketMilk::onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn)
{
    if (!playerIn->capabilities.isCreativeMode)
        --stack->stackSize;
    if (!worldIn->isRemote)
        playerIn->clearActivePotions();
    // Java: triggerAchievement(StatList.objectUseStats[...]) -- stage 12.
    return stack->stackSize <= 0 ? std::make_shared<ItemStack>(Items::bucket) : stack;
}

std::shared_ptr<ItemStack> ItemBucketMilk::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World*, EntityPlayer* playerIn)
{
    playerIn->setItemInUse(itemStackIn, getMaxItemUseDuration(*itemStackIn));
    return itemStackIn;
}

bool ItemHangingEntity::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World*, const BlockPos& pos, EnumFacing side, float, float, float)
{
    if (side == EnumFacing::DOWN || side == EnumFacing::UP)
        return false;

    const BlockPos blockpos = pos.offset(side);
    if (!playerIn->canPlayerEdit(blockpos, side, &stack))
        return false;

    // Java: creates the EntityPainting / EntityItemFrame and, when it has a valid surface, spawns it and uses up
    // the stack -- the hanging entities, stage 10.
    return true;
}

bool ItemSign::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing side, float, float, float)
{
    if (side == EnumFacing::DOWN)
        return false;
    if (!worldIn->getBlockState(posIn)->getBlock()->getMaterial().isSolid())
        return false;

    const BlockPos pos = posIn.offset(side);
    if (!playerIn->canPlayerEdit(pos, side, &stack))
        return false;
    if (!Blocks::standing_sign->canPlaceBlockAt(worldIn, pos))
        return false;
    if (worldIn->isRemote)
        return true;

    if (side == EnumFacing::UP) {
        const int i = MathHelper::floor_double(static_cast<double>((playerIn->rotationYaw + 180.0f) * 16.0f / 360.0f) + 0.5) & 15;
        worldIn->setBlockState(pos, Blocks::standing_sign->getDefaultState()->withProperty(BlockStandingSign::ROTATION, i), 3);
    } else {
        worldIn->setBlockState(pos, Blocks::wall_sign->getDefaultState()->withProperty(BlockWallSign::FACING, side), 3);
    }

    --stack.stackSize;
    if (auto* tileentity = dynamic_cast<TileEntitySign*>(worldIn->getTileEntity(pos))) {
        if (!ItemBlock::setTileEntityNBT(worldIn, playerIn, pos, stack))
            playerIn->openEditSign(tileentity);
    }
    return true;
}

bool ItemBed::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing side, float, float, float)
{
    if (worldIn->isRemote)
        return true;
    if (side != EnumFacing::UP)
        return false;

    BlockPos pos = posIn;
    const bool flag = worldIn->getBlockState(pos)->getBlock()->isReplaceable(worldIn, pos);
    if (!flag)
        pos = pos.up();

    const int i = MathHelper::floor_double(static_cast<double>(playerIn->rotationYaw * 4.0f / 360.0f) + 0.5) & 3;
    const EnumFacing enumfacing = getHorizontal(i);
    const BlockPos blockpos = pos.offset(enumfacing);

    if (!playerIn->canPlayerEdit(pos, side, &stack) || !playerIn->canPlayerEdit(blockpos, side, &stack))
        return false;

    const bool flag1 = worldIn->getBlockState(blockpos)->getBlock()->isReplaceable(worldIn, blockpos);
    const bool flag2 = flag || worldIn->isAirBlock(pos);
    const bool flag3 = flag1 || worldIn->isAirBlock(blockpos);

    if (flag2 && flag3 && World::doesBlockHaveSolidTopSurface(worldIn, pos.down()) && World::doesBlockHaveSolidTopSurface(worldIn, blockpos.down())) {
        const IBlockState* iblockstate1 = Blocks::bed->getDefaultState()
                                              ->withProperty(BlockBed::OCCUPIED, false)
                                              ->withProperty(BlockBed::FACING, enumfacing)
                                              ->withProperty(BlockBed::PART, BlockBed::EnumPartType::FOOT);
        if (worldIn->setBlockState(pos, iblockstate1, 3)) {
            const IBlockState* iblockstate2 = iblockstate1->withProperty(BlockBed::PART, BlockBed::EnumPartType::HEAD);
            worldIn->setBlockState(blockpos, iblockstate2, 3);
        }
        --stack.stackSize;
        return true;
    }

    return false;
}

bool ItemRedstone::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float, float, float)
{
    const bool flag = worldIn->getBlockState(pos)->getBlock()->isReplaceable(worldIn, pos);
    const BlockPos blockpos = flag ? pos : pos.offset(side);

    if (!playerIn->canPlayerEdit(blockpos, side, &stack))
        return false;

    Block* block = worldIn->getBlockState(blockpos)->getBlock();
    if (!worldIn->canBlockBePlaced(block, blockpos, false, side, nullptr, &stack))
        return false;

    if (Blocks::redstone_wire->canPlaceBlockAt(worldIn, blockpos)) {
        --stack.stackSize;
        worldIn->setBlockState(blockpos, Blocks::redstone_wire->getDefaultState());
        return true;
    }

    return false;
}

std::shared_ptr<ItemStack> ItemSnowball::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    if (!playerIn->capabilities.isCreativeMode)
        --itemStackIn->stackSize;
    playThrowSound(worldIn, playerIn);
    // Java: spawns an EntitySnowball (server side) -- stage 10.
    return itemStackIn;
}

std::shared_ptr<ItemStack> ItemEgg::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    if (!playerIn->capabilities.isCreativeMode)
        --itemStackIn->stackSize;
    playThrowSound(worldIn, playerIn);
    // Java: spawns an EntityEgg (server side) -- stage 10.
    return itemStackIn;
}

std::shared_ptr<ItemStack> ItemEnderPearl::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    if (playerIn->capabilities.isCreativeMode)
        return itemStackIn;
    --itemStackIn->stackSize;
    playThrowSound(worldIn, playerIn);
    // Java: spawns an EntityEnderPearl (server side) -- stage 10.
    return itemStackIn;
}

std::shared_ptr<ItemStack> ItemExpBottle::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    if (!playerIn->capabilities.isCreativeMode)
        --itemStackIn->stackSize;
    playThrowSound(worldIn, playerIn);
    // Java: spawns an EntityExpBottle (server side) -- stage 10.
    return itemStackIn;
}

std::shared_ptr<ItemStack> ItemBoat::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    const float f = 1.0f;
    const float f1 = playerIn->prevRotationPitch + (playerIn->rotationPitch - playerIn->prevRotationPitch) * f;
    const float f2 = playerIn->prevRotationYaw + (playerIn->rotationYaw - playerIn->prevRotationYaw) * f;
    const double d0 = playerIn->prevPosX + (playerIn->posX - playerIn->prevPosX) * static_cast<double>(f);
    const double d1 = playerIn->prevPosY + (playerIn->posY - playerIn->prevPosY) * static_cast<double>(f) + static_cast<double>(playerIn->getEyeHeight());
    const double d2 = playerIn->prevPosZ + (playerIn->posZ - playerIn->prevPosZ) * static_cast<double>(f);
    const Vec3 vec3(d0, d1, d2);
    const float f3 = MathHelper::cos(-f2 * 0.017453292f - 3.1415927f);
    const float f4 = MathHelper::sin(-f2 * 0.017453292f - 3.1415927f);
    const float f5 = -MathHelper::cos(-f1 * 0.017453292f);
    const float f6 = MathHelper::sin(-f1 * 0.017453292f);
    const float f7 = f4 * f5;
    const float f8 = f3 * f5;
    const double d3 = 5.0;
    const Vec3 vec31 = vec3.addVector(static_cast<double>(f7) * d3, static_cast<double>(f6) * d3, static_cast<double>(f8) * d3);
    const std::optional<MovingObjectPosition> movingobjectposition = worldIn->rayTraceBlocks(vec3, vec31, true);

    if (!movingobjectposition)
        return itemStackIn;

    const Vec3 vec32 = playerIn->getLook(f);
    bool flag = false;
    const float f9 = 1.0f;
    const std::vector<Entity*> list = worldIn->getEntitiesWithinAABBExcludingEntity(playerIn,
        playerIn->getEntityBoundingBox().addCoord(vec32.xCoord * d3, vec32.yCoord * d3, vec32.zCoord * d3).expand(f9, f9, f9));

    for (Entity* entity : list) {
        if (entity->canBeCollidedWith()) {
            const float f10 = entity->getCollisionBorderSize();
            const AxisAlignedBB axisalignedbb = entity->getEntityBoundingBox().expand(f10, f10, f10);
            if (axisalignedbb.isVecInside(vec3))
                flag = true;
        }
    }

    if (flag)
        return itemStackIn;

    if (movingobjectposition->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK) {
        BlockPos blockpos = *movingobjectposition->getBlockPos();
        if (worldIn->getBlockState(blockpos)->getBlock() == Blocks::snow_layer)
            blockpos = blockpos.down();

        // Java: new EntityBoat at the top middle of the block, turned to the player; refused when its box
        // (1.5 x 0.6, shrunk by 0.1) collides. Without the boat entity (stage 10) the box is the boat's own.
        const double x = static_cast<double>(static_cast<float>(blockpos.getX()) + 0.5f);
        const double y = static_cast<double>(static_cast<float>(blockpos.getY()) + 1.0f);
        const double z = static_cast<double>(static_cast<float>(blockpos.getZ()) + 0.5f);
        const AxisAlignedBB boatBox = AxisAlignedBB(x - 0.75, y, z - 0.75, x + 0.75, y + 0.6000000238418579, z + 0.75).expand(-0.1, -0.1, -0.1);
        if (!worldIn->getCollisionBoxes(boatBox).empty())
            return itemStackIn;

        // Java: spawns the boat (server side) -- stage 10.
        if (!playerIn->capabilities.isCreativeMode)
            --itemStackIn->stackSize;
        // Java: triggerAchievement(StatList.objectUseStats[...]) -- stage 12.
    }

    return itemStackIn;
}

bool ItemEnderEye::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float, float, float)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);

    if (!(playerIn->canPlayerEdit(pos.offset(side), side, &stack) && iblockstate->getBlock() == Blocks::end_portal_frame
            && !iblockstate->getValue(BlockEndPortalFrame::EYE)))
        return false;

    if (worldIn->isRemote)
        return true;

    worldIn->setBlockState(pos, iblockstate->withProperty(BlockEndPortalFrame::EYE, true), 2);
    worldIn->updateComparatorOutputLevel(pos, Blocks::end_portal_frame);
    --stack.stackSize;

    for (int i = 0; i < 16; ++i) {
        const double d0 = static_cast<double>(static_cast<float>(pos.getX()) + (5.0f + itemRand().nextFloat() * 6.0f) / 16.0f);
        const double d1 = static_cast<double>(static_cast<float>(pos.getY()) + 0.8125f);
        const double d2 = static_cast<double>(static_cast<float>(pos.getZ()) + (5.0f + itemRand().nextFloat() * 6.0f) / 16.0f);
        spawnParticle(worldIn, EnumParticleTypes::SMOKE_NORMAL, d0, d1, d2, 0.0, 0.0, 0.0);
    }

    const EnumFacing enumfacing = iblockstate->getValue(BlockEndPortalFrame::FACING);
    int l = 0;
    int j = 0;
    bool flag1 = false;
    bool flag = true;
    const EnumFacing enumfacing1 = rotateY(enumfacing);

    for (int k = -2; k <= 2; ++k) {
        const BlockPos blockpos1 = pos.offset(enumfacing1, k);
        const IBlockState* iblockstate1 = worldIn->getBlockState(blockpos1);
        if (iblockstate1->getBlock() == Blocks::end_portal_frame) {
            if (!iblockstate1->getValue(BlockEndPortalFrame::EYE)) {
                flag = false;
                break;
            }
            j = k;
            if (!flag1) {
                l = k;
                flag1 = true;
            }
        }
    }

    if (flag && j == l + 2) {
        BlockPos blockpos = pos.offset(enumfacing, 4);

        for (int i1 = l; i1 <= j; ++i1) {
            const BlockPos blockpos2 = blockpos.offset(enumfacing1, i1);
            const IBlockState* iblockstate3 = worldIn->getBlockState(blockpos2);
            if (iblockstate3->getBlock() != Blocks::end_portal_frame || !iblockstate3->getValue(BlockEndPortalFrame::EYE)) {
                flag = false;
                break;
            }
        }

        for (int j1 = l - 1; j1 <= j + 1; j1 += 4) {
            blockpos = pos.offset(enumfacing1, j1);
            for (int l1 = 1; l1 <= 3; ++l1) {
                const BlockPos blockpos3 = blockpos.offset(enumfacing, l1);
                const IBlockState* iblockstate2 = worldIn->getBlockState(blockpos3);
                if (iblockstate2->getBlock() != Blocks::end_portal_frame || !iblockstate2->getValue(BlockEndPortalFrame::EYE)) {
                    flag = false;
                    break;
                }
            }
        }

        if (flag) {
            for (int k1 = l; k1 <= j; ++k1) {
                blockpos = pos.offset(enumfacing1, k1);
                for (int i2 = 1; i2 <= 3; ++i2) {
                    const BlockPos blockpos4 = blockpos.offset(enumfacing, i2);
                    worldIn->setBlockState(blockpos4, Blocks::end_portal->getDefaultState(), 2);
                }
            }
        }
    }

    return true;
}

std::shared_ptr<ItemStack> ItemEnderEye::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    const std::optional<MovingObjectPosition> movingobjectposition = getMovingObjectPositionFromPlayer(worldIn, playerIn, false);
    if (movingobjectposition && movingobjectposition->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK
        && worldIn->getBlockState(*movingobjectposition->getBlockPos())->getBlock() == Blocks::end_portal_frame)
        return itemStackIn;

    if (!worldIn->isRemote) {
        const BlockPos playerPos(playerIn->posX, playerIn->posY, playerIn->posZ);
        // Java: World.getStrongholdPos
        const std::optional<BlockPos> blockpos = worldIn->getChunkProvider()->getStrongholdGen(worldIn, "Stronghold", playerPos);
        if (blockpos) {
            // Java: spawns an EntityEnderEye flying towards the stronghold -- stage 10.
            playThrowSound(worldIn, playerIn);
            worldIn->playAuxSFXAtEntity(nullptr, 1002, playerPos, 0);
            if (!playerIn->capabilities.isCreativeMode)
                --itemStackIn->stackSize;
            // Java: triggerAchievement(StatList.objectUseStats[...]) -- stage 12.
        }
    }

    return itemStackIn;
}

std::shared_ptr<ItemStack> ItemGlassBottle::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
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

        if (&worldIn->getBlockState(blockpos)->getBlock()->getMaterial() == &Material::water) {
            --itemStackIn->stackSize;
            // Java: triggerAchievement(StatList.objectUseStats[...]) -- stage 12.
            if (itemStackIn->stackSize <= 0)
                return std::make_shared<ItemStack>(Items::potionitem);
            giveOrDrop(playerIn, Items::potionitem);
        }
    }

    return itemStackIn;
}

bool ItemFireball::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing side, float, float, float)
{
    if (worldIn->isRemote)
        return true;

    const BlockPos pos = posIn.offset(side);
    if (!playerIn->canPlayerEdit(pos, side, &stack))
        return false;

    if (&worldIn->getBlockState(pos)->getBlock()->getMaterial() == &Material::air) {
        const float r1 = itemRand().nextFloat();
        const float r2 = itemRand().nextFloat();
        worldIn->playSoundEffect(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5, "item.fireCharge.use", 1.0f, (r1 - r2) * 0.2f + 1.0f);
        worldIn->setBlockState(pos, Blocks::fire->getDefaultState());
    }

    if (!playerIn->capabilities.isCreativeMode)
        --stack.stackSize;
    return true;
}

bool ItemLead::onItemUse(ItemStack&, EntityPlayer*, World* worldIn, const BlockPos& pos, EnumFacing, float, float, float)
{
    if (!dynamic_cast<BlockFence*>(worldIn->getBlockState(pos)->getBlock()))
        return false;
    // Java: attachToFence ties the leashed mobs within 7 blocks to a leash knot -- the mobs and the knot, stage 10.
    return true;
}

bool ItemArmorStand::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float, float, float)
{
    if (side == EnumFacing::DOWN)
        return false;

    const bool flag = worldIn->getBlockState(pos)->getBlock()->isReplaceable(worldIn, pos);
    const BlockPos blockpos = flag ? pos : pos.offset(side);

    if (!playerIn->canPlayerEdit(blockpos, side, &stack))
        return false;

    const BlockPos blockpos1 = blockpos.up();
    bool flag1 = !worldIn->isAirBlock(blockpos) && !worldIn->getBlockState(blockpos)->getBlock()->isReplaceable(worldIn, blockpos);
    flag1 = flag1 | (!worldIn->isAirBlock(blockpos1) && !worldIn->getBlockState(blockpos1)->getBlock()->isReplaceable(worldIn, blockpos1));

    if (flag1)
        return false;

    const double d0 = blockpos.getX();
    const double d1 = blockpos.getY();
    const double d2 = blockpos.getZ();
    if (!worldIn->getEntitiesWithinAABBExcludingEntity(nullptr, AxisAlignedBB(d0, d1, d2, d0 + 1.0, d1 + 2.0, d2 + 1.0)).empty())
        return false;

    if (!worldIn->isRemote) {
        worldIn->setBlockToAir(blockpos);
        worldIn->setBlockToAir(blockpos1);
        // Java: new EntityArmorStand facing the player in 45-degree steps, with random head and body turns
        // (applyRandomRotations) and the stack's EntityTag -- the armor stand, stage 10. The three draws of
        // applyRandomRotations are kept.
        worldIn->rand.nextFloat();
        worldIn->rand.nextFloat();
        worldIn->rand.nextFloat();
    }

    --stack.stackSize;
    return true;
}

bool ItemMonsterPlacer::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float, float, float)
{
    if (worldIn->isRemote)
        return true;
    if (!playerIn->canPlayerEdit(pos.offset(side), side, &stack))
        return false;

    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() == Blocks::mob_spawner) {
        if (auto* tileentity = dynamic_cast<TileEntityMobSpawner*>(worldIn->getTileEntity(pos))) {
            tileentity->getSpawnerBaseLogic().setEntityName(EntityList::getStringFromID(stack.getMetadata()).value_or(""));
            tileentity->markDirty();
            worldIn->markBlockForUpdate(pos);
            if (!playerIn->capabilities.isCreativeMode)
                --stack.stackSize;
            return true;
        }
    }

    // Java: spawnCreature at the clicked side (it uses up the stack only when a mob was made) -- the mobs, stage 10.
    return true;
}

std::shared_ptr<ItemStack> ItemMonsterPlacer::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World* worldIn, EntityPlayer* playerIn)
{
    if (worldIn->isRemote)
        return itemStackIn;

    const std::optional<MovingObjectPosition> movingobjectposition = getMovingObjectPositionFromPlayer(worldIn, playerIn, true);
    if (!movingobjectposition)
        return itemStackIn;

    if (movingobjectposition->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK) {
        const BlockPos blockpos = *movingobjectposition->getBlockPos();
        if (!worldIn->isBlockModifiable(playerIn, blockpos))
            return itemStackIn;
        if (!playerIn->canPlayerEdit(blockpos, *movingobjectposition->sideHit, itemStackIn.get()))
            return itemStackIn;
        // Java: spawnCreature in the liquid looked at -- the mobs, stage 10.
    }

    return itemStackIn;
}

bool ItemSkull::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing side, float, float, float)
{
    if (side == EnumFacing::DOWN)
        return false;

    BlockPos pos = posIn;
    const bool flag = worldIn->getBlockState(pos)->getBlock()->isReplaceable(worldIn, pos);
    if (!flag) {
        if (!worldIn->getBlockState(pos)->getBlock()->getMaterial().isSolid())
            return false;
        pos = pos.offset(side);
    }

    if (!playerIn->canPlayerEdit(pos, side, &stack))
        return false;
    if (!Blocks::skull->canPlaceBlockAt(worldIn, pos))
        return false;

    if (!worldIn->isRemote) {
        worldIn->setBlockState(pos, Blocks::skull->getDefaultState()->withProperty(BlockSkull::FACING, side), 3);
        int i = 0;
        if (side == EnumFacing::UP)
            i = MathHelper::floor_double(static_cast<double>(playerIn->rotationYaw * 16.0f / 360.0f) + 0.5) & 15;

        if (auto* tileentityskull = dynamic_cast<TileEntitySkull*>(worldIn->getTileEntity(pos))) {
            if (stack.getMetadata() == 3) {
                std::optional<GameProfile> gameprofile;
                if (stack.hasTagCompound()) {
                    const std::shared_ptr<NBTTagCompound>& nbttagcompound = stack.getTagCompound();
                    if (nbttagcompound->hasKey("SkullOwner", 10))
                        gameprofile = TileEntitySkull::readGameProfileFromNBT(*nbttagcompound->getCompoundTag("SkullOwner"));
                    else if (nbttagcompound->hasKey("SkullOwner", 8) && !nbttagcompound->getString("SkullOwner").empty())
                        gameprofile = GameProfile(std::nullopt, nbttagcompound->getString("SkullOwner"));
                }
                tileentityskull->setPlayerProfile(gameprofile);
            } else {
                tileentityskull->setType(stack.getMetadata());
            }
            tileentityskull->setSkullRotation(i);
            static_cast<BlockSkull*>(Blocks::skull)->checkWitherSpawn(worldIn, pos, tileentityskull);
        }

        --stack.stackSize;
    }

    return true;
}

bool ItemFirework::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos&, EnumFacing, float, float, float)
{
    if (worldIn->isRemote)
        return false;
    // Java: spawns an EntityFireworkRocket at the clicked point -- stage 10.
    if (!playerIn->capabilities.isCreativeMode)
        --stack.stackSize;
    return true;
}

bool ItemDye::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& posIn, EnumFacing side, float hitX, float hitY, float hitZ)
{
    if (!playerIn->canPlayerEdit(posIn.offset(side), side, &stack))
        return false;

    const EnumDyeColor enumdyecolor = dyeColorByDyeDamage(stack.getMetadata());

    if (enumdyecolor == EnumDyeColor::WHITE) {
        if (applyBonemeal(stack, worldIn, posIn)) {
            if (!worldIn->isRemote)
                worldIn->playAuxSFX(2005, posIn, 0);
            return true;
        }
    } else if (enumdyecolor == EnumDyeColor::BROWN) {
        const IBlockState* iblockstate = worldIn->getBlockState(posIn);
        if (iblockstate->getBlock() == Blocks::log && iblockstate->getValue(BlockPlanks::VARIANT) == BlockPlanks::EnumType::JUNGLE) {
            if (side == EnumFacing::DOWN || side == EnumFacing::UP)
                return false;

            const BlockPos pos = posIn.offset(side);
            if (worldIn->isAirBlock(pos)) {
                const IBlockState* iblockstate1 = Blocks::cocoa->onBlockPlaced(worldIn, pos, side, hitX, hitY, hitZ, 0, playerIn);
                worldIn->setBlockState(pos, iblockstate1, 2);
                if (!playerIn->capabilities.isCreativeMode)
                    --stack.stackSize;
            }
            return true;
        }
    }

    return false;
}

namespace {

// Java: (IGrowable) block -- the growable blocks implement canGrow / canUseBonemeal / grow without a common base
// here, so they are tried one by one.
template <class... Growables>
std::optional<bool> tryBonemeal(Block* block, ItemStack& stack, World* worldIn, const BlockPos& target, const IBlockState* iblockstate)
{
    std::optional<bool> result;
    const auto attempt = [&](auto* igrowable) {
        if (result || !igrowable)
            return;
        if (!igrowable->canGrow(worldIn, target, iblockstate, worldIn->isRemote)) {
            result = false;
            return;
        }
        if (!worldIn->isRemote) {
            if (igrowable->canUseBonemeal(worldIn, worldIn->rand, target, iblockstate))
                igrowable->grow(worldIn, worldIn->rand, target, iblockstate);
            --stack.stackSize;
        }
        result = true;
    };
    (attempt(dynamic_cast<Growables*>(block)), ...);
    return result;
}

} // namespace

bool ItemDye::applyBonemeal(ItemStack& stack, World* worldIn, const BlockPos& target)
{
    const IBlockState* iblockstate = worldIn->getBlockState(target);
    // Java: BlockDoublePlant is an IGrowable too -- its canGrow / grow are not ported yet.
    return tryBonemeal<BlockCocoa, BlockCrops, BlockGrass, BlockTallGrass, BlockMushroom, BlockSapling, BlockStem>(
        iblockstate->getBlock(), stack, worldIn, target, iblockstate)
        .value_or(false);
}

void ItemDye::spawnBonemealParticles(World* worldIn, const BlockPos& pos, int amount)
{
    if (amount == 0)
        amount = 15;

    Block* block = worldIn->getBlockState(pos)->getBlock();
    if (&block->getMaterial() == &Material::air)
        return;

    block->setBlockBoundsBasedOnState(worldIn, pos);
    Random& rand = itemRand();

    for (int i = 0; i < amount; ++i) {
        const double d0 = rand.nextGaussian() * 0.02;
        const double d1 = rand.nextGaussian() * 0.02;
        const double d2 = rand.nextGaussian() * 0.02;
        const double x = static_cast<double>(static_cast<float>(pos.getX()) + rand.nextFloat());
        const double y = static_cast<double>(pos.getY()) + static_cast<double>(rand.nextFloat()) * block->getBlockBoundsMaxY();
        const double z = static_cast<double>(static_cast<float>(pos.getZ()) + rand.nextFloat());
        spawnParticle(worldIn, EnumParticleTypes::VILLAGER_HAPPY, x, y, z, d0, d1, d2);
    }
}

bool ItemRecord::onItemUse(ItemStack& stack, EntityPlayer*, World* worldIn, const BlockPos& pos, EnumFacing, float, float, float)
{
    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    if (iblockstate->getBlock() != Blocks::jukebox || iblockstate->getValue(BlockJukebox::HAS_RECORD))
        return false;
    if (worldIn->isRemote)
        return true;

    static_cast<BlockJukebox*>(Blocks::jukebox)->insertRecord(worldIn, pos, iblockstate, stack);
    worldIn->playAuxSFXAtEntity(nullptr, 1005, pos, Item::getIdFromItem(this));
    --stack.stackSize;
    // Java: triggerAchievement(StatList.field_181740_X) -- stage 12.
    return true;
}

} // namespace mc
