#pragma once

#include <string>
#include <string_view>

namespace mc {

std::wstring toWide(std::string_view utf8);
std::string toUtf8(std::wstring_view wide);

// Java strings are UTF-16; code that indexes them by char (fonts, text fields) works on these.
std::u16string toUtf16(std::string_view utf8);
std::string fromUtf16(std::u16string_view utf16);

} // namespace mc
