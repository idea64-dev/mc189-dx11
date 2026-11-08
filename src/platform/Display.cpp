#include "platform/Display.h"

#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/WinError.h"
#include "platform/Keyboard.h"
#include "platform/Mouse.h"
#include "platform/Sys.h"
#include "render/RenderDevice.h"

#include <windowsx.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>
#include <vector>

namespace mc {

namespace {

constexpr wchar_t kWindowClassName[] = L"MinecraftDX11Window";

// Requests from the client thread, executed on the window thread (SendMessage blocks until done).
constexpr UINT WM_APP_SET_TITLE = WM_APP + 1;
constexpr UINT WM_APP_SET_FULLSCREEN = WM_APP + 2;
constexpr UINT WM_APP_SET_GRABBED = WM_APP + 3;
constexpr UINT WM_APP_SET_CURSOR_POS = WM_APP + 4;
constexpr UINT WM_APP_SHOW = WM_APP + 5;
constexpr UINT WM_APP_DESTROY = WM_APP + 6;

// Window thread -> client thread. Positions are client-area pixels with a top-left origin.
struct KeyMessage {
    int key;
    char16_t character;
    bool state;
    bool repeat;
    int64_t nanos;
};

struct MouseButtonMessage {
    int button;
    bool state;
    LONG x;
    LONG y;
    int64_t nanos;
};

struct MouseMoveMessage {
    LONG dx; // LWJGL convention: positive dy is upwards
    LONG dy;
    LONG x;
    LONG y;
    int64_t nanos;
};

struct MouseWheelMessage {
    int delta;
    int64_t nanos;
};

struct ResizeMessage {
    int width;
    int height;
};

struct CloseMessage { };

struct FocusMessage {
    bool active;
};

using WindowMessage = std::variant<KeyMessage, MouseButtonMessage, MouseMoveMessage, MouseWheelMessage,
    ResizeMessage, CloseMessage, FocusMessage>;

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

// Touched only by the window thread.
struct WindowThreadState {
    HWND hwnd = nullptr;
    bool active = false;
    bool grabbed = false;
    bool fullscreen = false;
    LONG_PTR windowedStyle = 0;
    WINDOWPLACEMENT windowedPlacement { sizeof(WINDOWPLACEMENT) };
    std::array<bool, Keyboard::KEYBOARD_SIZE> keyDown {};
    unsigned buttonsDown = 0;
    POINT cursor {};        // last cursor position while not grabbed
    POINT virtualCursor {}; // position reported while grabbed
    POINT grabOrigin {};    // cursor position restored on ungrab
    bool haveAbsolute = false;
    POINT lastAbsolute {};
    int clientWidth = 1;
    int clientHeight = 1;
};

struct DisplayState {
    // Client thread.
    std::wstring title = L"Game";
    DisplayMode mode { 640, 480 };
    bool fullscreen = false;
    bool resizable = false;
    bool vsync = false;
    bool created = false;
    bool closeRequested = false;
    bool active = false;
    bool resized = false;
    int width = 0;
    int height = 0;
    std::unique_ptr<RenderDevice> device;
    int64_t nextFrameNanos = 0;
    HANDLE syncTimer = nullptr;
    HWND hwnd = nullptr;
    std::thread thread;
    std::vector<DisplayIcon> icons;
    HICON smallIcon = nullptr;
    HICON bigIcon = nullptr;

    // Shared.
    std::mutex inboxMutex;
    std::vector<WindowMessage> inbox;

    WindowThreadState window;
};

DisplayState& state()
{
    static DisplayState instance;
    return instance;
}

HICON createIcon(const DisplayIcon& icon)
{
    BITMAPV5HEADER header {};
    header.bV5Size = sizeof(header);
    header.bV5Width = icon.width;
    header.bV5Height = -icon.height; // top-down rows
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask = 0x00FF0000;
    header.bV5GreenMask = 0x0000FF00;
    header.bV5BlueMask = 0x000000FF;
    header.bV5AlphaMask = 0xFF000000;

    void* bits = nullptr;
    const HDC screen = GetDC(nullptr);
    const HBITMAP color = CreateDIBSection(screen, reinterpret_cast<const BITMAPINFO*>(&header), DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, screen);
    if (!color)
        return nullptr;
    // 0xAARRGGBB in little-endian memory is exactly the BGRA layout of the DIB.
    std::memcpy(bits, icon.pixels.data(), icon.pixels.size() * sizeof(uint32_t));

    const HBITMAP mask = CreateBitmap(icon.width, icon.height, 1, 1, nullptr);
    ICONINFO info {};
    info.fIcon = TRUE;
    info.hbmColor = color;
    info.hbmMask = mask;
    const HICON handle = CreateIconIndirect(&info);
    DeleteObject(color);
    DeleteObject(mask);
    return handle;
}

const DisplayIcon* closestIcon(const std::vector<DisplayIcon>& icons, int size)
{
    const DisplayIcon* best = nullptr;
    for (const DisplayIcon& icon : icons) {
        if (icon.width <= 0 || icon.height <= 0 || icon.pixels.size() != static_cast<size_t>(icon.width) * static_cast<size_t>(icon.height))
            continue;
        if (!best || std::abs(icon.width - size) < std::abs(best->width - size))
            best = &icon;
    }
    return best;
}

void applyIcons(DisplayState& s)
{
    const HICON oldSmall = s.smallIcon;
    const HICON oldBig = s.bigIcon;
    const DisplayIcon* smallImage = closestIcon(s.icons, GetSystemMetrics(SM_CXSMICON));
    const DisplayIcon* bigImage = closestIcon(s.icons, GetSystemMetrics(SM_CXICON));
    s.smallIcon = smallImage ? createIcon(*smallImage) : nullptr;
    s.bigIcon = bigImage ? createIcon(*bigImage) : nullptr;

    SendMessageW(s.hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(s.smallIcon));
    SendMessageW(s.hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(s.bigIcon));
    if (oldSmall)
        DestroyIcon(oldSmall);
    if (oldBig)
        DestroyIcon(oldBig);
}

void post(WindowMessage message)
{
    DisplayState& s = state();
    std::lock_guard lock(s.inboxMutex);
    s.inbox.push_back(std::move(message));
}

// LWJGL key codes are DirectInput scan codes: the PS/2 set-1 scan code, +0x80 for E0-prefixed keys.
int toLwjglKey(WPARAM virtualKey, LPARAM lParam)
{
    // Pause arrives with NumLock's scan code and NumLock with an extended flag; PrintScreen varies by keyboard.
    if (virtualKey == VK_PAUSE)
        return Keyboard::KEY_PAUSE;
    if (virtualKey == VK_NUMLOCK)
        return Keyboard::KEY_NUMLOCK;
    if (virtualKey == VK_SNAPSHOT)
        return Keyboard::KEY_SYSRQ;

    UINT scanCode = (static_cast<UINT>(lParam) >> 16) & 0xFF;
    bool extended = (lParam & (1 << 24)) != 0;

    // Synthetic input (SendInput without KEYEVENTF_SCANCODE, remote desktop tools, macro software) often carries
    // a zero or bogus scan code. Trust the hardware one only if it agrees with the virtual key.
    const UINT expected = MapVirtualKeyW(static_cast<UINT>(virtualKey), MAPVK_VK_TO_VSC_EX);
    bool trusted;
    switch (virtualKey) {
    case VK_SHIFT:
        trusted = scanCode == 0x2A || scanCode == 0x36; // left / right
        break;
    case VK_CONTROL:
        trusted = scanCode == 0x1D;
        break;
    case VK_MENU:
        trusted = scanCode == 0x38;
        break;
    default:
        trusted = scanCode != 0 && (expected == 0 || (expected & 0xFF) == scanCode);
        break;
    }
    if (!trusted) {
        scanCode = expected & 0xFF;
        extended = (expected & 0xFF00) == 0xE000;
    }
    if (scanCode == 0)
        return Keyboard::KEY_NONE;
    // Fake Shift that some keyboards wrap around navigation keys while NumLock is on.
    if (scanCode == 0x2A && extended)
        return Keyboard::KEY_NONE;

    return static_cast<int>(scanCode | (extended ? 0x80u : 0u));
}

POINT reportedCursor(const WindowThreadState& w)
{
    return w.grabbed ? w.virtualCursor : w.cursor;
}

void releaseKey(WindowThreadState& w, int key, int64_t now)
{
    if (!w.keyDown[static_cast<size_t>(key)])
        return;
    w.keyDown[static_cast<size_t>(key)] = false;
    post(KeyMessage { key, Keyboard::CHAR_NONE, false, false, now });
}

void onKey(WindowThreadState& w, WPARAM virtualKey, LPARAM lParam, bool down)
{
    const int key = toLwjglKey(virtualKey, lParam);
    const int64_t now = Sys::nanoTime();

    if (!down) {
        if (key == Keyboard::KEY_NONE)
            return;
        releaseKey(w, key, now);
        // With both Shifts held, Windows reports only the last release; catch up on the other one.
        if (virtualKey == VK_SHIFT) {
            if (!(GetKeyState(VK_LSHIFT) & 0x8000))
                releaseKey(w, Keyboard::KEY_LSHIFT, now);
            if (!(GetKeyState(VK_RSHIFT) & 0x8000))
                releaseKey(w, Keyboard::KEY_RSHIFT, now);
        }
        return;
    }

    // TranslateMessage has already queued the characters this key produced. Like LWJGL on Windows, the
    // first one rides on the key event; any further ones (dead-key leftovers) become key-less char events.
    KeyMessage message { key, Keyboard::CHAR_NONE, true, (lParam & (1 << 30)) != 0, now };
    std::vector<char16_t> extraCharacters;
    MSG charMessage;
    while (PeekMessageW(&charMessage, w.hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
        const auto character = static_cast<char16_t>(charMessage.wParam);
        if (message.character == Keyboard::CHAR_NONE)
            message.character = character;
        else
            extraCharacters.push_back(character);
    }

    if (key == Keyboard::KEY_NONE && message.character == Keyboard::CHAR_NONE && extraCharacters.empty())
        return;
    if (key != Keyboard::KEY_NONE)
        w.keyDown[static_cast<size_t>(key)] = true;

    post(message);
    for (const char16_t character : extraCharacters)
        post(KeyMessage { Keyboard::KEY_NONE, character, true, false, now });
}

void releaseEverything(WindowThreadState& w)
{
    const int64_t now = Sys::nanoTime();
    for (int key = 1; key < Keyboard::KEYBOARD_SIZE; ++key)
        releaseKey(w, key, now);

    const POINT position = reportedCursor(w);
    for (int button = 0; button < Mouse::BUTTON_COUNT; ++button) {
        if (w.buttonsDown & (1u << button))
            post(MouseButtonMessage { button, false, position.x, position.y, now });
    }
    if (w.buttonsDown != 0) {
        w.buttonsDown = 0;
        ReleaseCapture();
    }
}

void onMouseButton(WindowThreadState& w, int button, bool down, LPARAM lParam)
{
    if (!w.grabbed)
        w.cursor = POINT { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    const unsigned bit = 1u << button;
    if (down) {
        if (w.buttonsDown == 0)
            SetCapture(w.hwnd); // keep receiving the release when dragging outside the window
        w.buttonsDown |= bit;
    } else {
        if (!(w.buttonsDown & bit))
            return;
        w.buttonsDown &= ~bit;
        if (w.buttonsDown == 0)
            ReleaseCapture();
    }

    const POINT position = reportedCursor(w);
    post(MouseButtonMessage { button, down, position.x, position.y, Sys::nanoTime() });
}

void onMouseMove(WindowThreadState& w, LPARAM lParam)
{
    if (w.grabbed)
        return; // relative motion comes from WM_INPUT

    const POINT position { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    const LONG dx = position.x - w.cursor.x;
    const LONG dy = position.y - w.cursor.y;
    w.cursor = position;
    if (dx != 0 || dy != 0)
        post(MouseMoveMessage { dx, -dy, position.x, position.y, Sys::nanoTime() });
}

void onRawInput(WindowThreadState& w, LPARAM lParam)
{
    RAWINPUT raw;
    UINT size = sizeof(raw);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1))
        return;
    if (raw.header.dwType != RIM_TYPEMOUSE || !w.grabbed || !w.active)
        return;

    const RAWMOUSE& mouse = raw.data.mouse;
    LONG dx = 0;
    LONG dy = 0;
    if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
        // Remote desktop, tablets and some VMs report absolute positions; derive deltas from them.
        const bool virtualDesktop = (mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
        const int screenWidth = GetSystemMetrics(virtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
        const int screenHeight = GetSystemMetrics(virtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
        const POINT absolute { MulDiv(mouse.lLastX, screenWidth, 65535), MulDiv(mouse.lLastY, screenHeight, 65535) };
        if (w.haveAbsolute) {
            dx = absolute.x - w.lastAbsolute.x;
            dy = absolute.y - w.lastAbsolute.y;
        }
        w.lastAbsolute = absolute;
        w.haveAbsolute = true;
    } else {
        dx = mouse.lLastX;
        dy = mouse.lLastY;
    }
    if (dx == 0 && dy == 0)
        return;

    w.virtualCursor.x = std::clamp<LONG>(w.virtualCursor.x + dx, 0, w.clientWidth - 1);
    w.virtualCursor.y = std::clamp<LONG>(w.virtualCursor.y + dy, 0, w.clientHeight - 1);
    post(MouseMoveMessage { dx, -dy, w.virtualCursor.x, w.virtualCursor.y, Sys::nanoTime() });
}

// While grabbed and focused the cursor is hidden and pinned to the window centre.
void applyCursorClip(WindowThreadState& w)
{
    if (w.grabbed && w.active) {
        RECT client;
        GetClientRect(w.hwnd, &client);
        POINT centre { client.right / 2, client.bottom / 2 };
        ClientToScreen(w.hwnd, &centre);
        const RECT clip { centre.x, centre.y, centre.x + 1, centre.y + 1 };
        ClipCursor(&clip);
        SetCursor(nullptr);
    } else {
        ClipCursor(nullptr);
    }
}

void setGrabbed(WindowThreadState& w, bool grab)
{
    if (grab == w.grabbed)
        return;

    w.grabbed = grab;
    if (grab) {
        w.grabOrigin = w.cursor;
        w.virtualCursor = w.cursor;
        w.haveAbsolute = false;
    } else {
        ClipCursor(nullptr);
        POINT screen = w.grabOrigin;
        ClientToScreen(w.hwnd, &screen);
        SetCursorPos(screen.x, screen.y);
        w.cursor = w.grabOrigin;
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
    }
    applyCursorClip(w);
}

void setCursorPosition(WindowThreadState& w, int x, int y)
{
    const POINT position { x, y };
    w.cursor = position;
    w.virtualCursor = position;
    if (!w.grabbed) {
        POINT screen = position;
        ClientToScreen(w.hwnd, &screen);
        SetCursorPos(screen.x, screen.y);
    }
}

void applyFullscreen(WindowThreadState& w, bool fullscreen)
{
    if (fullscreen == w.fullscreen)
        return;

    w.fullscreen = fullscreen;
    if (fullscreen) {
        w.windowedStyle = GetWindowLongPtrW(w.hwnd, GWL_STYLE);
        GetWindowPlacement(w.hwnd, &w.windowedPlacement);

        MONITORINFO monitor { sizeof(MONITORINFO) };
        GetMonitorInfoW(MonitorFromWindow(w.hwnd, MONITOR_DEFAULTTONEAREST), &monitor);
        const RECT& r = monitor.rcMonitor;

        SetWindowLongPtrW(w.hwnd, GWL_STYLE, (w.windowedStyle & ~static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW)) | WS_POPUP);
        SetWindowPos(w.hwnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top,
            SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
    } else {
        SetWindowLongPtrW(w.hwnd, GWL_STYLE, w.windowedStyle);
        SetWindowPlacement(w.hwnd, &w.windowedPlacement);
        SetWindowPos(w.hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowThreadState& w = state().window;

    switch (message) {
    case WM_CLOSE:
        post(CloseMessage {});
        return 0;

    case WM_DESTROY:
        ClipCursor(nullptr);
        PostQuitMessage(0);
        return 0;

    case WM_ACTIVATE:
        w.active = LOWORD(wParam) != WA_INACTIVE;
        if (!w.active)
            releaseEverything(w); // key releases are not delivered to unfocused windows
        applyCursorClip(w);
        post(FocusMessage { w.active });
        break;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            w.clientWidth = std::max(1, static_cast<int>(LOWORD(lParam)));
            w.clientHeight = std::max(1, static_cast<int>(HIWORD(lParam)));
            post(ResizeMessage { LOWORD(lParam), HIWORD(lParam) });
            applyCursorClip(w);
        }
        return 0;

    case WM_MOVE:
        applyCursorClip(w);
        return 0;

    case WM_DPICHANGED:
        if (!w.fullscreen) {
            const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                suggested->right - suggested->left, suggested->bottom - suggested->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        onKey(w, wParam, lParam, true);
        break; // DefWindowProc still handles Alt+F4

    case WM_KEYUP:
    case WM_SYSKEYUP:
        onKey(w, wParam, lParam, false);
        break;

    case WM_CHAR:
        // Characters without a key press of their own (IME, Alt codes).
        post(KeyMessage { Keyboard::KEY_NONE, static_cast<char16_t>(wParam), true, false, Sys::nanoTime() });
        return 0;

    case WM_SYSCHAR:
        return 0; // no beep on Alt+letter

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU)
            return 0; // Alt / F10 must not freeze the game in menu mode
        break;

    case WM_MOUSEMOVE:
        onMouseMove(w, lParam);
        return 0;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        onMouseButton(w, 0, message == WM_LBUTTONDOWN, lParam);
        return 0;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        onMouseButton(w, 1, message == WM_RBUTTONDOWN, lParam);
        return 0;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        onMouseButton(w, 2, message == WM_MBUTTONDOWN, lParam);
        return 0;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
        onMouseButton(w, GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? 3 : 4, message == WM_XBUTTONDOWN, lParam);
        return TRUE;

    case WM_MOUSEWHEEL:
        post(MouseWheelMessage { GET_WHEEL_DELTA_WPARAM(wParam), Sys::nanoTime() });
        return 0;

    case WM_INPUT:
        onRawInput(w, lParam);
        break; // DefWindowProc releases the raw input buffer

    case WM_SETCURSOR:
        if (w.grabbed && w.active && LOWORD(lParam) == HTCLIENT) {
            SetCursor(nullptr);
            return TRUE;
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        ValidateRect(hwnd, nullptr);
        return 0;

    case WM_APP_SET_TITLE:
        SetWindowTextW(hwnd, reinterpret_cast<const wchar_t*>(lParam));
        return 0;

    case WM_APP_SET_FULLSCREEN:
        applyFullscreen(w, wParam != 0);
        return 0;

    case WM_APP_SET_GRABBED:
        setGrabbed(w, wParam != 0);
        return 0;

    case WM_APP_SET_CURSOR_POS:
        setCursorPosition(w, static_cast<int>(static_cast<intptr_t>(wParam)), static_cast<int>(lParam));
        return 0;

    case WM_APP_SHOW:
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
        return 0;

    case WM_APP_DESTROY:
        DestroyWindow(hwnd);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

struct CreateParameters {
    std::wstring title;
    DisplayMode mode;
    bool resizable;
};

void windowThreadMain(CreateParameters parameters, std::promise<HWND> created)
{
    Log::setThreadName("Window thread");
    WindowThreadState& w = state().window;

    try {
        const HINSTANCE instance = GetModuleHandleW(nullptr);

        WNDCLASSEXW windowClass { sizeof(WNDCLASSEXW) };
        windowClass.lpfnWndProc = windowProc;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        windowClass.lpszClassName = kWindowClassName;
        if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw WinError("RegisterClassExW", HRESULT_FROM_WIN32(GetLastError()));

        const DWORD style = parameters.resizable ? WS_OVERLAPPEDWINDOW : (WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
        const HWND hwnd = CreateWindowExW(0, kWindowClassName, parameters.title.c_str(), style,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, instance, nullptr);
        if (!hwnd)
            throw WinError("CreateWindowExW", HRESULT_FROM_WIN32(GetLastError()));
        w.hwnd = hwnd;

        // LWJGL 2 was DPI-unaware, so Windows scaled its 854x480 window up on high-DPI screens. Scale the
        // requested size the same way, but render at native resolution (GUI scale "auto" picks it up).
        const UINT dpi = GetDpiForWindow(hwnd);
        RECT rect { 0, 0, MulDiv(parameters.mode.width, static_cast<int>(dpi), 96), MulDiv(parameters.mode.height, static_cast<int>(dpi), 96) };
        AdjustWindowRectExForDpi(&rect, style, FALSE, 0, dpi);

        MONITORINFO monitor { sizeof(MONITORINFO) };
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &monitor);
        const RECT& work = monitor.rcWork;
        const int windowWidth = rect.right - rect.left;
        const int windowHeight = rect.bottom - rect.top;
        const int x = work.left + std::max(0, static_cast<int>(work.right - work.left - windowWidth) / 2);
        const int y = work.top + std::max(0, static_cast<int>(work.bottom - work.top - windowHeight) / 2);
        SetWindowPos(hwnd, nullptr, x, y, windowWidth, windowHeight, SWP_NOZORDER | SWP_NOACTIVATE);

        const RAWINPUTDEVICE rawMouse { 0x01 /* generic desktop */, 0x02 /* mouse */, 0, hwnd };
        if (!RegisterRawInputDevices(&rawMouse, 1, sizeof(rawMouse)))
            Log::warn("Couldn't register raw mouse input: {}", hresultToString(HRESULT_FROM_WIN32(GetLastError())));

        POINT cursor;
        GetCursorPos(&cursor);
        ScreenToClient(hwnd, &cursor);
        w.cursor = cursor;

        created.set_value(hwnd);
    } catch (...) {
        created.set_exception(std::current_exception());
        return;
    }

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

} // namespace

void Display::setTitle(std::string_view title)
{
    DisplayState& s = state();
    s.title = toWide(title);
    if (s.created)
        SendMessageW(s.hwnd, WM_APP_SET_TITLE, 0, reinterpret_cast<LPARAM>(s.title.c_str()));
}

void Display::setDisplayMode(const DisplayMode& mode)
{
    // Applied by create(); the windowed size afterwards belongs to the user.
    state().mode = mode;
}

DisplayMode Display::getDisplayMode()
{
    const DisplayState& s = state();
    return s.fullscreen ? getDesktopDisplayMode() : s.mode;
}

DisplayMode Display::getDesktopDisplayMode()
{
    const DisplayState& s = state();
    const HMONITOR handle = s.hwnd ? MonitorFromWindow(s.hwnd, MONITOR_DEFAULTTONEAREST)
                                   : MonitorFromPoint(POINT { 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEXW monitor {};
    monitor.cbSize = sizeof(monitor);
    GetMonitorInfoW(handle, &monitor);
    DEVMODEW devMode { .dmSize = sizeof(DEVMODEW) };
    const int frequency = EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &devMode) ? static_cast<int>(devMode.dmDisplayFrequency) : 0;
    return DisplayMode { monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top, frequency };
}

void Display::setFullscreen(bool fullscreen)
{
    DisplayState& s = state();
    if (fullscreen == s.fullscreen)
        return;

    s.fullscreen = fullscreen;
    if (s.created) {
        SendMessageW(s.hwnd, WM_APP_SET_FULLSCREEN, fullscreen ? TRUE : FALSE, 0);
        processMessages(); // pick up the new size right away, like LWJGL's synchronous mode switch
    }
}

bool Display::isFullscreen()
{
    return state().fullscreen;
}

void Display::setResizable(bool resizable)
{
    state().resizable = resizable;
}

void Display::setVSyncEnabled(bool sync)
{
    state().vsync = sync;
}

void Display::setIcon(std::vector<DisplayIcon> icons)
{
    DisplayState& s = state();
    s.icons = std::move(icons);
    if (s.created)
        applyIcons(s);
}

void Display::create()
{
    DisplayState& s = state();
    if (s.created)
        throw std::logic_error("Display already created");

    std::promise<HWND> createdPromise;
    std::future<HWND> createdFuture = createdPromise.get_future();
    s.thread = std::thread(windowThreadMain, CreateParameters { s.title, s.mode, s.resizable }, std::move(createdPromise));
    try {
        s.hwnd = createdFuture.get();
    } catch (...) {
        s.thread.join();
        throw;
    }

    RECT client;
    GetClientRect(s.hwnd, &client);
    s.width = std::max(1, static_cast<int>(client.right));
    s.height = std::max(1, static_cast<int>(client.bottom));

    try {
        s.device = std::make_unique<RenderDevice>(s.hwnd, s.width, s.height);
    } catch (...) {
        SendMessageW(s.hwnd, WM_APP_DESTROY, 0, 0);
        s.thread.join();
        s.hwnd = nullptr;
        throw;
    }

    {
        // Size changes from window setup are already reflected in width/height.
        std::lock_guard lock(s.inboxMutex);
        s.inbox.clear();
    }

    s.created = true;
    s.closeRequested = false;
    if (!s.icons.empty())
        applyIcons(s);
    if (s.fullscreen)
        SendMessageW(s.hwnd, WM_APP_SET_FULLSCREEN, TRUE, 0);
    SendMessageW(s.hwnd, WM_APP_SHOW, 0, 0);
    processMessages();
    s.resized = false;
}

void Display::destroy()
{
    DisplayState& s = state();
    if (!s.created)
        return;

    s.device.reset();
    SendMessageW(s.hwnd, WM_APP_DESTROY, 0, 0);
    s.thread.join();

    s.hwnd = nullptr;
    s.created = false;
    s.window = WindowThreadState {};
    for (HICON* icon : { &s.smallIcon, &s.bigIcon }) {
        if (*icon) {
            DestroyIcon(*icon);
            *icon = nullptr;
        }
    }
    {
        std::lock_guard lock(s.inboxMutex);
        s.inbox.clear();
    }
    if (s.syncTimer) {
        CloseHandle(s.syncTimer);
        s.syncTimer = nullptr;
    }
    Keyboard::reset();
    Mouse::reset();
}

bool Display::isCreated()
{
    return state().created;
}

bool Display::isCloseRequested()
{
    return state().closeRequested;
}

bool Display::isActive()
{
    return state().active;
}

bool Display::wasResized()
{
    return state().resized;
}

int Display::getWidth()
{
    return state().width;
}

int Display::getHeight()
{
    return state().height;
}

void Display::update()
{
    DisplayState& s = state();
    s.resized = false;
    s.device->present(s.vsync);
    processMessages();
}

void Display::processMessages()
{
    DisplayState& s = state();

    std::vector<WindowMessage> messages;
    {
        std::lock_guard lock(s.inboxMutex);
        messages.swap(s.inbox);
    }

    bool sizeChanged = false;
    for (const WindowMessage& message : messages) {
        std::visit(Overloaded {
                       [](const KeyMessage& m) {
                           Keyboard::addEvent(m.key, m.character, m.state, m.repeat, m.nanos);
                       },
                       [&](const MouseButtonMessage& m) {
                           Mouse::addButtonEvent(m.button, m.state, m.x, s.height - 1 - m.y, m.nanos);
                       },
                       [&](const MouseMoveMessage& m) {
                           Mouse::addMoveEvent(m.dx, m.dy, m.x, s.height - 1 - m.y, m.nanos);
                       },
                       [](const MouseWheelMessage& m) {
                           Mouse::addWheelEvent(m.delta, m.nanos);
                       },
                       [&](const ResizeMessage& m) {
                           const int width = std::max(1, m.width);
                           const int height = std::max(1, m.height);
                           if (width != s.width || height != s.height) {
                               s.width = width;
                               s.height = height;
                               sizeChanged = true;
                           }
                       },
                       [&](const CloseMessage&) {
                           s.closeRequested = true;
                       },
                       [&](const FocusMessage& m) {
                           s.active = m.active;
                       },
                   },
            message);
    }

    // A live drag produces many sizes per frame; only the last one matters for the swapchain.
    if (sizeChanged) {
        s.resized = true;
        if (s.device)
            s.device->resize(s.width, s.height);
    }
}

void Display::sync(int fps)
{
    if (fps <= 0)
        return;

    DisplayState& s = state();
    if (!s.syncTimer)
        s.syncTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

    const int64_t frameNanos = 1'000'000'000LL / fps;
    int64_t now = Sys::nanoTime();
    if (s.nextFrameNanos == 0)
        s.nextFrameNanos = now;

    // Sleep on a high-resolution timer for the bulk of the wait, then yield for the last ~1 ms.
    while (now < s.nextFrameNanos) {
        const int64_t remaining = s.nextFrameNanos - now;
        if (s.syncTimer && remaining > 2'000'000) {
            LARGE_INTEGER due;
            due.QuadPart = -((remaining - 1'000'000) / 100);
            if (SetWaitableTimerEx(s.syncTimer, &due, 0, nullptr, nullptr, nullptr, 0))
                WaitForSingleObject(s.syncTimer, INFINITE);
        } else {
            SwitchToThread();
        }
        now = Sys::nanoTime();
    }

    // Keep a steady cadence, but don't try to catch up after a slow frame.
    s.nextFrameNanos = std::max(s.nextFrameNanos + frameNanos, now);
}

RenderDevice& Display::getDevice()
{
    DisplayState& s = state();
    if (!s.device)
        throw std::logic_error("Display not created");
    return *s.device;
}

HWND Display::getWindowHandle()
{
    return state().hwnd;
}

void Display::setGrabbedInternal(bool grabbed)
{
    const DisplayState& s = state();
    if (s.created)
        SendMessageW(s.hwnd, WM_APP_SET_GRABBED, grabbed ? TRUE : FALSE, 0);
}

void Display::setCursorPositionInternal(int x, int y)
{
    const DisplayState& s = state();
    if (s.created)
        SendMessageW(s.hwnd, WM_APP_SET_CURSOR_POS, static_cast<WPARAM>(static_cast<intptr_t>(x)), static_cast<LPARAM>(y));
}

} // namespace mc
