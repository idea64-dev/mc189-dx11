#include "client/renderer/vertex/VertexFormatElement.h"

#include "core/Log.h"

namespace mc {

namespace {

const char* usageName(VertexFormatElement::EnumUsage usage)
{
    switch (usage) {
    case VertexFormatElement::EnumUsage::POSITION: return "Position";
    case VertexFormatElement::EnumUsage::NORMAL: return "Normal";
    case VertexFormatElement::EnumUsage::COLOR: return "Vertex Color";
    case VertexFormatElement::EnumUsage::UV: return "UV";
    case VertexFormatElement::EnumUsage::MATRIX: return "Bone Matrix";
    case VertexFormatElement::EnumUsage::BLEND_WEIGHT: return "Blend Weight";
    case VertexFormatElement::EnumUsage::PADDING: return "Padding";
    }
    return "";
}

const char* typeName(VertexFormatElement::EnumType type)
{
    switch (type) {
    case VertexFormatElement::EnumType::FLOAT: return "Float";
    case VertexFormatElement::EnumType::UBYTE: return "Unsigned Byte";
    case VertexFormatElement::EnumType::BYTE: return "Byte";
    case VertexFormatElement::EnumType::USHORT: return "Unsigned Short";
    case VertexFormatElement::EnumType::SHORT: return "Short";
    case VertexFormatElement::EnumType::UINT: return "Unsigned Int";
    case VertexFormatElement::EnumType::INT: return "Int";
    }
    return "";
}

} // namespace

VertexFormatElement::VertexFormatElement(int indexIn, EnumType typeIn, EnumUsage usageIn, int count)
    : type(typeIn)
    , usage(usageIn)
    , index(indexIn)
    , elementCount(count)
{
    if (indexIn != 0 && usageIn != EnumUsage::UV) {
        Log::warn("Multiple vertex elements of the same type other than UVs are not supported. Forcing type to UV.");
        usage = EnumUsage::UV;
    }
}

int VertexFormatElement::typeSize(EnumType type)
{
    switch (type) {
    case EnumType::UBYTE:
    case EnumType::BYTE: return 1;
    case EnumType::USHORT:
    case EnumType::SHORT: return 2;
    default: return 4;
    }
}

std::string VertexFormatElement::toString() const
{
    return std::to_string(elementCount) + "," + usageName(usage) + "," + typeName(type);
}

} // namespace mc
