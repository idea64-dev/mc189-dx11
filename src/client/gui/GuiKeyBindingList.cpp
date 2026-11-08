#include "client/gui/GuiKeyBindingList.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiControls.h"
#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"

#include <algorithm>

namespace mc {

GuiKeyBindingList::GuiKeyBindingList(GuiControls& controlsIn, Minecraft& mcIn)
    : GuiListExtended(mcIn, controlsIn.width, controlsIn.height, 63, controlsIn.height - 32, 20)
    , controls(controlsIn)
{
    std::vector<KeyBinding*> akeybinding = mcIn.gameSettings->keyBindings;
    // Java: Arrays.sort on Comparable objects, a stable merge sort.
    std::stable_sort(akeybinding.begin(), akeybinding.end(), [](const KeyBinding* a, const KeyBinding* b) { return a->compareTo(*b) < 0; });

    const std::string* s = nullptr;
    for (KeyBinding* keybinding : akeybinding) {
        const std::string& s1 = keybinding->getKeyCategory();
        if (s == nullptr || s1 != *s) {
            s = &s1;
            listEntries.push_back(std::make_unique<CategoryEntry>(*this, s1));
        }

        const int j = mcIn.fontRendererObj->getStringWidth(I18n::format(keybinding->getKeyDescription()));
        if (j > maxListLabelWidth)
            maxListLabelWidth = j;
        listEntries.push_back(std::make_unique<KeyEntry>(*this, *keybinding));
    }
}

GuiKeyBindingList::CategoryEntry::CategoryEntry(GuiKeyBindingList& listIn, const std::string& category)
    : list(listIn)
    , labelText(I18n::format(category))
    , labelWidth(listIn.mc.fontRendererObj->getStringWidth(labelText))
{
}

void GuiKeyBindingList::CategoryEntry::drawEntry(int /*slotIndex*/, int /*x*/, int y, int /*listWidth*/, int entryHeight, int /*mouseX*/, int /*mouseY*/, bool /*isSelected*/)
{
    FontRenderer& font = *list.mc.fontRendererObj;
    font.drawString(labelText, list.mc.currentScreen->width / 2 - labelWidth / 2, y + entryHeight - font.FONT_HEIGHT - 1, 16777215);
}

GuiKeyBindingList::KeyEntry::KeyEntry(GuiKeyBindingList& listIn, KeyBinding& keybindingIn)
    : list(listIn)
    , keybinding(keybindingIn)
    , keyDesc(I18n::format(keybindingIn.getKeyDescription()))
    , btnChangeKeyBinding(0, 0, 0, 75, 20, I18n::format(keybindingIn.getKeyDescription()))
    , btnReset(0, 0, 0, 50, 20, I18n::format("controls.reset"))
{
}

void GuiKeyBindingList::KeyEntry::drawEntry(int /*slotIndex*/, int x, int y, int /*listWidth*/, int entryHeight, int mouseXIn, int mouseYIn, bool /*isSelected*/)
{
    Minecraft& minecraft = list.mc;
    FontRenderer& font = *minecraft.fontRendererObj;
    const bool flag = list.controls.buttonId == &keybinding;
    font.drawString(keyDesc, x + 90 - list.maxListLabelWidth, y + entryHeight / 2 - font.FONT_HEIGHT / 2, 16777215);
    btnReset.xPosition = x + 190;
    btnReset.yPosition = y;
    btnReset.enabled = keybinding.getKeyCode() != keybinding.getKeyCodeDefault();
    btnReset.drawButton(minecraft, mouseXIn, mouseYIn);
    btnChangeKeyBinding.xPosition = x + 105;
    btnChangeKeyBinding.yPosition = y;
    btnChangeKeyBinding.displayString = GameSettings::getKeyDisplayString(keybinding.getKeyCode());

    bool flag1 = false;
    if (keybinding.getKeyCode() != 0) {
        for (const KeyBinding* other : minecraft.gameSettings->keyBindings) {
            if (other != &keybinding && other->getKeyCode() == keybinding.getKeyCode()) {
                flag1 = true;
                break;
            }
        }
    }

    // EnumChatFormatting WHITE, YELLOW, RED
    if (flag)
        btnChangeKeyBinding.displayString = "§f> §e" + btnChangeKeyBinding.displayString + "§f <";
    else if (flag1)
        btnChangeKeyBinding.displayString = "§c" + btnChangeKeyBinding.displayString;

    btnChangeKeyBinding.drawButton(minecraft, mouseXIn, mouseYIn);
}

bool GuiKeyBindingList::KeyEntry::mousePressed(int /*slotIndex*/, int x, int y, int /*mouseEvent*/, int /*relativeX*/, int /*relativeY*/)
{
    Minecraft& minecraft = list.mc;
    if (btnChangeKeyBinding.mousePressed(minecraft, x, y)) {
        list.controls.buttonId = &keybinding;
        return true;
    }
    if (btnReset.mousePressed(minecraft, x, y)) {
        minecraft.gameSettings->setOptionKeyBinding(keybinding, keybinding.getKeyCodeDefault());
        KeyBinding::resetKeyBindingArrayAndHash();
        return true;
    }
    return false;
}

void GuiKeyBindingList::KeyEntry::mouseReleased(int /*slotIndex*/, int x, int y, int /*mouseEvent*/, int /*relativeX*/, int /*relativeY*/)
{
    btnChangeKeyBinding.mouseReleased(x, y);
    btnReset.mouseReleased(x, y);
}

} // namespace mc
