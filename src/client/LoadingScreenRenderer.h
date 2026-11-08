#pragma once

// Original: net/minecraft/client/LoadingScreenRenderer.java -- the dirt screen with a title, a message and a
// progress bar, drawn straight to the display while the client waits (world loading and saving).

#include "util/IProgressUpdate.h"

#include <cstdint>
#include <memory>
#include <string>

namespace mc {

class Framebuffer;
class Minecraft;

class LoadingScreenRenderer : public IProgressUpdate {
public:
    explicit LoadingScreenRenderer(Minecraft& mcIn);
    ~LoadingScreenRenderer() override;

    void resetProgressAndMessage(const std::string& message) override;
    void displaySavingString(const std::string& message) override;
    void displayLoadingString(const std::string& message) override;
    void setLoadingProgress(int progress) override;
    void setDoneWorking() override { }

private:
    void displayString(const std::string& message);

    Minecraft& mc;
    std::string message;
    std::string currentlyDisplayedText;
    int64_t systemTime;
    bool loadingSuccess = false;
    std::unique_ptr<Framebuffer> framebuffer;
};

} // namespace mc
