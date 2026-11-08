#include "block/state/BlockState.h"

#include "block/Block.h"

#include <algorithm>
#include <map>

namespace mc {

// --- IBlockState ---

const std::vector<const IProperty*>& IBlockState::getPropertyNames() const
{
    return container->getProperties();
}

int IBlockState::propertyIndex(const IProperty& property) const
{
    const auto& properties = container->getProperties();
    for (size_t i = 0; i < properties.size(); ++i) {
        if (properties[i] == &property)
            return static_cast<int>(i);
    }
    // Java looks properties up by equals(): an equal property from elsewhere matches too.
    for (size_t i = 0; i < properties.size(); ++i) {
        if (properties[i]->equals(property))
            return static_cast<int>(i);
    }
    return -1;
}

int IBlockState::getRawValue(const IProperty& property) const
{
    const int i = propertyIndex(property);
    if (i < 0)
        throw std::invalid_argument("Cannot get property " + property.toString() + " as it does not exist in " + container->toString());
    return values[static_cast<size_t>(i)];
}

const IBlockState* IBlockState::withRawValue(const IProperty& property, int raw) const
{
    const int i = propertyIndex(property);
    if (i < 0)
        throw std::invalid_argument("Cannot set property " + property.toString() + " as it does not exist in " + container->toString());
    const IProperty& own = *container->getProperties()[static_cast<size_t>(i)];
    const int index = own.indexOf(raw);
    if (index < 0)
        throw std::invalid_argument("Cannot set property " + property.toString() + " to " + std::to_string(raw) + " on block "
            + Block::getNameForBlock(block) + ", it is not an allowed value");
    return propertyValueTable[static_cast<size_t>(i)][static_cast<size_t>(index)];
}

const IBlockState* IBlockState::cycleProperty(const IProperty& property) const
{
    const int current = getRawValue(property);
    const auto& allowed = property.getAllowedRawValues();
    const auto it = std::find(allowed.begin(), allowed.end(), current);
    const int next = it == allowed.end() || it + 1 == allowed.end() ? allowed.front() : *(it + 1);
    return withRawValue(property, next);
}

std::string IBlockState::toString() const
{
    std::string s = Block::getNameForBlock(block);
    const auto& properties = container->getProperties();
    if (!properties.empty()) {
        s += '[';
        for (size_t i = 0; i < properties.size(); ++i) {
            if (i)
                s += ',';
            s += properties[i]->getName() + "=" + properties[i]->getValueName(values[i]);
        }
        s += ']';
    }
    return s;
}

// --- BlockState ---

BlockState::BlockState(Block* blockIn, std::vector<const IProperty*> propertiesIn)
    : block(blockIn)
    , properties(std::move(propertiesIn))
{
    // Java sorts with a comparator on the names (a stable merge sort).
    std::stable_sort(properties.begin(), properties.end(), [](const IProperty* a, const IProperty* b) { return a->getName() < b->getName(); });

    // Cartesian product, the last property varying fastest.
    std::vector<size_t> counter(properties.size(), 0);
    std::map<std::vector<int>, IBlockState*> byValues;
    while (true) {
        std::vector<int> values;
        for (size_t i = 0; i < properties.size(); ++i)
            values.push_back(properties[i]->getAllowedRawValues()[counter[i]]);
        auto state = std::unique_ptr<IBlockState>(new IBlockState(block, this, values));
        byValues.emplace(values, state.get());
        validStates.push_back(std::move(state));

        size_t i = properties.size();
        while (i > 0) {
            --i;
            if (++counter[i] < properties[i]->getAllowedRawValues().size())
                break;
            counter[i] = 0;
            if (i == 0) {
                i = SIZE_MAX;
                break;
            }
        }
        if (properties.empty() || i == SIZE_MAX)
            break;
    }

    for (const auto& state : validStates) {
        state->propertyValueTable.resize(properties.size());
        for (size_t i = 0; i < properties.size(); ++i) {
            for (const int raw : properties[i]->getAllowedRawValues()) {
                std::vector<int> values = state->values;
                values[i] = raw;
                state->propertyValueTable[i].push_back(byValues.at(values));
            }
        }
    }
}

std::vector<const IBlockState*> BlockState::getValidStates() const
{
    std::vector<const IBlockState*> states;
    states.reserve(validStates.size());
    for (const auto& state : validStates)
        states.push_back(state.get());
    return states;
}

std::string BlockState::toString() const
{
    std::string names;
    for (size_t i = 0; i < properties.size(); ++i)
        names += (i ? ", " : "") + properties[i]->getName();
    return "BlockState{block=" + Block::getNameForBlock(block) + ", properties=[" + names + "]}";
}

} // namespace mc
