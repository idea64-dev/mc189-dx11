#include "util/CombatTracker.h"

#include "block/Block.h"
#include "entity/EntityLivingBase.h"
#include "entity/player/EntityPlayer.h"
#include "init/Blocks.h"
#include "item/ItemStack.h"
#include "util/ChatComponent.h"
#include "util/DamageSource.h"
#include "world/World.h"

#include <cfloat>

namespace mc {

namespace {

using Args = std::vector<ChatComponentTranslation::Argument>;

std::shared_ptr<IChatComponent> translation(const std::string& key, Args args)
{
    return std::make_shared<ChatComponentTranslation>(key, std::move(args));
}

// Java: IChatComponent.equals -- the same text, style and siblings; the formatted text stands in for it.
bool sameComponent(const std::shared_ptr<IChatComponent>& a, const std::shared_ptr<IChatComponent>& b)
{
    return a->getFormattedText() == b->getFormattedText();
}

std::shared_ptr<IChatComponent> itemChatComponent(const ItemStack& stack)
{
    // Java: ItemStack.getChatComponent (with the hover event, stage 13).
    auto name = std::make_shared<ChatComponentText>(stack.getDisplayName());
    name->getChatStyle()->setItalic(true);
    auto bracketed = std::make_shared<ChatComponentText>("[");
    bracketed->appendSibling(name);
    bracketed->appendText("]");
    return bracketed;
}

} // namespace

CombatEntry::CombatEntry(const DamageSource& damageSrcIn, int ticks, float healthAmount, float damageAmount, std::optional<std::string> location,
    float fallDistanceIn)
    : field_94567_b(ticks)
    , damage(damageAmount)
    , health(healthAmount)
    , field_94566_e(std::move(location))
    , fallDistance(fallDistanceIn)
{
    if (damageSrcIn.isStaticSource()) {
        damageSrc = &damageSrcIn;
    } else {
        keepAlive = damageSrcIn.clone();
        damageSrc = keepAlive.get();
    }
}

bool CombatEntry::isLivingDamageSrc() const
{
    return dynamic_cast<EntityLivingBase*>(damageSrc->getEntity()) != nullptr;
}

std::shared_ptr<IChatComponent> CombatEntry::getDamageSrcDisplayName() const
{
    return damageSrc->getEntity() ? damageSrc->getEntity()->getDisplayName() : nullptr;
}

float CombatEntry::getDamageAmount() const
{
    return damageSrc == &DamageSource::outOfWorld ? FLT_MAX : fallDistance;
}

void CombatTracker::func_94545_a()
{
    field_94551_f.reset();
    if (fighter->isOnLadder()) {
        const Block* block = fighter->worldObj->getBlockState(BlockPos(fighter->posX, fighter->getEntityBoundingBox().minY, fighter->posZ))->getBlock();
        if (block == Blocks::ladder)
            field_94551_f = "ladder";
        else if (block == Blocks::vine)
            field_94551_f = "vines";
    } else if (fighter->isInWater()) {
        field_94551_f = "water";
    }
}

void CombatTracker::trackDamage(const DamageSource& damageSrc, float healthIn, float damageAmount)
{
    reset();
    func_94545_a();
    combatEntries.emplace_back(damageSrc, fighter->ticksExisted, healthIn, damageAmount, field_94551_f, fighter->fallDistance);
    const CombatEntry& combatentry = combatEntries.back();
    field_94555_c = fighter->ticksExisted;
    field_94553_e = true;
    if (combatentry.isLivingDamageSrc() && !field_94552_d && fighter->isEntityAlive()) {
        field_94552_d = true;
        field_152775_d = fighter->ticksExisted;
        field_152776_e = field_152775_d;
        fighter->sendEnterCombat();
    }
}

std::shared_ptr<IChatComponent> CombatTracker::getDeathMessage()
{
    if (combatEntries.empty())
        return translation("death.attack.generic", { fighter->getDisplayName() });
    const CombatEntry* combatentry = func_94544_f();
    const CombatEntry& combatentry1 = combatEntries.back();
    const std::shared_ptr<IChatComponent> ichatcomponent1 = combatentry1.getDamageSrcDisplayName();
    Entity* entity = combatentry1.getDamageSrc().getEntity();
    if (combatentry && &combatentry1.getDamageSrc() == &DamageSource::fall) {
        const std::shared_ptr<IChatComponent> ichatcomponent2 = combatentry->getDamageSrcDisplayName();
        if (&combatentry->getDamageSrc() != &DamageSource::fall && &combatentry->getDamageSrc() != &DamageSource::outOfWorld) {
            if (ichatcomponent2 && (!ichatcomponent1 || !sameComponent(ichatcomponent2, ichatcomponent1))) {
                auto* entity1 = dynamic_cast<EntityLivingBase*>(combatentry->getDamageSrc().getEntity());
                const std::shared_ptr<ItemStack> itemstack1 = entity1 ? entity1->getHeldItem() : nullptr;
                if (itemstack1 && itemstack1->hasDisplayName())
                    return translation("death.fell.assist.item", { fighter->getDisplayName(), ichatcomponent2, itemChatComponent(*itemstack1) });
                return translation("death.fell.assist", { fighter->getDisplayName(), ichatcomponent2 });
            }
            if (ichatcomponent1) {
                auto* living = dynamic_cast<EntityLivingBase*>(entity);
                const std::shared_ptr<ItemStack> itemstack = living ? living->getHeldItem() : nullptr;
                if (itemstack && itemstack->hasDisplayName())
                    return translation("death.fell.finish.item", { fighter->getDisplayName(), ichatcomponent1, itemChatComponent(*itemstack) });
                return translation("death.fell.finish", { fighter->getDisplayName(), ichatcomponent1 });
            }
            return translation("death.fell.killer", { fighter->getDisplayName() });
        }
        return translation("death.fell.accident." + func_94548_b(*combatentry), { fighter->getDisplayName() });
    }
    return combatentry1.getDamageSrc().getDeathMessage(fighter);
}

EntityLivingBase* CombatTracker::func_94550_c()
{
    EntityLivingBase* entitylivingbase = nullptr;
    EntityPlayer* entityplayer = nullptr;
    float f = 0.0f;
    float f1 = 0.0f;
    for (const CombatEntry& combatentry : combatEntries) {
        Entity* source = combatentry.getDamageSrc().getEntity();
        if (auto* player = dynamic_cast<EntityPlayer*>(source); player && (!entityplayer || combatentry.func_94563_c() > f1)) {
            f1 = combatentry.func_94563_c();
            entityplayer = player;
        }
        if (auto* living = dynamic_cast<EntityLivingBase*>(source); living && (!entitylivingbase || combatentry.func_94563_c() > f)) {
            f = combatentry.func_94563_c();
            entitylivingbase = living;
        }
    }
    if (entityplayer && f1 >= f / 3.0f)
        return entityplayer;
    return entitylivingbase;
}

const CombatEntry* CombatTracker::func_94544_f() const
{
    const CombatEntry* combatentry = nullptr;
    const CombatEntry* combatentry1 = nullptr;
    const int i = 0;
    float f = 0.0f;
    for (size_t j = 0; j < combatEntries.size(); ++j) {
        const CombatEntry& combatentry2 = combatEntries[j];
        const CombatEntry* combatentry3 = j > 0 ? &combatEntries[j - 1] : nullptr;
        const DamageSource* src = &combatentry2.getDamageSrc();
        if ((src == &DamageSource::fall || src == &DamageSource::outOfWorld) && combatentry2.getDamageAmount() > 0.0f
            && (!combatentry || combatentry2.getDamageAmount() > f)) {
            combatentry = j > 0 ? combatentry3 : &combatentry2;
            f = combatentry2.getDamageAmount();
        }
        if (combatentry2.func_94562_g() && (!combatentry1 || combatentry2.func_94563_c() > static_cast<float>(i)))
            combatentry1 = &combatentry2;
    }
    if (f > 5.0f && combatentry)
        return combatentry;
    // Java never raises i, so this branch is dead.
    if (i > 5 && combatentry1)
        return combatentry1;
    return nullptr;
}

std::string CombatTracker::func_94548_b(const CombatEntry& entry)
{
    return entry.func_94562_g().value_or("generic");
}

int CombatTracker::func_180134_f() const
{
    return field_94552_d ? fighter->ticksExisted - field_152775_d : field_152776_e - field_152775_d;
}

void CombatTracker::reset()
{
    const int i = field_94552_d ? 300 : 100;
    if (field_94553_e && (!fighter->isEntityAlive() || fighter->ticksExisted - field_94555_c > i)) {
        const bool flag = field_94552_d;
        field_94553_e = false;
        field_94552_d = false;
        field_152776_e = fighter->ticksExisted;
        if (flag)
            fighter->sendEndCombat();
        combatEntries.clear();
    }
}

} // namespace mc
