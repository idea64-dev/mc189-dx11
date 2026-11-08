#pragma once

#include <Windows.h>
#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace mc {

struct AdapterInfo {
    std::string description;
    uint32_t vendorId = 0;
    uint64_t dedicatedVideoMemory = 0;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    bool software = false;
};

// Work for the immediate context: a recorded command list, then a function run on the context.
struct RenderJob {
    Microsoft::WRL::ComPtr<ID3D11CommandList> commands;
    std::function<void(ID3D11DeviceContext*)> after;
};

// Direct3D 11 device, immediate context and flip-model swapchain for one window.
// Plays the role of the OpenGL context that LWJGL's Display.create() made in the original.
// The frames are recorded elsewhere (the GL emulation's deferred context) and executed and presented on a
// render thread, which owns the immediate context; the next frame is recorded meanwhile. Anything else that
// needs the immediate context waits until the render thread is idle (runOnImmediateContext, resize).
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
    // Ends the frame: the present handler when one is set (the GL emulation forwards it to its own thread),
    // else submitFrame.
    void present(bool vsync);
    void setPresentHandler(std::function<void(bool)> handler) { presentHandler = std::move(handler); }
    // Hands the frame to the render thread (waiting while the previous one is still queued), which executes
    // the job of the pre-present callback and presents.
    void submitFrame(bool vsync);
    // Records the end of a frame: the GL emulation finishes its command list and copies the window
    // framebuffer to the back buffer (the job's function, on the render thread).
    void setPrePresentCallback(std::function<RenderJob()> callback) { prePresent = std::move(callback); }
    // Runs the job on the calling thread once the render thread is idle (read-backs).
    void runOnImmediateContext(const RenderJob& job);
    // Until every submitted frame has been executed and presented.
    void waitForRenderThread();
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
    void renderThreadMain();
    void execute(const RenderJob& job);
    void logDebugMessages();

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

    std::function<RenderJob()> prePresent;
    std::function<void(bool)> presentHandler;

    struct Frame {
        RenderJob job;
        bool vsync = false;
    };
    std::thread renderThread;
    std::mutex renderMutex;
    std::condition_variable renderCondition;
    std::optional<Frame> queuedFrame;
    bool renderBusy = false;
    bool renderStop = false;
    HRESULT presentResult = S_OK;
    AdapterInfo adapterInfo;
    bool tearingSupported = false;
    UINT swapChainFlags = 0;
};

} // namespace mc
