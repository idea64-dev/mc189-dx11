#pragma once

// Original: net/minecraft/entity/ai/attributes/IAttribute.java, BaseAttribute.java, RangedAttribute.java,
// AttributeModifier.java, IAttributeInstance.java, ModifiableAttributeInstance.java, BaseAttributeMap.java and
// ServersideAttributeMap.java (with LowerStringMap).
// The modifier sets are HashSets keyed by the modifier's UUID; JavaHashMap keeps their iteration order, which
// decides the order the amounts are summed in and the order of the saved modifier lists.

#include "core/JavaHashMap.h"
#include "util/UUID.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

template <>
struct JavaHash<UUID> {
    int32_t operator()(const UUID& key) const { return key.hashCode(); }
};

class IAttribute {
public:
    virtual ~IAttribute() = default;
    virtual const std::string& getAttributeUnlocalizedName() const = 0;
    virtual double clampValue(double value) const = 0;
    virtual double getDefaultValue() const = 0;
    virtual bool getShouldWatch() const = 0;
    // Java: func_180372_d -- the parent attribute whose modifiers also apply, null for none.
    virtual const IAttribute* func_180372_d() const = 0;
};

class BaseAttribute : public IAttribute {
public:
    const std::string& getAttributeUnlocalizedName() const override { return unlocalizedName; }
    double getDefaultValue() const override { return defaultValue; }
    bool getShouldWatch() const override { return shouldWatch; }
    BaseAttribute& setShouldWatch(bool value)
    {
        shouldWatch = value;
        return *this;
    }
    const IAttribute* func_180372_d() const override { return field_180373_a; }

protected:
    // Throws std::invalid_argument for an empty name (Java: null).
    BaseAttribute(const IAttribute* parent, std::string unlocalizedNameIn, double defaultValueIn);

private:
    const IAttribute* field_180373_a;
    std::string unlocalizedName;
    double defaultValue;
    bool shouldWatch = false;
};

class RangedAttribute : public BaseAttribute {
public:
    // Throws std::invalid_argument for an inconsistent range, as Java.
    RangedAttribute(const IAttribute* parent, std::string unlocalizedNameIn, double defaultValue, double minimumValueIn, double maximumValueIn);

    RangedAttribute& setDescription(std::string descriptionIn)
    {
        description = std::move(descriptionIn);
        return *this;
    }
    RangedAttribute& setShouldWatch(bool value)
    {
        BaseAttribute::setShouldWatch(value);
        return *this;
    }
    const std::optional<std::string>& getDescription() const { return description; }
    double clampValue(double value) const override;

private:
    double minimumValue;
    double maximumValue;
    std::optional<std::string> description;
};

class AttributeModifier {
public:
    // A random id (Java: ThreadLocalRandom).
    AttributeModifier(std::string nameIn, double amountIn, int operationIn);
    // Throws std::invalid_argument for an empty name or an operation outside 0..2.
    AttributeModifier(const UUID& idIn, std::string nameIn, double amountIn, int operationIn);

    const UUID& getID() const { return id; }
    const std::string& getName() const { return name; }
    int getOperation() const { return operation; }
    double getAmount() const { return amount; }
    bool isSaved() const { return saved; }
    AttributeModifier& setSaved(bool value)
    {
        saved = value;
        return *this;
    }
    // Java: equals compares the ids only.
    bool operator==(const AttributeModifier& other) const { return id == other.id; }
    int32_t hashCode() const { return id.hashCode(); }
    std::string toString() const;

private:
    double amount;
    int operation;
    std::string name;
    UUID id;
    bool saved = true;
};

// A HashSet<AttributeModifier>, in Java's order.
using AttributeModifierSet = JavaHashMap<UUID, AttributeModifier>;

class BaseAttributeMap;

class IAttributeInstance {
public:
    virtual ~IAttributeInstance() = default;
    virtual const IAttribute& getAttribute() const = 0;
    virtual double getBaseValue() const = 0;
    virtual void setBaseValue(double baseValue) = 0;
    virtual std::vector<AttributeModifier> getModifiersByOperation(int operation) const = 0;
    // Java: func_111122_c -- every modifier.
    virtual std::vector<AttributeModifier> func_111122_c() const = 0;
    virtual bool hasModifier(const AttributeModifier& modifier) const = 0;
    // Null when none has that id.
    virtual const AttributeModifier* getModifier(const UUID& uuid) const = 0;
    // Throws std::invalid_argument when a modifier with that id is already applied.
    virtual void applyModifier(const AttributeModifier& modifier) = 0;
    virtual void removeModifier(const AttributeModifier& modifier) = 0;
    virtual void removeAllModifiers() = 0;
    virtual double getAttributeValue() = 0;
};

class ModifiableAttributeInstance : public IAttributeInstance {
public:
    ModifiableAttributeInstance(BaseAttributeMap& attributeMapIn, const IAttribute& genericAttributeIn);

    const IAttribute& getAttribute() const override { return genericAttribute; }
    double getBaseValue() const override { return baseValue; }
    void setBaseValue(double value) override;
    std::vector<AttributeModifier> getModifiersByOperation(int operation) const override;
    std::vector<AttributeModifier> func_111122_c() const override;
    const AttributeModifier* getModifier(const UUID& uuid) const override { return mapByUUID.find(uuid); }
    bool hasModifier(const AttributeModifier& modifier) const override { return mapByUUID.contains(modifier.getID()); }
    void applyModifier(const AttributeModifier& modifier) override;
    void removeModifier(const AttributeModifier& modifier) override;
    void removeAllModifiers() override;
    double getAttributeValue() override;
    void flagForUpdate();

private:
    double computeValue();
    AttributeModifierSet func_180375_b(int operation) const;

    BaseAttributeMap& attributeMap;
    const IAttribute& genericAttribute;
    AttributeModifierSet mapByOperation[3];
    JavaHashMap<std::string, AttributeModifierSet> mapByName;
    AttributeModifierSet mapByUUID;
    double baseValue;
    bool needsUpdate = true;
    double cachedValue = 0.0;
};

// Java: Multimap<String, AttributeModifier> as items and potions hand them out (a HashMultimap; the entries
// are listed in the order they were added, which is how the callers build them).
using AttributeModifierMultimap = std::vector<std::pair<std::string, AttributeModifier>>;

class BaseAttributeMap {
public:
    virtual ~BaseAttributeMap() = default;

    // Null when the attribute isn't registered.
    IAttributeInstance* getAttributeInstance(const IAttribute& attribute) const;
    virtual IAttributeInstance* getAttributeInstanceByName(const std::string& attributeName) const;
    // Throws std::invalid_argument when already registered.
    virtual IAttributeInstance& registerAttribute(const IAttribute& attribute);
    // For attributes made at run time (the client's S20PacketEntityProperties): the map keeps it alive.
    IAttributeInstance& registerOwnedAttribute(std::unique_ptr<IAttribute> attribute);
    // Java: getAllAttributes -- in the order of the lower-cased names' HashMap.
    std::vector<IAttributeInstance*> getAllAttributes() const;
    virtual void func_180794_a(IAttributeInstance&) { }
    void removeAttributeModifiers(const AttributeModifierMultimap& modifiers);
    void applyAttributeModifiers(const AttributeModifierMultimap& modifiers);

protected:
    virtual std::unique_ptr<IAttributeInstance> func_180376_c(const IAttribute& attribute) = 0;

    std::vector<std::unique_ptr<IAttributeInstance>> ownedInstances;
    std::vector<std::unique_ptr<IAttribute>> ownedAttributes;
    // Java: attributes (by the attribute, whose hash and equality are its name) and attributesByName (a
    // LowerStringMap).
    JavaHashMap<std::string, IAttributeInstance*> attributes;
    JavaHashMap<std::string, IAttributeInstance*> attributesByName;
    // Java: field_180377_c -- parent name to the attributes deriving from it.
    std::unordered_map<std::string, std::vector<const IAttribute*>> field_180377_c;
};

class ServersideAttributeMap : public BaseAttributeMap {
public:
    ModifiableAttributeInstance* getAttributeInstance(const IAttribute& attribute) const
    {
        return static_cast<ModifiableAttributeInstance*>(BaseAttributeMap::getAttributeInstance(attribute));
    }
    IAttributeInstance* getAttributeInstanceByName(const std::string& attributeName) const override;
    IAttributeInstance& registerAttribute(const IAttribute& attribute) override;
    void func_180794_a(IAttributeInstance& instance) override;
    // Java: a HashSet of instances (identity order); here the order they changed in.
    std::vector<IAttributeInstance*>& getAttributeInstanceSet() { return attributeInstanceSet; }
    std::vector<IAttributeInstance*> getWatchedAttributes() const;

protected:
    std::unique_ptr<IAttributeInstance> func_180376_c(const IAttribute& attribute) override;

private:
    std::vector<IAttributeInstance*> attributeInstanceSet;
    JavaHashMap<std::string, IAttributeInstance*> descriptionToAttributeInstanceMap;
};

} // namespace mc
