#pragma once

// Original: net/minecraft/client/gui/stream/GuiStreamUnavailable.java
// Twitch broadcasting is not ported (the service integration died with Minecraft's Twitch SDK); the options
// screen always ends up here, with the reason vanilla 1.8.9 gives for an offline (legacy) session.

#include "client/gui/GuiScreen.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class GuiStreamUnavailable : public GuiScreen {
public:
    enum class Reason {
        NO_FBO,
        LIBRARY_ARCH_MISMATCH,
        LIBRARY_FAILURE,
        UNSUPPORTED_OS_WINDOWS,
        UNSUPPORTED_OS_MAC,
        UNSUPPORTED_OS_OTHER,
        ACCOUNT_NOT_MIGRATED,
        ACCOUNT_NOT_BOUND,
        FAILED_TWITCH_AUTH,
        FAILED_TWITCH_AUTH_ERROR,
        INITIALIZATION_FAILURE,
        UNKNOWN
    };

    GuiStreamUnavailable(std::shared_ptr<GuiScreen> parentScreen, Reason reason, std::vector<std::string> extraLines = {});

    void initGui() override;
    void onGuiClosed() override { }
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

    // Java: picks the reason from the stream state and shows the screen.
    static void func_152321_a(std::shared_ptr<GuiScreen> parentScreen);

protected:
    void actionPerformed(GuiButton& button) override;

private:
    static void openUrl(const std::string& url);

    std::shared_ptr<GuiScreen> parentScreen;
    Reason reason;
    std::vector<std::string> extraLines;
    std::vector<std::string> lines;
};

} // namespace mc
