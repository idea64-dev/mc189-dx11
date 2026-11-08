#include "util/FoodStats.h"

#include "entity/player/EntityPlayer.h"
#include "item/ItemFood.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "util/DamageSource.h"
#include "world/EnumDifficulty.h"
#include "world/GameRules.h"
#include "world/World.h"

#include <algorithm>

namespace mc {

void FoodStats::addStats(int foodLevelIn, float foodSaturationModifier)
{
    foodLevel = std::min(foodLevelIn + foodLevel, 20);
    foodSaturationLevel = std::min(foodSaturationLevel + static_cast<float>(foodLevelIn) * foodSaturationModifier * 2.0f, static_cast<float>(foodLevel));
}

void FoodStats::addStats(const ItemFood& foodItem, const ItemStack& stack)
{
    addStats(foodItem.getHealAmount(stack), foodItem.getSaturationModifier(stack));
}

void FoodStats::onUpdate(EntityPlayer* player)
{
    const EnumDifficulty enumdifficulty = player->worldObj->getDifficulty();
    prevFoodLevel = foodLevel;
    if (foodExhaustionLevel > 4.0f) {
        foodExhaustionLevel -= 4.0f;
        if (foodSaturationLevel > 0.0f)
            foodSaturationLevel = std::max(foodSaturationLevel - 1.0f, 0.0f);
        else if (enumdifficulty != EnumDifficulty::PEACEFUL)
            foodLevel = std::max(foodLevel - 1, 0);
    }
    if (player->worldObj->getGameRules().getBoolean("naturalRegeneration") && foodLevel >= 18 && player->shouldHeal()) {
        ++foodTimer;
        if (foodTimer >= 80) {
            player->heal(1.0f);
            addExhaustion(3.0f);
            foodTimer = 0;
        }
    } else if (foodLevel <= 0) {
        ++foodTimer;
        if (foodTimer >= 80) {
            if (player->getHealth() > 10.0f || enumdifficulty == EnumDifficulty::HARD || (player->getHealth() > 1.0f && enumdifficulty == EnumDifficulty::NORMAL))
                player->attackEntityFrom(DamageSource::starve, 1.0f);
            foodTimer = 0;
        }
    } else {
        foodTimer = 0;
    }
}

void FoodStats::readNBT(const NBTTagCompound& compound)
{
    if (compound.hasKey("foodLevel", 99)) {
        foodLevel = compound.getInteger("foodLevel");
        foodTimer = compound.getInteger("foodTickTimer");
        foodSaturationLevel = compound.getFloat("foodSaturationLevel");
        foodExhaustionLevel = compound.getFloat("foodExhaustionLevel");
    }
}

void FoodStats::writeNBT(NBTTagCompound& compound) const
{
    compound.setInteger("foodLevel", foodLevel);
    compound.setInteger("foodTickTimer", foodTimer);
    compound.setFloat("foodSaturationLevel", foodSaturationLevel);
    compound.setFloat("foodExhaustionLevel", foodExhaustionLevel);
}

void FoodStats::addExhaustion(float exhaustion)
{
    foodExhaustionLevel = std::min(foodExhaustionLevel + exhaustion, 40.0f);
}

} // namespace mc
