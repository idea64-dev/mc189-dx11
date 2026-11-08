#include "tileentity/TileEntityBeacon.h"

#include "block/AllBlocks.h"
#include "init/Items.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "network/play/ServerPlayPackets.h"
#include "potion/Potion.h"
#include "world/World.h"

namespace mc {

namespace {

// Java: EntitySheep.getDyeRgb -- the fleece colour of each dye.
const std::array<float, 3>& getDyeRgb(EnumDyeColor dyeColor)
{
    static const std::array<std::array<float, 3>, 16> kDyeToRgb { {
        { 1.0f, 1.0f, 1.0f },
        { 0.85f, 0.5f, 0.2f },
        { 0.7f, 0.3f, 0.85f },
        { 0.4f, 0.6f, 0.85f },
        { 0.9f, 0.9f, 0.2f },
        { 0.5f, 0.8f, 0.1f },
        { 0.95f, 0.5f, 0.65f },
        { 0.3f, 0.3f, 0.3f },
        { 0.6f, 0.6f, 0.6f },
        { 0.3f, 0.5f, 0.6f },
        { 0.5f, 0.25f, 0.7f },
        { 0.2f, 0.3f, 0.7f },
        { 0.4f, 0.3f, 0.2f },
        { 0.4f, 0.5f, 0.2f },
        { 0.6f, 0.2f, 0.2f },
        { 0.1f, 0.1f, 0.1f },
    } };
    return kDyeToRgb[static_cast<size_t>(dyeColor)];
}

} // namespace

void TileEntityBeacon::update()
{
    if (worldObj->getTotalWorldTime() % 80 == 0)
        updateBeacon();
}

void TileEntityBeacon::updateBeacon()
{
    updateSegmentColors();
    addEffectsToPlayers();
}

void TileEntityBeacon::addEffectsToPlayers()
{
    if (isComplete && levels > 0 && !worldObj->isRemote && primaryEffect > 0) {
        // Java: gives the players within levels * 10 + 10 blocks (and all the way up) the primary effect for
        // 180 ticks (amplifier 1 at four levels with both effects equal), and the secondary one at four levels --
        // players and their potion effects come with stage 8.
    }
}

void TileEntityBeacon::updateSegmentColors()
{
    const int i = levels;
    const int j = pos.getX();
    const int k = pos.getY();
    const int l = pos.getZ();
    levels = 0;
    beamSegments.clear();
    isComplete = true;
    beamSegments.emplace_back(getDyeRgb(EnumDyeColor::WHITE));
    // Java keeps a reference to the last segment; here it is always beamSegments.back().
    bool flag = true;
    for (int i1 = k + 1; i1 < 256; ++i1) {
        const IBlockState* iblockstate = worldObj->getBlockState(BlockPos(j, i1, l));
        std::array<float, 3> afloat;
        if (iblockstate->getBlock() == Blocks::stained_glass) {
            afloat = getDyeRgb(iblockstate->getValue(BlockStainedGlass::COLOR));
        } else {
            if (iblockstate->getBlock() != Blocks::stained_glass_pane) {
                if (iblockstate->getBlock()->getLightOpacity() >= 15 && iblockstate->getBlock() != Blocks::bedrock) {
                    isComplete = false;
                    beamSegments.clear();
                    break;
                }
                beamSegments.back().incrementHeight();
                continue;
            }
            afloat = getDyeRgb(iblockstate->getValue(BlockStainedGlassPane::COLOR));
        }
        if (!flag) {
            const std::array<float, 3>& colors = beamSegments.back().getColors();
            afloat = { (colors[0] + afloat[0]) / 2.0f, (colors[1] + afloat[1]) / 2.0f, (colors[2] + afloat[2]) / 2.0f };
        }
        if (afloat == beamSegments.back().getColors())
            beamSegments.back().incrementHeight();
        else
            beamSegments.emplace_back(afloat);
        flag = false;
    }
    if (isComplete) {
        for (int l1 = 1; l1 <= 4; levels = l1++) {
            const int i2 = k - l1;
            if (i2 < 0)
                break;
            bool flag1 = true;
            for (int j1 = j - l1; j1 <= j + l1 && flag1; ++j1) {
                for (int k1 = l - l1; k1 <= l + l1; ++k1) {
                    const Block* block = worldObj->getBlockState(BlockPos(j1, i2, k1))->getBlock();
                    if (block != Blocks::emerald_block && block != Blocks::gold_block && block != Blocks::diamond_block && block != Blocks::iron_block) {
                        flag1 = false;
                        break;
                    }
                }
            }
            if (!flag1)
                break;
        }
        if (levels == 0)
            isComplete = false;
    }
    if (!worldObj->isRemote && levels == 4 && i < levels) {
        // Java: the players near the beacon get AchievementList.fullBeacon -- stage 8.
    }
}

float TileEntityBeacon::shouldBeamRender()
{
    if (!isComplete)
        return 0.0f;
    const int i = static_cast<int>(worldObj->getTotalWorldTime() - beamRenderCounter);
    beamRenderCounter = worldObj->getTotalWorldTime();
    if (i > 1) {
        field_146014_j -= static_cast<float>(i) / 40.0f;
        if (field_146014_j < 0.0f)
            field_146014_j = 0.0f;
    }
    field_146014_j += 0.025f;
    if (field_146014_j > 1.0f)
        field_146014_j = 1.0f;
    return field_146014_j;
}

int TileEntityBeacon::func_183001_h(int p_183001_1_)
{
    if (p_183001_1_ >= 0 && p_183001_1_ < static_cast<int>(Potion::potionTypes.size()) && Potion::potionTypes[static_cast<size_t>(p_183001_1_)] != nullptr) {
        const Potion* potion = Potion::potionTypes[static_cast<size_t>(p_183001_1_)];
        return potion != Potion::moveSpeed && potion != Potion::digSpeed && potion != Potion::resistance && potion != Potion::jump && potion != Potion::damageBoost
                && potion != Potion::regeneration
            ? 0
            : p_183001_1_;
    }
    return 0;
}

void TileEntityBeacon::readFromNBT(const NBTTagCompound& compound)
{
    TileEntityLockable::readFromNBT(compound);
    primaryEffect = func_183001_h(compound.getInteger("Primary"));
    secondaryEffect = func_183001_h(compound.getInteger("Secondary"));
    levels = compound.getInteger("Levels");
}

void TileEntityBeacon::writeToNBT(NBTTagCompound& compound) const
{
    TileEntityLockable::writeToNBT(compound);
    compound.setInteger("Primary", primaryEffect);
    compound.setInteger("Secondary", secondaryEffect);
    compound.setInteger("Levels", levels);
}

std::shared_ptr<ItemStack> TileEntityBeacon::decrStackSize(int index, int count)
{
    if (index == 0 && payment) {
        if (count >= payment->stackSize) {
            std::shared_ptr<ItemStack> itemstack = std::move(payment);
            payment = nullptr;
            return itemstack;
        }
        payment->stackSize -= count;
        return std::make_shared<ItemStack>(payment->getItem(), count, payment->getMetadata());
    }
    return nullptr;
}

std::shared_ptr<ItemStack> TileEntityBeacon::removeStackFromSlot(int index)
{
    if (index == 0 && payment) {
        std::shared_ptr<ItemStack> itemstack = std::move(payment);
        payment = nullptr;
        return itemstack;
    }
    return nullptr;
}

void TileEntityBeacon::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    if (index == 0)
        payment = std::move(stack);
}

bool TileEntityBeacon::isUseableByPlayer(EntityPlayer*)
{
    // Java: and the player within 8 blocks (stage 10).
    return worldObj->getTileEntity(pos) == this;
}

bool TileEntityBeacon::isItemValidForSlot(int, const ItemStack& stack) const
{
    return stack.getItem() == Items::emerald || stack.getItem() == Items::diamond || stack.getItem() == Items::gold_ingot || stack.getItem() == Items::iron_ingot;
}

int TileEntityBeacon::getField(int id) const
{
    switch (id) {
    case 0: return levels;
    case 1: return primaryEffect;
    case 2: return secondaryEffect;
    default: return 0;
    }
}

void TileEntityBeacon::setField(int id, int value)
{
    switch (id) {
    case 0: levels = value; break;
    case 1: primaryEffect = func_183001_h(value); break;
    case 2: secondaryEffect = func_183001_h(value); break;
    default: break;
    }
}

bool TileEntityBeacon::receiveClientEvent(int id, int type)
{
    if (id == 1) {
        updateBeacon();
        return true;
    }
    return TileEntityLockable::receiveClientEvent(id, type);
}

std::shared_ptr<Packet> TileEntityBeacon::getDescriptionPacket()
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    writeToNBT(*nbttagcompound);
    return std::make_shared<S35PacketUpdateTileEntity>(pos, 3, nbttagcompound);
}

} // namespace mc
