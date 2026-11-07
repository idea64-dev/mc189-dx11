#include "client/renderer/vertex/VertexFormat.h"

#include "core/Log.h"

namespace mc {

VertexFormat::VertexFormat(std::initializer_list<VertexFormatElement> list)
{
    for (const VertexFormatElement& element : list)
        addElement(element);
}

void VertexFormat::clear()
{
    elements.clear();
    offsets.clear();
    colorElementOffset = -1;
    uvOffsetsById.clear();
    normalElementOffset = -1;
    nextOffset = 0;
}

VertexFormat& VertexFormat::addElement(const VertexFormatElement& element)
{
    if (element.isPositionElement() && hasPosition()) {
        Log::warn("VertexFormat error: Trying to add a position VertexFormatElement when one already exists, ignoring.");
        return *this;
    }

    elements.push_back(element);
    offsets.push_back(nextOffset);
    switch (element.getUsage()) {
    case VertexFormatElement::EnumUsage::NORMAL:
        normalElementOffset = nextOffset;
        break;
    case VertexFormatElement::EnumUsage::COLOR:
        colorElementOffset = nextOffset;
        break;
    case VertexFormatElement::EnumUsage::UV:
        // Java: uvOffsetsById.add(index, offset), i.e. insert at that position.
        uvOffsetsById.insert(uvOffsetsById.begin() + std::min<size_t>(static_cast<size_t>(element.getIndex()), uvOffsetsById.size()), nextOffset);
        break;
    default:
        break;
    }
    nextOffset += element.getSize();
    return *this;
}

bool VertexFormat::hasPosition() const
{
    for (const VertexFormatElement& element : elements) {
        if (element.isPositionElement())
            return true;
    }
    return false;
}

std::string VertexFormat::toString() const
{
    std::string s = "format: " + std::to_string(elements.size()) + " elements: ";
    for (size_t i = 0; i < elements.size(); ++i) {
        s += elements[i].toString();
        if (i != elements.size() - 1)
            s += " ";
    }
    return s;
}

} // namespace mc
