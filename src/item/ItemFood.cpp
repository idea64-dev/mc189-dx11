#include "item/ItemFood.h"

#include "block/Block.h"
#include "creativetab/CreativeTabs.h"
#include "entity/player/EntityPlayer.h"
#include "init/Items.h"
#include "item/ItemStack.h"
#include "potion/Potion.h"
#include "potion/PotionHelper.h"
#include "world/World.h"

namespace mc {

ItemFood::ItemFood(int amount, float saturation, bool isWolfFood)
    : healAmount(amount)
    , saturationModifier(saturation)
    , wolfsFavoriteMeat(isWolfFood)
{
    setCreativeTab(&CreativeTabs::tabFood);
}

// --- ItemAppleGold ---

bool ItemAppleGold::hasEffect(const ItemStack& stack) const
{
    return stack.getMetadata() > 0;
}

EnumRarity ItemAppleGold::getRarity(const ItemStack& stack) const
{
    return stack.getMetadata() == 0 ? EnumRarity::RARE : EnumRarity::EPIC;
}

void ItemAppleGold::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    subItems.emplace_back(item, 1, 0);
    subItems.emplace_back(item, 1, 1);
}

// --- ItemFishFood ---

ItemFishFood::FishType ItemFishFood::byItemStack(const ItemStack& stack)
{
    return dynamic_cast<const ItemFishFood*>(stack.getItem()) ? byMetadata(stack.getMetadata()) : FishType::COD;
}

int ItemFishFood::getHealAmount(const ItemStack& stack) const
{
    const FishTypeInfo& type = info(byItemStack(stack));
    return cooked && type.cookable ? type.cookedHealAmount : type.uncookedHealAmount;
}

float ItemFishFood::getSaturationModifier(const ItemStack& stack) const
{
    const FishTypeInfo& type = info(byItemStack(stack));
    return cooked && type.cookable ? type.cookedSaturationModifier : type.uncookedSaturationModifier;
}

std::optional<std::string> ItemFishFood::getPotionEffect(const ItemStack& stack) const
{
    if (byItemStack(stack) == FishType::PUFFERFISH)
        return std::string(PotionHelper::pufferfishEffect);
    return std::nullopt;
}

void ItemFishFood::getSubItems(Item*, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (const FishTypeInfo& type : kFishTypes) {
        if (!cooked || type.cookable)
            subItems.emplace_back(const_cast<ItemFishFood*>(this), 1, type.meta);
    }
}

std::string ItemFishFood::getUnlocalizedName(const ItemStack& stack) const
{
    const FishTypeInfo& type = info(byItemStack(stack));
    return getUnlocalizedName() + "." + std::string(type.unlocalizedName) + "." + (cooked && type.cookable ? "cooked" : "raw");
}

// --- ItemSeeds ---

ItemSeeds::ItemSeeds(Block* cropsIn, Block* soil)
    : crops(cropsIn)
    , soilBlockID(soil)
{
    setCreativeTab(&CreativeTabs::tabMaterials);
}

std::shared_ptr<ItemStack> ItemFood::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World*, EntityPlayer* playerIn)
{
    if (playerIn->canEat(alwaysEdible))
        playerIn->setItemInUse(itemStackIn, getMaxItemUseDuration(*itemStackIn));
    return itemStackIn;
}

std::shared_ptr<ItemStack> ItemFood::onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn)
{
    --stack->stackSize;
    playerIn->getFoodStats().addStats(*this, *stack);
    worldIn->playSoundAtEntity(playerIn, "random.burp", 0.5f, worldIn->rand.nextFloat() * 0.1f + 0.9f);
    onFoodEaten(*stack, worldIn, playerIn);
    // Java: triggerAchievement(StatList.objectUseStats[...]) -- stage 12.
    return stack;
}

void ItemFood::onFoodEaten(ItemStack&, World* worldIn, EntityPlayer* player)
{
    if (!worldIn->isRemote && potionId > 0 && worldIn->rand.nextFloat() < potionEffectProbability)
        player->addPotionEffect(PotionEffect(potionId, potionDuration * 20, potionAmplifier));
}

void ItemAppleGold::onFoodEaten(ItemStack& stack, World* worldIn, EntityPlayer* player)
{
    if (!worldIn->isRemote)
        player->addPotionEffect(PotionEffect(Potion::absorption->id, 2400, 0));

    if (stack.getMetadata() > 0) {
        if (!worldIn->isRemote) {
            player->addPotionEffect(PotionEffect(Potion::regeneration->id, 600, 4));
            player->addPotionEffect(PotionEffect(Potion::resistance->id, 6000, 0));
            player->addPotionEffect(PotionEffect(Potion::fireResistance->id, 6000, 0));
        }
    } else {
        ItemFood::onFoodEaten(stack, worldIn, player);
    }
}

void ItemFishFood::onFoodEaten(ItemStack& stack, World* worldIn, EntityPlayer* player)
{
    if (byItemStack(stack) == FishType::PUFFERFISH) {
        player->addPotionEffect(PotionEffect(Potion::poison->id, 1200, 3));
        player->addPotionEffect(PotionEffect(Potion::hunger->id, 300, 2));
        player->addPotionEffect(PotionEffect(Potion::confusion->id, 300, 1));
    }
    ItemFood::onFoodEaten(stack, worldIn, player);
}

std::shared_ptr<ItemStack> ItemSoup::onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn)
{
    ItemFood::onItemUseFinish(stack, worldIn, playerIn);
    return std::make_shared<ItemStack>(Items::bowl);
}

namespace {

// ItemSeeds.onItemUse and ItemSeedFood.onItemUse.
bool plant(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, Block* crops, Block* soil)
{
    if (side != EnumFacing::UP)
        return false;
    if (!playerIn->canPlayerEdit(pos.offset(side), side, &stack))
        return false;
    if (worldIn->getBlockState(pos)->getBlock() == soil && worldIn->isAirBlock(pos.up())) {
        worldIn->setBlockState(pos.up(), crops->getDefaultState());
        --stack.stackSize;
        return true;
    }
    return false;
}

} // namespace

bool ItemSeedFood::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float, float, float)
{
    return plant(stack, playerIn, worldIn, pos, side, crops, soilId);
}

bool ItemSeeds::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float, float, float)
{
    return plant(stack, playerIn, worldIn, pos, side, crops, soilBlockID);
}

} // namespace mc
