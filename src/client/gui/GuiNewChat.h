#pragma once

// Original: net/minecraft/client/gui/GuiNewChat.java, ChatLine.java and GuiUtilRenderComponents.java
// The chat lines over the hotbar. Hover/click events on chat components come with GuiChat's component handling.

#include "client/gui/Gui.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class FontRenderer;
class IChatComponent;
class Minecraft;

class ChatLine {
public:
    ChatLine(int updateCounterCreatedIn, std::shared_ptr<IChatComponent> lineStringIn, int chatLineIDIn)
        : updateCounterCreated(updateCounterCreatedIn)
        , lineString(std::move(lineStringIn))
        , chatLineID(chatLineIDIn)
    {
    }

    const std::shared_ptr<IChatComponent>& getChatComponent() const { return lineString; }
    int getUpdatedCounter() const { return updateCounterCreated; }
    int getChatLineID() const { return chatLineID; }

private:
    int updateCounterCreated;
    std::shared_ptr<IChatComponent> lineString;
    int chatLineID;
};

class GuiUtilRenderComponents {
public:
    // Strips the formatting codes unless forced or chat colours are on.
    static std::string func_178909_a(const std::string& text, bool forceColor);
    static std::vector<std::shared_ptr<IChatComponent>> splitText(
        IChatComponent& component, int maxTextLength, FontRenderer& fontRenderer, bool trimSpace, bool forceTextColor);
    // Java: EnumChatFormatting.getTextWithoutFormattingCodes
    static std::string getTextWithoutFormattingCodes(const std::string& text);
};

class GuiNewChat : public Gui {
public:
    explicit GuiNewChat(Minecraft* mcIn)
        : mc(mcIn)
    {
    }

    void drawChat(int updateCounter);
    void clearChatMessages();
    void printChatMessage(const std::shared_ptr<IChatComponent>& chatComponent) { printChatMessageWithOptionalDeletion(chatComponent, 0); }
    void printChatMessageWithOptionalDeletion(const std::shared_ptr<IChatComponent>& chatComponent, int chatLineId);
    void refreshChat();
    const std::vector<std::string>& getSentMessages() const { return sentMessages; }
    void addToSentMessages(const std::string& message);
    void resetScroll();
    void scroll(int amount);
    // Null when the mouse is over no text component.
    std::shared_ptr<IChatComponent> getChatComponent(int mouseX, int mouseY);
    bool getChatOpen() const;
    void deleteChatLine(int id);
    int getChatWidth() const;
    int getChatHeight() const;
    float getChatScale() const;
    static int calculateChatboxWidth(float scale);
    static int calculateChatboxHeight(float scale);
    int getLineCount() const { return getChatHeight() / 9; }

private:
    void setChatLine(const std::shared_ptr<IChatComponent>& chatComponent, int chatLineId, int updateCounter, bool displayOnly);

    Minecraft* mc;
    std::vector<std::string> sentMessages;
    std::vector<ChatLine> chatLines;
    std::vector<ChatLine> drawnChatLines;
    int scrollPos = 0;
    bool isScrolled = false;
};

} // namespace mc
