#include "platform/Mouse.h"

#include "platform/Display.h"

#include <array>
#include <deque>

namespace mc {

namespace {

struct MouseEvent {
    int button = -1;
    bool state = false;
    int dx = 0;
    int dy = 0;
    int dwheel = 0;
    int x = 0;
    int y = 0;
    int64_t nanos = 0;
};

std::deque<MouseEvent> eventQueue;
MouseEvent currentEvent;
std::array<bool, Mouse::BUTTON_COUNT> buttonDown{};
int mouseX = 0;
int mouseY = 0;
int accumDX = 0;
int accumDY = 0;
int accumDWheel = 0;
bool grabbed = false;

} // namespace

bool Mouse::next()
{
    if (eventQueue.empty())
        return false;

    currentEvent = eventQueue.front();
    eventQueue.pop_front();
    return true;
}

int Mouse::getEventButton()
{
    return currentEvent.button;
}

bool Mouse::getEventButtonState()
{
    return currentEvent.state;
}

int Mouse::getEventDX()
{
    return currentEvent.dx;
}

int Mouse::getEventDY()
{
    return currentEvent.dy;
}

int Mouse::getEventX()
{
    return currentEvent.x;
}

int Mouse::getEventY()
{
    return currentEvent.y;
}

int Mouse::getEventDWheel()
{
    return currentEvent.dwheel;
}

int64_t Mouse::getEventNanoseconds()
{
    return currentEvent.nanos;
}

int Mouse::getX()
{
    return mouseX;
}

int Mouse::getY()
{
    return mouseY;
}

int Mouse::getDX()
{
    const int result = accumDX;
    accumDX = 0;
    return result;
}

int Mouse::getDY()
{
    const int result = accumDY;
    accumDY = 0;
    return result;
}

int Mouse::getDWheel()
{
    const int result = accumDWheel;
    accumDWheel = 0;
    return result;
}

bool Mouse::isButtonDown(int button)
{
    return button >= 0 && button < BUTTON_COUNT && buttonDown[static_cast<size_t>(button)];
}

void Mouse::setGrabbed(bool grab)
{
    if (grab == grabbed)
        return;

    grabbed = grab;
    accumDX = 0;
    accumDY = 0;
    Display::setGrabbedInternal(grab);
}

bool Mouse::isGrabbed()
{
    return grabbed;
}

void Mouse::setCursorPosition(int x, int y)
{
    mouseX = x;
    mouseY = y;
    Display::setCursorPositionInternal(x, Display::getHeight() - 1 - y);
}

void Mouse::addButtonEvent(int button, bool state, int x, int y, int64_t nanos)
{
    if (button >= 0 && button < BUTTON_COUNT)
        buttonDown[static_cast<size_t>(button)] = state;

    mouseX = x;
    mouseY = y;
    eventQueue.push_back(MouseEvent { button, state, 0, 0, 0, x, y, nanos });
}

void Mouse::addMoveEvent(int dx, int dy, int x, int y, int64_t nanos)
{
    accumDX += dx;
    accumDY += dy;
    mouseX = x;
    mouseY = y;
    eventQueue.push_back(MouseEvent { -1, false, dx, dy, 0, x, y, nanos });
}

void Mouse::addWheelEvent(int delta, int64_t nanos)
{
    accumDWheel += delta;
    eventQueue.push_back(MouseEvent { -1, false, 0, 0, delta, mouseX, mouseY, nanos });
}

void Mouse::reset()
{
    eventQueue.clear();
    currentEvent = MouseEvent {};
    buttonDown.fill(false);
    accumDX = 0;
    accumDY = 0;
    accumDWheel = 0;
    grabbed = false;
}

} // namespace mc
