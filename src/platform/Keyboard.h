#pragma once

#include <cstdint>
#include <string_view>

namespace mc {

// Replacement for org.lwjgl.input.Keyboard (LWJGL 2).
// Key codes are the LWJGL ones (DirectInput scan codes), because GameSettings stores them in options.txt
// and the game logic compares against them directly (e.g. 87 = F11).
// Events are buffered by Display::processMessages() and read with next(), as in LWJGL.
class Keyboard {
public:
    static constexpr int KEYBOARD_SIZE = 256;
    static constexpr char16_t CHAR_NONE = u'\0';

    static constexpr int KEY_NONE = 0x00;
    static constexpr int KEY_ESCAPE = 0x01;
    static constexpr int KEY_1 = 0x02;
    static constexpr int KEY_2 = 0x03;
    static constexpr int KEY_3 = 0x04;
    static constexpr int KEY_4 = 0x05;
    static constexpr int KEY_5 = 0x06;
    static constexpr int KEY_6 = 0x07;
    static constexpr int KEY_7 = 0x08;
    static constexpr int KEY_8 = 0x09;
    static constexpr int KEY_9 = 0x0A;
    static constexpr int KEY_0 = 0x0B;
    static constexpr int KEY_MINUS = 0x0C;
    static constexpr int KEY_EQUALS = 0x0D;
    static constexpr int KEY_BACK = 0x0E;
    static constexpr int KEY_TAB = 0x0F;
    static constexpr int KEY_Q = 0x10;
    static constexpr int KEY_W = 0x11;
    static constexpr int KEY_E = 0x12;
    static constexpr int KEY_R = 0x13;
    static constexpr int KEY_T = 0x14;
    static constexpr int KEY_Y = 0x15;
    static constexpr int KEY_U = 0x16;
    static constexpr int KEY_I = 0x17;
    static constexpr int KEY_O = 0x18;
    static constexpr int KEY_P = 0x19;
    static constexpr int KEY_LBRACKET = 0x1A;
    static constexpr int KEY_RBRACKET = 0x1B;
    static constexpr int KEY_RETURN = 0x1C;
    static constexpr int KEY_LCONTROL = 0x1D;
    static constexpr int KEY_A = 0x1E;
    static constexpr int KEY_S = 0x1F;
    static constexpr int KEY_D = 0x20;
    static constexpr int KEY_F = 0x21;
    static constexpr int KEY_G = 0x22;
    static constexpr int KEY_H = 0x23;
    static constexpr int KEY_J = 0x24;
    static constexpr int KEY_K = 0x25;
    static constexpr int KEY_L = 0x26;
    static constexpr int KEY_SEMICOLON = 0x27;
    static constexpr int KEY_APOSTROPHE = 0x28;
    static constexpr int KEY_GRAVE = 0x29;
    static constexpr int KEY_LSHIFT = 0x2A;
    static constexpr int KEY_BACKSLASH = 0x2B;
    static constexpr int KEY_Z = 0x2C;
    static constexpr int KEY_X = 0x2D;
    static constexpr int KEY_C = 0x2E;
    static constexpr int KEY_V = 0x2F;
    static constexpr int KEY_B = 0x30;
    static constexpr int KEY_N = 0x31;
    static constexpr int KEY_M = 0x32;
    static constexpr int KEY_COMMA = 0x33;
    static constexpr int KEY_PERIOD = 0x34;
    static constexpr int KEY_SLASH = 0x35;
    static constexpr int KEY_RSHIFT = 0x36;
    static constexpr int KEY_MULTIPLY = 0x37;
    static constexpr int KEY_LMENU = 0x38;
    static constexpr int KEY_SPACE = 0x39;
    static constexpr int KEY_CAPITAL = 0x3A;
    static constexpr int KEY_F1 = 0x3B;
    static constexpr int KEY_F2 = 0x3C;
    static constexpr int KEY_F3 = 0x3D;
    static constexpr int KEY_F4 = 0x3E;
    static constexpr int KEY_F5 = 0x3F;
    static constexpr int KEY_F6 = 0x40;
    static constexpr int KEY_F7 = 0x41;
    static constexpr int KEY_F8 = 0x42;
    static constexpr int KEY_F9 = 0x43;
    static constexpr int KEY_F10 = 0x44;
    static constexpr int KEY_NUMLOCK = 0x45;
    static constexpr int KEY_SCROLL = 0x46;
    static constexpr int KEY_NUMPAD7 = 0x47;
    static constexpr int KEY_NUMPAD8 = 0x48;
    static constexpr int KEY_NUMPAD9 = 0x49;
    static constexpr int KEY_SUBTRACT = 0x4A;
    static constexpr int KEY_NUMPAD4 = 0x4B;
    static constexpr int KEY_NUMPAD5 = 0x4C;
    static constexpr int KEY_NUMPAD6 = 0x4D;
    static constexpr int KEY_ADD = 0x4E;
    static constexpr int KEY_NUMPAD1 = 0x4F;
    static constexpr int KEY_NUMPAD2 = 0x50;
    static constexpr int KEY_NUMPAD3 = 0x51;
    static constexpr int KEY_NUMPAD0 = 0x52;
    static constexpr int KEY_DECIMAL = 0x53;
    static constexpr int KEY_F11 = 0x57;
    static constexpr int KEY_F12 = 0x58;
    static constexpr int KEY_F13 = 0x64;
    static constexpr int KEY_F14 = 0x65;
    static constexpr int KEY_F15 = 0x66;
    static constexpr int KEY_F16 = 0x67;
    static constexpr int KEY_F17 = 0x68;
    static constexpr int KEY_F18 = 0x69;
    static constexpr int KEY_KANA = 0x70;
    static constexpr int KEY_F19 = 0x71;
    static constexpr int KEY_CONVERT = 0x79;
    static constexpr int KEY_NOCONVERT = 0x7B;
    static constexpr int KEY_YEN = 0x7D;
    static constexpr int KEY_NUMPADEQUALS = 0x8D;
    static constexpr int KEY_CIRCUMFLEX = 0x90;
    static constexpr int KEY_AT = 0x91;
    static constexpr int KEY_COLON = 0x92;
    static constexpr int KEY_UNDERLINE = 0x93;
    static constexpr int KEY_KANJI = 0x94;
    static constexpr int KEY_STOP = 0x95;
    static constexpr int KEY_AX = 0x96;
    static constexpr int KEY_UNLABELED = 0x97;
    static constexpr int KEY_NUMPADENTER = 0x9C;
    static constexpr int KEY_RCONTROL = 0x9D;
    static constexpr int KEY_SECTION = 0xA7;
    static constexpr int KEY_NUMPADCOMMA = 0xB3;
    static constexpr int KEY_DIVIDE = 0xB5;
    static constexpr int KEY_SYSRQ = 0xB7;
    static constexpr int KEY_RMENU = 0xB8;
    static constexpr int KEY_FUNCTION = 0xC4;
    static constexpr int KEY_PAUSE = 0xC5;
    static constexpr int KEY_HOME = 0xC7;
    static constexpr int KEY_UP = 0xC8;
    static constexpr int KEY_PRIOR = 0xC9;
    static constexpr int KEY_LEFT = 0xCB;
    static constexpr int KEY_RIGHT = 0xCD;
    static constexpr int KEY_END = 0xCF;
    static constexpr int KEY_DOWN = 0xD0;
    static constexpr int KEY_NEXT = 0xD1;
    static constexpr int KEY_INSERT = 0xD2;
    static constexpr int KEY_DELETE = 0xD3;
    static constexpr int KEY_CLEAR = 0xDA;
    static constexpr int KEY_LMETA = 0xDB;
    static constexpr int KEY_RMETA = 0xDC;
    static constexpr int KEY_APPS = 0xDD;
    static constexpr int KEY_POWER = 0xDE;
    static constexpr int KEY_SLEEP = 0xDF;

    static bool next();
    static int getEventKey();
    static char16_t getEventCharacter();
    static bool getEventKeyState();
    static int64_t getEventNanoseconds();
    static bool isRepeatEvent();
    static int getNumKeyboardEvents();

    static bool isKeyDown(int key);
    static void enableRepeatEvents(bool enable);
    static bool areRepeatEventsEnabled();

    // LWJGL constant name without the KEY_ prefix ("F11", "LSHIFT"); empty for unknown codes.
    static std::string_view getKeyName(int key);
    // Inverse of getKeyName; KEY_NONE for unknown names.
    static int getKeyIndex(std::string_view name);

private:
    friend class Display;
    static void addEvent(int key, char16_t character, bool state, bool repeat, int64_t nanos);
    static void reset();
};

} // namespace mc
