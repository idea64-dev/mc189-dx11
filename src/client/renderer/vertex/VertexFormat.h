#pragma once

// Original: net/minecraft/client/renderer/vertex/VertexFormat.java

#include "client/renderer/vertex/VertexFormatElement.h"

#include <initializer_list>
#include <string>
#include <vector>

namespace mc {

class VertexFormat {
public:
    VertexFormat() = default;
    VertexFormat(std::initializer_list<VertexFormatElement> elements);

    void clear();
    VertexFormat& addElement(const VertexFormatElement& element);

    bool hasNormal() const { return normalElementOffset >= 0; }
    int getNormalOffset() const { return normalElementOffset; }
    bool hasColor() const { return colorElementOffset >= 0; }
    int getColorOffset() const { return colorElementOffset; }
    bool hasUvOffset(int id) const { return static_cast<int>(uvOffsetsById.size()) - 1 >= id; }
    int getUvOffsetById(int id) const { return uvOffsetsById[static_cast<size_t>(id)]; }
    int getIntegerSize() const { return nextOffset / 4; }
    int getNextOffset() const { return nextOffset; }
    const std::vector<VertexFormatElement>& getElements() const { return elements; }
    int getElementCount() const { return static_cast<int>(elements.size()); }
    const VertexFormatElement& getElement(int index) const { return elements[static_cast<size_t>(index)]; }
    int getOffset(int index) const { return offsets[static_cast<size_t>(index)]; }
    std::string toString() const;

    bool operator==(const VertexFormat& other) const
    {
        return nextOffset == other.nextOffset && elements == other.elements && offsets == other.offsets;
    }

private:
    bool hasPosition() const;

    std::vector<VertexFormatElement> elements;
    std::vector<int> offsets;
    int nextOffset = 0;
    int colorElementOffset = -1;
    std::vector<int> uvOffsetsById;
    int normalElementOffset = -1;
};

} // namespace mc
