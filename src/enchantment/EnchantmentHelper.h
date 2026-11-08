#pragma once

// Original: net/minecraft/enchantment/EnchantmentHelper.java
// Ported: reading and writing enchantments, enchantability and the random enchanting used by enchanting tables
// and loot. The entity-facing helpers (damage modifiers, thorns, knockback, fortune of the held item, ...) come
// with entities (stage 10).

#include "enchantment/Enchantment.h"

#include <span>
#include <utility>
#include <vector>

namespace mc {

class ItemStack;
class Random;

namespace EnchantmentHelper {

// 0 for a null stack or one without the enchantment.
int getEnchantmentLevel(int enchID, const ItemStack* stack);
// Java: a LinkedHashMap<Integer, Integer> (id -> level) in tag order.
std::vector<std::pair<int, int>> getEnchantments(const ItemStack& stack);
void setEnchantments(const std::vector<std::pair<int, int>>& enchMap, ItemStack& stack);
int getMaxEnchantmentLevel(int enchID, std::span<const ItemStack* const> stacks);
int calcItemStackEnchantability(Random& rand, int enchantNum, int power, const ItemStack& stack);
// Adds the enchantments of buildEnchantmentList; a book turns into an enchanted book with stored enchantments.
ItemStack& addRandomEnchantment(Random& rand, ItemStack& stack, int level);
// Empty where Java returns null.
std::vector<EnchantmentData> buildEnchantmentList(Random& randomIn, const ItemStack& itemStackIn, int level);
// Java: a HashMap<Integer, EnchantmentData> -- returned as its entries in Java's iteration order; empty where
// Java returns null.
std::vector<EnchantmentData> mapEnchantmentData(int level, const ItemStack& stack);

} // namespace EnchantmentHelper

} // namespace mc
