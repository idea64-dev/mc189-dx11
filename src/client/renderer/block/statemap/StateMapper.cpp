#include "client/renderer/block/statemap/StateMapper.h"

#include "block/Block.h"
#include "block/state/BlockState.h"

#include <algorithm>

namespace mc {

StatePropertyMap::StatePropertyMap(const IBlockState* state)
{
    const std::vector<const IProperty*>& properties = state->getPropertyNames();
    for (size_t i = 0; i < properties.size(); ++i)
        entries.emplace_back(properties[i], state->getRawValues()[i]);
}

std::string StatePropertyMap::remove(const std::string& name)
{
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) { return entry.first->getName() == name; });
    if (it == entries.end())
        return "";
    std::string value = it->first->getValueName(it->second);
    entries.erase(it);
    return value;
}

std::string StatePropertyMap::toPropertyString() const
{
    std::string s;
    for (const auto& [property, raw] : entries) {
        if (!s.empty())
            s += ',';
        s += property->getName();
        s += '=';
        s += property->getValueName(raw);
    }
    return s.empty() ? "normal" : s;
}

std::vector<std::pair<const IBlockState*, ModelResourceLocation>> IStateMapper::putStateModelLocations(const Block* block) const
{
    std::vector<std::pair<const IBlockState*, ModelResourceLocation>> result;
    for (const IBlockState* state : block->getBlockState().getValidStates())
        result.emplace_back(state, getModelResourceLocation(state));
    return result;
}

ModelResourceLocation DefaultStateMapper::getModelResourceLocation(const IBlockState* state) const
{
    return ModelResourceLocation(Block::getNameForBlock(state->getBlock()), StatePropertyMap(state).toPropertyString());
}

ModelResourceLocation StateMap::getModelResourceLocation(const IBlockState* state) const
{
    StatePropertyMap map(state);
    std::string s = name.empty() ? Block::getNameForBlock(state->getBlock()) : map.remove(name);
    s += suffix;
    for (const std::string& property : ignored)
        map.remove(property);
    return ModelResourceLocation(s, map.toPropertyString());
}

void BlockStateMapper::registerBlockStateMapper(const Block* block, std::unique_ptr<IStateMapper> stateMapper)
{
    blockStateMap[block] = std::move(stateMapper);
}

void BlockStateMapper::registerBuiltInBlocks(std::initializer_list<const Block*> blocks)
{
    setBuiltInBlocks.insert(blocks.begin(), blocks.end());
}

std::vector<std::pair<const IBlockState*, ModelResourceLocation>> BlockStateMapper::putAllStateModelLocations() const
{
    static const DefaultStateMapper defaultMapper;
    std::vector<std::pair<const IBlockState*, ModelResourceLocation>> result;
    for (const Block* block : Block::blockRegistry.values()) {
        if (setBuiltInBlocks.contains(block))
            continue;
        const auto it = blockStateMap.find(block);
        const IStateMapper& mapper = it == blockStateMap.end() ? static_cast<const IStateMapper&>(defaultMapper) : *it->second;
        auto locations = mapper.putStateModelLocations(block);
        result.insert(result.end(), std::make_move_iterator(locations.begin()), std::make_move_iterator(locations.end()));
    }
    return result;
}

} // namespace mc
