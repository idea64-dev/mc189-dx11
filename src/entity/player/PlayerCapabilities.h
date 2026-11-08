#pragma once

// Original: net/minecraft/entity/player/PlayerCapabilities.java

namespace mc {

class NBTTagCompound;

class PlayerCapabilities {
public:
    void writeCapabilitiesToNBT(NBTTagCompound& tagCompound) const;
    void readCapabilitiesFromNBT(const NBTTagCompound& tagCompound);
    float getFlySpeed() const { return flySpeed; }
    void setFlySpeed(float speed) { flySpeed = speed; }
    float getWalkSpeed() const { return walkSpeed; }
    void setPlayerWalkSpeed(float speed) { walkSpeed = speed; }

    bool disableDamage = false;
    bool isFlying = false;
    bool allowFlying = false;
    bool isCreativeMode = false;
    bool allowEdit = true;

private:
    float flySpeed = 0.05f;
    float walkSpeed = 0.1f;
};

} // namespace mc
