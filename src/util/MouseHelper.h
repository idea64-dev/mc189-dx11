#pragma once

// Original: net/minecraft/util/MouseHelper.java

#include "platform/Display.h"
#include "platform/Mouse.h"

namespace mc {

class MouseHelper {
public:
    void grabMouseCursor()
    {
        Mouse::setGrabbed(true);
        deltaX = 0;
        deltaY = 0;
    }

    void ungrabMouseCursor()
    {
        Mouse::setCursorPosition(Display::getWidth() / 2, Display::getHeight() / 2);
        Mouse::setGrabbed(false);
    }

    void mouseXYChange()
    {
        deltaX = Mouse::getDX();
        deltaY = Mouse::getDY();
    }

    int deltaX = 0;
    int deltaY = 0;
};

} // namespace mc
