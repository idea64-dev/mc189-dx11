#include "core/WinError.h"

#include "core/StringUtil.h"

#include <dxgi.h>

#include <format>

namespace mc {

namespace {

const char* knownName(HRESULT hr)
{
    switch (hr) {
    case E_FAIL: return "E_FAIL";
    case E_INVALIDARG: return "E_INVALIDARG";
    case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
    case E_NOTIMPL: return "E_NOTIMPL";
    case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG";
    case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED";
    case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
    case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
    case DXGI_ERROR_UNSUPPORTED: return "DXGI_ERROR_UNSUPPORTED";
    case DXGI_ERROR_SDK_COMPONENT_MISSING: return "DXGI_ERROR_SDK_COMPONENT_MISSING";
    default: return nullptr;
    }
}

} // namespace

std::string hresultToString(HRESULT hr)
{
    if (const char* name = knownName(hr))
        return std::format("0x{:08X} ({})", static_cast<unsigned long>(hr), name);

    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, static_cast<DWORD>(hr), MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
        reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);

    std::string message;
    if (length > 0 && buffer) {
        message = toUtf8(std::wstring_view(buffer, length));
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r' || message.back() == '.'))
            message.pop_back();
    }
    if (buffer)
        LocalFree(buffer);

    if (message.empty())
        return std::format("0x{:08X}", static_cast<unsigned long>(hr));
    return std::format("0x{:08X} ({})", static_cast<unsigned long>(hr), message);
}

WinError::WinError(const std::string& what, HRESULT hr)
    : std::runtime_error(what + " failed: " + hresultToString(hr))
    , hr_(hr)
{
}

} // namespace mc
