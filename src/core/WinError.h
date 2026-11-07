#pragma once

#include <Windows.h>

#include <stdexcept>
#include <string>

namespace mc {

// Formats an HRESULT as "0x887A0005 (DXGI_ERROR_DEVICE_REMOVED)" where the name is known.
std::string hresultToString(HRESULT hr);

class WinError : public std::runtime_error {
public:
    WinError(const std::string& what, HRESULT hr);
    HRESULT code() const { return hr_; }

private:
    HRESULT hr_;
};

inline void throwIfFailed(HRESULT hr, const char* what)
{
    if (FAILED(hr))
        throw WinError(what, hr);
}

} // namespace mc
