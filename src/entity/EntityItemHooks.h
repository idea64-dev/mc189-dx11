#pragma once

// The entity-facing parts of net/minecraft/item/ItemStack.java, Item.java (getItemAttributeModifiers) and
// enchantment/EnchantmentHelper.java (the IModifier iterators and the held/worn enchantment queries), with the
// Enchantment methods they call (calcModifierDamage, calcDamageByCreature, onEntityDamaged, onUserHurt,
// EnchantmentDurability.negateDamage, EnchantmentProtection.getFireTimeForEntity / func_92092_a).
// They live here because they need EntityLivingBase; EnchantmentHelper's namespace is reopened so the calls keep
// their Java spelling.

#include "entity/EnumCreatureAttribute.h"
#include "entity/ai/attributes/Attributes.h"
#include "util/BlockPos.h"

#include <memory>

namespace mc {

class Block;
class DamageSource;
class Enchantment;
class Entity;
class EntityLivingBase;
class EntityPlayer;
class ItemStack;
class Random;
class World;

// Java: Item.itemModifierUUID
const UUID& itemModifierUUID();

// Java: ItemStack.attemptDamageItem -- true once the stack broke.
bool attemptDamageItem(ItemStack& stack, int amount, Random& rand);
// Java: ItemStack.damageItem
void damageItem(ItemStack& stack, int amount, EntityLivingBase* entityIn);
// Java: ItemStack.hitEntity -- Item.hitEntity of swords (1 damage) and tools (2); saddles (which saddle the
// target) come with pigs. The use statistic comes with the stats.
void hitEntity(ItemStack& stack, EntityLivingBase* entityIn, EntityPlayer* playerIn);
// Java: ItemStack.onBlockDestroyed -- Item.onBlockDestroyed of swords (2 damage on non-instant blocks), tools
// (1) and shears (1 on leaves, webs, grass, vines, tripwire and wool, else none).
void onBlockDestroyed(ItemStack& stack, World* worldIn, Block* blockIn, const BlockPos& pos, EntityPlayer* playerIn);
// Java: ItemStack.getAttributeModifiers (the tag's list, else Item.getItemAttributeModifiers).
AttributeModifierMultimap getAttributeModifiers(const ItemStack& stack);

namespace EnchantmentHelper {

// Java: getEnchantmentModifierDamage(ItemStack[], DamageSource) -- draws from the helper's own Random.
int getEnchantmentModifierDamage(const std::vector<std::shared_ptr<ItemStack>>& stacks, const DamageSource& source);
// Java: getModifierForCreature
float getModifierForCreature(const ItemStack* stack, EnumCreatureAttribute creatureAttribute);
// Java: applyThornEnchantments (func_151384_a)
void applyThornEnchantments(EntityLivingBase* user, Entity* attacker);
// Java: applyArthropodEnchantments (func_151385_b)
void applyArthropodEnchantments(EntityLivingBase* user, Entity* target);
int getKnockbackModifier(EntityLivingBase* player);
int getFireAspectModifier(EntityLivingBase* player);
int getRespiration(Entity* player);
int getDepthStriderModifier(Entity* player);
int getEfficiencyModifier(EntityLivingBase* player);
bool getSilkTouchModifier(EntityLivingBase* player);
int getFortuneModifier(EntityLivingBase* player);
int getLuckOfSeaModifier(EntityLivingBase* player);
int getLureModifier(EntityLivingBase* player);
int getLootingModifier(EntityLivingBase* player);
bool getAquaAffinityModifier(EntityLivingBase* player);
std::shared_ptr<ItemStack> getEnchantedItem(const Enchantment* enchantment, EntityLivingBase* entity);
// Java: getMaxEnchantmentLevel(int, ItemStack[]) over an entity's equipment.
int getMaxEnchantmentLevel(int enchID, const std::vector<std::shared_ptr<ItemStack>>& stacks);

} // namespace EnchantmentHelper

// Java: EnchantmentProtection.getFireTimeForEntity
int getFireTimeForEntity(Entity* entity, int fireTime);
// Java: EnchantmentProtection.func_92092_a -- the explosion knockback after blast protection.
double func_92092_a(Entity* entity, double strength);

} // namespace mc
