#pragma once

// Original: net/minecraft/client/entity/EntityPlayerSP.java -- the client's own player.
// Not ported yet (marked where used): statistics (StatFileWriter), horses, minecart sounds, the particle
// effects of critical hits, and the container GUIs (chests, signs, books, crafting tables, villagers).

#include "client/entity/AbstractClientPlayer.h"
#include "util/MovementInput.h"

#include <memory>
#include <string>

namespace mc {

class Minecraft;
class NetHandlerPlayClient;

class EntityPlayerSP : public AbstractClientPlayer {
public:
    EntityPlayerSP(Minecraft* mcIn, World* worldIn, NetHandlerPlayClient* netHandler);

    std::string_view getClassName() const override { return "EntityPlayerSP"; }

    bool attackEntityFrom(const DamageSource&, float) override { return false; }
    void heal(float) override { }
    void mountEntity(Entity* entityIn) override;
    void onUpdate() override;
    void onUpdateWalkingPlayer();
    std::shared_ptr<EntityItem> dropOneItem(bool dropAll) override;
    void sendChatMessage(const std::string& message);
    void swingItem() override;
    void respawnPlayer() override;
    void closeScreen() override;
    void closeScreenAndDropStack();
    void setPlayerSPHealth(float health);
    void addStat(const StatBase* stat, int amount) override;
    void sendPlayerAbilities() override;
    bool isUser() const override { return true; }
    void sendHorseInventory();
    void setClientBrand(std::string brand) { clientBrand = std::move(brand); }
    const std::string& getClientBrand() const { return clientBrand; }
    void addChatComponentMessage(std::shared_ptr<IChatComponent> chatComponent) override;
    void setSprinting(bool sprinting) override;
    void setXPStats(float currentXP, int maxXP, int level);
    void addChatMessage(std::shared_ptr<IChatComponent> component) override;
    bool canCommandSenderUseCommand(int permLevel, const std::string&) override { return permLevel <= 0; }
    BlockPos getPosition() const { return BlockPos(posX + 0.5, posY + 0.5, posZ + 0.5); }
    void playSound(const std::string& name, float volume, float pitch) override;
    bool isServerWorld() override { return true; }
    bool isRidingHorse() const;
    float getHorseJumpPower() const { return horseJumpPower; }
    void openEditSign(TileEntitySign* signTile) override;
    void displayGUIBook(const std::shared_ptr<ItemStack>& bookStack) override;
    void displayGUIChest(IInventory* chestInventory) override;
    void onCriticalHit(Entity* entityHit) override;
    void onEnchantmentCritical(Entity* entityHit) override;
    bool isSneaking() const override;
    void updateEntityActionState() override;
    void onLivingUpdate() override;

    NetHandlerPlayClient* const sendQueue;
    // Java: set by Minecraft (a MovementInputFromOptions) once the player exists.
    std::unique_ptr<MovementInput> movementInput;
    int sprintingTicksLeft = 0;
    float renderArmYaw = 0.0f;
    float renderArmPitch = 0.0f;
    float prevRenderArmYaw = 0.0f;
    float prevRenderArmPitch = 0.0f;
    float timeInPortal = 0.0f;
    float prevTimeInPortal = 0.0f;

protected:
    void construct() override;
    void joinEntityItemWithWorld(const std::shared_ptr<EntityItem>&) override { }
    void damageEntity(const DamageSource& damageSrc, float damageAmount) override;
    void sendHorseJump();
    // Java overrides Entity.pushOutOfBlocks, which isn't virtual here; only this class calls its own.
    bool pushOutOfBlocks(double x, double y, double z);
    bool isCurrentViewEntity();

    Minecraft* mc;
    int sprintToggleTimer = 0;

private:
    bool isOpenBlockSpace(const BlockPos& pos);

    double lastReportedPosX = 0.0;
    double lastReportedPosY = 0.0;
    double lastReportedPosZ = 0.0;
    float lastReportedYaw = 0.0f;
    float lastReportedPitch = 0.0f;
    bool serverSneakState = false;
    bool serverSprintState = false;
    int positionUpdateTicks = 0;
    bool hasValidHealth = false;
    std::string clientBrand;
    int horseJumpPowerCounter = 0;
    float horseJumpPower = 0.0f;
};

} // namespace mc
