#include "platform/Keyboard.h"

#include <array>
#include <deque>

namespace mc {

namespace {

struct KeyEvent {
    int key = Keyboard::KEY_NONE;
    char16_t character = Keyboard::CHAR_NONE;
    bool state = false;
    bool repeat = false;
    int64_t nanos = 0;
};

std::deque<KeyEvent> eventQueue;
KeyEvent currentEvent;
std::array<bool, Keyboard::KEYBOARD_SIZE> keyDownBuffer{};
bool repeatEventsEnabled = false;

const std::array<std::string_view, Keyboard::KEYBOARD_SIZE>& keyNames()
{
    static const auto table = [] {
        std::array<std::string_view, Keyboard::KEYBOARD_SIZE> names{};
        const auto set = [&](int key, std::string_view name) { names[static_cast<size_t>(key)] = name; };
        set(Keyboard::KEY_NONE, "NONE");
        set(Keyboard::KEY_ESCAPE, "ESCAPE");
        set(Keyboard::KEY_1, "1");
        set(Keyboard::KEY_2, "2");
        set(Keyboard::KEY_3, "3");
        set(Keyboard::KEY_4, "4");
        set(Keyboard::KEY_5, "5");
        set(Keyboard::KEY_6, "6");
        set(Keyboard::KEY_7, "7");
        set(Keyboard::KEY_8, "8");
        set(Keyboard::KEY_9, "9");
        set(Keyboard::KEY_0, "0");
        set(Keyboard::KEY_MINUS, "MINUS");
        set(Keyboard::KEY_EQUALS, "EQUALS");
        set(Keyboard::KEY_BACK, "BACK");
        set(Keyboard::KEY_TAB, "TAB");
        set(Keyboard::KEY_Q, "Q");
        set(Keyboard::KEY_W, "W");
        set(Keyboard::KEY_E, "E");
        set(Keyboard::KEY_R, "R");
        set(Keyboard::KEY_T, "T");
        set(Keyboard::KEY_Y, "Y");
        set(Keyboard::KEY_U, "U");
        set(Keyboard::KEY_I, "I");
        set(Keyboard::KEY_O, "O");
        set(Keyboard::KEY_P, "P");
        set(Keyboard::KEY_LBRACKET, "LBRACKET");
        set(Keyboard::KEY_RBRACKET, "RBRACKET");
        set(Keyboard::KEY_RETURN, "RETURN");
        set(Keyboard::KEY_LCONTROL, "LCONTROL");
        set(Keyboard::KEY_A, "A");
        set(Keyboard::KEY_S, "S");
        set(Keyboard::KEY_D, "D");
        set(Keyboard::KEY_F, "F");
        set(Keyboard::KEY_G, "G");
        set(Keyboard::KEY_H, "H");
        set(Keyboard::KEY_J, "J");
        set(Keyboard::KEY_K, "K");
        set(Keyboard::KEY_L, "L");
        set(Keyboard::KEY_SEMICOLON, "SEMICOLON");
        set(Keyboard::KEY_APOSTROPHE, "APOSTROPHE");
        set(Keyboard::KEY_GRAVE, "GRAVE");
        set(Keyboard::KEY_LSHIFT, "LSHIFT");
        set(Keyboard::KEY_BACKSLASH, "BACKSLASH");
        set(Keyboard::KEY_Z, "Z");
        set(Keyboard::KEY_X, "X");
        set(Keyboard::KEY_C, "C");
        set(Keyboard::KEY_V, "V");
        set(Keyboard::KEY_B, "B");
        set(Keyboard::KEY_N, "N");
        set(Keyboard::KEY_M, "M");
        set(Keyboard::KEY_COMMA, "COMMA");
        set(Keyboard::KEY_PERIOD, "PERIOD");
        set(Keyboard::KEY_SLASH, "SLASH");
        set(Keyboard::KEY_RSHIFT, "RSHIFT");
        set(Keyboard::KEY_MULTIPLY, "MULTIPLY");
        set(Keyboard::KEY_LMENU, "LMENU");
        set(Keyboard::KEY_SPACE, "SPACE");
        set(Keyboard::KEY_CAPITAL, "CAPITAL");
        set(Keyboard::KEY_F1, "F1");
        set(Keyboard::KEY_F2, "F2");
        set(Keyboard::KEY_F3, "F3");
        set(Keyboard::KEY_F4, "F4");
        set(Keyboard::KEY_F5, "F5");
        set(Keyboard::KEY_F6, "F6");
        set(Keyboard::KEY_F7, "F7");
        set(Keyboard::KEY_F8, "F8");
        set(Keyboard::KEY_F9, "F9");
        set(Keyboard::KEY_F10, "F10");
        set(Keyboard::KEY_NUMLOCK, "NUMLOCK");
        set(Keyboard::KEY_SCROLL, "SCROLL");
        set(Keyboard::KEY_NUMPAD7, "NUMPAD7");
        set(Keyboard::KEY_NUMPAD8, "NUMPAD8");
        set(Keyboard::KEY_NUMPAD9, "NUMPAD9");
        set(Keyboard::KEY_SUBTRACT, "SUBTRACT");
        set(Keyboard::KEY_NUMPAD4, "NUMPAD4");
        set(Keyboard::KEY_NUMPAD5, "NUMPAD5");
        set(Keyboard::KEY_NUMPAD6, "NUMPAD6");
        set(Keyboard::KEY_ADD, "ADD");
        set(Keyboard::KEY_NUMPAD1, "NUMPAD1");
        set(Keyboard::KEY_NUMPAD2, "NUMPAD2");
        set(Keyboard::KEY_NUMPAD3, "NUMPAD3");
        set(Keyboard::KEY_NUMPAD0, "NUMPAD0");
        set(Keyboard::KEY_DECIMAL, "DECIMAL");
        set(Keyboard::KEY_F11, "F11");
        set(Keyboard::KEY_F12, "F12");
        set(Keyboard::KEY_F13, "F13");
        set(Keyboard::KEY_F14, "F14");
        set(Keyboard::KEY_F15, "F15");
        set(Keyboard::KEY_F16, "F16");
        set(Keyboard::KEY_F17, "F17");
        set(Keyboard::KEY_F18, "F18");
        set(Keyboard::KEY_KANA, "KANA");
        set(Keyboard::KEY_F19, "F19");
        set(Keyboard::KEY_CONVERT, "CONVERT");
        set(Keyboard::KEY_NOCONVERT, "NOCONVERT");
        set(Keyboard::KEY_YEN, "YEN");
        set(Keyboard::KEY_NUMPADEQUALS, "NUMPADEQUALS");
        set(Keyboard::KEY_CIRCUMFLEX, "CIRCUMFLEX");
        set(Keyboard::KEY_AT, "AT");
        set(Keyboard::KEY_COLON, "COLON");
        set(Keyboard::KEY_UNDERLINE, "UNDERLINE");
        set(Keyboard::KEY_KANJI, "KANJI");
        set(Keyboard::KEY_STOP, "STOP");
        set(Keyboard::KEY_AX, "AX");
        set(Keyboard::KEY_UNLABELED, "UNLABELED");
        set(Keyboard::KEY_NUMPADENTER, "NUMPADENTER");
        set(Keyboard::KEY_RCONTROL, "RCONTROL");
        set(Keyboard::KEY_SECTION, "SECTION");
        set(Keyboard::KEY_NUMPADCOMMA, "NUMPADCOMMA");
        set(Keyboard::KEY_DIVIDE, "DIVIDE");
        set(Keyboard::KEY_SYSRQ, "SYSRQ");
        set(Keyboard::KEY_RMENU, "RMENU");
        set(Keyboard::KEY_FUNCTION, "FUNCTION");
        set(Keyboard::KEY_PAUSE, "PAUSE");
        set(Keyboard::KEY_HOME, "HOME");
        set(Keyboard::KEY_UP, "UP");
        set(Keyboard::KEY_PRIOR, "PRIOR");
        set(Keyboard::KEY_LEFT, "LEFT");
        set(Keyboard::KEY_RIGHT, "RIGHT");
        set(Keyboard::KEY_END, "END");
        set(Keyboard::KEY_DOWN, "DOWN");
        set(Keyboard::KEY_NEXT, "NEXT");
        set(Keyboard::KEY_INSERT, "INSERT");
        set(Keyboard::KEY_DELETE, "DELETE");
        set(Keyboard::KEY_CLEAR, "CLEAR");
        set(Keyboard::KEY_LMETA, "LMETA");
        set(Keyboard::KEY_RMETA, "RMETA");
        set(Keyboard::KEY_APPS, "APPS");
        set(Keyboard::KEY_POWER, "POWER");
        set(Keyboard::KEY_SLEEP, "SLEEP");
        return names;
    }();
    return table;
}

} // namespace

bool Keyboard::next()
{
    if (eventQueue.empty())
        return false;

    currentEvent = eventQueue.front();
    eventQueue.pop_front();
    return true;
}

int Keyboard::getEventKey()
{
    return currentEvent.key;
}

char16_t Keyboard::getEventCharacter()
{
    return currentEvent.character;
}

bool Keyboard::getEventKeyState()
{
    return currentEvent.state;
}

int64_t Keyboard::getEventNanoseconds()
{
    return currentEvent.nanos;
}

bool Keyboard::isRepeatEvent()
{
    return currentEvent.repeat;
}

int Keyboard::getNumKeyboardEvents()
{
    return static_cast<int>(eventQueue.size());
}

bool Keyboard::isKeyDown(int key)
{
    return key >= 0 && key < KEYBOARD_SIZE && keyDownBuffer[static_cast<size_t>(key)];
}

void Keyboard::enableRepeatEvents(bool enable)
{
    repeatEventsEnabled = enable;
}

bool Keyboard::areRepeatEventsEnabled()
{
    return repeatEventsEnabled;
}

std::string_view Keyboard::getKeyName(int key)
{
    if (key < 0 || key >= KEYBOARD_SIZE)
        return {};
    return keyNames()[static_cast<size_t>(key)];
}

int Keyboard::getKeyIndex(std::string_view name)
{
    const auto& names = keyNames();
    for (size_t i = 0; i < names.size(); ++i) {
        if (!names[i].empty() && names[i] == name)
            return static_cast<int>(i);
    }
    return KEY_NONE;
}

void Keyboard::addEvent(int key, char16_t character, bool state, bool repeat, int64_t nanos)
{
    if (key > KEY_NONE && key < KEYBOARD_SIZE)
        keyDownBuffer[static_cast<size_t>(key)] = state;

    if (repeat && !repeatEventsEnabled)
        return;

    eventQueue.push_back(KeyEvent { key, character, state, repeat, nanos });
}

void Keyboard::reset()
{
    eventQueue.clear();
    currentEvent = KeyEvent {};
    keyDownBuffer.fill(false);
}

} // namespace mc
