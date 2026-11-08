#include "entity/ai/attributes/Attributes.h"

#include "core/JavaString.h"
#include "util/MathHelper.h"
#include "util/Random.h"

#include <algorithm>
#include <stdexcept>

namespace mc {

// --- BaseAttribute / RangedAttribute ---

BaseAttribute::BaseAttribute(const IAttribute* parent, std::string unlocalizedNameIn, double defaultValueIn)
    : field_180373_a(parent)
    , unlocalizedName(std::move(unlocalizedNameIn))
    , defaultValue(defaultValueIn)
{
    if (unlocalizedName.empty())
        throw std::invalid_argument("Name cannot be null!");
}

RangedAttribute::RangedAttribute(const IAttribute* parent, std::string unlocalizedNameIn, double defaultValue, double minimumValueIn, double maximumValueIn)
    : BaseAttribute(parent, std::move(unlocalizedNameIn), defaultValue)
    , minimumValue(minimumValueIn)
    , maximumValue(maximumValueIn)
{
    if (minimumValueIn > maximumValueIn)
        throw std::invalid_argument("Minimum value cannot be bigger than maximum value!");
    if (defaultValue < minimumValueIn)
        throw std::invalid_argument("Default value cannot be lower than minimum value!");
    if (defaultValue > maximumValueIn)
        throw std::invalid_argument("Default value cannot be bigger than maximum value!");
}

double RangedAttribute::clampValue(double value) const
{
    return MathHelper::clamp_double(value, minimumValue, maximumValue);
}

// --- AttributeModifier ---

namespace {

UUID randomModifierId()
{
    static Random random;
    return UUID::getRandomUuid(random);
}

std::string lower(const std::string& s)
{
    return java::toLowerCase(s);
}

} // namespace

AttributeModifier::AttributeModifier(std::string nameIn, double amountIn, int operationIn)
    : AttributeModifier(randomModifierId(), std::move(nameIn), amountIn, operationIn)
{
}

AttributeModifier::AttributeModifier(const UUID& idIn, std::string nameIn, double amountIn, int operationIn)
    : amount(amountIn)
    , operation(operationIn)
    , name(std::move(nameIn))
    , id(idIn)
{
    if (name.empty())
        throw std::invalid_argument("Modifier name cannot be empty");
    if (operation < 0 || operation > 2)
        throw std::invalid_argument("Invalid operation");
}

std::string AttributeModifier::toString() const
{
    return "AttributeModifier{amount=" + java::toString(amount) + ", operation=" + std::to_string(operation) + ", name='" + name + "', id=" + id.toString()
        + ", serialize=" + java::toString(saved) + "}";
}

// --- ModifiableAttributeInstance ---

ModifiableAttributeInstance::ModifiableAttributeInstance(BaseAttributeMap& attributeMapIn, const IAttribute& genericAttributeIn)
    : attributeMap(attributeMapIn)
    , genericAttribute(genericAttributeIn)
    , baseValue(genericAttributeIn.getDefaultValue())
{
}

void ModifiableAttributeInstance::setBaseValue(double value)
{
    if (value != getBaseValue()) {
        baseValue = value;
        flagForUpdate();
    }
}

std::vector<AttributeModifier> ModifiableAttributeInstance::getModifiersByOperation(int operation) const
{
    std::vector<AttributeModifier> list;
    for (const auto& entry : mapByOperation[operation])
        list.push_back(entry.second);
    return list;
}

std::vector<AttributeModifier> ModifiableAttributeInstance::func_111122_c() const
{
    AttributeModifierSet set;
    for (int i = 0; i < 3; ++i) {
        for (const auto& entry : mapByOperation[i]) {
            if (!set.contains(entry.first))
                set.put(entry.first, entry.second);
        }
    }
    std::vector<AttributeModifier> list;
    for (const auto& entry : set)
        list.push_back(entry.second);
    return list;
}

void ModifiableAttributeInstance::applyModifier(const AttributeModifier& modifier)
{
    if (getModifier(modifier.getID()))
        throw std::invalid_argument("Modifier is already applied on this attribute!");
    AttributeModifierSet* set = mapByName.find(modifier.getName());
    if (!set) {
        mapByName.put(modifier.getName(), AttributeModifierSet());
        set = mapByName.find(modifier.getName());
    }
    AttributeModifierSet& byOperation = mapByOperation[modifier.getOperation()];
    if (!byOperation.contains(modifier.getID()))
        byOperation.put(modifier.getID(), modifier);
    if (!set->contains(modifier.getID()))
        set->put(modifier.getID(), modifier);
    mapByUUID.put(modifier.getID(), modifier);
    flagForUpdate();
}

void ModifiableAttributeInstance::flagForUpdate()
{
    needsUpdate = true;
    attributeMap.func_180794_a(*this);
}

void ModifiableAttributeInstance::removeModifier(const AttributeModifier& modifier)
{
    for (int i = 0; i < 3; ++i)
        mapByOperation[i].erase(modifier.getID());
    if (AttributeModifierSet* set1 = mapByName.find(modifier.getName())) {
        set1->erase(modifier.getID());
        if (set1->empty())
            mapByName.erase(modifier.getName());
    }
    mapByUUID.erase(modifier.getID());
    flagForUpdate();
}

void ModifiableAttributeInstance::removeAllModifiers()
{
    for (const AttributeModifier& modifier : func_111122_c())
        removeModifier(modifier);
}

double ModifiableAttributeInstance::getAttributeValue()
{
    if (needsUpdate) {
        cachedValue = computeValue();
        needsUpdate = false;
    }
    return cachedValue;
}

double ModifiableAttributeInstance::computeValue()
{
    double d0 = getBaseValue();
    for (const auto& entry : func_180375_b(0))
        d0 += entry.second.getAmount();
    double d1 = d0;
    for (const auto& entry : func_180375_b(1))
        d1 += d0 * entry.second.getAmount();
    for (const auto& entry : func_180375_b(2))
        d1 *= 1.0 + entry.second.getAmount();
    return genericAttribute.clampValue(d1);
}

AttributeModifierSet ModifiableAttributeInstance::func_180375_b(int operation) const
{
    AttributeModifierSet set;
    for (const auto& entry : mapByOperation[operation])
        set.put(entry.first, entry.second);
    for (const IAttribute* iattribute = genericAttribute.func_180372_d(); iattribute; iattribute = iattribute->func_180372_d()) {
        if (const IAttributeInstance* instance = attributeMap.getAttributeInstance(*iattribute)) {
            for (const AttributeModifier& modifier : instance->getModifiersByOperation(operation)) {
                if (!set.contains(modifier.getID()))
                    set.put(modifier.getID(), modifier);
            }
        }
    }
    return set;
}

// --- BaseAttributeMap ---

IAttributeInstance* BaseAttributeMap::getAttributeInstance(const IAttribute& attribute) const
{
    IAttributeInstance* const* instance = attributes.find(attribute.getAttributeUnlocalizedName());
    return instance ? *instance : nullptr;
}

IAttributeInstance* BaseAttributeMap::getAttributeInstanceByName(const std::string& attributeName) const
{
    IAttributeInstance* const* instance = attributesByName.find(lower(attributeName));
    return instance ? *instance : nullptr;
}

IAttributeInstance& BaseAttributeMap::registerAttribute(const IAttribute& attribute)
{
    if (attributesByName.contains(lower(attribute.getAttributeUnlocalizedName())))
        throw std::invalid_argument("Attribute is already registered!");
    ownedInstances.push_back(func_180376_c(attribute));
    IAttributeInstance* iattributeinstance = ownedInstances.back().get();
    attributesByName.put(lower(attribute.getAttributeUnlocalizedName()), iattributeinstance);
    attributes.put(attribute.getAttributeUnlocalizedName(), iattributeinstance);
    for (const IAttribute* iattribute = attribute.func_180372_d(); iattribute; iattribute = iattribute->func_180372_d())
        field_180377_c[iattribute->getAttributeUnlocalizedName()].push_back(&attribute);
    return *iattributeinstance;
}

IAttributeInstance& BaseAttributeMap::registerOwnedAttribute(std::unique_ptr<IAttribute> attribute)
{
    ownedAttributes.push_back(std::move(attribute));
    return registerAttribute(*ownedAttributes.back());
}

std::vector<IAttributeInstance*> BaseAttributeMap::getAllAttributes() const
{
    std::vector<IAttributeInstance*> list;
    for (const auto& entry : attributesByName)
        list.push_back(entry.second);
    return list;
}

void BaseAttributeMap::removeAttributeModifiers(const AttributeModifierMultimap& modifiers)
{
    for (const auto& [name, modifier] : modifiers) {
        if (IAttributeInstance* instance = getAttributeInstanceByName(name))
            instance->removeModifier(modifier);
    }
}

void BaseAttributeMap::applyAttributeModifiers(const AttributeModifierMultimap& modifiers)
{
    for (const auto& [name, modifier] : modifiers) {
        if (IAttributeInstance* instance = getAttributeInstanceByName(name)) {
            instance->removeModifier(modifier);
            instance->applyModifier(modifier);
        }
    }
}

// --- ServersideAttributeMap ---

IAttributeInstance* ServersideAttributeMap::getAttributeInstanceByName(const std::string& attributeName) const
{
    IAttributeInstance* instance = BaseAttributeMap::getAttributeInstanceByName(attributeName);
    if (!instance) {
        IAttributeInstance* const* byDescription = descriptionToAttributeInstanceMap.find(lower(attributeName));
        instance = byDescription ? *byDescription : nullptr;
    }
    return instance;
}

IAttributeInstance& ServersideAttributeMap::registerAttribute(const IAttribute& attribute)
{
    IAttributeInstance& instance = BaseAttributeMap::registerAttribute(attribute);
    if (const auto* ranged = dynamic_cast<const RangedAttribute*>(&attribute); ranged && ranged->getDescription())
        descriptionToAttributeInstanceMap.put(lower(*ranged->getDescription()), &instance);
    return instance;
}

std::unique_ptr<IAttributeInstance> ServersideAttributeMap::func_180376_c(const IAttribute& attribute)
{
    return std::make_unique<ModifiableAttributeInstance>(*this, attribute);
}

void ServersideAttributeMap::func_180794_a(IAttributeInstance& instance)
{
    if (instance.getAttribute().getShouldWatch()) {
        if (std::find(attributeInstanceSet.begin(), attributeInstanceSet.end(), &instance) == attributeInstanceSet.end())
            attributeInstanceSet.push_back(&instance);
    }
    const auto it = field_180377_c.find(instance.getAttribute().getAttributeUnlocalizedName());
    if (it != field_180377_c.end()) {
        for (const IAttribute* iattribute : it->second) {
            if (ModifiableAttributeInstance* child = getAttributeInstance(*iattribute))
                child->flagForUpdate();
        }
    }
}

std::vector<IAttributeInstance*> ServersideAttributeMap::getWatchedAttributes() const
{
    std::vector<IAttributeInstance*> set;
    for (IAttributeInstance* instance : getAllAttributes()) {
        if (instance->getAttribute().getShouldWatch())
            set.push_back(instance);
    }
    return set;
}

} // namespace mc
