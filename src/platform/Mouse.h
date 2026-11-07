#pragma once

#include <cstdint>

namespace mc {

// Replacement for org.lwjgl.input.Mouse (LWJGL 2).
// Coordinates have the LWJGL origin: (0, 0) is the bottom-left pixel of the window, Y grows upwards.
// Buttons: 0 left, 1 right, 2 middle, 3/4 side buttons. Wheel deltas are in WHEEL_DELTA units (120 per notch).
class Mouse {
public:
    static constexpr int BUTTON_COUNT = 5;

    static bool next();
    // Button of the current event, or -1 for move/wheel events.
    static int getEventButton();
    static bool getEventButtonState();
    static int getEventDX();
    static int getEventDY();
    static int getEventX();
    static int getEventY();
    static int getEventDWheel();
    static int64_t getEventNanoseconds();

    static int getX();
    static int getY();
    // Movement since the previous call (reset on read).
    static int getDX();
    static int getDY();
    static int getDWheel();

    static bool isButtonDown(int button);
    static int getButtonCount() { return BUTTON_COUNT; }

    // Hides the cursor, locks it to the window and reports relative motion (Raw Input).
    static void setGrabbed(bool grab);
    static bool isGrabbed();
    static void setCursorPosition(int x, int y);

private:
    friend class Display;
    static void addButtonEvent(int button, bool state, int x, int y, int64_t nanos);
    static void addMoveEvent(int dx, int dy, int x, int y, int64_t nanos);
    static void addWheelEvent(int delta, int64_t nanos);
    static void reset();
};

} // namespace mc
