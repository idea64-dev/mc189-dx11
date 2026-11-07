#include "core/Java.h"

namespace mc::java {

int32_t hashCode(std::string_view utf8)
{
    int32_t hash = 0;
    const auto append = [&](uint32_t unit) { hash = add(mul(31, hash), static_cast<int32_t>(unit)); };

    for (size_t i = 0; i < utf8.size();) {
        const auto lead = static_cast<unsigned char>(utf8[i]);
        uint32_t codePoint;
        size_t length;
        if (lead < 0x80) {
            codePoint = lead;
            length = 1;
        } else if ((lead >> 5) == 0x6) {
            codePoint = lead & 0x1F;
            length = 2;
        } else if ((lead >> 4) == 0xE) {
            codePoint = lead & 0x0F;
            length = 3;
        } else if ((lead >> 3) == 0x1E) {
            codePoint = lead & 0x07;
            length = 4;
        } else {
            append(0xFFFD); // invalid lead byte, as Java's UTF-8 decoder would substitute
            ++i;
            continue;
        }

        if (i + length > utf8.size()) {
            append(0xFFFD);
            break;
        }
        for (size_t k = 1; k < length; ++k)
            codePoint = (codePoint << 6) | (static_cast<unsigned char>(utf8[i + k]) & 0x3F);
        i += length;

        if (codePoint >= 0x10000) {
            codePoint -= 0x10000;
            append(0xD800 + (codePoint >> 10));
            append(0xDC00 + (codePoint & 0x3FF));
        } else {
            append(codePoint);
        }
    }
    return hash;
}

} // namespace mc::java
