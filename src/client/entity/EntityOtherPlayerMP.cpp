#include "client/entity/EntityOtherPlayerMP.h"

#include "client/Minecraft.h"
#include "client/gui/GuiIngame.h"
#include "item/ItemStack.h"
#include "util/MathHelper.h"

#include <cmath>

namespace mc {

void EntityOtherPlayerMP::construct()
{
    AbstractClientPlayer::construct();
    stepHeight = 0.0f;
    noClip = true;
    renderOffsetY = 0.25f;
    renderDistanceWeight = 10.0;
}

void EntityOtherPlayerMP::setPositionAndRotation2(double x, double y, double z, float yaw, float pitch, int posRotationIncrements, bool)
{
    otherPlayerMPX = x;
    otherPlayerMPY = y;
    otherPlayerMPZ = z;
    otherPlayerMPYaw = static_cast<double>(yaw);
    otherPlayerMPPitch = static_cast<double>(pitch);
    otherPlayerMPPosRotationIncrements = posRotationIncrements;
}

void EntityOtherPlayerMP::onUpdate()
{
    renderOffsetY = 0.0f;
    AbstractClientPlayer::onUpdate();
    prevLimbSwingAmount = limbSwingAmount;
    const double d0 = posX - prevPosX;
    const double d1 = posZ - prevPosZ;
    float f = MathHelper::sqrt_double(d0 * d0 + d1 * d1) * 4.0f;
    if (f > 1.0f)
        f = 1.0f;
    limbSwingAmount += (f - limbSwingAmount) * 0.4f;
    limbSwing += limbSwingAmount;
    const std::shared_ptr<ItemStack>& held = inventory.mainInventory[static_cast<size_t>(inventory.currentItem)];
    if (!isItemInUse && isEating() && held) {
        const std::shared_ptr<ItemStack> itemstack = held;
        setItemInUse(itemstack, itemstack->getItem()->getMaxItemUseDuration(*itemstack));
        isItemInUse = true;
    } else if (isItemInUse && !isEating()) {
        clearItemInUse();
        isItemInUse = false;
    }
}

void EntityOtherPlayerMP::onLivingUpdate()
{
    if (otherPlayerMPPosRotationIncrements > 0) {
        const double d0 = posX + (otherPlayerMPX - posX) / static_cast<double>(otherPlayerMPPosRotationIncrements);
        const double d1 = posY + (otherPlayerMPY - posY) / static_cast<double>(otherPlayerMPPosRotationIncrements);
        const double d2 = posZ + (otherPlayerMPZ - posZ) / static_cast<double>(otherPlayerMPPosRotationIncrements);
        double d3;
        for (d3 = otherPlayerMPYaw - static_cast<double>(rotationYaw); d3 < -180.0; d3 += 360.0) {
        }
        while (d3 >= 180.0)
            d3 -= 360.0;
        rotationYaw = static_cast<float>(static_cast<double>(rotationYaw) + d3 / static_cast<double>(otherPlayerMPPosRotationIncrements));
        rotationPitch = static_cast<float>(static_cast<double>(rotationPitch)
            + (otherPlayerMPPitch - static_cast<double>(rotationPitch)) / static_cast<double>(otherPlayerMPPosRotationIncrements));
        --otherPlayerMPPosRotationIncrements;
        setPosition(d0, d1, d2);
        setRotation(rotationYaw, rotationPitch);
    }
    prevCameraYaw = cameraYaw;
    updateArmSwingProgress();
    float f1 = MathHelper::sqrt_double(motionX * motionX + motionZ * motionZ);
    // Java: (float)Math.atan(...) * 15.0F -- cast before the multiply, unlike EntityPlayer.
    float f = static_cast<float>(std::atan(-motionY * 0.20000000298023224)) * 15.0f;
    if (f1 > 0.1f)
        f1 = 0.1f;
    if (!onGround || getHealth() <= 0.0f)
        f1 = 0.0f;
    if (onGround || getHealth() <= 0.0f)
        f = 0.0f;
    cameraYaw += (f1 - cameraYaw) * 0.4f;
    cameraPitch += (f - cameraPitch) * 0.8f;
}

void EntityOtherPlayerMP::setCurrentItemOrArmor(int slotIn, std::shared_ptr<ItemStack> stack)
{
    if (slotIn == 0)
        inventory.mainInventory[static_cast<size_t>(inventory.currentItem)] = std::move(stack);
    else
        inventory.armorInventory.at(static_cast<size_t>(slotIn - 1)) = std::move(stack);
}

void EntityOtherPlayerMP::addChatMessage(std::shared_ptr<IChatComponent> component)
{
    Minecraft::getMinecraft()->ingameGUI->getChatGUI().printChatMessage(component);
}

} // namespace mc
