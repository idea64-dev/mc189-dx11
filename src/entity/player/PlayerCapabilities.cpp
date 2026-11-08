#include "entity/player/PlayerCapabilities.h"

#include "nbt/NBTTagCompound.h"

#include <memory>

namespace mc {

void PlayerCapabilities::writeCapabilitiesToNBT(NBTTagCompound& tagCompound) const
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    nbttagcompound->setBoolean("invulnerable", disableDamage);
    nbttagcompound->setBoolean("flying", isFlying);
    nbttagcompound->setBoolean("mayfly", allowFlying);
    nbttagcompound->setBoolean("instabuild", isCreativeMode);
    nbttagcompound->setBoolean("mayBuild", allowEdit);
    nbttagcompound->setFloat("flySpeed", flySpeed);
    nbttagcompound->setFloat("walkSpeed", walkSpeed);
    tagCompound.setTag("abilities", nbttagcompound);
}

void PlayerCapabilities::readCapabilitiesFromNBT(const NBTTagCompound& tagCompound)
{
    if (tagCompound.hasKey("abilities", 10)) {
        const auto nbttagcompound = tagCompound.getCompoundTag("abilities");
        disableDamage = nbttagcompound->getBoolean("invulnerable");
        isFlying = nbttagcompound->getBoolean("flying");
        allowFlying = nbttagcompound->getBoolean("mayfly");
        isCreativeMode = nbttagcompound->getBoolean("instabuild");
        if (nbttagcompound->hasKey("flySpeed", 99)) {
            flySpeed = nbttagcompound->getFloat("flySpeed");
            walkSpeed = nbttagcompound->getFloat("walkSpeed");
        }
        if (nbttagcompound->hasKey("mayBuild", 1))
            allowEdit = nbttagcompound->getBoolean("mayBuild");
    }
}

} // namespace mc
