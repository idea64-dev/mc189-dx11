#include "world/GameRules.h"

#include "nbt/NBTTagCompound.h"

#include <stdexcept>

namespace mc {

GameRules::GameRules()
{
    addGameRule("doFireTick", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("mobGriefing", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("keepInventory", "false", ValueType::BOOLEAN_VALUE);
    addGameRule("doMobSpawning", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("doMobLoot", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("doTileDrops", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("doEntityDrops", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("commandBlockOutput", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("naturalRegeneration", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("doDaylightCycle", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("logAdminCommands", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("showDeathMessages", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("randomTickSpeed", "3", ValueType::NUMERICAL_VALUE);
    addGameRule("sendCommandFeedback", "true", ValueType::BOOLEAN_VALUE);
    addGameRule("reducedDebugInfo", "false", ValueType::BOOLEAN_VALUE);
}

void GameRules::addGameRule(const std::string& key, const std::string& value, ValueType type)
{
    theGameRules.insert_or_assign(key, Value(value, type));
}

void GameRules::setOrCreateGameRule(const std::string& key, const std::string& ruleValue)
{
    if (const auto it = theGameRules.find(key); it != theGameRules.end())
        it->second.setValue(ruleValue);
    else
        addGameRule(key, ruleValue, ValueType::ANY_VALUE);
}

std::string GameRules::getString(const std::string& name) const
{
    const auto it = theGameRules.find(name);
    return it != theGameRules.end() ? it->second.valueString : "";
}

bool GameRules::getBoolean(const std::string& name) const
{
    const auto it = theGameRules.find(name);
    return it != theGameRules.end() && it->second.valueBoolean;
}

int GameRules::getInt(const std::string& name) const
{
    const auto it = theGameRules.find(name);
    return it != theGameRules.end() ? it->second.valueInteger : 0;
}

std::shared_ptr<NBTTagCompound> GameRules::writeToNBT() const
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    for (const auto& [s, value] : theGameRules)
        nbttagcompound->setString(s, value.valueString);
    return nbttagcompound;
}

void GameRules::readFromNBT(const NBTTagCompound& nbt)
{
    for (const std::string& s : nbt.getKeySet())
        setOrCreateGameRule(s, nbt.getString(s));
}

std::vector<std::string> GameRules::getRules() const
{
    std::vector<std::string> rules;
    for (const auto& entry : theGameRules)
        rules.push_back(entry.first);
    return rules;
}

bool GameRules::areSameType(const std::string& key, ValueType otherValue) const
{
    const auto it = theGameRules.find(key);
    return it != theGameRules.end() && (it->second.type == otherValue || otherValue == ValueType::ANY_VALUE);
}

GameRules::Value::Value(const std::string& value, ValueType typeIn)
    : type(typeIn)
{
    setValue(value);
}

void GameRules::Value::setValue(const std::string& value)
{
    valueString = value;
    // Boolean.parseBoolean: "true" ignoring case.
    valueBoolean = value.size() == 4 && (value[0] | 32) == 't' && (value[1] | 32) == 'r' && (value[2] | 32) == 'u' && (value[3] | 32) == 'e';
    valueInteger = valueBoolean ? 1 : 0;
    try {
        valueInteger = java::parseInt(value);
    } catch (const std::invalid_argument&) {
    }
    try {
        valueDouble = java::parseDouble(value);
    } catch (const std::invalid_argument&) {
    }
}

} // namespace mc
