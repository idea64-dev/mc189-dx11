#pragma once

// Original: net/minecraft/block/state/BlockState.java (the per-block container of states), IBlockState.java,
// BlockStateBase.java and BlockState.StateImplementation.
// States are created once per block and compared by address, like Java's identity-compared instances; code
// passes them around as `const IBlockState*`.

#include "block/properties/IProperty.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class Block;
class BlockState;

class IBlockState {
public:
    Block* getBlock() const { return block; }
    // The block's properties, sorted by name.
    const std::vector<const IProperty*>& getPropertyNames() const;

    template <class P>
    typename P::ValueType getValue(const P& property) const
    {
        return P::fromRaw(getRawValue(property));
    }

    template <class P, class V>
    const IBlockState* withProperty(const P& property, V value) const
    {
        return withRawValue(property, P::toRaw(static_cast<typename P::ValueType>(value)));
    }

    // The next allowed value, wrapping around.
    const IBlockState* cycleProperty(const IProperty& property) const;

    int getRawValue(const IProperty& property) const;
    const IBlockState* withRawValue(const IProperty& property, int raw) const;
    // Raw values in property order.
    const std::vector<int>& getRawValues() const { return values; }

    // "minecraft:stone[variant=granite]"
    std::string toString() const;

private:
    friend class BlockState;

    IBlockState(Block* blockIn, const BlockState* containerIn, std::vector<int> valuesIn)
        : block(blockIn)
        , container(containerIn)
        , values(std::move(valuesIn))
    {
    }

    int propertyIndex(const IProperty& property) const;

    Block* block;
    const BlockState* container;
    std::vector<int> values;
    // [property][value index] -> the state with that one value changed (this state for its own values).
    std::vector<std::vector<const IBlockState*>> propertyValueTable;
};

class BlockState {
public:
    BlockState(Block* blockIn, std::vector<const IProperty*> properties);

    BlockState(const BlockState&) = delete;
    BlockState& operator=(const BlockState&) = delete;

    // Every combination of property values; the last property varies fastest.
    std::vector<const IBlockState*> getValidStates() const;
    const IBlockState* getBaseState() const { return validStates.front().get(); }
    Block* getBlock() const { return block; }
    const std::vector<const IProperty*>& getProperties() const { return properties; }
    // "BlockState{block=minecraft:stone, properties=[variant]}"
    std::string toString() const;

private:
    Block* block;
    std::vector<const IProperty*> properties;
    std::vector<std::unique_ptr<IBlockState>> validStates;
};

} // namespace mc
