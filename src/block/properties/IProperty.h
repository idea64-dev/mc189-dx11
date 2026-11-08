#pragma once

// Original: net/minecraft/block/properties/IProperty.java, PropertyHelper.java, PropertyInteger.java,
// PropertyBool.java, PropertyEnum.java and PropertyDirection.java (plus util/IStringSerializable.java).
// A state stores every property value as an int ("raw" value): the integer itself, 1/0 for booleans and the
// ordinal for enums. The typed property classes convert, so block code reads like Java's:
//   state->getValue(VARIANT) returns the enum, withProperty(AGE, 3) takes an int.
// Allowed values keep Java's iteration order (booleans are true then false), which fixes the order of a
// block's valid states.

#include "util/EnumFacing.h"

#include <algorithm>
#include <array>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mc {

// Java enums used as property values specialize this with their values (in ordinal order) and
// IStringSerializable.getName().
template <class E>
struct EnumTraits;

template <>
struct EnumTraits<EnumFacing> {
    static constexpr std::span<const EnumFacing> values() { return kFacings; }
    static constexpr std::string_view getName(EnumFacing value) { return mc::getName(value); }
};

template <>
struct EnumTraits<Axis> {
    static constexpr std::span<const Axis> values() { return kAxes; }
    static constexpr std::string_view getName(Axis value) { return mc::getName(value); }
};

class IProperty {
public:
    virtual ~IProperty() = default;

    const std::string& getName() const { return name; }
    // Raw values in Java's iteration order.
    const std::vector<int>& getAllowedRawValues() const { return allowedValues; }
    virtual std::string getValueName(int raw) const = 0;
    // Index of a raw value in getAllowedRawValues(), or -1.
    int indexOf(int raw) const
    {
        const auto it = std::find(allowedValues.begin(), allowedValues.end(), raw);
        return it == allowedValues.end() ? -1 : static_cast<int>(it - allowedValues.begin());
    }
    // Java's equals: same kind, value class, name and allowed values.
    bool equals(const IProperty& other) const
    {
        return this == &other || (kind == other.kind && name == other.name && allowedValues == other.allowedValues);
    }
    // Guava's toStringHelper output, used in exception messages.
    std::string toString() const;

protected:
    IProperty(std::string nameIn, std::string kindIn, std::vector<int> allowed)
        : name(std::move(nameIn))
        , kind(std::move(kindIn))
        , allowedValues(std::move(allowed))
    {
    }

    std::string getKindName() const;

    std::string name;
    // Stands in for the value class.
    std::string kind;
    std::vector<int> allowedValues;
};

class PropertyInteger : public IProperty {
public:
    using ValueType = int;

    // Throws std::invalid_argument like Java's IllegalArgumentException.
    PropertyInteger(std::string nameIn, int min, int max);
    static PropertyInteger create(std::string name, int min, int max) { return PropertyInteger(std::move(name), min, max); }

    std::string getValueName(int raw) const override { return std::to_string(raw); }
    static int toRaw(int value) { return value; }
    static int fromRaw(int raw) { return raw; }
};

class PropertyBool : public IProperty {
public:
    using ValueType = bool;

    explicit PropertyBool(std::string nameIn)
        : IProperty(std::move(nameIn), "Boolean", { 1, 0 })
    {
    }
    static PropertyBool create(std::string name) { return PropertyBool(std::move(name)); }

    std::string getValueName(int raw) const override { return raw ? "true" : "false"; }
    static int toRaw(bool value) { return value ? 1 : 0; }
    static bool fromRaw(int raw) { return raw != 0; }
};

template <class E>
class PropertyEnum : public IProperty {
public:
    using ValueType = E;

    // All values of the enum.
    explicit PropertyEnum(std::string nameIn)
        : PropertyEnum(std::move(nameIn), [](E) { return true; })
    {
    }
    // The values the filter accepts, in ordinal order (Java: Collections2.filter of values()).
    PropertyEnum(std::string nameIn, const std::function<bool(E)>& filter)
        : IProperty(std::move(nameIn), "enum", filtered(filter))
    {
    }
    // Exactly these values, in this order.
    PropertyEnum(std::string nameIn, std::initializer_list<E> values)
        : IProperty(std::move(nameIn), "enum", toRawList(values))
    {
    }

    std::string getValueName(int raw) const override { return std::string(EnumTraits<E>::getName(static_cast<E>(raw))); }
    static int toRaw(E value) { return static_cast<int>(value); }
    static E fromRaw(int raw) { return static_cast<E>(raw); }
    std::vector<E> getAllowedValues() const
    {
        std::vector<E> values;
        for (const int raw : allowedValues)
            values.push_back(fromRaw(raw));
        return values;
    }

private:
    static std::vector<int> filtered(const std::function<bool(E)>& filter)
    {
        std::vector<int> values;
        for (const E value : EnumTraits<E>::values()) {
            if (filter(value))
                values.push_back(static_cast<int>(value));
        }
        checkNames(values);
        return values;
    }
    static std::vector<int> toRawList(std::initializer_list<E> list)
    {
        std::vector<int> values;
        for (const E value : list)
            values.push_back(static_cast<int>(value));
        checkNames(values);
        return values;
    }
    static void checkNames(const std::vector<int>& values)
    {
        for (size_t i = 0; i < values.size(); ++i) {
            for (size_t j = 0; j < i; ++j) {
                if (EnumTraits<E>::getName(static_cast<E>(values[i])) == EnumTraits<E>::getName(static_cast<E>(values[j])))
                    throw std::invalid_argument("Multiple values have the same name '" + std::string(EnumTraits<E>::getName(static_cast<E>(values[i]))) + "'");
            }
        }
    }
};

using PropertyDirection = PropertyEnum<EnumFacing>;

} // namespace mc
