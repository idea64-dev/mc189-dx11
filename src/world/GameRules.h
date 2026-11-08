#pragma once

// Original: net/minecraft/world/GameRules.java -- a TreeMap of rule names to values.

#include "core/JavaString.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class NBTTagCompound;

class GameRules {
public:
    enum class ValueType { ANY_VALUE, BOOLEAN_VALUE, NUMERICAL_VALUE };

    GameRules();

    void addGameRule(const std::string& key, const std::string& value, ValueType type);
    void setOrCreateGameRule(const std::string& key, const std::string& ruleValue);
    // "" / false / 0 for unknown rules.
    std::string getString(const std::string& name) const;
    bool getBoolean(const std::string& name) const;
    int getInt(const std::string& name) const;
    std::shared_ptr<NBTTagCompound> writeToNBT() const;
    void readFromNBT(const NBTTagCompound& nbt);
    std::vector<std::string> getRules() const;
    bool hasRule(const std::string& name) const { return theGameRules.contains(name); }
    bool areSameType(const std::string& key, ValueType otherValue) const;

private:
    struct Value {
        Value(const std::string& value, ValueType typeIn);
        void setValue(const std::string& value);

        std::string valueString;
        bool valueBoolean = false;
        int valueInteger = 0;
        double valueDouble = 0.0;
        ValueType type;
    };

    std::map<std::string, Value, java::StringLess> theGameRules;
};

} // namespace mc
