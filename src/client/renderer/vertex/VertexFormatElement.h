#pragma once

// Original: net/minecraft/client/renderer/vertex/VertexFormatElement.java

#include <string>

namespace mc {

class VertexFormatElement {
public:
    enum class EnumType { FLOAT, UBYTE, BYTE, USHORT, SHORT, UINT, INT };
    enum class EnumUsage { POSITION, NORMAL, COLOR, UV, MATRIX, BLEND_WEIGHT, PADDING };

    VertexFormatElement(int indexIn, EnumType typeIn, EnumUsage usageIn, int count);

    EnumType getType() const { return type; }
    EnumUsage getUsage() const { return usage; }
    int getElementCount() const { return elementCount; }
    int getIndex() const { return index; }
    int getSize() const { return typeSize(type) * elementCount; }
    bool isPositionElement() const { return usage == EnumUsage::POSITION; }
    std::string toString() const;

    bool operator==(const VertexFormatElement& other) const = default;

    static int typeSize(EnumType type);

private:
    EnumType type;
    EnumUsage usage;
    int index;
    int elementCount;
};

} // namespace mc
