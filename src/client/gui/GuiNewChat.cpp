#include "client/gui/GuiNewChat.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiChat.h"
#include "client/gui/GuiIngame.h"
#include "client/gui/ScaledResolution.h"
#include "client/renderer/DxStateManager.h"
#include "client/settings/GameSettings.h"
#include "core/Java.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "entity/player/PlayerEnums.h"
#include "util/ChatComponent.h"
#include "util/MathHelper.h"

#include <algorithm>

namespace mc {

namespace {

bool isFormattingCodeChar(char16_t c)
{
    return (c >= u'0' && c <= u'9') || (c >= u'a' && c <= u'f') || (c >= u'A' && c <= u'F') || (c >= u'k' && c <= u'o') || (c >= u'K' && c <= u'O') || c == u'r'
        || c == u'R';
}

} // namespace

std::string GuiUtilRenderComponents::getTextWithoutFormattingCodes(const std::string& text)
{
    // Java: (?i)§[0-9A-FK-OR] replaced by nothing.
    const std::u16string s = toUtf16(text);
    std::u16string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == u'§' && i + 1 < s.size() && isFormattingCodeChar(s[i + 1])) {
            ++i;
            continue;
        }
        out.push_back(s[i]);
    }
    return fromUtf16(out);
}

std::string GuiUtilRenderComponents::func_178909_a(const std::string& text, bool forceColor)
{
    return !forceColor && !Minecraft::getMinecraft()->gameSettings->chatColours ? getTextWithoutFormattingCodes(text) : text;
}

std::vector<std::shared_ptr<IChatComponent>> GuiUtilRenderComponents::splitText(
    IChatComponent& component, int maxTextLength, FontRenderer& fontRenderer, bool trimSpace, bool forceTextColor)
{
    int i = 0;
    std::shared_ptr<IChatComponent> ichatcomponent = std::make_shared<ChatComponentText>("");
    std::vector<std::shared_ptr<IChatComponent>> list;

    // Java: Lists.newArrayList(component) -- the components the chat component iterates over. The split-off
    // remainders inserted below are owned here.
    std::vector<IChatComponent*> collected;
    component.collect(collected);
    std::vector<std::shared_ptr<IChatComponent>> owned;
    std::vector<IChatComponent*> list1 = collected;

    for (size_t j = 0; j < list1.size(); ++j) {
        IChatComponent* ichatcomponent1 = list1[j];
        std::u16string s = toUtf16(ichatcomponent1->getUnformattedTextForChat());
        bool flag = false;

        const size_t newline = s.find(u'\n');
        if (newline != std::u16string::npos) {
            const std::u16string s1 = s.substr(newline + 1);
            s = s.substr(0, newline + 1);
            auto chatcomponenttext = std::make_shared<ChatComponentText>(fromUtf16(s1));
            chatcomponenttext->setChatStyle(ichatcomponent1->getChatStyle()->createShallowCopy());
            owned.push_back(chatcomponenttext);
            list1.insert(list1.begin() + static_cast<std::ptrdiff_t>(j + 1), chatcomponenttext.get());
            flag = true;
        }

        const std::u16string s4 = toUtf16(func_178909_a(ichatcomponent1->getChatStyle()->getFormattingCode() + fromUtf16(s), forceTextColor));
        const std::u16string s5 = !s4.empty() && s4.back() == u'\n' ? s4.substr(0, s4.size() - 1) : s4;
        int i1 = fontRenderer.getStringWidth16(s5);
        auto chatcomponenttext1 = std::make_shared<ChatComponentText>(fromUtf16(s5));
        chatcomponenttext1->setChatStyle(ichatcomponent1->getChatStyle()->createShallowCopy());

        if (i + i1 > maxTextLength) {
            std::u16string s2 = fontRenderer.trimStringToWidth16(s4, maxTextLength - i, false);
            std::optional<std::u16string> s3;
            if (s2.size() < s4.size())
                s3 = s4.substr(s2.size());

            if (s3 && !s3->empty()) {
                const size_t l0 = s2.rfind(u' ');
                const int l = l0 == std::u16string::npos ? -1 : static_cast<int>(l0);

                if (l >= 0 && fontRenderer.getStringWidth16(s4.substr(0, static_cast<size_t>(l))) > 0) {
                    s2 = s4.substr(0, static_cast<size_t>(l));
                    const int start = trimSpace ? l + 1 : l;
                    s3 = s4.substr(static_cast<size_t>(start));
                } else if (i > 0 && s4.find(u' ') == std::u16string::npos) {
                    s2.clear();
                    s3 = s4;
                }

                auto chatcomponenttext2 = std::make_shared<ChatComponentText>(fromUtf16(*s3));
                chatcomponenttext2->setChatStyle(ichatcomponent1->getChatStyle()->createShallowCopy());
                owned.push_back(chatcomponenttext2);
                list1.insert(list1.begin() + static_cast<std::ptrdiff_t>(j + 1), chatcomponenttext2.get());
            }

            i1 = fontRenderer.getStringWidth16(s2);
            chatcomponenttext1 = std::make_shared<ChatComponentText>(fromUtf16(s2));
            chatcomponenttext1->setChatStyle(ichatcomponent1->getChatStyle()->createShallowCopy());
            flag = true;
        }

        if (i + i1 <= maxTextLength) {
            i += i1;
            ichatcomponent->appendSibling(chatcomponenttext1);
        } else {
            flag = true;
        }

        if (flag) {
            list.push_back(ichatcomponent);
            i = 0;
            ichatcomponent = std::make_shared<ChatComponentText>("");
        }
    }

    list.push_back(ichatcomponent);
    return list;
}

void GuiNewChat::drawChat(int updateCounter)
{
    if (mc->gameSettings->chatVisibility == EnumChatVisibility::HIDDEN)
        return;

    const int i = getLineCount();
    bool flag = false;
    int j = 0;
    const int k = static_cast<int>(drawnChatLines.size());
    const float f = mc->gameSettings->chatOpacity * 0.9f + 0.1f;

    if (k <= 0)
        return;

    if (getChatOpen())
        flag = true;

    const float f1 = getChatScale();
    const int l = MathHelper::ceiling_float_int(static_cast<float>(getChatWidth()) / f1);
    DxStateManager::pushMatrix();
    DxStateManager::translate(2.0f, 20.0f, 0.0f);
    DxStateManager::scale(f1, f1, 1.0f);

    for (int i1 = 0; i1 + scrollPos < static_cast<int>(drawnChatLines.size()) && i1 < i; ++i1) {
        const ChatLine& chatline = drawnChatLines[static_cast<size_t>(i1 + scrollPos)];
        const int j1 = updateCounter - chatline.getUpdatedCounter();

        if (j1 < 200 || flag) {
            double d0 = static_cast<double>(j1) / 200.0;
            d0 = 1.0 - d0;
            d0 = d0 * 10.0;
            d0 = MathHelper::clamp_double(d0, 0.0, 1.0);
            d0 = d0 * d0;
            int l1 = java::d2i(255.0 * d0);

            if (flag)
                l1 = 255;

            l1 = java::f2i(static_cast<float>(l1) * f);
            ++j;

            if (l1 > 3) {
                const int i2 = 0;
                const int j2 = -i1 * 9;
                drawRect(i2, j2 - 9, i2 + l + 4, j2, l1 / 2 << 24);
                const std::string s = chatline.getChatComponent()->getFormattedText();
                DxStateManager::enableBlend();
                mc->fontRendererObj->drawStringWithShadow(s, static_cast<float>(i2), static_cast<float>(j2 - 8), 16777215 + (l1 << 24));
                DxStateManager::disableAlpha();
                DxStateManager::disableBlend();
            }
        }
    }

    if (flag) {
        const int k2 = mc->fontRendererObj->FONT_HEIGHT;
        DxStateManager::translate(-3.0f, 0.0f, 0.0f);
        const int l2 = k * k2 + k;
        const int i3 = j * k2 + j;
        const int j3 = scrollPos * i3 / k;
        const int k1 = i3 * i3 / l2;

        if (l2 != i3) {
            const int k3 = j3 > 0 ? 170 : 96;
            const int l3 = isScrolled ? 13382451 : 3355562;
            drawRect(0, -j3, 2, -j3 - k1, l3 + (k3 << 24));
            drawRect(2, -j3, 1, -j3 - k1, 13421772 + (k3 << 24));
        }
    }

    DxStateManager::popMatrix();
}

void GuiNewChat::clearChatMessages()
{
    drawnChatLines.clear();
    chatLines.clear();
    sentMessages.clear();
}

void GuiNewChat::printChatMessageWithOptionalDeletion(const std::shared_ptr<IChatComponent>& chatComponent, int chatLineId)
{
    setChatLine(chatComponent, chatLineId, mc->ingameGUI->getUpdateCounter(), false);
    Log::info("[CHAT] {}", chatComponent->getUnformattedText());
}

void GuiNewChat::setChatLine(const std::shared_ptr<IChatComponent>& chatComponent, int chatLineId, int updateCounter, bool displayOnly)
{
    if (chatLineId != 0)
        deleteChatLine(chatLineId);

    const int i = MathHelper::floor_float(static_cast<float>(getChatWidth()) / getChatScale());
    const std::vector<std::shared_ptr<IChatComponent>> list = GuiUtilRenderComponents::splitText(*chatComponent, i, *mc->fontRendererObj, false, false);
    const bool flag = getChatOpen();

    for (const auto& ichatcomponent : list) {
        if (flag && scrollPos > 0) {
            isScrolled = true;
            scroll(1);
        }

        drawnChatLines.insert(drawnChatLines.begin(), ChatLine(updateCounter, ichatcomponent, chatLineId));
    }

    while (drawnChatLines.size() > 100)
        drawnChatLines.pop_back();

    if (!displayOnly) {
        chatLines.insert(chatLines.begin(), ChatLine(updateCounter, chatComponent, chatLineId));

        while (chatLines.size() > 100)
            chatLines.pop_back();
    }
}

void GuiNewChat::refreshChat()
{
    drawnChatLines.clear();
    resetScroll();

    for (int i = static_cast<int>(chatLines.size()) - 1; i >= 0; --i) {
        const ChatLine chatline = chatLines[static_cast<size_t>(i)];
        setChatLine(chatline.getChatComponent(), chatline.getChatLineID(), chatline.getUpdatedCounter(), true);
    }
}

void GuiNewChat::addToSentMessages(const std::string& message)
{
    if (sentMessages.empty() || sentMessages.back() != message)
        sentMessages.push_back(message);
}

void GuiNewChat::resetScroll()
{
    scrollPos = 0;
    isScrolled = false;
}

void GuiNewChat::scroll(int amount)
{
    scrollPos += amount;
    const int i = static_cast<int>(drawnChatLines.size());

    if (scrollPos > i - getLineCount())
        scrollPos = i - getLineCount();

    if (scrollPos <= 0) {
        scrollPos = 0;
        isScrolled = false;
    }
}

std::shared_ptr<IChatComponent> GuiNewChat::getChatComponent(int mouseX, int mouseY)
{
    if (!getChatOpen())
        return nullptr;

    const ScaledResolution scaledresolution(*mc);
    const int i = scaledresolution.getScaleFactor();
    const float f = getChatScale();
    int j = mouseX / i - 3;
    int k = mouseY / i - 27;
    j = MathHelper::floor_float(static_cast<float>(j) / f);
    k = MathHelper::floor_float(static_cast<float>(k) / f);

    if (j < 0 || k < 0)
        return nullptr;

    const int l = std::min(getLineCount(), static_cast<int>(drawnChatLines.size()));

    if (j <= MathHelper::floor_float(static_cast<float>(getChatWidth()) / getChatScale()) && k < mc->fontRendererObj->FONT_HEIGHT * l + l) {
        const int i1 = k / mc->fontRendererObj->FONT_HEIGHT + scrollPos;

        if (i1 >= 0 && i1 < static_cast<int>(drawnChatLines.size())) {
            const ChatLine& chatline = drawnChatLines[static_cast<size_t>(i1)];
            int j1 = 0;

            std::vector<IChatComponent*> parts;
            chatline.getChatComponent()->collect(parts);
            for (IChatComponent* ichatcomponent : parts) {
                if (auto* text = dynamic_cast<ChatComponentText*>(ichatcomponent)) {
                    j1 += mc->fontRendererObj->getStringWidth(GuiUtilRenderComponents::func_178909_a(text->getChatComponentText_TextValue(), false));

                    if (j1 > j) {
                        // The parts are the line's own siblings: hand out a reference that keeps the line alive.
                        return std::shared_ptr<IChatComponent>(chatline.getChatComponent(), ichatcomponent);
                    }
                }
            }
        }
    }

    return nullptr;
}

bool GuiNewChat::getChatOpen() const
{
    return dynamic_cast<GuiChat*>(mc->currentScreen.get()) != nullptr;
}

void GuiNewChat::deleteChatLine(int id)
{
    std::erase_if(drawnChatLines, [id](const ChatLine& chatline) { return chatline.getChatLineID() == id; });

    for (auto it = chatLines.begin(); it != chatLines.end(); ++it) {
        if (it->getChatLineID() == id) {
            chatLines.erase(it);
            break;
        }
    }
}

int GuiNewChat::getChatWidth() const
{
    return calculateChatboxWidth(mc->gameSettings->chatWidth);
}

int GuiNewChat::getChatHeight() const
{
    return calculateChatboxHeight(getChatOpen() ? mc->gameSettings->chatHeightFocused : mc->gameSettings->chatHeightUnfocused);
}

float GuiNewChat::getChatScale() const
{
    return mc->gameSettings->chatScale;
}

int GuiNewChat::calculateChatboxWidth(float scale)
{
    const int i = 320;
    const int j = 40;
    return MathHelper::floor_float(scale * static_cast<float>(i - j) + static_cast<float>(j));
}

int GuiNewChat::calculateChatboxHeight(float scale)
{
    const int i = 180;
    const int j = 20;
    return MathHelper::floor_float(scale * static_cast<float>(i - j) + static_cast<float>(j));
}

} // namespace mc
