#pragma once

// Original: net/minecraft/util/FoodStats.java -- hunger, saturation and exhaustion.

namespace mc {

class EntityPlayer;
class ItemFood;
class ItemStack;
class NBTTagCompound;

class FoodStats {
public:
    void addStats(int foodLevelIn, float foodSaturationModifier);
    void addStats(const ItemFood& foodItem, const ItemStack& stack);
    void onUpdate(EntityPlayer* player);
    void readNBT(const NBTTagCompound& compound);
    void writeNBT(NBTTagCompound& compound) const;
    int getFoodLevel() const { return foodLevel; }
    int getPrevFoodLevel() const { return prevFoodLevel; }
    bool needFood() const { return foodLevel < 20; }
    void addExhaustion(float exhaustion);
    float getSaturationLevel() const { return foodSaturationLevel; }
    void setFoodLevel(int foodLevelIn) { foodLevel = foodLevelIn; }
    void setFoodSaturationLevel(float value) { foodSaturationLevel = value; }

private:
    int foodLevel = 20;
    float foodSaturationLevel = 5.0f;
    float foodExhaustionLevel = 0.0f;
    int foodTimer = 0;
    int prevFoodLevel = 20;
};

} // namespace mc
