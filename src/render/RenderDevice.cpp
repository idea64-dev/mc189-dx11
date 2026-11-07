#include "render/RenderDevice.h"

#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/WinError.h"

#include <algorithm>
#include <array>

using Microsoft::WRL::ComPtr;

namespace mc {

namespace {

// Minecraft renders in gamma space with non-sRGB blending; keep the back buffer UNORM for identical output.
constexpr DXGI_FORMAT kBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
// LWJGL requested PixelFormat().withDepthBits(24).
constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
constexpr UINT kBackBufferCount = 2;

const char* featureLevelName(D3D_FEATURE_LEVEL level)
{
    switch (level) {
    case D3D_FEATURE_LEVEL_11_1: return "11_1";
    case D3D_FEATURE_LEVEL_11_0: return "11_0";
    default: return "unknown";
    }
}

// Prefers the discrete GPU on hybrid laptops; returns null if no hardware adapter exists.
ComPtr<IDXGIAdapter1> findHardwareAdapter()
{
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return nullptr;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(factory.As(&factory6))) {
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; factory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
                return adapter;
        }
    }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
            return adapter;
    }
    return nullptr;
}

} // namespace

RenderDevice::RenderDevice(HWND windowHandle, int initialWidth, int initialHeight)
    : hwnd(windowHandle)
    , width(std::max(1, initialWidth))
    , height(std::max(1, initialHeight))
{
    createDevice();
    createSwapChain();
    createBackBufferTargets();
}

RenderDevice::~RenderDevice()
{
    if (context) {
        context->ClearState();
        context->Flush();
    }
}

void RenderDevice::createDevice()
{
    static constexpr std::array<D3D_FEATURE_LEVEL, 2> featureLevels = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef MC_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const auto tryCreate = [&](IDXGIAdapter* adapter, D3D_DRIVER_TYPE driverType, UINT createFlags) {
        D3D_FEATURE_LEVEL obtained {};
        HRESULT hr = D3D11CreateDevice(adapter, driverType, nullptr, createFlags,
            featureLevels.data(), static_cast<UINT>(featureLevels.size()), D3D11_SDK_VERSION,
            device.ReleaseAndGetAddressOf(), &obtained, context.ReleaseAndGetAddressOf());
        if (hr == E_INVALIDARG) {
            // Runtimes without 11_1 reject the whole list.
            hr = D3D11CreateDevice(adapter, driverType, nullptr, createFlags,
                featureLevels.data() + 1, 1, D3D11_SDK_VERSION,
                device.ReleaseAndGetAddressOf(), &obtained, context.ReleaseAndGetAddressOf());
        }
        return hr;
    };

    const ComPtr<IDXGIAdapter1> adapter = findHardwareAdapter();
    const D3D_DRIVER_TYPE driverType = adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;

    HRESULT hr = tryCreate(adapter.Get(), driverType, flags);
    if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING && (flags & D3D11_CREATE_DEVICE_DEBUG)) {
        Log::warn("Direct3D debug layer is not installed (Settings > Optional features > Graphics Tools), continuing without it");
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = tryCreate(adapter.Get(), driverType, flags);
    }
    if (FAILED(hr)) {
        Log::error("Couldn't create a hardware Direct3D 11 device: {}, falling back to WARP", hresultToString(hr));
        hr = tryCreate(nullptr, D3D_DRIVER_TYPE_WARP, flags);
    }
    throwIfFailed(hr, "D3D11CreateDevice");

    // The swapchain must come from the factory that owns the device's adapter.
    ComPtr<IDXGIDevice1> dxgiDevice;
    throwIfFailed(device.As(&dxgiDevice), "QueryInterface(IDXGIDevice1)");
    ComPtr<IDXGIAdapter> deviceAdapter;
    throwIfFailed(dxgiDevice->GetAdapter(&deviceAdapter), "IDXGIDevice::GetAdapter");
    throwIfFailed(deviceAdapter->GetParent(IID_PPV_ARGS(&factory)), "IDXGIAdapter::GetParent");

    // One queued frame keeps input latency close to the original's synchronous glSwapBuffers.
    dxgiDevice->SetMaximumFrameLatency(1);

    tearingSupported = false;
    ComPtr<IDXGIFactory5> factory5;
    if (SUCCEEDED(factory.As(&factory5))) {
        BOOL allowTearing = FALSE;
        if (SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            tearingSupported = allowTearing == TRUE;
    }
    swapChainFlags = tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGIAdapter1> deviceAdapter1;
    DXGI_ADAPTER_DESC1 desc {};
    if (SUCCEEDED(deviceAdapter.As(&deviceAdapter1)))
        deviceAdapter1->GetDesc1(&desc);

    adapterInfo.description = toUtf8(desc.Description);
    adapterInfo.vendorId = desc.VendorId;
    adapterInfo.dedicatedVideoMemory = desc.DedicatedVideoMemory;
    adapterInfo.featureLevel = device->GetFeatureLevel();
    adapterInfo.software = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;

    Log::info("Direct3D 11 feature level {}, adapter: {} ({} MB VRAM){}{}",
        featureLevelName(adapterInfo.featureLevel), adapterInfo.description,
        adapterInfo.dedicatedVideoMemory / (1024 * 1024),
        (flags & D3D11_CREATE_DEVICE_DEBUG) ? ", debug layer" : "",
        tearingSupported ? ", tearing supported" : "");

#ifdef MC_DEBUG
    if (IsDebuggerPresent()) {
        ComPtr<ID3D11InfoQueue> infoQueue;
        if (SUCCEEDED(device.As(&infoQueue))) {
            infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        }
    }
#endif
}

void RenderDevice::createSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 desc {};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.Format = kBackBufferFormat;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = kBackBufferCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc.Flags = swapChainFlags;

    throwIfFailed(factory->CreateSwapChainForHwnd(device.Get(), hwnd, &desc, nullptr, nullptr, swapChain.ReleaseAndGetAddressOf()),
        "IDXGIFactory2::CreateSwapChainForHwnd");

    // Fullscreen is borderless and handled by Display; keep DXGI away from Alt+Enter and the window.
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
}

void RenderDevice::createBackBufferTargets()
{
    ComPtr<ID3D11Texture2D> backBuffer;
    throwIfFailed(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "IDXGISwapChain::GetBuffer");
    throwIfFailed(device->CreateRenderTargetView(backBuffer.Get(), nullptr, backBufferView.ReleaseAndGetAddressOf()),
        "CreateRenderTargetView(back buffer)");

    D3D11_TEXTURE2D_DESC depthDesc {};
    depthDesc.Width = static_cast<UINT>(width);
    depthDesc.Height = static_cast<UINT>(height);
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = kDepthFormat;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    throwIfFailed(device->CreateTexture2D(&depthDesc, nullptr, depthBuffer.ReleaseAndGetAddressOf()), "CreateTexture2D(depth)");
    throwIfFailed(device->CreateDepthStencilView(depthBuffer.Get(), nullptr, depthView.ReleaseAndGetAddressOf()),
        "CreateDepthStencilView");
}

void RenderDevice::releaseBackBufferTargets()
{
    context->OMSetRenderTargets(0, nullptr, nullptr);
    backBufferView.Reset();
    depthView.Reset();
    depthBuffer.Reset();
    context->Flush();
}

void RenderDevice::resize(int newWidth, int newHeight)
{
    newWidth = std::max(1, newWidth);
    newHeight = std::max(1, newHeight);
    if (newWidth == width && newHeight == height)
        return;

    width = newWidth;
    height = newHeight;

    releaseBackBufferTargets();
    const HRESULT hr = swapChain->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height), DXGI_FORMAT_UNKNOWN, swapChainFlags);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        recreateAfterDeviceLoss(hr == DXGI_ERROR_DEVICE_REMOVED ? device->GetDeviceRemovedReason() : hr);
        return;
    }
    throwIfFailed(hr, "IDXGISwapChain::ResizeBuffers");
    createBackBufferTargets();
}

void RenderDevice::bindBackBuffer()
{
    ID3D11RenderTargetView* views[] = { backBufferView.Get() };
    context->OMSetRenderTargets(1, views, depthView.Get());

    D3D11_VIEWPORT viewport {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);
}

void RenderDevice::clear(float red, float green, float blue, float alpha)
{
    const float color[4] = { red, green, blue, alpha };
    context->ClearRenderTargetView(backBufferView.Get(), color);
    context->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void RenderDevice::present(bool vsync)
{
    if (prePresent)
        prePresent();

    // Tearing lets "vsync off" actually exceed the refresh rate in windowed and borderless modes.
    const UINT syncInterval = vsync ? 1 : 0;
    const UINT flags = (!vsync && tearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;

    const HRESULT hr = swapChain->Present(syncInterval, flags);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        recreateAfterDeviceLoss(hr == DXGI_ERROR_DEVICE_REMOVED ? device->GetDeviceRemovedReason() : hr);
        return;
    }
    throwIfFailed(hr, "IDXGISwapChain::Present");
}

void RenderDevice::recreateAfterDeviceLoss(HRESULT reason)
{
    // Driver update, TDR or GPU switch. Nothing but the swapchain exists yet; once textures and buffers
    // appear (stage 2) their owners will need a hook to rebuild them here.
    Log::error("Direct3D device lost: {}, recreating", hresultToString(reason));

    if (context)
        context->ClearState();
    backBufferView.Reset();
    depthView.Reset();
    depthBuffer.Reset();
    swapChain.Reset();
    context.Reset();
    device.Reset();
    factory.Reset();

    createDevice();
    createSwapChain();
    createBackBufferTargets();
}

} // namespace mc
