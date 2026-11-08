#pragma once

#include <Windows.h>

#include <cstdint>
#include <string_view>
#include <vector>

namespace mc {

class RenderDevice;

// Original: org.lwjgl.opengl.DisplayMode
struct DisplayMode {
    int width = 0;
    int height = 0;
    // Refresh rate in Hz; 0 for windowed modes, like LWJGL's DisplayMode(width, height).
    int frequency = 0;
};

// Window icon image, 0xAARRGGBB pixels, rows top to bottom.
struct DisplayIcon {
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;
};

// Replacement for org.lwjgl.opengl.Display: a Win32 window plus the Direct3D 11 device that renders into it.
//
// The window and its message loop live on a dedicated thread, so moving or resizing the window never
// blocks the game loop. Input and window events are queued there and handed to the client thread in
// processMessages() (called from update()), which is when Keyboard/Mouse state changes, like LWJGL's
// buffered input. All public functions are for the client thread.
class Display {
public:
    // Settings that can be changed before and after create().
    static void setTitle(std::string_view title);
    static void setDisplayMode(const DisplayMode& mode);
    static DisplayMode getDisplayMode();
    static DisplayMode getDesktopDisplayMode();
    // Borderless fullscreen on the monitor that holds the window.
    static void setFullscreen(bool fullscreen);
    static bool isFullscreen();
    static void setResizable(bool resizable);
    static void setVSyncEnabled(bool sync);
    // Like LWJGL, several sizes may be given; the best match is used for the title bar and the taskbar.
    static void setIcon(std::vector<DisplayIcon> icons);

    static void create();
    static void destroy();
    static bool isCreated();

    static bool isCloseRequested();
    static bool isActive();
    // True if the window size changed during the last update().
    static bool wasResized();
    static int getWidth();
    static int getHeight();

    // Presents the frame and processes window messages.
    static void update();
    static void processMessages();
    // Frame limiter: call once per frame to run at most `fps` frames per second.
    static void sync(int fps);

    static RenderDevice& getDevice();
    static HWND getWindowHandle();

private:
    friend class Mouse;
    static void setGrabbedInternal(bool grabbed);
    // Client-area coordinates with a top-left origin.
    static void setCursorPositionInternal(int x, int y);
};

} // namespace mc
