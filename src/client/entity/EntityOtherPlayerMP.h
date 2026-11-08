#pragma once

// Original: net/minecraft/client/entity/EntityOtherPlayerMP.java -- another player as the client sees it.

#include "client/entity/AbstractClientPlayer.h"

namespace mc {

class EntityOtherPlayerMP : public AbstractClientPlayer {
public:
    EntityOtherPlayerMP(World* worldIn, GameProfile gameProfileIn) : AbstractClientPlayer(worldIn, std::move(gameProfileIn)) { }

    std::string_view getClassName() const override { return "EntityOtherPlayerMP"; }

    bool attackEntityFrom(const DamageSource&, float) override { return true; }
    void setPositionAndRotation2(double x, double y, double z, float yaw, float pitch, int posRotationIncrements, bool p_180426_10_) override;
    void onUpdate() override;
    void onLivingUpdate() override;
    void setCurrentItemOrArmor(int slotIn, std::shared_ptr<ItemStack> stack) override;
    void addChatMessage(std::shared_ptr<IChatComponent> component) override;
    bool canCommandSenderUseCommand(int, const std::string&) override { return false; }
    BlockPos getPosition() const { return BlockPos(posX + 0.5, posY + 0.5, posZ + 0.5); }

protected:
    void construct() override;

private:
    bool isItemInUse = false;
    int otherPlayerMPPosRotationIncrements = 0;
    double otherPlayerMPX = 0.0;
    double otherPlayerMPY = 0.0;
    double otherPlayerMPZ = 0.0;
    double otherPlayerMPYaw = 0.0;
    double otherPlayerMPPitch = 0.0;
};

} // namespace mc
