#pragma once

// Original: net/minecraft/util/WeightedRandom.java and WeightedRandomChestContent.java -- weighted picks and the
// loot tables of generated chests and dispensers.

#include "util/Random.h"

#include <memory>
#include <stdexcept>
#include <vector>

namespace mc {

class IInventory;
class Item;
class ItemStack;
class TileEntityDispenser;

namespace WeightedRandom {

// T has an int itemWeight.
template <class T>
int getTotalWeight(const std::vector<T>& collection)
{
    int i = 0;
    for (const T& item : collection)
        i += item.itemWeight;
    return i;
}

// Null when the weight runs past the end (Java's null).
template <class T>
const T* getRandomItem(const std::vector<T>& collection, int weight)
{
    for (const T& t : collection) {
        weight -= t.itemWeight;
        if (weight < 0)
            return &t;
    }
    return nullptr;
}

template <class T>
const T* getRandomItem(Random& random, const std::vector<T>& collection, int totalWeight)
{
    if (totalWeight <= 0)
        throw std::invalid_argument("WeightedRandom: total weight must be positive");
    return getRandomItem(collection, random.nextInt(totalWeight));
}

template <class T>
const T* getRandomItem(Random& random, const std::vector<T>& collection)
{
    return getRandomItem(random, collection, getTotalWeight(collection));
}

} // namespace WeightedRandom

struct WeightedRandomChestContent {
    WeightedRandomChestContent(Item* item, int meta, int minimumChance, int maximumChance, int itemWeightIn);
    WeightedRandomChestContent(std::shared_ptr<ItemStack> stack, int minimumChance, int maximumChance, int itemWeightIn);

    static void generateChestContents(Random& random, const std::vector<WeightedRandomChestContent>& listIn, IInventory& inv, int max);
    static void generateDispenserContents(Random& random, const std::vector<WeightedRandomChestContent>& listIn, TileEntityDispenser& dispenser, int max);
    // Java: func_177629_a -- the list with more entries appended.
    static std::vector<WeightedRandomChestContent> func_177629_a(std::vector<WeightedRandomChestContent> list,
        std::initializer_list<WeightedRandomChestContent> extra);

    int itemWeight;
    std::shared_ptr<ItemStack> theItemId;
    int minStackSize;
    int maxStackSize;
};

} // namespace mc
