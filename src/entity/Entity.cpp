#include "entity/Entity.h"

#include "block/AllBlocks.h"
#include "client/resources/Locale.h"
#include "core/Java.h"
#include "core/JavaString.h"
#include "entity/EntityList.h"
#include "entity/player/EntityPlayer.h"
#include "init/Blocks.h"
#include "nbt/NBTTagCompound.h"
#include "util/ChatComponent.h"
#include "util/DamageSource.h"
#include "util/EnumParticleTypes.h"
#include "util/MathHelper.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/storage/WorldInfo.h"

#include <cmath>
#include <cstdio>

namespace mc {

std::atomic<int> Entity::nextEntityID { 0 };

Entity::Entity(World* worldIn)
    : worldObj(worldIn)
    , dataWatcher(this)
    , entityId(nextEntityID++)
{
}

Entity::~Entity() = default;

void Entity::construct()
{
    entityUniqueID = UUID::getRandomUuid(rand);
    setPosition(0.0, 0.0, 0.0);
    if (worldObj)
        dimension = worldObj->provider->getDimensionId();
    dataWatcher.addObject(0, int8_t(0));
    dataWatcher.addObject(1, int16_t(300));
    dataWatcher.addObject(3, int8_t(0));
    dataWatcher.addObject(2, std::string());
    dataWatcher.addObject(4, int8_t(0));
    entityInit();
}

void Entity::preparePlayerToSpawn()
{
    if (worldObj) {
        while (posY > 0.0 && posY < 256.0) {
            setPosition(posX, posY, posZ);
            if (worldObj->getCollidingBoundingBoxes(this, getEntityBoundingBox()).empty())
                break;
            ++posY;
        }
        motionX = motionY = motionZ = 0.0;
        rotationPitch = 0.0f;
    }
}

void Entity::setSize(float widthIn, float heightIn)
{
    if (widthIn != width || heightIn != height) {
        const float f = width;
        width = widthIn;
        height = heightIn;
        const AxisAlignedBB& bb = getEntityBoundingBox();
        setEntityBoundingBox(AxisAlignedBB(bb.minX, bb.minY, bb.minZ, bb.minX + static_cast<double>(width), bb.minY + static_cast<double>(height),
            bb.minZ + static_cast<double>(width)));
        if (width > f && !firstUpdate && !worldObj->isRemote)
            moveEntity(static_cast<double>(f - width), 0.0, static_cast<double>(f - width));
    }
}

void Entity::setRotation(float yaw, float pitch)
{
    rotationYaw = std::fmod(yaw, 360.0f);
    rotationPitch = std::fmod(pitch, 360.0f);
}

void Entity::setPosition(double x, double y, double z)
{
    posX = x;
    posY = y;
    posZ = z;
    const float f = width / 2.0f;
    const float f1 = height;
    setEntityBoundingBox(AxisAlignedBB(x - static_cast<double>(f), y, z - static_cast<double>(f), x + static_cast<double>(f), y + static_cast<double>(f1),
        z + static_cast<double>(f)));
}

void Entity::setAngles(float yaw, float pitch)
{
    const float f = rotationPitch;
    const float f1 = rotationYaw;
    rotationYaw = static_cast<float>(static_cast<double>(rotationYaw) + static_cast<double>(yaw) * 0.15);
    rotationPitch = static_cast<float>(static_cast<double>(rotationPitch) - static_cast<double>(pitch) * 0.15);
    rotationPitch = MathHelper::clamp_float(rotationPitch, -90.0f, 90.0f);
    prevRotationPitch += rotationPitch - f;
    prevRotationYaw += rotationYaw - f1;
}

void Entity::onEntityUpdate()
{
    worldObj->theProfiler.startSection("entityBaseTick");
    if (ridingEntity && ridingEntity->isDead)
        ridingEntity = nullptr;
    prevDistanceWalkedModified = distanceWalkedModified;
    prevPosX = posX;
    prevPosY = posY;
    prevPosZ = posZ;
    prevRotationPitch = rotationPitch;
    prevRotationYaw = rotationYaw;
    if (!worldObj->isRemote) {
        worldObj->theProfiler.startSection("portal");
        const int i = getMaxInPortalTime();
        if (inPortal) {
            // Java: only when the server allows the Nether, which the integrated server does.
            if (!ridingEntity && portalCounter++ >= i) {
                portalCounter = i;
                timeUntilPortal = getPortalCooldown();
                travelToDimension(worldObj->provider->getDimensionId() == -1 ? 0 : -1);
            }
            inPortal = false;
        } else {
            if (portalCounter > 0)
                portalCounter -= 4;
            if (portalCounter < 0)
                portalCounter = 0;
        }
        if (timeUntilPortal > 0)
            --timeUntilPortal;
        worldObj->theProfiler.endSection();
    }
    spawnRunningParticles();
    handleWaterMovement();
    if (worldObj->isRemote) {
        fire = 0;
    } else if (fire > 0) {
        if (isImmuneToFireFlag) {
            fire -= 4;
            if (fire < 0)
                fire = 0;
        } else {
            if (fire % 20 == 0)
                attackEntityFrom(DamageSource::onFire, 1.0f);
            --fire;
        }
    }
    if (isInLava()) {
        setOnFireFromLava();
        fallDistance *= 0.5f;
    }
    if (posY < -64.0)
        kill();
    if (!worldObj->isRemote)
        setFlag(0, fire > 0);
    firstUpdate = false;
    worldObj->theProfiler.endSection();
}

void Entity::setOnFireFromLava()
{
    if (!isImmuneToFireFlag) {
        attackEntityFrom(DamageSource::lava, 4.0f);
        setFire(15);
    }
}

void Entity::setFire(int seconds)
{
    const int i = seconds * 20;
    // Java: EnchantmentProtection.getFireTimeForEntity -- fire protection on the armor shortens it (the
    // enchantments of living entities' equipment).
    if (fire < i)
        fire = i;
}

bool Entity::isOffsetPositionInLiquid(double x, double y, double z)
{
    return isLiquidPresentInAABB(getEntityBoundingBox().offset(x, y, z));
}

bool Entity::isLiquidPresentInAABB(const AxisAlignedBB& bb)
{
    return worldObj->getCollidingBoundingBoxes(this, bb).empty() && !worldObj->isAnyLiquid(bb);
}

void Entity::moveEntity(double x, double y, double z)
{
    if (noClip) {
        setEntityBoundingBox(getEntityBoundingBox().offset(x, y, z));
        resetPositionToBB();
        return;
    }
    worldObj->theProfiler.startSection("move");
    const double d0 = posX;
    const double d1 = posY;
    const double d2 = posZ;
    if (isInWeb) {
        isInWeb = false;
        x *= 0.25;
        y *= 0.05000000074505806;
        z *= 0.25;
        motionX = 0.0;
        motionY = 0.0;
        motionZ = 0.0;
    }
    double d3 = x;
    const double d4 = y;
    double d5 = z;
    const bool flag = onGround && isSneaking() && dynamic_cast<EntityPlayer*>(this) != nullptr;
    if (flag) {
        const double d6 = 0.05;
        const auto step = [d6](double& v) {
            if (v < d6 && v >= -d6)
                v = 0.0;
            else if (v > 0.0)
                v -= d6;
            else
                v += d6;
        };
        for (; x != 0.0 && worldObj->getCollidingBoundingBoxes(this, getEntityBoundingBox().offset(x, -1.0, 0.0)).empty(); d3 = x)
            step(x);
        for (; z != 0.0 && worldObj->getCollidingBoundingBoxes(this, getEntityBoundingBox().offset(0.0, -1.0, z)).empty(); d5 = z)
            step(z);
        for (; x != 0.0 && z != 0.0 && worldObj->getCollidingBoundingBoxes(this, getEntityBoundingBox().offset(x, -1.0, z)).empty(); d5 = z) {
            step(x);
            d3 = x;
            step(z);
        }
    }
    const std::vector<AxisAlignedBB> list1 = worldObj->getCollidingBoundingBoxes(this, getEntityBoundingBox().addCoord(x, y, z));
    const AxisAlignedBB axisalignedbb = getEntityBoundingBox();
    for (const AxisAlignedBB& bb : list1)
        y = bb.calculateYOffset(getEntityBoundingBox(), y);
    setEntityBoundingBox(getEntityBoundingBox().offset(0.0, y, 0.0));
    const bool flag1 = onGround || (d4 != y && d4 < 0.0);
    for (const AxisAlignedBB& bb : list1)
        x = bb.calculateXOffset(getEntityBoundingBox(), x);
    setEntityBoundingBox(getEntityBoundingBox().offset(x, 0.0, 0.0));
    for (const AxisAlignedBB& bb : list1)
        z = bb.calculateZOffset(getEntityBoundingBox(), z);
    setEntityBoundingBox(getEntityBoundingBox().offset(0.0, 0.0, z));
    if (stepHeight > 0.0f && flag1 && (d3 != x || d5 != z)) {
        const double d11 = x;
        const double d7 = y;
        const double d8 = z;
        const AxisAlignedBB axisalignedbb3 = getEntityBoundingBox();
        setEntityBoundingBox(axisalignedbb);
        y = static_cast<double>(stepHeight);
        const std::vector<AxisAlignedBB> list = worldObj->getCollidingBoundingBoxes(this, getEntityBoundingBox().addCoord(d3, y, d5));
        AxisAlignedBB axisalignedbb4 = getEntityBoundingBox();
        const AxisAlignedBB axisalignedbb5 = axisalignedbb4.addCoord(d3, 0.0, d5);
        double d9 = y;
        for (const AxisAlignedBB& bb : list)
            d9 = bb.calculateYOffset(axisalignedbb5, d9);
        axisalignedbb4 = axisalignedbb4.offset(0.0, d9, 0.0);
        double d15 = d3;
        for (const AxisAlignedBB& bb : list)
            d15 = bb.calculateXOffset(axisalignedbb4, d15);
        axisalignedbb4 = axisalignedbb4.offset(d15, 0.0, 0.0);
        double d16 = d5;
        for (const AxisAlignedBB& bb : list)
            d16 = bb.calculateZOffset(axisalignedbb4, d16);
        axisalignedbb4 = axisalignedbb4.offset(0.0, 0.0, d16);
        AxisAlignedBB axisalignedbb14 = getEntityBoundingBox();
        double d17 = y;
        for (const AxisAlignedBB& bb : list)
            d17 = bb.calculateYOffset(axisalignedbb14, d17);
        axisalignedbb14 = axisalignedbb14.offset(0.0, d17, 0.0);
        double d18 = d3;
        for (const AxisAlignedBB& bb : list)
            d18 = bb.calculateXOffset(axisalignedbb14, d18);
        axisalignedbb14 = axisalignedbb14.offset(d18, 0.0, 0.0);
        double d19 = d5;
        for (const AxisAlignedBB& bb : list)
            d19 = bb.calculateZOffset(axisalignedbb14, d19);
        axisalignedbb14 = axisalignedbb14.offset(0.0, 0.0, d19);
        const double d20 = d15 * d15 + d16 * d16;
        const double d10 = d18 * d18 + d19 * d19;
        if (d20 > d10) {
            x = d15;
            z = d16;
            y = -d9;
            setEntityBoundingBox(axisalignedbb4);
        } else {
            x = d18;
            z = d19;
            y = -d17;
            setEntityBoundingBox(axisalignedbb14);
        }
        for (const AxisAlignedBB& bb : list)
            y = bb.calculateYOffset(getEntityBoundingBox(), y);
        setEntityBoundingBox(getEntityBoundingBox().offset(0.0, y, 0.0));
        if (d11 * d11 + d8 * d8 >= x * x + z * z) {
            x = d11;
            y = d7;
            z = d8;
            setEntityBoundingBox(axisalignedbb3);
        }
    }
    worldObj->theProfiler.endSection();
    worldObj->theProfiler.startSection("rest");
    resetPositionToBB();
    isCollidedHorizontally = d3 != x || d5 != z;
    isCollidedVertically = d4 != y;
    onGround = isCollidedVertically && d4 < 0.0;
    isCollided = isCollidedHorizontally || isCollidedVertically;
    const int i = MathHelper::floor_double(posX);
    const int j = MathHelper::floor_double(posY - 0.20000000298023224);
    const int k = MathHelper::floor_double(posZ);
    BlockPos blockpos(i, j, k);
    Block* block1 = worldObj->getBlockState(blockpos)->getBlock();
    if (&block1->getMaterial() == &Material::air) {
        Block* block = worldObj->getBlockState(blockpos.down())->getBlock();
        if (dynamic_cast<BlockFence*>(block) || dynamic_cast<BlockWall*>(block) || dynamic_cast<BlockFenceGate*>(block)) {
            block1 = block;
            blockpos = blockpos.down();
        }
    }
    updateFallState(y, onGround, block1, blockpos);
    if (d3 != x)
        motionX = 0.0;
    if (d5 != z)
        motionZ = 0.0;
    if (d4 != y)
        block1->onLanded(worldObj, this);
    if (canTriggerWalking() && !flag && !ridingEntity) {
        const double d12 = posX - d0;
        double d13 = posY - d1;
        const double d14 = posZ - d2;
        if (block1 != Blocks::ladder)
            d13 = 0.0;
        if (block1 && onGround)
            block1->onEntityCollidedWithBlock(worldObj, blockpos, this);
        distanceWalkedModified = static_cast<float>(static_cast<double>(distanceWalkedModified) + static_cast<double>(MathHelper::sqrt_double(d12 * d12 + d14 * d14)) * 0.6);
        distanceWalkedOnStepModified = static_cast<float>(
            static_cast<double>(distanceWalkedOnStepModified) + static_cast<double>(MathHelper::sqrt_double(d12 * d12 + d13 * d13 + d14 * d14)) * 0.6);
        if (distanceWalkedOnStepModified > static_cast<float>(nextStepDistance) && &block1->getMaterial() != &Material::air) {
            nextStepDistance = static_cast<int>(distanceWalkedOnStepModified) + 1;
            if (isInWater()) {
                float f = MathHelper::sqrt_double(motionX * motionX * 0.20000000298023224 + motionY * motionY + motionZ * motionZ * 0.20000000298023224) * 0.35f;
                if (f > 1.0f)
                    f = 1.0f;
                const float a = rand.nextFloat();
                const float b = rand.nextFloat();
                playSound(getSwimSound(), f, 1.0f + (a - b) * 0.4f);
            }
            playStepSound(blockpos, block1);
        }
    }
    doBlockCollisions();
    const bool flag2 = isWet();
    if (worldObj->isFlammableWithin(getEntityBoundingBox().contract(0.001, 0.001, 0.001))) {
        dealFireDamage(1);
        if (!flag2) {
            ++fire;
            if (fire == 0)
                setFire(8);
        }
    } else if (fire <= 0) {
        fire = -fireResistance;
    }
    if (flag2 && fire > 0) {
        const float a = rand.nextFloat();
        const float b = rand.nextFloat();
        playSound("random.fizz", 0.7f, 1.6f + (a - b) * 0.4f);
        fire = -fireResistance;
    }
    worldObj->theProfiler.endSection();
}

void Entity::resetPositionToBB()
{
    const AxisAlignedBB& bb = getEntityBoundingBox();
    posX = (bb.minX + bb.maxX) / 2.0;
    posY = bb.minY;
    posZ = (bb.minZ + bb.maxZ) / 2.0;
}

void Entity::doBlockCollisions()
{
    const AxisAlignedBB& bb = getEntityBoundingBox();
    const BlockPos blockpos(bb.minX + 0.001, bb.minY + 0.001, bb.minZ + 0.001);
    const BlockPos blockpos1(bb.maxX - 0.001, bb.maxY - 0.001, bb.maxZ - 0.001);
    if (worldObj->isAreaLoaded(blockpos, blockpos1)) {
        for (int i = blockpos.getX(); i <= blockpos1.getX(); ++i) {
            for (int j = blockpos.getY(); j <= blockpos1.getY(); ++j) {
                for (int k = blockpos.getZ(); k <= blockpos1.getZ(); ++k) {
                    const BlockPos blockpos2(i, j, k);
                    const IBlockState* iblockstate = worldObj->getBlockState(blockpos2);
                    iblockstate->getBlock()->onEntityCollidedWithBlock(worldObj, blockpos2, iblockstate, this);
                }
            }
        }
    }
}

void Entity::playStepSound(const BlockPos& pos, Block* blockIn)
{
    const Block::SoundType* soundtype = blockIn->stepSound;
    if (worldObj->getBlockState(pos.up())->getBlock() == Blocks::snow_layer) {
        soundtype = Blocks::snow_layer->stepSound;
        playSound(soundtype->getStepSound(), soundtype->getVolume() * 0.15f, soundtype->getFrequency());
    } else if (!blockIn->getMaterial().isLiquid()) {
        playSound(soundtype->getStepSound(), soundtype->getVolume() * 0.15f, soundtype->getFrequency());
    }
}

void Entity::playSound(const std::string& name, float volume, float pitch)
{
    if (!isSilent())
        worldObj->playSoundAtEntity(this, name, volume, pitch);
}

void Entity::updateFallState(double y, bool onGroundIn, Block* blockIn, const BlockPos& pos)
{
    if (onGroundIn) {
        if (fallDistance > 0.0f) {
            if (blockIn)
                blockIn->onFallenUpon(worldObj, pos, this, fallDistance);
            else
                fall(fallDistance, 1.0f);
            fallDistance = 0.0f;
        }
    } else if (y < 0.0) {
        fallDistance = static_cast<float>(static_cast<double>(fallDistance) - y);
    }
}

void Entity::dealFireDamage(int amount)
{
    if (!isImmuneToFireFlag)
        attackEntityFrom(DamageSource::inFire, static_cast<float>(amount));
}

void Entity::fall(float distance, float damageMultiplier)
{
    if (riddenByEntity)
        riddenByEntity->fall(distance, damageMultiplier);
}

bool Entity::isWet()
{
    return inWater || worldObj->isRainingAt(BlockPos(posX, posY, posZ)) || worldObj->isRainingAt(BlockPos(posX, posY + static_cast<double>(height), posZ));
}

bool Entity::handleWaterMovement()
{
    if (worldObj->handleMaterialAcceleration(getEntityBoundingBox().expand(0.0, -0.4000000059604645, 0.0).contract(0.001, 0.001, 0.001), Material::water, this)) {
        if (!inWater && !firstUpdate)
            resetHeight();
        fallDistance = 0.0f;
        inWater = true;
        fire = 0;
    } else {
        inWater = false;
    }
    return inWater;
}

void Entity::resetHeight()
{
    float f = MathHelper::sqrt_double(motionX * motionX * 0.20000000298023224 + motionY * motionY + motionZ * motionZ * 0.20000000298023224) * 0.2f;
    if (f > 1.0f)
        f = 1.0f;
    {
        const float a = rand.nextFloat();
        const float b = rand.nextFloat();
        playSound(getSplashSound(), f, 1.0f + (a - b) * 0.4f);
    }
    const float f1 = static_cast<float>(MathHelper::floor_double(getEntityBoundingBox().minY));
    for (int i = 0; static_cast<float>(i) < 1.0f + width * 20.0f; ++i) {
        const float f2 = (rand.nextFloat() * 2.0f - 1.0f) * width;
        const float f3 = (rand.nextFloat() * 2.0f - 1.0f) * width;
        worldObj->spawnParticle(static_cast<int>(EnumParticleTypes::WATER_BUBBLE), false, posX + static_cast<double>(f2), static_cast<double>(f1 + 1.0f),
            posZ + static_cast<double>(f3), motionX, motionY - static_cast<double>(rand.nextFloat() * 0.2f), motionZ);
    }
    for (int j = 0; static_cast<float>(j) < 1.0f + width * 20.0f; ++j) {
        const float f4 = (rand.nextFloat() * 2.0f - 1.0f) * width;
        const float f5 = (rand.nextFloat() * 2.0f - 1.0f) * width;
        worldObj->spawnParticle(static_cast<int>(EnumParticleTypes::WATER_SPLASH), false, posX + static_cast<double>(f4), static_cast<double>(f1 + 1.0f),
            posZ + static_cast<double>(f5), motionX, motionY, motionZ);
    }
}

void Entity::spawnRunningParticles()
{
    if (isSprinting() && !isInWater())
        createRunningParticles();
}

void Entity::createRunningParticles()
{
    const int i = MathHelper::floor_double(posX);
    const int j = MathHelper::floor_double(posY - 0.20000000298023224);
    const int k = MathHelper::floor_double(posZ);
    const BlockPos blockpos(i, j, k);
    const IBlockState* iblockstate = worldObj->getBlockState(blockpos);
    const Block* block = iblockstate->getBlock();
    if (block->getRenderType() != -1) {
        const double x = posX + (static_cast<double>(rand.nextFloat()) - 0.5) * static_cast<double>(width);
        const double z = posZ + (static_cast<double>(rand.nextFloat()) - 0.5) * static_cast<double>(width);
        worldObj->spawnParticle(static_cast<int>(EnumParticleTypes::BLOCK_CRACK), false, x, getEntityBoundingBox().minY + 0.1, z, -motionX * 4.0, 1.5,
            -motionZ * 4.0, { Block::getStateId(iblockstate) });
    }
}

bool Entity::isInsideOfMaterial(const Material& materialIn)
{
    const double d0 = posY + static_cast<double>(getEyeHeight());
    const BlockPos blockpos(posX, d0, posZ);
    const IBlockState* iblockstate = worldObj->getBlockState(blockpos);
    const Block* block = iblockstate->getBlock();
    if (&block->getMaterial() == &materialIn) {
        const float f = BlockLiquid::getLiquidHeightPercent(block->getMetaFromState(iblockstate)) - 0.11111111f;
        const float f1 = static_cast<float>(blockpos.getY() + 1) - f;
        const bool flag = d0 < static_cast<double>(f1);
        return !flag && dynamic_cast<EntityPlayer*>(this) ? false : flag;
    }
    return false;
}

bool Entity::isInLava()
{
    return worldObj->isMaterialInBB(getEntityBoundingBox().expand(-0.10000000149011612, -0.4000000059604645, -0.10000000149011612), Material::lava);
}

void Entity::moveFlying(float strafe, float forward, float friction)
{
    float f = strafe * strafe + forward * forward;
    if (f >= 1.0E-4f) {
        f = MathHelper::sqrt_float(f);
        if (f < 1.0f)
            f = 1.0f;
        f = friction / f;
        strafe = strafe * f;
        forward = forward * f;
        const float f1 = MathHelper::sin(rotationYaw * 3.1415927f / 180.0f);
        const float f2 = MathHelper::cos(rotationYaw * 3.1415927f / 180.0f);
        motionX += static_cast<double>(strafe * f2 - forward * f1);
        motionZ += static_cast<double>(forward * f2 + strafe * f1);
    }
}

int Entity::getBrightnessForRender(float)
{
    const BlockPos blockpos(posX, posY + static_cast<double>(getEyeHeight()), posZ);
    return worldObj->isBlockLoaded(blockpos) ? worldObj->getCombinedLight(blockpos, 0) : 0;
}

float Entity::getBrightness(float)
{
    const BlockPos blockpos(posX, posY + static_cast<double>(getEyeHeight()), posZ);
    return worldObj->isBlockLoaded(blockpos) ? worldObj->getLightBrightness(blockpos) : 0.0f;
}

void Entity::setPositionAndRotation(double x, double y, double z, float yaw, float pitch)
{
    prevPosX = posX = x;
    prevPosY = posY = y;
    prevPosZ = posZ = z;
    prevRotationYaw = rotationYaw = yaw;
    prevRotationPitch = rotationPitch = pitch;
    const double d0 = static_cast<double>(prevRotationYaw - yaw);
    if (d0 < -180.0)
        prevRotationYaw += 360.0f;
    if (d0 >= 180.0)
        prevRotationYaw -= 360.0f;
    setPosition(posX, posY, posZ);
    setRotation(yaw, pitch);
}

void Entity::moveToBlockPosAndAngles(const BlockPos& pos, float yaw, float pitch)
{
    setLocationAndAngles(static_cast<double>(pos.getX()) + 0.5, static_cast<double>(pos.getY()), static_cast<double>(pos.getZ()) + 0.5, yaw, pitch);
}

void Entity::setLocationAndAngles(double x, double y, double z, float yaw, float pitch)
{
    lastTickPosX = prevPosX = posX = x;
    lastTickPosY = prevPosY = posY = y;
    lastTickPosZ = prevPosZ = posZ = z;
    rotationYaw = yaw;
    rotationPitch = pitch;
    setPosition(posX, posY, posZ);
}

float Entity::getDistanceToEntity(const Entity* entityIn) const
{
    const float f = static_cast<float>(posX - entityIn->posX);
    const float f1 = static_cast<float>(posY - entityIn->posY);
    const float f2 = static_cast<float>(posZ - entityIn->posZ);
    return MathHelper::sqrt_float(f * f + f1 * f1 + f2 * f2);
}

double Entity::getDistanceSq(double x, double y, double z) const
{
    const double d0 = posX - x;
    const double d1 = posY - y;
    const double d2 = posZ - z;
    return d0 * d0 + d1 * d1 + d2 * d2;
}

double Entity::getDistance(double x, double y, double z) const
{
    const double d0 = posX - x;
    const double d1 = posY - y;
    const double d2 = posZ - z;
    return static_cast<double>(MathHelper::sqrt_double(d0 * d0 + d1 * d1 + d2 * d2));
}

double Entity::getDistanceSqToEntity(const Entity* entityIn) const
{
    const double d0 = posX - entityIn->posX;
    const double d1 = posY - entityIn->posY;
    const double d2 = posZ - entityIn->posZ;
    return d0 * d0 + d1 * d1 + d2 * d2;
}

void Entity::applyEntityCollision(Entity* entityIn)
{
    if (entityIn->riddenByEntity.get() != this && entityIn->ridingEntity.get() != this) {
        if (!entityIn->noClip && !noClip) {
            double d0 = entityIn->posX - posX;
            double d1 = entityIn->posZ - posZ;
            double d2 = MathHelper::abs_max(d0, d1);
            if (d2 >= 0.009999999776482582) {
                d2 = static_cast<double>(MathHelper::sqrt_double(d2));
                d0 = d0 / d2;
                d1 = d1 / d2;
                double d3 = 1.0 / d2;
                if (d3 > 1.0)
                    d3 = 1.0;
                d0 = d0 * d3;
                d1 = d1 * d3;
                d0 = d0 * 0.05000000074505806;
                d1 = d1 * 0.05000000074505806;
                d0 = d0 * static_cast<double>(1.0f - entityCollisionReduction);
                d1 = d1 * static_cast<double>(1.0f - entityCollisionReduction);
                if (!riddenByEntity)
                    addVelocity(-d0, 0.0, -d1);
                if (!entityIn->riddenByEntity)
                    entityIn->addVelocity(d0, 0.0, d1);
            }
        }
    }
}

void Entity::addVelocity(double x, double y, double z)
{
    motionX += x;
    motionY += y;
    motionZ += z;
    isAirBorne = true;
}

bool Entity::attackEntityFrom(const DamageSource& source, float)
{
    if (isEntityInvulnerable(source))
        return false;
    setBeenAttacked();
    return false;
}

Vec3 Entity::getLook(float partialTicks)
{
    if (partialTicks == 1.0f)
        return getVectorForRotation(rotationPitch, rotationYaw);
    const float f = prevRotationPitch + (rotationPitch - prevRotationPitch) * partialTicks;
    const float f1 = prevRotationYaw + (rotationYaw - prevRotationYaw) * partialTicks;
    return getVectorForRotation(f, f1);
}

Vec3 Entity::getVectorForRotation(float pitch, float yaw) const
{
    const float f = MathHelper::cos(-yaw * 0.017453292f - 3.1415927f);
    const float f1 = MathHelper::sin(-yaw * 0.017453292f - 3.1415927f);
    const float f2 = -MathHelper::cos(-pitch * 0.017453292f);
    const float f3 = MathHelper::sin(-pitch * 0.017453292f);
    return Vec3(static_cast<double>(f1 * f2), static_cast<double>(f3), static_cast<double>(f * f2));
}

Vec3 Entity::getPositionEyes(float partialTicks)
{
    if (partialTicks == 1.0f)
        return Vec3(posX, posY + static_cast<double>(getEyeHeight()), posZ);
    const double d0 = prevPosX + (posX - prevPosX) * static_cast<double>(partialTicks);
    const double d1 = prevPosY + (posY - prevPosY) * static_cast<double>(partialTicks) + static_cast<double>(getEyeHeight());
    const double d2 = prevPosZ + (posZ - prevPosZ) * static_cast<double>(partialTicks);
    return Vec3(d0, d1, d2);
}

std::optional<MovingObjectPosition> Entity::rayTrace(double blockReachDistance, float partialTicks)
{
    const Vec3 vec3 = getPositionEyes(partialTicks);
    const Vec3 vec31 = getLook(partialTicks);
    const Vec3 vec32 = vec3.addVector(vec31.xCoord * blockReachDistance, vec31.yCoord * blockReachDistance, vec31.zCoord * blockReachDistance);
    return worldObj->rayTraceBlocks(vec3, vec32, false, false, true);
}

bool Entity::isInRangeToRender3d(double x, double y, double z)
{
    const double d0 = posX - x;
    const double d1 = posY - y;
    const double d2 = posZ - z;
    return isInRangeToRenderDist(d0 * d0 + d1 * d1 + d2 * d2);
}

bool Entity::isInRangeToRenderDist(double distance)
{
    double d0 = getEntityBoundingBox().getAverageEdgeLength();
    if (std::isnan(d0))
        d0 = 1.0;
    d0 = d0 * 64.0 * renderDistanceWeight;
    return distance < d0 * d0;
}

bool Entity::writeMountToNBT(NBTTagCompound& tagCompund)
{
    const auto s = getEntityString();
    if (!isDead && s) {
        tagCompund.setString("id", *s);
        writeToNBT(tagCompund);
        return true;
    }
    return false;
}

bool Entity::writeToNBTOptional(NBTTagCompound& tagCompund)
{
    const auto s = getEntityString();
    if (!isDead && s && !riddenByEntity) {
        tagCompund.setString("id", *s);
        writeToNBT(tagCompund);
        return true;
    }
    return false;
}

void Entity::writeToNBT(NBTTagCompound& tagCompund)
{
    tagCompund.setTag("Pos", newDoubleNBTList({ posX, posY, posZ }));
    tagCompund.setTag("Motion", newDoubleNBTList({ motionX, motionY, motionZ }));
    tagCompund.setTag("Rotation", newFloatNBTList({ rotationYaw, rotationPitch }));
    tagCompund.setFloat("FallDistance", fallDistance);
    tagCompund.setShort("Fire", static_cast<int16_t>(fire));
    tagCompund.setShort("Air", static_cast<int16_t>(getAir()));
    tagCompund.setBoolean("OnGround", onGround);
    tagCompund.setInteger("Dimension", dimension);
    tagCompund.setBoolean("Invulnerable", invulnerable);
    tagCompund.setInteger("PortalCooldown", timeUntilPortal);
    tagCompund.setLong("UUIDMost", getUniqueID().getMostSignificantBits());
    tagCompund.setLong("UUIDLeast", getUniqueID().getLeastSignificantBits());
    if (!getCustomNameTag().empty()) {
        tagCompund.setString("CustomName", getCustomNameTag());
        tagCompund.setBoolean("CustomNameVisible", getAlwaysRenderNameTag());
    }
    // Java: cmdResultStats.writeStatsToNBT -- the command statistics (stage 12); nothing set means nothing written.
    if (isSilent())
        tagCompund.setBoolean("Silent", isSilent());
    writeEntityToNBT(tagCompund);
    if (ridingEntity) {
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        if (ridingEntity->writeMountToNBT(*nbttagcompound))
            tagCompund.setTag("Riding", nbttagcompound);
    }
}

void Entity::readFromNBT(const NBTTagCompound& tagCompund)
{
    const auto nbttaglist = tagCompund.getTagList("Pos", 6);
    const auto nbttaglist1 = tagCompund.getTagList("Motion", 6);
    const auto nbttaglist2 = tagCompund.getTagList("Rotation", 5);
    motionX = nbttaglist1->getDoubleAt(0);
    motionY = nbttaglist1->getDoubleAt(1);
    motionZ = nbttaglist1->getDoubleAt(2);
    if (std::abs(motionX) > 10.0)
        motionX = 0.0;
    if (std::abs(motionY) > 10.0)
        motionY = 0.0;
    if (std::abs(motionZ) > 10.0)
        motionZ = 0.0;
    prevPosX = lastTickPosX = posX = nbttaglist->getDoubleAt(0);
    prevPosY = lastTickPosY = posY = nbttaglist->getDoubleAt(1);
    prevPosZ = lastTickPosZ = posZ = nbttaglist->getDoubleAt(2);
    prevRotationYaw = rotationYaw = nbttaglist2->getFloatAt(0);
    prevRotationPitch = rotationPitch = nbttaglist2->getFloatAt(1);
    setRotationYawHead(rotationYaw);
    setRenderYawOffset(rotationYaw);
    fallDistance = tagCompund.getFloat("FallDistance");
    fire = tagCompund.getShort("Fire");
    setAir(tagCompund.getShort("Air"));
    onGround = tagCompund.getBoolean("OnGround");
    dimension = tagCompund.getInteger("Dimension");
    invulnerable = tagCompund.getBoolean("Invulnerable");
    timeUntilPortal = tagCompund.getInteger("PortalCooldown");
    if (tagCompund.hasKey("UUIDMost", 4) && tagCompund.hasKey("UUIDLeast", 4)) {
        entityUniqueID = UUID(tagCompund.getLong("UUIDMost"), tagCompund.getLong("UUIDLeast"));
    } else if (tagCompund.hasKey("UUID", 8)) {
        // Java: UUID.fromString throws on a bad string.
        if (const auto parsed = UUID::fromString(tagCompund.getString("UUID")))
            entityUniqueID = *parsed;
    }
    setPosition(posX, posY, posZ);
    setRotation(rotationYaw, rotationPitch);
    if (tagCompund.hasKey("CustomName", 8) && !tagCompund.getString("CustomName").empty())
        setCustomNameTag(tagCompund.getString("CustomName"));
    setAlwaysRenderNameTag(tagCompund.getBoolean("CustomNameVisible"));
    setSilent(tagCompund.getBoolean("Silent"));
    readEntityFromNBT(tagCompund);
    if (shouldSetPosAfterLoading())
        setPosition(posX, posY, posZ);
}

std::optional<std::string> Entity::getEntityString() const
{
    return EntityList::getEntityString(getClassName());
}

std::shared_ptr<NBTTagList> Entity::newDoubleNBTList(std::initializer_list<double> numbers)
{
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (const double d0 : numbers)
        nbttaglist->appendTag(std::make_shared<NBTTagDouble>(d0));
    return nbttaglist;
}

std::shared_ptr<NBTTagList> Entity::newFloatNBTList(std::initializer_list<float> numbers)
{
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (const float f : numbers)
        nbttaglist->appendTag(std::make_shared<NBTTagFloat>(f));
    return nbttaglist;
}

bool Entity::isEntityInsideOpaqueBlock()
{
    if (noClip)
        return false;
    BlockPos mutablePos(INT32_MIN, INT32_MIN, INT32_MIN);
    for (int i = 0; i < 8; ++i) {
        const int j = MathHelper::floor_double(posY + static_cast<double>((static_cast<float>((i >> 0) % 2) - 0.5f) * 0.1f) + static_cast<double>(getEyeHeight()));
        const int k = MathHelper::floor_double(posX + static_cast<double>((static_cast<float>((i >> 1) % 2) - 0.5f) * width * 0.8f));
        const int l = MathHelper::floor_double(posZ + static_cast<double>((static_cast<float>((i >> 2) % 2) - 0.5f) * width * 0.8f));
        if (mutablePos.getX() != k || mutablePos.getY() != j || mutablePos.getZ() != l) {
            mutablePos.set(k, j, l);
            if (worldObj->getBlockState(mutablePos)->getBlock()->isVisuallyOpaque())
                return true;
        }
    }
    return false;
}

void Entity::updateRidden()
{
    if (ridingEntity->isDead) {
        ridingEntity = nullptr;
        return;
    }
    motionX = 0.0;
    motionY = 0.0;
    motionZ = 0.0;
    onUpdate();
    if (ridingEntity) {
        ridingEntity->updateRiderPosition();
        entityRiderYawDelta += static_cast<double>(ridingEntity->rotationYaw - ridingEntity->prevRotationYaw);
        for (entityRiderPitchDelta += static_cast<double>(ridingEntity->rotationPitch - ridingEntity->prevRotationPitch); entityRiderYawDelta >= 180.0;
             entityRiderYawDelta -= 360.0) {
        }
        while (entityRiderYawDelta < -180.0)
            entityRiderYawDelta += 360.0;
        while (entityRiderPitchDelta >= 180.0)
            entityRiderPitchDelta -= 360.0;
        while (entityRiderPitchDelta < -180.0)
            entityRiderPitchDelta += 360.0;
        double d0 = entityRiderYawDelta * 0.5;
        double d1 = entityRiderPitchDelta * 0.5;
        const float f = 10.0f;
        if (d0 > static_cast<double>(f))
            d0 = static_cast<double>(f);
        if (d0 < static_cast<double>(-f))
            d0 = static_cast<double>(-f);
        if (d1 > static_cast<double>(f))
            d1 = static_cast<double>(f);
        if (d1 < static_cast<double>(-f))
            d1 = static_cast<double>(-f);
        entityRiderYawDelta -= d0;
        entityRiderPitchDelta -= d1;
    }
}

void Entity::updateRiderPosition()
{
    if (riddenByEntity)
        riddenByEntity->setPosition(posX, posY + getMountedYOffset() + riddenByEntity->getYOffset(), posZ);
}

void Entity::mountEntity(Entity* entityIn)
{
    entityRiderPitchDelta = 0.0;
    entityRiderYawDelta = 0.0;
    if (!entityIn) {
        if (ridingEntity) {
            setLocationAndAngles(ridingEntity->posX, ridingEntity->getEntityBoundingBox().minY + static_cast<double>(ridingEntity->height), ridingEntity->posZ,
                rotationYaw, rotationPitch);
            ridingEntity->riddenByEntity = nullptr;
        }
        ridingEntity = nullptr;
    } else {
        if (ridingEntity)
            ridingEntity->riddenByEntity = nullptr;
        for (Entity* entity = entityIn->ridingEntity.get(); entity; entity = entity->ridingEntity.get()) {
            if (entity == this)
                return;
        }
        ridingEntity = entityIn->shared_from_this();
        entityIn->riddenByEntity = shared_from_this();
    }
}

void Entity::setPositionAndRotation2(double x, double y, double z, float yaw, float pitch, int, bool)
{
    setPosition(x, y, z);
    setRotation(yaw, pitch);
    const std::vector<AxisAlignedBB> list = worldObj->getCollidingBoundingBoxes(this, getEntityBoundingBox().contract(0.03125, 0.0, 0.03125));
    if (!list.empty()) {
        double d0 = 0.0;
        for (const AxisAlignedBB& bb : list) {
            if (bb.maxY > d0)
                d0 = bb.maxY;
        }
        y = y + (d0 - getEntityBoundingBox().minY);
        setPosition(x, y, z);
    }
}

void Entity::setPortal(const BlockPos& pos)
{
    if (timeUntilPortal > 0) {
        timeUntilPortal = getPortalCooldown();
        return;
    }
    // Java: on the server, the portal's pattern sets lastPortalPos/Vec and teleportDirection (stage 9 with the
    // dimensions).
    if (!worldObj->isRemote && !(lastPortalPos && *lastPortalPos == pos))
        lastPortalPos = pos;
    inPortal = true;
}

void Entity::setVelocity(double x, double y, double z)
{
    motionX = x;
    motionY = y;
    motionZ = z;
}

bool Entity::isBurning() const
{
    const bool flag = worldObj && worldObj->isRemote;
    return !isImmuneToFireFlag && (fire > 0 || (flag && getFlag(0)));
}

bool Entity::isInvisibleToPlayer(EntityPlayer*)
{
    // Java: false for spectators (stage 8 with the game modes of players).
    return isInvisible();
}

void Entity::setFlag(int flag, bool set)
{
    const int8_t b0 = dataWatcher.getWatchableObjectByte(0);
    if (set)
        dataWatcher.updateObject(0, static_cast<int8_t>(b0 | 1 << flag));
    else
        dataWatcher.updateObject(0, static_cast<int8_t>(b0 & ~(1 << flag)));
}

bool Entity::pushOutOfBlocks(double x, double y, double z)
{
    const BlockPos blockpos(x, y, z);
    const double d0 = x - static_cast<double>(blockpos.getX());
    const double d1 = y - static_cast<double>(blockpos.getY());
    const double d2 = z - static_cast<double>(blockpos.getZ());
    const std::vector<AxisAlignedBB> list = worldObj->getCollisionBoxes(getEntityBoundingBox());
    if (list.empty() && !worldObj->isBlockFullCube(blockpos))
        return false;
    int i = 3;
    double d3 = 9999.0;
    if (!worldObj->isBlockFullCube(blockpos.west()) && d0 < d3) {
        d3 = d0;
        i = 0;
    }
    if (!worldObj->isBlockFullCube(blockpos.east()) && 1.0 - d0 < d3) {
        d3 = 1.0 - d0;
        i = 1;
    }
    if (!worldObj->isBlockFullCube(blockpos.up()) && 1.0 - d1 < d3) {
        d3 = 1.0 - d1;
        i = 3;
    }
    if (!worldObj->isBlockFullCube(blockpos.north()) && d2 < d3) {
        d3 = d2;
        i = 4;
    }
    if (!worldObj->isBlockFullCube(blockpos.south()) && 1.0 - d2 < d3) {
        d3 = 1.0 - d2;
        i = 5;
    }
    const float f = rand.nextFloat() * 0.2f + 0.1f;
    if (i == 0)
        motionX = static_cast<double>(-f);
    if (i == 1)
        motionX = static_cast<double>(f);
    if (i == 3)
        motionY = static_cast<double>(f);
    if (i == 4)
        motionZ = static_cast<double>(-f);
    if (i == 5)
        motionZ = static_cast<double>(f);
    return true;
}

std::string Entity::getName()
{
    if (hasCustomName())
        return getCustomNameTag();
    const std::string s = getEntityString().value_or("generic");
    return StatCollector::translateToLocal("entity." + s + ".name");
}

std::string Entity::toString()
{
    char buffer[256];
    std::snprintf(buffer, sizeof buffer, "%s['%s'/%d, l='%s', x=%.2f, y=%.2f, z=%.2f]", std::string(getClassName()).c_str(), getName().c_str(), entityId,
        worldObj ? worldObj->getWorldInfo().getWorldName().c_str() : "~NULL~", posX, posY, posZ);
    // Java formats with the default locale (decimal comma in ru_RU); the debug text only.
    return buffer;
}

bool Entity::isEntityInvulnerable(const DamageSource& source) const
{
    return invulnerable && &source != &DamageSource::outOfWorld && !source.isCreativePlayer();
}

void Entity::copyLocationAndAnglesFrom(const Entity* entityIn)
{
    setLocationAndAngles(entityIn->posX, entityIn->posY, entityIn->posZ, entityIn->rotationYaw, entityIn->rotationPitch);
}

void Entity::copyDataFromOld(Entity* entityIn)
{
    NBTTagCompound nbttagcompound;
    entityIn->writeToNBT(nbttagcompound);
    readFromNBT(nbttagcompound);
    timeUntilPortal = entityIn->timeUntilPortal;
    lastPortalPos = entityIn->lastPortalPos;
    lastPortalVec = entityIn->lastPortalVec;
    teleportDirection = entityIn->teleportDirection;
}

void Entity::travelToDimension(int)
{
    // Java: moves the entity to another WorldServer through the configuration manager -- stage 9 with the
    // dimensions.
}

std::shared_ptr<IChatComponent> Entity::getDisplayName()
{
    auto component = std::make_shared<ChatComponentText>(getName());
    // Java: also the hover event showing the entity and the UUID as insertion text.
    component->getChatStyle()->setInsertion(getUniqueID().toString());
    return component;
}

void Entity::setPositionAndUpdate(double x, double y, double z)
{
    setLocationAndAngles(x, y, z, rotationYaw, rotationPitch);
}

EnumFacing Entity::getHorizontalFacing() const
{
    return getHorizontal(MathHelper::floor_double(static_cast<double>(rotationYaw * 4.0f / 360.0f) + 0.5) & 3);
}

} // namespace mc
