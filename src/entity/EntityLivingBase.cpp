#include "entity/EntityLivingBase.h"

#include "block/Block.h"
#include "block/BlockWorldHelpers.h"
#include "block/material/Material.h"
#include "block/state/BlockState.h"
#include "core/Java.h"
#include "enchantment/EnchantmentHelper.h"
#include "entity/EntityItemHooks.h"
#include "entity/EntityTracker.h"
#include "entity/SharedMonsterAttributes.h"
#include "entity/player/EntityPlayer.h"
#include "entity/player/EntityPlayerMP.h"
#include "init/Blocks.h"
#include "item/ItemArmor.h"
#include "item/ItemStack.h"
#include "nbt/NBTBase.h"
#include "nbt/NBTTagCompound.h"
#include "network/play/ServerPlayPackets.h"
#include "potion/PotionHelper.h"
#include "util/DamageSource.h"
#include "util/EnumParticleTypes.h"
#include "util/MathHelper.h"
#include "world/GameRules.h"
#include "world/World.h"
#include "world/WorldServer.h"
#include "world/border/WorldBorder.h"
#include "world/chunk/Chunk.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace mc {

namespace {

constexpr double kPi = 3.141592653589793;

const UUID& sprintingSpeedBoostModifierUUID()
{
    static const UUID uuid = *UUID::fromString("662A6B8D-DA3E-4C1C-8813-96EA6097278D");
    return uuid;
}

const AttributeModifier& sprintingSpeedBoostModifier()
{
    static const AttributeModifier modifier = [] {
        AttributeModifier m(sprintingSpeedBoostModifierUUID(), "Sprinting speed boost", 0.30000001192092896, 2);
        m.setSaved(false);
        return m;
    }();
    return modifier;
}

std::weak_ptr<Entity> weakOf(Entity* entity)
{
    return entity ? entity->weak_from_this() : std::weak_ptr<Entity>();
}

// Whether the pointer was set (even if the entity is gone since): Java's non-null reference.
bool isSet(const std::weak_ptr<Entity>& ref)
{
    const std::weak_ptr<Entity> empty;
    return ref.owner_before(empty) || empty.owner_before(ref);
}

// Java: EntitySelectors.NOT_SPECTATING
bool notSpectating(Entity* entity)
{
    auto* player = dynamic_cast<EntityPlayer*>(entity);
    return !player || !player->isSpectator();
}

bool isFlyingPlayer(Entity* entity)
{
    auto* player = dynamic_cast<EntityPlayer*>(entity);
    return player && player->capabilities.isFlying;
}

} // namespace

EntityLivingBase::EntityLivingBase(World* worldIn)
    : Entity(worldIn)
{
}

EntityLivingBase::~EntityLivingBase() = default;

void EntityLivingBase::construct()
{
    Entity::construct();
    applyEntityAttributes();
    setHealth(getMaxHealth());
    preventEntitySpawning = true;
    randomUnused1 = static_cast<float>((mathRandom() + 1.0) * 0.009999999776482582);
    setPosition(posX, posY, posZ);
    randomUnused2 = static_cast<float>(mathRandom()) * 12398.0f;
    rotationYaw = static_cast<float>(mathRandom() * kPi * 2.0);
    rotationYawHead = rotationYaw;
    stepHeight = 0.6f;
}

void EntityLivingBase::onKillCommand()
{
    attackEntityFrom(DamageSource::outOfWorld, FLT_MAX);
}

void EntityLivingBase::entityInit()
{
    dataWatcher.addObject(7, int32_t(0));
    dataWatcher.addObject(8, int8_t(0));
    dataWatcher.addObject(9, int8_t(0));
    dataWatcher.addObject(6, 1.0f);
}

void EntityLivingBase::applyEntityAttributes()
{
    getAttributeMap().registerAttribute(SharedMonsterAttributes::maxHealth());
    getAttributeMap().registerAttribute(SharedMonsterAttributes::knockbackResistance());
    getAttributeMap().registerAttribute(SharedMonsterAttributes::movementSpeed());
}

void EntityLivingBase::updateFallState(double y, bool onGroundIn, Block* blockIn, const BlockPos& pos)
{
    if (!isInWater())
        handleWaterMovement();
    if (!worldObj->isRemote && fallDistance > 3.0f && onGroundIn) {
        const IBlockState* iblockstate = worldObj->getBlockState(pos);
        Block* block = iblockstate->getBlock();
        const float f = static_cast<float>(MathHelper::ceiling_float_int(fallDistance - 3.0f));
        if (&block->getMaterial() != &Material::air) {
            double d0 = static_cast<double>(std::min(0.2f + f / 15.0f, 10.0f));
            if (d0 > 2.5)
                d0 = 2.5;
            const int i = java::d2i(150.0 * d0);
            static_cast<WorldServer*>(worldObj)->spawnParticle(EnumParticleTypes::BLOCK_DUST, posX, posY, posZ, i, 0.0, 0.0, 0.0, 0.15000000596046448,
                { Block::getStateId(iblockstate) });
        }
    }
    Entity::updateFallState(y, onGroundIn, blockIn, pos);
}

void EntityLivingBase::onEntityUpdate()
{
    prevSwingProgress = swingProgress;
    Entity::onEntityUpdate();
    worldObj->theProfiler.startSection("livingEntityBaseTick");
    const bool flag = dynamic_cast<EntityPlayer*>(this) != nullptr;
    if (isEntityAlive()) {
        if (isEntityInsideOpaqueBlock()) {
            attackEntityFrom(DamageSource::inWall, 1.0f);
        } else if (flag && !worldObj->getWorldBorder().contains(getEntityBoundingBox())) {
            const WorldBorder& border = worldObj->getWorldBorder();
            const double d0 = border.getClosestDistance(posX, posZ) + border.getDamageBuffer();
            if (d0 < 0.0)
                attackEntityFrom(DamageSource::inWall, static_cast<float>(std::max(1, MathHelper::floor_double(-d0 * border.getDamageAmount()))));
        }
    }
    if (isImmuneToFire() || worldObj->isRemote)
        extinguish();
    const bool flag1 = flag && static_cast<EntityPlayer*>(this)->capabilities.disableDamage;
    if (isEntityAlive()) {
        if (isInsideOfMaterial(Material::water)) {
            if (!canBreatheUnderwater() && !isPotionActive(Potion::waterBreathing->id) && !flag1) {
                setAir(decreaseAirSupply(getAir()));
                if (getAir() == -20) {
                    setAir(0);
                    for (int i = 0; i < 8; ++i) {
                        const float a = rand.nextFloat();
                        const float f = a - rand.nextFloat();
                        const float b = rand.nextFloat();
                        const float f1 = b - rand.nextFloat();
                        const float c = rand.nextFloat();
                        const float f2 = c - rand.nextFloat();
                        spawnParticle(worldObj, EnumParticleTypes::WATER_BUBBLE, posX + static_cast<double>(f), posY + static_cast<double>(f1),
                            posZ + static_cast<double>(f2), motionX, motionY, motionZ);
                    }
                    attackEntityFrom(DamageSource::drown, 2.0f);
                }
            }
            if (!worldObj->isRemote && isRiding() && dynamic_cast<EntityLivingBase*>(ridingEntity.get()))
                mountEntity(nullptr);
        } else {
            setAir(300);
        }
    }
    if (isEntityAlive() && isWet())
        extinguish();
    prevCameraPitch = cameraPitch;
    if (hurtTime > 0)
        --hurtTime;
    if (hurtResistantTime > 0 && !dynamic_cast<EntityPlayerMP*>(this))
        --hurtResistantTime;
    if (getHealth() <= 0.0f)
        onDeathUpdate();
    if (recentlyHit > 0)
        --recentlyHit;
    else
        attackingPlayer.reset();
    if (isSet(lastAttacker)) {
        Entity* last = lastAttacker.lock().get();
        if (!last || !last->isEntityAlive())
            lastAttacker.reset();
    }
    if (isSet(entityLivingToAttack)) {
        Entity* target = entityLivingToAttack.lock().get();
        if (!target || !target->isEntityAlive())
            setRevengeTarget(nullptr);
        else if (ticksExisted - revengeTimer > 100)
            setRevengeTarget(nullptr);
    }
    updatePotionEffects();
    prevMovedDistance = movedDistance;
    prevRenderYawOffset = renderYawOffset;
    prevRotationYawHead = rotationYawHead;
    prevRotationYaw = rotationYaw;
    prevRotationPitch = rotationPitch;
    worldObj->theProfiler.endSection();
}

void EntityLivingBase::onDeathUpdate()
{
    ++deathTime;
    if (deathTime == 20) {
        if (!worldObj->isRemote && (recentlyHit > 0 || isPlayer()) && canDropLoot() && worldObj->getGameRules().getBoolean("doMobLoot")) {
            int i = getExperiencePoints(getAttackingPlayer());
            // Java: splits i into EntityXPOrb.getXPSplit pieces, spawning an orb for each -- experience orbs
            // come with their entity.
            (void)i;
        }
        setDead();
        for (int k = 0; k < 20; ++k) {
            const double d2 = rand.nextGaussian() * 0.02;
            const double d0 = rand.nextGaussian() * 0.02;
            const double d1 = rand.nextGaussian() * 0.02;
            const double x = posX + static_cast<double>(rand.nextFloat() * width * 2.0f) - static_cast<double>(width);
            const double y = posY + static_cast<double>(rand.nextFloat() * height);
            const double z = posZ + static_cast<double>(rand.nextFloat() * width * 2.0f) - static_cast<double>(width);
            spawnParticle(worldObj, EnumParticleTypes::EXPLOSION_NORMAL, x, y, z, d2, d0, d1);
        }
    }
}

int EntityLivingBase::decreaseAirSupply(int p_70682_1_)
{
    const int i = EnchantmentHelper::getRespiration(this);
    return i > 0 && rand.nextInt(i + 1) > 0 ? p_70682_1_ : p_70682_1_ - 1;
}

EntityLivingBase* EntityLivingBase::getAITarget() const
{
    return static_cast<EntityLivingBase*>(entityLivingToAttack.lock().get());
}

void EntityLivingBase::setRevengeTarget(EntityLivingBase* livingBase)
{
    entityLivingToAttack = weakOf(livingBase);
    revengeTimer = ticksExisted;
}

EntityLivingBase* EntityLivingBase::getLastAttacker() const
{
    return static_cast<EntityLivingBase*>(lastAttacker.lock().get());
}

void EntityLivingBase::setLastAttacker(Entity* entityIn)
{
    if (auto* living = dynamic_cast<EntityLivingBase*>(entityIn))
        lastAttacker = weakOf(living);
    else
        lastAttacker.reset();
    lastAttackerTime = ticksExisted;
}

EntityPlayer* EntityLivingBase::getAttackingPlayer() const
{
    return static_cast<EntityPlayer*>(attackingPlayer.lock().get());
}

void EntityLivingBase::setAttackingPlayer(EntityPlayer* player)
{
    attackingPlayer = weakOf(player);
}

void EntityLivingBase::writeEntityToNBT(NBTTagCompound& tagCompound)
{
    tagCompound.setFloat("HealF", getHealth());
    tagCompound.setShort("Health", static_cast<int16_t>(java::d2i(std::ceil(static_cast<double>(getHealth())))));
    tagCompound.setShort("HurtTime", static_cast<int16_t>(hurtTime));
    tagCompound.setInteger("HurtByTimestamp", revengeTimer);
    tagCompound.setShort("DeathTime", static_cast<int16_t>(deathTime));
    tagCompound.setFloat("AbsorptionAmount", getAbsorptionAmount());
    for (const auto& itemstack : getInventory()) {
        if (itemstack)
            getAttributeMap().removeAttributeModifiers(getAttributeModifiers(*itemstack));
    }
    tagCompound.setTag("Attributes", SharedMonsterAttributes::writeBaseAttributeMapToNBT(getAttributeMap()));
    for (const auto& itemstack1 : getInventory()) {
        if (itemstack1)
            getAttributeMap().applyAttributeModifiers(getAttributeModifiers(*itemstack1));
    }
    if (!activePotionsMap.empty()) {
        auto nbttaglist = std::make_shared<NBTTagList>();
        for (const auto& entry : activePotionsMap) {
            auto nbttagcompound = std::make_shared<NBTTagCompound>();
            entry.second.writeCustomPotionEffectToNBT(*nbttagcompound);
            nbttaglist->appendTag(nbttagcompound);
        }
        tagCompound.setTag("ActiveEffects", nbttaglist);
    }
}

void EntityLivingBase::readEntityFromNBT(const NBTTagCompound& tagCompund)
{
    setAbsorptionAmount(tagCompund.getFloat("AbsorptionAmount"));
    if (tagCompund.hasKey("Attributes", 9) && worldObj && !worldObj->isRemote)
        SharedMonsterAttributes::setAttributeModifiers(getAttributeMap(), *tagCompund.getTagList("Attributes", 10));
    if (tagCompund.hasKey("ActiveEffects", 9)) {
        const std::shared_ptr<NBTTagList> nbttaglist = tagCompund.getTagList("ActiveEffects", 10);
        for (int i = 0; i < nbttaglist->tagCount(); ++i) {
            const std::shared_ptr<NBTTagCompound> nbttagcompound = nbttaglist->getCompoundTagAt(i);
            if (const std::unique_ptr<PotionEffect> potioneffect = PotionEffect::readCustomPotionEffectFromNBT(*nbttagcompound))
                activePotionsMap.put(potioneffect->getPotionID(), *potioneffect);
        }
    }
    if (tagCompund.hasKey("HealF", 99)) {
        setHealth(tagCompund.getFloat("HealF"));
    } else {
        const std::shared_ptr<NBTBase> nbtbase = tagCompund.getTag("Health");
        if (!nbtbase)
            setHealth(getMaxHealth());
        else if (nbtbase->getId() == 5)
            setHealth(static_cast<const NBTTagFloat&>(*nbtbase).getFloat());
        else if (nbtbase->getId() == 2)
            setHealth(static_cast<float>(static_cast<const NBTTagShort&>(*nbtbase).getShort()));
    }
    hurtTime = tagCompund.getShort("HurtTime");
    deathTime = tagCompund.getShort("DeathTime");
    revengeTimer = tagCompund.getInteger("HurtByTimestamp");
}

void EntityLivingBase::updatePotionEffects()
{
    // Java iterates the key set; nothing adds effects meanwhile, so a snapshot of the keys visits the same ones.
    std::vector<int32_t> keys;
    keys.reserve(activePotionsMap.size());
    for (const auto& entry : activePotionsMap)
        keys.push_back(entry.first);
    for (const int32_t key : keys) {
        PotionEffect* potioneffect = activePotionsMap.find(key);
        if (!potioneffect)
            continue;
        const bool running = potioneffect->onUpdate(this);
        potioneffect = activePotionsMap.find(key);
        if (!potioneffect)
            continue;
        if (!running) {
            if (!worldObj->isRemote) {
                const PotionEffect finished = *potioneffect;
                activePotionsMap.erase(key);
                onFinishedPotionEffect(finished);
            }
        } else if (potioneffect->getDuration() % 600 == 0) {
            onChangedPotionEffect(*potioneffect, false);
        }
    }
    if (potionsNeedUpdate) {
        if (!worldObj->isRemote)
            updatePotionMetadata();
        potionsNeedUpdate = false;
    }
    const int i = dataWatcher.getWatchableObjectInt(7);
    const bool flag1 = dataWatcher.getWatchableObjectByte(8) > 0;
    if (i > 0) {
        bool flag = false;
        if (!isInvisible())
            flag = rand.nextBoolean();
        else
            flag = rand.nextInt(15) == 0;
        if (flag1)
            flag &= rand.nextInt(5) == 0;
        if (flag && i > 0) {
            const double d0 = static_cast<double>(i >> 16 & 255) / 255.0;
            const double d1 = static_cast<double>(i >> 8 & 255) / 255.0;
            const double d2 = static_cast<double>(i >> 0 & 255) / 255.0;
            const double x = posX + (rand.nextDouble() - 0.5) * static_cast<double>(width);
            const double y = posY + rand.nextDouble() * static_cast<double>(height);
            const double z = posZ + (rand.nextDouble() - 0.5) * static_cast<double>(width);
            spawnParticle(worldObj, flag1 ? EnumParticleTypes::SPELL_MOB_AMBIENT : EnumParticleTypes::SPELL_MOB, x, y, z, d0, d1, d2);
        }
    }
}

void EntityLivingBase::updatePotionMetadata()
{
    if (activePotionsMap.empty()) {
        resetPotionEffectMetadata();
        setInvisible(false);
    } else {
        const std::vector<PotionEffect> effects = getActivePotionEffects();
        const int i = PotionHelper::calcPotionLiquidColor(effects);
        dataWatcher.updateObject(8, static_cast<int8_t>(PotionHelper::getAreAmbient(effects) ? 1 : 0));
        dataWatcher.updateObject(7, static_cast<int32_t>(i));
        setInvisible(isPotionActive(Potion::invisibility->id));
    }
}

void EntityLivingBase::resetPotionEffectMetadata()
{
    dataWatcher.updateObject(8, int8_t(0));
    dataWatcher.updateObject(7, int32_t(0));
}

void EntityLivingBase::clearActivePotions()
{
    if (worldObj->isRemote)
        return;
    std::vector<int32_t> keys;
    for (const auto& entry : activePotionsMap)
        keys.push_back(entry.first);
    for (const int32_t key : keys) {
        const PotionEffect* potioneffect = activePotionsMap.find(key);
        if (!potioneffect)
            continue;
        const PotionEffect finished = *potioneffect;
        activePotionsMap.erase(key);
        onFinishedPotionEffect(finished);
    }
}

std::vector<PotionEffect> EntityLivingBase::getActivePotionEffects() const
{
    std::vector<PotionEffect> effects;
    effects.reserve(activePotionsMap.size());
    for (const auto& entry : activePotionsMap)
        effects.push_back(entry.second);
    return effects;
}

void EntityLivingBase::addPotionEffect(const PotionEffect& potioneffectIn)
{
    if (!isPotionApplicable(potioneffectIn))
        return;
    if (PotionEffect* existing = activePotionsMap.find(potioneffectIn.getPotionID())) {
        existing->combine(potioneffectIn);
        onChangedPotionEffect(*existing, true);
    } else {
        activePotionsMap.put(potioneffectIn.getPotionID(), potioneffectIn);
        onNewPotionEffect(potioneffectIn);
    }
}

bool EntityLivingBase::isPotionApplicable(const PotionEffect& potioneffectIn)
{
    if (getCreatureAttribute() == EnumCreatureAttribute::UNDEAD) {
        const int i = potioneffectIn.getPotionID();
        if (i == Potion::regeneration->id || i == Potion::poison->id)
            return false;
    }
    return true;
}

void EntityLivingBase::removePotionEffect(int potionId)
{
    const PotionEffect* potioneffect = activePotionsMap.find(potionId);
    if (!potioneffect)
        return;
    const PotionEffect removed = *potioneffect;
    activePotionsMap.erase(potionId);
    onFinishedPotionEffect(removed);
}

void EntityLivingBase::onNewPotionEffect(const PotionEffect& id)
{
    potionsNeedUpdate = true;
    if (!worldObj->isRemote)
        Potion::potionTypes[static_cast<size_t>(id.getPotionID())]->applyAttributesModifiersToEntity(this, getAttributeMap(), id.getAmplifier());
}

void EntityLivingBase::onChangedPotionEffect(const PotionEffect& id, bool p_70695_2_)
{
    potionsNeedUpdate = true;
    if (p_70695_2_ && !worldObj->isRemote) {
        const Potion* potion = Potion::potionTypes[static_cast<size_t>(id.getPotionID())];
        potion->removeAttributesModifiersFromEntity(this, getAttributeMap(), id.getAmplifier());
        potion->applyAttributesModifiersToEntity(this, getAttributeMap(), id.getAmplifier());
    }
}

void EntityLivingBase::onFinishedPotionEffect(const PotionEffect& effect)
{
    potionsNeedUpdate = true;
    if (!worldObj->isRemote)
        Potion::potionTypes[static_cast<size_t>(effect.getPotionID())]->removeAttributesModifiersFromEntity(this, getAttributeMap(), effect.getAmplifier());
}

void EntityLivingBase::heal(float healAmount)
{
    const float f = getHealth();
    if (f > 0.0f)
        setHealth(f + healAmount);
}

void EntityLivingBase::setHealth(float health)
{
    dataWatcher.updateObject(6, MathHelper::clamp_float(health, 0.0f, getMaxHealth()));
}

bool EntityLivingBase::attackEntityFrom(const DamageSource& source, float amount)
{
    if (isEntityInvulnerable(source))
        return false;
    if (worldObj->isRemote)
        return false;
    entityAge = 0;
    if (getHealth() <= 0.0f)
        return false;
    if (source.isFireDamage() && isPotionActive(Potion::fireResistance))
        return false;
    if (&source == &DamageSource::anvil || &source == &DamageSource::fallingBlock) {
        if (const std::shared_ptr<ItemStack> helmet = getEquipmentInSlot(4)) {
            const float r = rand.nextFloat();
            damageItem(*helmet, java::f2i(amount * 4.0f + r * amount * 2.0f), this);
            amount *= 0.75f;
        }
    }
    limbSwingAmount = 1.5f;
    bool flag = true;
    if (static_cast<float>(hurtResistantTime) > static_cast<float>(maxHurtResistantTime) / 2.0f) {
        if (amount <= lastDamage)
            return false;
        damageEntity(source, amount - lastDamage);
        lastDamage = amount;
        flag = false;
    } else {
        lastDamage = amount;
        hurtResistantTime = maxHurtResistantTime;
        damageEntity(source, amount);
        hurtTime = maxHurtTime = 10;
    }
    attackedAtYaw = 0.0f;
    Entity* entity = source.getEntity();
    if (entity) {
        if (auto* living = dynamic_cast<EntityLivingBase*>(entity))
            setRevengeTarget(living);
        if (auto* player = dynamic_cast<EntityPlayer*>(entity)) {
            recentlyHit = 100;
            setAttackingPlayer(player);
        }
        // Java: else a tamed EntityWolf sets recentlyHit = 100 and clears attackingPlayer -- wolves come with
        // the mobs.
    }
    if (flag) {
        worldObj->setEntityState(this, 2);
        if (&source != &DamageSource::drown)
            setBeenAttacked();
        if (entity) {
            double d1 = entity->posX - posX;
            double d0;
            for (d0 = entity->posZ - posZ; d1 * d1 + d0 * d0 < 1.0E-4; d0 = (mathRandom() - mathRandom()) * 0.01) {
                const double a = mathRandom();
                d1 = (a - mathRandom()) * 0.01;
            }
            attackedAtYaw = static_cast<float>(MathHelper::atan2(d0, d1) * 180.0 / kPi - static_cast<double>(rotationYaw));
            knockBack(entity, amount, d1, d0);
        } else {
            attackedAtYaw = static_cast<float>(java::d2i(mathRandom() * 2.0) * 180);
        }
    }
    if (getHealth() <= 0.0f) {
        const std::optional<std::string> s = getDeathSound();
        if (flag && s) {
            const float volume = getSoundVolume();
            playSound(*s, volume, getSoundPitch());
        }
        onDeath(source);
    } else {
        const std::optional<std::string> s1 = getHurtSound();
        if (flag && s1) {
            const float volume = getSoundVolume();
            playSound(*s1, volume, getSoundPitch());
        }
    }
    return true;
}

void EntityLivingBase::renderBrokenItemStack(const ItemStack& stack)
{
    playSound("random.break", 0.8f, 0.8f + worldObj->rand.nextFloat() * 0.4f);
    const float pi = static_cast<float>(kPi);
    for (int i = 0; i < 5; ++i) {
        const double vx = (static_cast<double>(rand.nextFloat()) - 0.5) * 0.1;
        Vec3 vec3(vx, mathRandom() * 0.1 + 0.1, 0.0);
        vec3 = vec3.rotatePitch(-rotationPitch * pi / 180.0f);
        vec3 = vec3.rotateYaw(-rotationYaw * pi / 180.0f);
        const double d0 = static_cast<double>(-rand.nextFloat()) * 0.6 - 0.3;
        Vec3 vec31((static_cast<double>(rand.nextFloat()) - 0.5) * 0.3, d0, 0.6);
        vec31 = vec31.rotatePitch(-rotationPitch * pi / 180.0f);
        vec31 = vec31.rotateYaw(-rotationYaw * pi / 180.0f);
        vec31 = vec31.addVector(posX, posY + static_cast<double>(getEyeHeight()), posZ);
        spawnParticle(worldObj, EnumParticleTypes::ITEM_CRACK, vec31.xCoord, vec31.yCoord, vec31.zCoord, vec3.xCoord, vec3.yCoord + 0.05, vec3.zCoord,
            { Item::getIdFromItem(stack.getItem()) });
    }
}

void EntityLivingBase::onDeath(const DamageSource& cause)
{
    Entity* entity = cause.getEntity();
    EntityLivingBase* entitylivingbase = getAttackingEntity();
    if (scoreValue >= 0 && entitylivingbase)
        entitylivingbase->addToPlayerScore(this, scoreValue);
    if (entity)
        entity->onKillEntity(this);
    dead = true;
    getCombatTracker().reset();
    if (!worldObj->isRemote) {
        int i = 0;
        if (auto* player = dynamic_cast<EntityPlayer*>(entity))
            i = EnchantmentHelper::getLootingModifier(player);
        if (canDropLoot() && worldObj->getGameRules().getBoolean("doMobLoot")) {
            dropFewItems(recentlyHit > 0, i);
            dropEquipment(recentlyHit > 0, i);
            if (recentlyHit > 0 && rand.nextFloat() < 0.025f + static_cast<float>(i) * 0.01f)
                addRandomDrop();
        }
    }
    worldObj->setEntityState(this, 3);
}

void EntityLivingBase::knockBack(Entity*, float, double p_70653_3_, double p_70653_5_)
{
    if (rand.nextDouble() >= getEntityAttribute(SharedMonsterAttributes::knockbackResistance())->getAttributeValue()) {
        isAirBorne = true;
        const float f = MathHelper::sqrt_double(p_70653_3_ * p_70653_3_ + p_70653_5_ * p_70653_5_);
        const float f1 = 0.4f;
        motionX /= 2.0;
        motionY /= 2.0;
        motionZ /= 2.0;
        motionX -= p_70653_3_ / static_cast<double>(f) * static_cast<double>(f1);
        motionY += static_cast<double>(f1);
        motionZ -= p_70653_5_ / static_cast<double>(f) * static_cast<double>(f1);
        if (motionY > 0.4000000059604645)
            motionY = 0.4000000059604645;
    }
}

bool EntityLivingBase::isOnLadder()
{
    const int i = MathHelper::floor_double(posX);
    const int j = MathHelper::floor_double(getEntityBoundingBox().minY);
    const int k = MathHelper::floor_double(posZ);
    const Block* block = worldObj->getBlockState(BlockPos(i, j, k))->getBlock();
    auto* player = dynamic_cast<EntityPlayer*>(this);
    return (block == Blocks::ladder || block == Blocks::vine) && (!player || !player->isSpectator());
}

void EntityLivingBase::fall(float distance, float damageMultiplier)
{
    Entity::fall(distance, damageMultiplier);
    const PotionEffect* potioneffect = getActivePotionEffect(Potion::jump);
    const float f = potioneffect ? static_cast<float>(potioneffect->getAmplifier() + 1) : 0.0f;
    const int i = MathHelper::ceiling_float_int((distance - 3.0f - f) * damageMultiplier);
    if (i > 0) {
        playSound(getFallSoundString(i), 1.0f, 1.0f);
        attackEntityFrom(DamageSource::fall, static_cast<float>(i));
        const int j = MathHelper::floor_double(posX);
        const int k = MathHelper::floor_double(posY - 0.20000000298023224);
        const int l = MathHelper::floor_double(posZ);
        const Block* block = worldObj->getBlockState(BlockPos(j, k, l))->getBlock();
        if (&block->getMaterial() != &Material::air) {
            const Block::SoundType* soundtype = block->stepSound;
            playSound(soundtype->getStepSound(), soundtype->getVolume() * 0.5f, soundtype->getFrequency() * 0.75f);
        }
    }
}

void EntityLivingBase::performHurtAnimation()
{
    hurtTime = maxHurtTime = 10;
    attackedAtYaw = 0.0f;
}

int EntityLivingBase::getTotalArmorValue()
{
    int i = 0;
    for (const auto& itemstack : getInventory()) {
        if (itemstack) {
            if (const auto* armor = dynamic_cast<const ItemArmor*>(itemstack->getItem()))
                i += armor->damageReduceAmount;
        }
    }
    return i;
}

float EntityLivingBase::applyArmorCalculations(const DamageSource& source, float damage)
{
    if (!source.isUnblockable()) {
        const int i = 25 - getTotalArmorValue();
        const float f = damage * static_cast<float>(i);
        damageArmor(damage);
        damage = f / 25.0f;
    }
    return damage;
}

float EntityLivingBase::applyPotionDamageCalculations(const DamageSource& source, float damage)
{
    if (source.isDamageAbsolute())
        return damage;
    if (isPotionActive(Potion::resistance) && &source != &DamageSource::outOfWorld) {
        const int i = (getActivePotionEffect(Potion::resistance)->getAmplifier() + 1) * 5;
        const int j = 25 - i;
        const float f = damage * static_cast<float>(j);
        damage = f / 25.0f;
    }
    if (damage <= 0.0f)
        return 0.0f;
    int k = EnchantmentHelper::getEnchantmentModifierDamage(getInventory(), source);
    if (k > 20)
        k = 20;
    if (k > 0 && k <= 20) {
        const int l = 25 - k;
        const float f1 = damage * static_cast<float>(l);
        damage = f1 / 25.0f;
    }
    return damage;
}

void EntityLivingBase::damageEntity(const DamageSource& damageSrc, float damageAmount)
{
    if (isEntityInvulnerable(damageSrc))
        return;
    damageAmount = applyArmorCalculations(damageSrc, damageAmount);
    damageAmount = applyPotionDamageCalculations(damageSrc, damageAmount);
    const float f = damageAmount;
    damageAmount = std::max(damageAmount - getAbsorptionAmount(), 0.0f);
    setAbsorptionAmount(getAbsorptionAmount() - (f - damageAmount));
    if (damageAmount != 0.0f) {
        const float f1 = getHealth();
        setHealth(f1 - damageAmount);
        getCombatTracker().trackDamage(damageSrc, f1, damageAmount);
        setAbsorptionAmount(getAbsorptionAmount() - damageAmount);
    }
}

EntityLivingBase* EntityLivingBase::getAttackingEntity()
{
    if (EntityLivingBase* fighter = _combatTracker.func_94550_c())
        return fighter;
    if (EntityPlayer* player = getAttackingPlayer())
        return player;
    return getAITarget();
}

float EntityLivingBase::getMaxHealth() const
{
    auto* self = const_cast<EntityLivingBase*>(this);
    return static_cast<float>(self->getEntityAttribute(SharedMonsterAttributes::maxHealth())->getAttributeValue());
}

int EntityLivingBase::getArmSwingAnimationEnd() const
{
    if (const PotionEffect* haste = getActivePotionEffect(Potion::digSpeed))
        return 6 - (1 + haste->getAmplifier()) * 1;
    if (const PotionEffect* fatigue = getActivePotionEffect(Potion::digSlowdown))
        return 6 + (1 + fatigue->getAmplifier()) * 2;
    return 6;
}

void EntityLivingBase::swingItem()
{
    if (!isSwingInProgress || swingProgressInt >= getArmSwingAnimationEnd() / 2 || swingProgressInt < 0) {
        swingProgressInt = -1;
        isSwingInProgress = true;
        if (auto* worldServer = dynamic_cast<WorldServer*>(worldObj))
            worldServer->getEntityTracker()->sendToAllTrackingEntity(this, std::make_shared<S0BPacketAnimation>(this, 0));
    }
}

void EntityLivingBase::handleStatusUpdate(int8_t id)
{
    if (id == 2) {
        limbSwingAmount = 1.5f;
        hurtResistantTime = maxHurtResistantTime;
        hurtTime = maxHurtTime = 10;
        attackedAtYaw = 0.0f;
        if (getHurtSound()) {
            const std::optional<std::string> s = getHurtSound();
            const float volume = getSoundVolume();
            const float a = rand.nextFloat();
            const float pitch = (a - rand.nextFloat()) * 0.2f + 1.0f;
            if (s)
                playSound(*s, volume, pitch);
        }
        attackEntityFrom(DamageSource::generic, 0.0f);
    } else if (id == 3) {
        if (getDeathSound()) {
            const std::optional<std::string> s1 = getDeathSound();
            const float volume = getSoundVolume();
            const float a = rand.nextFloat();
            const float pitch = (a - rand.nextFloat()) * 0.2f + 1.0f;
            if (s1)
                playSound(*s1, volume, pitch);
        }
        setHealth(0.0f);
        onDeath(DamageSource::generic);
    } else {
        Entity::handleStatusUpdate(id);
    }
}

void EntityLivingBase::kill()
{
    attackEntityFrom(DamageSource::outOfWorld, 4.0f);
}

void EntityLivingBase::updateArmSwingProgress()
{
    const int i = getArmSwingAnimationEnd();
    if (isSwingInProgress) {
        ++swingProgressInt;
        if (swingProgressInt >= i) {
            swingProgressInt = 0;
            isSwingInProgress = false;
        }
    } else {
        swingProgressInt = 0;
    }
    swingProgress = static_cast<float>(swingProgressInt) / static_cast<float>(i);
}

BaseAttributeMap& EntityLivingBase::getAttributeMap()
{
    if (!attributeMap)
        attributeMap = std::make_unique<ServersideAttributeMap>();
    return *attributeMap;
}

void EntityLivingBase::setSprinting(bool sprinting)
{
    Entity::setSprinting(sprinting);
    IAttributeInstance* iattributeinstance = getEntityAttribute(SharedMonsterAttributes::movementSpeed());
    if (iattributeinstance->getModifier(sprintingSpeedBoostModifierUUID()))
        iattributeinstance->removeModifier(sprintingSpeedBoostModifier());
    if (sprinting)
        iattributeinstance->applyModifier(sprintingSpeedBoostModifier());
}

float EntityLivingBase::getSoundPitch()
{
    const float a = rand.nextFloat();
    const float b = rand.nextFloat();
    return isChild() ? (a - b) * 0.2f + 1.5f : (a - b) * 0.2f + 1.0f;
}

void EntityLivingBase::dismountEntity(Entity* entityIn)
{
    double d0 = entityIn->posX;
    double d1 = entityIn->getEntityBoundingBox().minY + static_cast<double>(entityIn->height);
    double d2 = entityIn->posZ;
    const int i = 1;
    for (int j = -i; j <= i; ++j) {
        for (int k = -i; k < i; ++k) {
            if (j != 0 || k != 0) {
                const int l = java::d2i(posX + static_cast<double>(j));
                const int i1 = java::d2i(posZ + static_cast<double>(k));
                const AxisAlignedBB axisalignedbb = getEntityBoundingBox().offset(static_cast<double>(j), 1.0, static_cast<double>(k));
                if (worldObj->getCollisionBoxes(axisalignedbb).empty()) {
                    if (World::doesBlockHaveSolidTopSurface(worldObj, BlockPos(l, java::d2i(posY), i1))) {
                        setPositionAndUpdate(posX + static_cast<double>(j), posY + 1.0, posZ + static_cast<double>(k));
                        return;
                    }
                    const BlockPos below(l, java::d2i(posY) - 1, i1);
                    if (World::doesBlockHaveSolidTopSurface(worldObj, below) || &worldObj->getBlockState(below)->getBlock()->getMaterial() == &Material::water) {
                        d0 = posX + static_cast<double>(j);
                        d1 = posY + 1.0;
                        d2 = posZ + static_cast<double>(k);
                    }
                }
            }
        }
    }
    setPositionAndUpdate(d0, d1, d2);
}

void EntityLivingBase::jump()
{
    motionY = static_cast<double>(getJumpUpwardsMotion());
    if (const PotionEffect* jumpBoost = getActivePotionEffect(Potion::jump))
        motionY += static_cast<double>(static_cast<float>(jumpBoost->getAmplifier() + 1) * 0.1f);
    if (isSprinting()) {
        const float f = rotationYaw * 0.017453292f;
        motionX -= static_cast<double>(MathHelper::sin(f) * 0.2f);
        motionZ += static_cast<double>(MathHelper::cos(f) * 0.2f);
    }
    isAirBorne = true;
}

void EntityLivingBase::moveEntityWithHeading(float strafe, float forward)
{
    if (isServerWorld()) {
        if (!isInWater() || isFlyingPlayer(this)) {
            if (!isInLava() || isFlyingPlayer(this)) {
                float f4 = 0.91f;
                if (onGround) {
                    f4 = worldObj
                             ->getBlockState(BlockPos(MathHelper::floor_double(posX), MathHelper::floor_double(getEntityBoundingBox().minY) - 1,
                                 MathHelper::floor_double(posZ)))
                             ->getBlock()
                             ->slipperiness
                        * 0.91f;
                }
                const float f = 0.16277136f / (f4 * f4 * f4);
                float f5;
                if (onGround)
                    f5 = getAIMoveSpeed() * f;
                else
                    f5 = jumpMovementFactor;
                moveFlying(strafe, forward, f5);
                f4 = 0.91f;
                if (onGround) {
                    f4 = worldObj
                             ->getBlockState(BlockPos(MathHelper::floor_double(posX), MathHelper::floor_double(getEntityBoundingBox().minY) - 1,
                                 MathHelper::floor_double(posZ)))
                             ->getBlock()
                             ->slipperiness
                        * 0.91f;
                }
                if (isOnLadder()) {
                    const float f6 = 0.15f;
                    motionX = MathHelper::clamp_double(motionX, static_cast<double>(-f6), static_cast<double>(f6));
                    motionZ = MathHelper::clamp_double(motionZ, static_cast<double>(-f6), static_cast<double>(f6));
                    fallDistance = 0.0f;
                    if (motionY < -0.15)
                        motionY = -0.15;
                    const bool flag = isSneaking() && dynamic_cast<EntityPlayer*>(this);
                    if (flag && motionY < 0.0)
                        motionY = 0.0;
                }
                moveEntity(motionX, motionY, motionZ);
                if (isCollidedHorizontally && isOnLadder())
                    motionY = 0.2;
                const BlockPos column(java::d2i(posX), 0, java::d2i(posZ));
                if (worldObj->isRemote && (!worldObj->isBlockLoaded(column) || !worldObj->getChunkFromBlockCoords(column)->isLoaded())) {
                    if (posY > 0.0)
                        motionY = -0.1;
                    else
                        motionY = 0.0;
                } else {
                    motionY -= 0.08;
                }
                motionY *= 0.9800000190734863;
                motionX *= static_cast<double>(f4);
                motionZ *= static_cast<double>(f4);
            } else {
                const double d1 = posY;
                moveFlying(strafe, forward, 0.02f);
                moveEntity(motionX, motionY, motionZ);
                motionX *= 0.5;
                motionY *= 0.5;
                motionZ *= 0.5;
                motionY -= 0.02;
                if (isCollidedHorizontally && isOffsetPositionInLiquid(motionX, motionY + 0.6000000238418579 - posY + d1, motionZ))
                    motionY = 0.30000001192092896;
            }
        } else {
            const double d0 = posY;
            float f1 = 0.8f;
            float f2 = 0.02f;
            float f3 = static_cast<float>(EnchantmentHelper::getDepthStriderModifier(this));
            if (f3 > 3.0f)
                f3 = 3.0f;
            if (!onGround)
                f3 *= 0.5f;
            if (f3 > 0.0f) {
                f1 += (0.54600006f - f1) * f3 / 3.0f;
                f2 += (getAIMoveSpeed() * 1.0f - f2) * f3 / 3.0f;
            }
            moveFlying(strafe, forward, f2);
            moveEntity(motionX, motionY, motionZ);
            motionX *= static_cast<double>(f1);
            motionY *= 0.800000011920929;
            motionZ *= static_cast<double>(f1);
            motionY -= 0.02;
            if (isCollidedHorizontally && isOffsetPositionInLiquid(motionX, motionY + 0.6000000238418579 - posY + d0, motionZ))
                motionY = 0.30000001192092896;
        }
    }
    prevLimbSwingAmount = limbSwingAmount;
    const double d2 = posX - prevPosX;
    const double d3 = posZ - prevPosZ;
    float f7 = MathHelper::sqrt_double(d2 * d2 + d3 * d3) * 4.0f;
    if (f7 > 1.0f)
        f7 = 1.0f;
    limbSwingAmount += (f7 - limbSwingAmount) * 0.4f;
    limbSwing += limbSwingAmount;
}

bool EntityLivingBase::attackEntityAsMob(Entity* entityIn)
{
    setLastAttacker(entityIn);
    return false;
}

void EntityLivingBase::onUpdate()
{
    Entity::onUpdate();
    if (!worldObj->isRemote) {
        const int i = getArrowCountInEntity();
        if (i > 0) {
            if (arrowHitTimer <= 0)
                arrowHitTimer = 20 * (30 - i);
            --arrowHitTimer;
            if (arrowHitTimer <= 0)
                setArrowCountInEntity(i - 1);
        }
        for (int j = 0; j < 5; ++j) {
            const std::shared_ptr<ItemStack> itemstack = previousEquipment[static_cast<size_t>(j)];
            const std::shared_ptr<ItemStack> itemstack1 = getEquipmentInSlot(j);
            if (!ItemStack::areItemStacksEqual(itemstack1.get(), itemstack.get())) {
                static_cast<WorldServer*>(worldObj)->getEntityTracker()->sendToAllTrackingEntity(this,
                    std::make_shared<S04PacketEntityEquipment>(getEntityId(), j, itemstack1.get()));
                if (itemstack)
                    getAttributeMap().removeAttributeModifiers(getAttributeModifiers(*itemstack));
                if (itemstack1)
                    getAttributeMap().applyAttributeModifiers(getAttributeModifiers(*itemstack1));
                previousEquipment[static_cast<size_t>(j)] = itemstack1 ? std::make_shared<ItemStack>(itemstack1->copy()) : nullptr;
            }
        }
        if (ticksExisted % 20 == 0)
            getCombatTracker().reset();
    }
    onLivingUpdate();
    const double d0 = posX - prevPosX;
    const double d1 = posZ - prevPosZ;
    const float f = static_cast<float>(d0 * d0 + d1 * d1);
    float f1 = renderYawOffset;
    float f2 = 0.0f;
    prevOnGroundSpeedFactor = onGroundSpeedFactor;
    float f3 = 0.0f;
    if (f > 0.0025000002f) {
        f3 = 1.0f;
        f2 = static_cast<float>(std::sqrt(static_cast<double>(f))) * 3.0f;
        f1 = static_cast<float>(MathHelper::atan2(d1, d0)) * 180.0f / static_cast<float>(kPi) - 90.0f;
    }
    if (swingProgress > 0.0f)
        f1 = rotationYaw;
    if (!onGround)
        f3 = 0.0f;
    onGroundSpeedFactor += (f3 - onGroundSpeedFactor) * 0.3f;
    worldObj->theProfiler.startSection("headTurn");
    f2 = updateDistance(f1, f2);
    worldObj->theProfiler.endSection();
    worldObj->theProfiler.startSection("rangeChecks");
    while (rotationYaw - prevRotationYaw < -180.0f)
        prevRotationYaw -= 360.0f;
    while (rotationYaw - prevRotationYaw >= 180.0f)
        prevRotationYaw += 360.0f;
    while (renderYawOffset - prevRenderYawOffset < -180.0f)
        prevRenderYawOffset -= 360.0f;
    while (renderYawOffset - prevRenderYawOffset >= 180.0f)
        prevRenderYawOffset += 360.0f;
    while (rotationPitch - prevRotationPitch < -180.0f)
        prevRotationPitch -= 360.0f;
    while (rotationPitch - prevRotationPitch >= 180.0f)
        prevRotationPitch += 360.0f;
    while (rotationYawHead - prevRotationYawHead < -180.0f)
        prevRotationYawHead -= 360.0f;
    while (rotationYawHead - prevRotationYawHead >= 180.0f)
        prevRotationYawHead += 360.0f;
    worldObj->theProfiler.endSection();
    movedDistance += f2;
}

float EntityLivingBase::updateDistance(float p_110146_1_, float p_110146_2_)
{
    const float f = MathHelper::wrapAngleTo180_float(p_110146_1_ - renderYawOffset);
    renderYawOffset += f * 0.3f;
    float f1 = MathHelper::wrapAngleTo180_float(rotationYaw - renderYawOffset);
    const bool flag = f1 < -90.0f || f1 >= 90.0f;
    if (f1 < -75.0f)
        f1 = -75.0f;
    if (f1 >= 75.0f)
        f1 = 75.0f;
    renderYawOffset = rotationYaw - f1;
    if (f1 * f1 > 2500.0f)
        renderYawOffset += f1 * 0.2f;
    if (flag)
        p_110146_2_ *= -1.0f;
    return p_110146_2_;
}

void EntityLivingBase::onLivingUpdate()
{
    if (jumpTicks > 0)
        --jumpTicks;
    if (newPosRotationIncrements > 0) {
        const double d0 = posX + (newPosX - posX) / static_cast<double>(newPosRotationIncrements);
        const double d1 = posY + (newPosY - posY) / static_cast<double>(newPosRotationIncrements);
        const double d2 = posZ + (newPosZ - posZ) / static_cast<double>(newPosRotationIncrements);
        const double d3 = MathHelper::wrapAngleTo180_double(newRotationYaw - static_cast<double>(rotationYaw));
        rotationYaw = static_cast<float>(static_cast<double>(rotationYaw) + d3 / static_cast<double>(newPosRotationIncrements));
        rotationPitch = static_cast<float>(
            static_cast<double>(rotationPitch) + (newRotationPitch - static_cast<double>(rotationPitch)) / static_cast<double>(newPosRotationIncrements));
        --newPosRotationIncrements;
        setPosition(d0, d1, d2);
        setRotation(rotationYaw, rotationPitch);
    } else if (!isServerWorld()) {
        motionX *= 0.98;
        motionY *= 0.98;
        motionZ *= 0.98;
    }
    if (std::abs(motionX) < 0.005)
        motionX = 0.0;
    if (std::abs(motionY) < 0.005)
        motionY = 0.0;
    if (std::abs(motionZ) < 0.005)
        motionZ = 0.0;
    worldObj->theProfiler.startSection("ai");
    if (isMovementBlocked()) {
        isJumping = false;
        moveStrafing = 0.0f;
        moveForward = 0.0f;
        randomYawVelocity = 0.0f;
    } else if (isServerWorld()) {
        worldObj->theProfiler.startSection("newAi");
        updateEntityActionState();
        worldObj->theProfiler.endSection();
    }
    worldObj->theProfiler.endSection();
    worldObj->theProfiler.startSection("jump");
    if (isJumping) {
        if (isInWater()) {
            updateAITick();
        } else if (isInLava()) {
            handleJumpLava();
        } else if (onGround && jumpTicks == 0) {
            jump();
            jumpTicks = 10;
        }
    } else {
        jumpTicks = 0;
    }
    worldObj->theProfiler.endSection();
    worldObj->theProfiler.startSection("travel");
    moveStrafing *= 0.98f;
    moveForward *= 0.98f;
    randomYawVelocity *= 0.9f;
    moveEntityWithHeading(moveStrafing, moveForward);
    worldObj->theProfiler.endSection();
    worldObj->theProfiler.startSection("push");
    if (!worldObj->isRemote)
        collideWithNearbyEntities();
    worldObj->theProfiler.endSection();
}

void EntityLivingBase::collideWithNearbyEntities()
{
    const std::vector<Entity*> list = worldObj->getEntitiesInAABBexcluding(this, getEntityBoundingBox().expand(0.20000000298023224, 0.0, 0.20000000298023224),
        [](Entity* entity) { return notSpectating(entity) && entity->canBePushed(); });
    for (Entity* entity : list)
        collideWithEntity(entity);
}

void EntityLivingBase::mountEntity(Entity* entityIn)
{
    if (ridingEntity && !entityIn) {
        // Keep the mount alive while it is unlinked.
        const std::shared_ptr<Entity> mount = ridingEntity;
        if (!worldObj->isRemote)
            dismountEntity(mount.get());
        if (ridingEntity)
            ridingEntity->riddenByEntity = nullptr;
        ridingEntity = nullptr;
    } else {
        Entity::mountEntity(entityIn);
    }
}

void EntityLivingBase::updateRidden()
{
    Entity::updateRidden();
    prevOnGroundSpeedFactor = onGroundSpeedFactor;
    onGroundSpeedFactor = 0.0f;
    fallDistance = 0.0f;
}

void EntityLivingBase::setPositionAndRotation2(double x, double y, double z, float yaw, float pitch, int posRotationIncrements, bool)
{
    newPosX = x;
    newPosY = y;
    newPosZ = z;
    newRotationYaw = static_cast<double>(yaw);
    newRotationPitch = static_cast<double>(pitch);
    newPosRotationIncrements = posRotationIncrements;
}

void EntityLivingBase::onItemPickup(Entity* p_71001_1_, int)
{
    if (!p_71001_1_->isDead && !worldObj->isRemote) {
        EntityTracker* entitytracker = static_cast<WorldServer*>(worldObj)->getEntityTracker();
        const std::string_view name = p_71001_1_->getClassName();
        if (name == "EntityItem" || name == "EntityArrow" || name == "EntityXPOrb")
            entitytracker->sendToAllTrackingEntity(p_71001_1_, std::make_shared<S0DPacketCollectItem>(p_71001_1_->getEntityId(), getEntityId()));
    }
}

bool EntityLivingBase::canEntityBeSeen(Entity* entityIn)
{
    return !worldObj
                ->rayTraceBlocks(Vec3(posX, posY + static_cast<double>(getEyeHeight()), posZ),
                    Vec3(entityIn->posX, entityIn->posY + static_cast<double>(entityIn->getEyeHeight()), entityIn->posZ))
                .has_value();
}

Vec3 EntityLivingBase::getLook(float partialTicks)
{
    if (partialTicks == 1.0f)
        return getVectorForRotation(rotationPitch, rotationYawHead);
    const float f = prevRotationPitch + (rotationPitch - prevRotationPitch) * partialTicks;
    const float f1 = prevRotationYawHead + (rotationYawHead - prevRotationYawHead) * partialTicks;
    return getVectorForRotation(f, f1);
}

float EntityLivingBase::getSwingProgress(float partialTickTime) const
{
    float f = swingProgress - prevSwingProgress;
    if (f < 0.0f)
        ++f;
    return prevSwingProgress + f * partialTickTime;
}

void EntityLivingBase::setBeenAttacked()
{
    velocityChanged = rand.nextDouble() >= getEntityAttribute(SharedMonsterAttributes::knockbackResistance())->getAttributeValue();
}

void EntityLivingBase::setAbsorptionAmount(float amount)
{
    if (amount < 0.0f)
        amount = 0.0f;
    absorptionAmount = amount;
}

bool EntityLivingBase::isOnTeam(Team*)
{
    // Java: getTeam() != null && getTeam().isSameTeam(teamIn) -- teams come with the scoreboard (stage 12).
    return false;
}

bool EntityLivingBase::isServerWorld()
{
    return !worldObj->isRemote;
}

} // namespace mc
