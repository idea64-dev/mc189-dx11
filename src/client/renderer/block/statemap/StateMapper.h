#pragma once

// Original: net/minecraft/client/renderer/block/statemap/IStateMapper.java, StateMapperBase.java, StateMap.java,
// DefaultStateMapper.java and BlockStateMapper.java
// Maps every block state to the blockstates variant that draws it: "minecraft:oak_stairs#facing=east,half=top,..."
// by default; per-block mappers rename the file, drop properties or build the name themselves.
// Properties are named by their name here: the original removes them from the state's property map with
// equals(), which compares names and values, and a block has one property per name.

#include "client/resources/model/ModelResourceLocation.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {

class Block;
class IBlockState;
class IProperty;

// A state's properties in name order, as a mutable map (Java: a LinkedHashMap copy of getProperties()).
class StatePropertyMap {
public:
    explicit StatePropertyMap(const IBlockState* state);

    // Removes the property and returns its value's name ("" when absent, where Java would fail).
    std::string remove(const std::string& name);
    // "a=b,c=d", or "normal" when empty (StateMapperBase.getPropertyString).
    std::string toPropertyString() const;

private:
    std::vector<std::pair<const IProperty*, int>> entries;
};

class IStateMapper {
public:
    virtual ~IStateMapper() = default;
    virtual std::vector<std::pair<const IBlockState*, ModelResourceLocation>> putStateModelLocations(const Block* block) const;

protected:
    virtual ModelResourceLocation getModelResourceLocation(const IBlockState* state) const = 0;
};

class DefaultStateMapper : public IStateMapper {
protected:
    ModelResourceLocation getModelResourceLocation(const IBlockState* state) const override;
};

class StateMap : public IStateMapper {
public:
    class Builder {
    public:
        Builder& withName(std::string property)
        {
            name = std::move(property);
            return *this;
        }
        Builder& withSuffix(std::string suffixIn)
        {
            suffix = std::move(suffixIn);
            return *this;
        }
        Builder& ignore(std::initializer_list<std::string> properties)
        {
            ignored.insert(ignored.end(), properties.begin(), properties.end());
            return *this;
        }
        std::unique_ptr<StateMap> build() const { return std::unique_ptr<StateMap>(new StateMap(name, suffix, ignored)); }

    private:
        std::string name;
        std::string suffix;
        std::vector<std::string> ignored;
    };

protected:
    ModelResourceLocation getModelResourceLocation(const IBlockState* state) const override;

private:
    StateMap(std::string nameIn, std::string suffixIn, std::vector<std::string> ignoredIn)
        : name(std::move(nameIn))
        , suffix(std::move(suffixIn))
        , ignored(std::move(ignoredIn))
    {
    }

    std::string name;
    std::string suffix;
    std::vector<std::string> ignored;
};

// Java: the anonymous StateMapperBase subclasses of BlockModelShapes.
class LambdaStateMapper : public IStateMapper {
public:
    explicit LambdaStateMapper(std::function<ModelResourceLocation(const IBlockState*)> mapperIn)
        : mapper(std::move(mapperIn))
    {
    }

protected:
    ModelResourceLocation getModelResourceLocation(const IBlockState* state) const override { return mapper(state); }

private:
    std::function<ModelResourceLocation(const IBlockState*)> mapper;
};

class BlockStateMapper {
public:
    void registerBlockStateMapper(const Block* block, std::unique_ptr<IStateMapper> stateMapper);
    void registerBuiltInBlocks(std::initializer_list<const Block*> blocks);
    // Every state of every block that has a JSON model (built-in blocks are drawn by code), in registry order.
    std::vector<std::pair<const IBlockState*, ModelResourceLocation>> putAllStateModelLocations() const;

private:
    std::unordered_map<const Block*, std::unique_ptr<IStateMapper>> blockStateMap;
    std::unordered_set<const Block*> setBuiltInBlocks;
};

} // namespace mc
