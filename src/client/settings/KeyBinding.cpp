#include "client/settings/KeyBinding.h"

#include "client/resources/Locale.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace mc {

namespace {

struct Registry {
    std::vector<KeyBinding*> keybindArray;
    std::unordered_map<int, KeyBinding*> hash; // Java IntHashMap: the last binding added for a code wins
    std::set<std::string> keybindSet;
};

Registry& registry()
{
    static Registry instance;
    return instance;
}

// String.compareTo on UTF-16 code units; UTF-8 byte order agrees except across the surrogate range, which
// translated key names never reach.
int compareStrings(const std::string& a, const std::string& b)
{
    const int c = a.compare(b);
    return c < 0 ? -1 : (c > 0 ? 1 : 0);
}

} // namespace

KeyBinding::KeyBinding(std::string description, int code, std::string category)
    : keyDescription(std::move(description))
    , keyCodeDefault(code)
    , keyCategory(std::move(category))
    , keyCode(code)
{
    Registry& r = registry();
    r.keybindArray.push_back(this);
    r.hash[code] = this;
    r.keybindSet.insert(keyCategory);
}

KeyBinding::~KeyBinding()
{
    Registry& r = registry();
    r.keybindArray.erase(std::remove(r.keybindArray.begin(), r.keybindArray.end(), this), r.keybindArray.end());
    for (auto it = r.hash.begin(); it != r.hash.end();) {
        if (it->second == this)
            it = r.hash.erase(it);
        else
            ++it;
    }
}

void KeyBinding::onTick(int code)
{
    if (code != 0) {
        const auto it = registry().hash.find(code);
        if (it != registry().hash.end())
            ++it->second->pressTime;
    }
}

void KeyBinding::setKeyBindState(int code, bool isPressed)
{
    if (code != 0) {
        const auto it = registry().hash.find(code);
        if (it != registry().hash.end())
            it->second->pressed = isPressed;
    }
}

void KeyBinding::unPressAllKeys()
{
    for (KeyBinding* binding : registry().keybindArray)
        binding->unpressKey();
}

void KeyBinding::resetKeyBindingArrayAndHash()
{
    Registry& r = registry();
    r.hash.clear();
    for (KeyBinding* binding : r.keybindArray)
        r.hash[binding->keyCode] = binding;
}

const std::set<std::string>& KeyBinding::getKeybinds()
{
    return registry().keybindSet;
}

bool KeyBinding::isPressed()
{
    if (pressTime == 0)
        return false;
    --pressTime;
    return true;
}

void KeyBinding::unpressKey()
{
    pressTime = 0;
    pressed = false;
}

int KeyBinding::compareTo(const KeyBinding& other) const
{
    int i = compareStrings(I18n::format(keyCategory), I18n::format(other.keyCategory));
    if (i == 0)
        i = compareStrings(I18n::format(keyDescription), I18n::format(other.keyDescription));
    return i;
}

} // namespace mc
