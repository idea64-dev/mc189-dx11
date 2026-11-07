#pragma once

#include <string>
#include <string_view>

namespace mc {

std::wstring toWide(std::string_view utf8);
std::string toUtf8(std::wstring_view wide);

} // namespace mc
