#include "core/StringUtil.h"

#include <Windows.h>

namespace mc {

std::wstring toWide(std::string_view utf8)
{
    if (utf8.empty())
        return {};

    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), result.data(), length);
    return result;
}

std::string toUtf8(std::wstring_view wide)
{
    if (wide.empty())
        return {};

    const int length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), result.data(), length, nullptr, nullptr);
    return result;
}

std::u16string toUtf16(std::string_view utf8)
{
    const std::wstring wide = toWide(utf8);
    return std::u16string(wide.begin(), wide.end());
}

std::string fromUtf16(std::u16string_view utf16)
{
    return toUtf8(std::wstring_view(reinterpret_cast<const wchar_t*>(utf16.data()), utf16.size()));
}

} // namespace mc
