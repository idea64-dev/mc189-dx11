#pragma once

// Original: net/minecraft/client/settings/KeyBinding.java
// Bindings register themselves in a global list and code->binding map, as in Java, so they must not move:
// GameSettings owns them and is neither copyable nor movable.

#include <set>
#include <string>

namespace mc {

class KeyBinding {
public:
    KeyBinding(std::string description, int keyCode, std::string category);
    ~KeyBinding();

    KeyBinding(const KeyBinding&) = delete;
    KeyBinding& operator=(const KeyBinding&) = delete;

    static void onTick(int keyCode);
    static void setKeyBindState(int keyCode, bool pressed);
    static void unPressAllKeys();
    static void resetKeyBindingArrayAndHash();
    static const std::set<std::string>& getKeybinds();

    bool isKeyDown() const { return pressed; }
    const std::string& getKeyCategory() const { return keyCategory; }
    // Consumes one press.
    bool isPressed();
    const std::string& getKeyDescription() const { return keyDescription; }
    int getKeyCodeDefault() const { return keyCodeDefault; }
    int getKeyCode() const { return keyCode; }
    void setKeyCode(int code) { keyCode = code; }
    // Orders by translated category, then translated description (the controls screen).
    int compareTo(const KeyBinding& other) const;

private:
    void unpressKey();

    std::string keyDescription;
    int keyCodeDefault;
    std::string keyCategory;
    int keyCode;
    bool pressed = false;
    int pressTime = 0;
};

} // namespace mc
