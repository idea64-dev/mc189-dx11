#include "client/gui/GuiOptionsRowList.h"

#include "client/Minecraft.h"
#include "client/gui/GuiOptionButton.h"

namespace mc {

GuiOptionsRowList::GuiOptionsRowList(Minecraft& mcIn, int widthIn, int heightIn, int topIn, int bottomIn, int slotHeightIn, std::span<const GameSettings::Options* const> options)
    : GuiListExtended(mcIn, widthIn, heightIn, topIn, bottomIn, slotHeightIn)
{
    centerListVertically = false;
    for (size_t i = 0; i < options.size(); i += 2) {
        const GameSettings::Options* option = options[i];
        const GameSettings::Options* option1 = i + 1 < options.size() ? options[i + 1] : nullptr;
        auto button = createButton(mcIn, widthIn / 2 - 155, 0, option);
        auto button1 = createButton(mcIn, widthIn / 2 - 155 + 160, 0, option1);
        rows.emplace_back(std::move(button), std::move(button1));
    }
}

std::unique_ptr<GuiButton> GuiOptionsRowList::createButton(Minecraft& mcIn, int x, int y, const GameSettings::Options* option)
{
    if (option == nullptr)
        return nullptr;
    const int i = option->returnEnumOrdinal();
    if (option->getEnumFloat())
        return std::make_unique<GuiOptionSlider>(i, x, y, *option);
    return std::make_unique<GuiOptionButton>(i, x, y, option, mcIn.gameSettings->getKeyBinding(*option));
}

GuiOptionsRowList::Row::Row(std::unique_ptr<GuiButton> left, std::unique_ptr<GuiButton> right)
    : mc(*Minecraft::getMinecraft())
    , buttonA(std::move(left))
    , buttonB(std::move(right))
{
}

void GuiOptionsRowList::Row::drawEntry(int /*slotIndex*/, int /*x*/, int y, int /*listWidth*/, int /*slotHeight*/, int mouseXIn, int mouseYIn, bool /*isSelected*/)
{
    if (buttonA) {
        buttonA->yPosition = y;
        buttonA->drawButton(mc, mouseXIn, mouseYIn);
    }
    if (buttonB) {
        buttonB->yPosition = y;
        buttonB->drawButton(mc, mouseXIn, mouseYIn);
    }
}

bool GuiOptionsRowList::Row::mousePressed(int /*slotIndex*/, int x, int y, int /*mouseEvent*/, int /*relativeX*/, int /*relativeY*/)
{
    // Java dereferences the left button unconditionally; every row has one.
    if (buttonA->mousePressed(mc, x, y)) {
        if (auto* optionButton = dynamic_cast<GuiOptionButton*>(buttonA.get())) {
            mc.gameSettings->setOptionValue(*optionButton->returnEnumOptions(), 1);
            buttonA->displayString = mc.gameSettings->getKeyBinding(*GameSettings::Options::getEnumOptions(buttonA->id));
        }
        return true;
    }
    if (buttonB && buttonB->mousePressed(mc, x, y)) {
        if (auto* optionButton = dynamic_cast<GuiOptionButton*>(buttonB.get())) {
            mc.gameSettings->setOptionValue(*optionButton->returnEnumOptions(), 1);
            buttonB->displayString = mc.gameSettings->getKeyBinding(*GameSettings::Options::getEnumOptions(buttonB->id));
        }
        return true;
    }
    return false;
}

void GuiOptionsRowList::Row::mouseReleased(int /*slotIndex*/, int x, int y, int /*mouseEvent*/, int /*relativeX*/, int /*relativeY*/)
{
    if (buttonA)
        buttonA->mouseReleased(x, y);
    if (buttonB)
        buttonB->mouseReleased(x, y);
}

} // namespace mc
