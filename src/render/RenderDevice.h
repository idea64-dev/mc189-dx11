#pragma once

#include <Windows.h>
#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>
#include <functional>
#include <string>

namespace mc {

struct AdapterInfo {
    std::string description;
    uint32_t vendorId = 0;
    uint64_t dedicatedVideoMemory = 0;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    bool software = false;
};

// Direct3D 11 device, immediate context and flip-model swapchain for one window.
// Plays the role of the OpenGL context that LWJGL's Display.create() made in the original.
class RenderDevice {
public:
    RenderDevice(HWND hwnd, int width, int height);
    ~RenderDevice();

    RenderDevice(const RenderDevice&) = delete;
    RenderDevice& operator=(const RenderDevice&) = delete;

    void resize(int width, int height);

    // Binds the back buffer and depth buffer with a full-window viewport.
    // Flip-model swapchains unbind the back buffer on Present, so this runs every frame.
    void bindBackBuffer();
    // Equivalent of Java GlStateManager.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT).
    void clear(float red, float green, float blue, float alpha);
    void present(bool vsync);
    // Runs right before every Present (the GL emulation copies the window framebuffer to the back buffer here).
    void setPrePresentCallback(std::function<void()> callback) { prePresent = std::move(callback); }
    ID3D11RenderTargetView* getBackBufferView() const { return backBufferView.Get(); }

    ID3D11Device* getDevice() const { return device.Get(); }
    ID3D11DeviceContext* getContext() const { return context.Get(); }
    const AdapterInfo& getAdapterInfo() const { return adapterInfo; }
    bool isTearingSupported() const { return tearingSupported; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    void createDevice();
    void createSwapChain();
    void createBackBufferTargets();
    void releaseBackBufferTargets();
    void recreateAfterDeviceLoss(HRESULT reason);

    HWND hwnd;
    int width;
    int height;

    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBuffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthView;

    std::function<void()> prePresent;
    AdapterInfo adapterInfo;
    bool tearingSupported = false;
    UINT swapChainFlags = 0;
};

} // namespace mc
