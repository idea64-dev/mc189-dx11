#include "client/gui/stream/GuiStreamUnavailable.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/resources/Locale.h"
#include "core/Log.h"
#include "core/StringUtil.h"

#include <Windows.h>
#include <shellapi.h>

#include <algorithm>

namespace mc {

namespace {

// ChatComponentTranslation.getFormattedText(): the translation followed by a reset code.
std::string formatted(const std::string& key)
{
    return I18n::format(key) + "§r";
}

const char* messageKey(GuiStreamUnavailable::Reason reason)
{
    using Reason = GuiStreamUnavailable::Reason;
    switch (reason) {
    case Reason::NO_FBO: return "stream.unavailable.no_fbo";
    case Reason::LIBRARY_ARCH_MISMATCH: return "stream.unavailable.library_arch_mismatch";
    case Reason::LIBRARY_FAILURE: return "stream.unavailable.library_failure";
    case Reason::UNSUPPORTED_OS_WINDOWS: return "stream.unavailable.not_supported.windows";
    case Reason::UNSUPPORTED_OS_MAC: return "stream.unavailable.not_supported.mac";
    case Reason::UNSUPPORTED_OS_OTHER: return "stream.unavailable.not_supported.other";
    case Reason::ACCOUNT_NOT_MIGRATED: return "stream.unavailable.account_not_migrated";
    case Reason::ACCOUNT_NOT_BOUND: return "stream.unavailable.account_not_bound";
    case Reason::FAILED_TWITCH_AUTH: return "stream.unavailable.failed_auth";
    case Reason::FAILED_TWITCH_AUTH_ERROR: return "stream.unavailable.failed_auth_error";
    case Reason::INITIALIZATION_FAILURE: return "stream.unavailable.initialization_failure";
    case Reason::UNKNOWN: return "stream.unavailable.unknown";
    }
    return "";
}

// The second button's label, if the reason has one.
const char* actionKey(GuiStreamUnavailable::Reason reason)
{
    using Reason = GuiStreamUnavailable::Reason;
    switch (reason) {
    case Reason::LIBRARY_FAILURE:
    case Reason::INITIALIZATION_FAILURE:
    case Reason::UNKNOWN: return "stream.unavailable.report_to_mojang";
    case Reason::UNSUPPORTED_OS_MAC: return "stream.unavailable.not_supported.mac.okay";
    case Reason::ACCOUNT_NOT_MIGRATED: return "stream.unavailable.account_not_migrated.okay";
    case Reason::ACCOUNT_NOT_BOUND: return "stream.unavailable.account_not_bound.okay";
    case Reason::FAILED_TWITCH_AUTH: return "stream.unavailable.failed_auth.okay";
    default: return nullptr;
    }
}

} // namespace

GuiStreamUnavailable::GuiStreamUnavailable(std::shared_ptr<GuiScreen> parentScreenIn, Reason reasonIn, std::vector<std::string> extraLinesIn)
    : parentScreen(std::move(parentScreenIn))
    , reason(reasonIn)
    , extraLines(std::move(extraLinesIn))
{
}

void GuiStreamUnavailable::func_152321_a(std::shared_ptr<GuiScreen> parentScreen)
{
    // Java checks FBO support, the Twitch library, the OS version and then the session: with framebuffers
    // available and no twitch_access_token, a legacy (offline) session gets ACCOUNT_NOT_MIGRATED.
    Minecraft::getMinecraft()->displayGuiScreen(std::make_shared<GuiStreamUnavailable>(std::move(parentScreen), Reason::ACCOUNT_NOT_MIGRATED));
}

void GuiStreamUnavailable::initGui()
{
    if (lines.empty()) {
        lines = fontRendererObj->listFormattedStringToWidth(formatted(messageKey(reason)), static_cast<int>(static_cast<float>(width) * 0.75f));
        if (!extraLines.empty()) {
            lines.emplace_back();
            lines.insert(lines.end(), extraLines.begin(), extraLines.end());
        }
    }

    if (const char* action = actionKey(reason)) {
        addButton<GuiButton>(0, width / 2 - 155, height - 50, 150, 20, I18n::format("gui.cancel"));
        addButton<GuiButton>(1, width / 2 - 155 + 160, height - 50, 150, 20, I18n::format(formatted(action)));
    } else {
        addButton<GuiButton>(0, width / 2 - 75, height - 50, 150, 20, I18n::format("gui.cancel"));
    }
}

void GuiStreamUnavailable::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    int i = std::max(static_cast<int>(static_cast<double>(height) * 0.85 / 2.0 - static_cast<double>(static_cast<float>(static_cast<int>(lines.size()) * fontRendererObj->FONT_HEIGHT) / 2.0f)), 50);
    drawCenteredString(*fontRendererObj, formatted("stream.unavailable.title"), width / 2, i - fontRendererObj->FONT_HEIGHT * 2, 16777215);
    for (const std::string& s : lines) {
        drawCenteredString(*fontRendererObj, s, width / 2, i, 10526880);
        i += fontRendererObj->FONT_HEIGHT;
    }
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

void GuiStreamUnavailable::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;

    if (button.id == 1) {
        switch (reason) {
        case Reason::ACCOUNT_NOT_BOUND:
        case Reason::FAILED_TWITCH_AUTH:
            openUrl("https://account.mojang.com/me/settings");
            break;
        case Reason::ACCOUNT_NOT_MIGRATED:
            openUrl("https://account.mojang.com/migrate");
            break;
        case Reason::UNSUPPORTED_OS_MAC:
            openUrl("http://www.apple.com/osx/");
            break;
        case Reason::UNKNOWN:
        case Reason::LIBRARY_FAILURE:
        case Reason::INITIALIZATION_FAILURE:
            openUrl("http://bugs.mojang.com/browse/MC");
            break;
        default:
            break;
        }
    }

    mc->displayGuiScreen(parentScreen);
}

void GuiStreamUnavailable::openUrl(const std::string& url)
{
    // Java: java.awt.Desktop.browse
    const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", toWide(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    if (result <= 32)
        Log::error("Couldn't open link");
}

} // namespace mc
