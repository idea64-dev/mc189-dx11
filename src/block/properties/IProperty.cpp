#include "block/properties/IProperty.h"

namespace mc {

std::string IProperty::toString() const
{
    std::string values;
    for (size_t i = 0; i < allowedValues.size(); ++i)
        values += (i ? ", " : "") + getValueName(allowedValues[i]);
    return getKindName() + "{name=" + name + ", clazz=" + kind + ", values=[" + values + "]}";
}

std::string IProperty::getKindName() const
{
    if (kind == "Integer")
        return "PropertyInteger";
    if (kind == "Boolean")
        return "PropertyBool";
    return "PropertyEnum";
}

PropertyInteger::PropertyInteger(std::string nameIn, int min, int max)
    : IProperty(std::move(nameIn), "Integer", {})
{
    if (min < 0)
        throw std::invalid_argument("Min value of " + name + " must be 0 or greater");
    if (max <= min)
        throw std::invalid_argument("Max value of " + name + " must be greater than min (" + std::to_string(min) + ")");
    // Java collects them in a HashSet; small non-negative Integers iterate in ascending order.
    for (int i = min; i <= max; ++i)
        allowedValues.push_back(i);
}

} // namespace mc
