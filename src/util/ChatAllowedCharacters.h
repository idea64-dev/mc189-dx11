#pragma once

// Original: net/minecraft/util/ChatAllowedCharacters.java

#include <array>
#include <string>
#include <string_view>

namespace mc {

class ChatAllowedCharacters {
public:
    // Characters not allowed in file names (world directories).
    static constexpr std::array<char16_t, 15> allowedCharactersArray { u'/', u'\n', u'\r', u'\t', u'\0', u'\f', u'`', u'?', u'*', u'\\', u'<', u'>', u'|',
        u'"', u':' };

    static constexpr bool isAllowedCharacter(char16_t character) { return character != 167 && character >= 32 && character != 127; }

    static std::u16string filterAllowedCharacters(std::u16string_view input)
    {
        std::u16string result;
        for (const char16_t c0 : input) {
            if (isAllowedCharacter(c0))
                result += c0;
        }
        return result;
    }
};

} // namespace mc
