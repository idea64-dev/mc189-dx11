#include "client/gui/GuiChat.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiIngame.h"
#include "client/gui/GuiNewChat.h"
#include "client/network/NetHandlerPlayClient.h"
#include "core/JavaString.h"
#include "core/StringUtil.h"
#include "entity/player/EntityPlayerSP.h"
#include "network/play/ClientPlayPackets.h"
#include "platform/Keyboard.h"
#include "platform/Mouse.h"
#include "util/ChatComponent.h"
#include "util/MathHelper.h"

#include <climits>
#include <optional>

namespace mc {

namespace {

std::u16string lower16(const std::u16string& s)
{
    return toUtf16(java::toLowerCase(fromUtf16(s)));
}

// Java: StringUtils.getCommonPrefix
std::u16string commonPrefix(const std::vector<std::string>& strings)
{
    if (strings.empty())
        return {};
    std::u16string prefix = toUtf16(strings.front());
    for (size_t i = 1; i < strings.size(); ++i) {
        const std::u16string s = toUtf16(strings[i]);
        size_t n = 0;
        while (n < prefix.size() && n < s.size() && prefix[n] == s[n])
            ++n;
        prefix.resize(n);
    }
    return prefix;
}

} // namespace

void GuiChat::initGui()
{
    Keyboard::enableRepeatEvents(true);
    sentHistoryCursor = static_cast<int>(mc->ingameGUI->getChatGUI().getSentMessages().size());
    inputField = std::make_unique<GuiTextField>(0, *fontRendererObj, 4, height - 12, width - 4, 12);
    inputField->setMaxStringLength(100);
    inputField->setEnableBackgroundDrawing(false);
    inputField->setFocused(true);
    inputField->setText(defaultInputFieldText);
    inputField->setCanLoseFocus(false);
}

void GuiChat::onGuiClosed()
{
    Keyboard::enableRepeatEvents(false);
    mc->ingameGUI->getChatGUI().resetScroll();
}

void GuiChat::updateScreen()
{
    inputField->updateCursorCounter();
}

void GuiChat::keyTyped(char16_t typedChar, int keyCode)
{
    waitingOnAutocomplete = false;

    if (keyCode == 15)
        autocompletePlayerNames();
    else
        playerNamesFound = false;

    if (keyCode == 1) {
        mc->displayGuiScreen(nullptr);
    } else if (keyCode != 28 && keyCode != 156) {
        if (keyCode == 200)
            getSentHistory(-1);
        else if (keyCode == 208)
            getSentHistory(1);
        else if (keyCode == 201)
            mc->ingameGUI->getChatGUI().scroll(mc->ingameGUI->getChatGUI().getLineCount() - 1);
        else if (keyCode == 209)
            mc->ingameGUI->getChatGUI().scroll(-mc->ingameGUI->getChatGUI().getLineCount() + 1);
        else
            inputField->textboxKeyTyped(typedChar, keyCode);
    } else {
        const std::string s = java::trim(inputField->getText());

        if (!s.empty())
            sendChatMessage(s);

        mc->displayGuiScreen(nullptr);
    }
}

void GuiChat::sendChatMessage(const std::string& msg)
{
    mc->ingameGUI->getChatGUI().addToSentMessages(msg);
    mc->thePlayer->sendChatMessage(msg);
}

void GuiChat::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    int i = Mouse::getEventDWheel();

    if (i != 0) {
        if (i > 1)
            i = 1;
        if (i < -1)
            i = -1;
        if (!isShiftKeyDown())
            i *= 7;

        mc->ingameGUI->getChatGUI().scroll(i);
    }
}

void GuiChat::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    // Java: a click on a chat component runs its click event first (not ported yet).
    inputField->mouseClicked(mouseX, mouseY, mouseButton);
    GuiScreen::mouseClicked(mouseX, mouseY, mouseButton);
}

void GuiChat::setText(const std::string& newChatText, bool shouldOverwrite)
{
    if (shouldOverwrite)
        inputField->setText(newChatText);
    else
        inputField->writeText(newChatText);
}

void GuiChat::autocompletePlayerNames()
{
    if (playerNamesFound) {
        inputField->deleteFromCursor(inputField->func_146197_a(-1, inputField->getCursorPosition(), false) - inputField->getCursorPosition());

        if (autocompleteIndex >= static_cast<int>(foundPlayerNames.size()))
            autocompleteIndex = 0;
    } else {
        const int i = inputField->func_146197_a(-1, inputField->getCursorPosition(), false);
        foundPlayerNames.clear();
        autocompleteIndex = 0;
        const std::u16string& text = inputField->getText16();
        const std::u16string s = lower16(text.substr(static_cast<size_t>(i)));
        const std::u16string s1 = text.substr(0, static_cast<size_t>(inputField->getCursorPosition()));
        sendAutocompleteRequest(fromUtf16(s1), fromUtf16(s));

        if (foundPlayerNames.empty())
            return;

        playerNamesFound = true;
        inputField->deleteFromCursor(i - inputField->getCursorPosition());
    }

    if (foundPlayerNames.size() > 1) {
        std::string stringbuilder;

        for (const std::string& s2 : foundPlayerNames) {
            if (!stringbuilder.empty())
                stringbuilder += ", ";
            stringbuilder += s2;
        }

        mc->ingameGUI->getChatGUI().printChatMessageWithOptionalDeletion(std::make_shared<ChatComponentText>(stringbuilder), 1);
    }

    inputField->writeText(foundPlayerNames[static_cast<size_t>(autocompleteIndex++)]);
}

void GuiChat::sendAutocompleteRequest(const std::string& leftOfCursor, const std::string&)
{
    if (!leftOfCursor.empty()) {
        std::optional<BlockPos> blockpos;

        if (mc->objectMouseOver && mc->objectMouseOver->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK)
            blockpos = mc->objectMouseOver->getBlockPos();

        mc->thePlayer->sendQueue->addToSendQueue(std::make_shared<C14PacketTabComplete>(leftOfCursor, blockpos));
        waitingOnAutocomplete = true;
    }
}

void GuiChat::getSentHistory(int msgPos)
{
    int i = sentHistoryCursor + msgPos;
    const int j = static_cast<int>(mc->ingameGUI->getChatGUI().getSentMessages().size());
    i = MathHelper::clamp_int(i, 0, j);

    if (i != sentHistoryCursor) {
        if (i == j) {
            sentHistoryCursor = j;
            inputField->setText(historyBuffer);
        } else {
            if (sentHistoryCursor == j)
                historyBuffer = inputField->getText();

            inputField->setText(mc->ingameGUI->getChatGUI().getSentMessages()[static_cast<size_t>(i)]);
            sentHistoryCursor = i;
        }
    }
}

void GuiChat::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawRect(2, height - 14, width - 2, height - 2, INT_MIN);
    inputField->drawTextBox();
    // Java: the hover event of the component under the mouse (not ported yet).
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

void GuiChat::onAutocompleteResponse(const std::vector<std::string>& matches)
{
    if (!waitingOnAutocomplete)
        return;

    playerNamesFound = false;
    foundPlayerNames.clear();

    for (const std::string& s : matches) {
        if (!s.empty())
            foundPlayerNames.push_back(s);
    }

    const std::u16string s1 = inputField->getText16().substr(static_cast<size_t>(inputField->func_146197_a(-1, inputField->getCursorPosition(), false)));
    const std::u16string s2 = commonPrefix(matches);

    if (!s2.empty() && lower16(s1) != lower16(s2)) {
        inputField->deleteFromCursor(inputField->func_146197_a(-1, inputField->getCursorPosition(), false) - inputField->getCursorPosition());
        inputField->writeText16(s2);
    } else if (!foundPlayerNames.empty()) {
        playerNamesFound = true;
        autocompletePlayerNames();
    }
}

} // namespace mc
