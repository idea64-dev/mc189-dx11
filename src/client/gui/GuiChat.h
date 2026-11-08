#pragma once

// Original: net/minecraft/client/gui/GuiChat.java
// Not ported yet: clicking and hovering chat components (GuiScreen.handleComponentClick/Hover, with the chat
// events).

#include "client/gui/GuiScreen.h"
#include "client/gui/GuiTextField.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class GuiChat : public GuiScreen {
public:
    GuiChat() = default;
    explicit GuiChat(std::string defaultText)
        : defaultInputFieldText(std::move(defaultText))
    {
    }

    void initGui() override;
    void onGuiClosed() override;
    void updateScreen() override;
    void handleMouseInput() override;
    void autocompletePlayerNames();
    void getSentHistory(int msgPos);
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void onAutocompleteResponse(const std::vector<std::string>& matches);
    bool doesGuiPauseGame() const override { return false; }

protected:
    void keyTyped(char16_t typedChar, int keyCode) override;
    void mouseClicked(int mouseX, int mouseY, int mouseButton) override;
    void setText(const std::string& newChatText, bool shouldOverwrite);
    // Java: GuiScreen.sendChatMessage(msg) -- adds it to the history and sends it.
    void sendChatMessage(const std::string& msg);

    std::unique_ptr<GuiTextField> inputField;

private:
    void sendAutocompleteRequest(const std::string& leftOfCursor, const std::string& full);

    std::string historyBuffer;
    int sentHistoryCursor = -1;
    bool playerNamesFound = false;
    bool waitingOnAutocomplete = false;
    int autocompleteIndex = 0;
    std::vector<std::string> foundPlayerNames;
    std::string defaultInputFieldText;
};

} // namespace mc
