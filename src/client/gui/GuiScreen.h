#pragma once

// Original: net/minecraft/client/gui/GuiScreen.java
// Screens are shared_ptr-owned: a child screen keeps its parent alive (Java's GC did), and code that calls into
// a screen holds a reference so the screen survives replacing itself through displayGuiScreen.
// Not ported yet: item tooltips, chat component hover/click (with the inventory and chat), GuiLabel.

#include "client/gui/Gui.h"
#include "client/gui/GuiButton.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class FontRenderer;
class Minecraft;

// Java: GuiYesNoCallback
class GuiYesNoCallback {
public:
    virtual ~GuiYesNoCallback() = default;
    virtual void confirmClicked(bool result, int id) = 0;
};

class GuiScreen : public Gui, public GuiYesNoCallback, public std::enable_shared_from_this<GuiScreen> {
public:
    virtual void drawScreen(int mouseX, int mouseY, float partialTicks);
    virtual void setWorldAndResolution(Minecraft& mc, int width, int height);
    void setGuiSize(int w, int h);
    virtual void initGui();
    virtual void handleInput();
    virtual void handleMouseInput();
    virtual void handleKeyboardInput();
    virtual void updateScreen();
    virtual void onGuiClosed();
    void drawDefaultBackground();
    void drawWorldBackground(int tint);
    void drawBackground(int tint);
    virtual bool doesGuiPauseGame() const { return true; }
    void confirmClicked(bool result, int id) override;
    virtual void onResize(Minecraft& mcIn, int w, int h);

    static std::string getClipboardString();
    static void setClipboardString(const std::string& copyText);
    static bool isCtrlKeyDown();
    static bool isShiftKeyDown();
    static bool isAltKeyDown();
    static bool isKeyComboCtrlX(int keyID);
    static bool isKeyComboCtrlV(int keyID);
    static bool isKeyComboCtrlC(int keyID);
    static bool isKeyComboCtrlA(int keyID);

    int width = 0;
    int height = 0;
    bool allowUserInput = false;

protected:
    // keyCode is the LWJGL key; typedChar is the UTF-16 unit (Java char).
    virtual void keyTyped(char16_t typedChar, int keyCode);
    virtual void mouseClicked(int mouseX, int mouseY, int mouseButton);
    virtual void mouseReleased(int mouseX, int mouseY, int state);
    virtual void mouseClickMove(int mouseX, int mouseY, int clickedMouseButton, int64_t timeSinceLastClick);
    virtual void actionPerformed(GuiButton& button);

    template <class T, class... Args>
    T& addButton(Args&&... args)
    {
        auto button = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *button;
        buttonList.push_back(std::move(button));
        return ref;
    }

    Minecraft* mc = nullptr;
    std::vector<std::unique_ptr<GuiButton>> buttonList;
    FontRenderer* fontRendererObj = nullptr;

private:
    GuiButton* selectedButton = nullptr;
    int eventButton = 0;
    int64_t lastMouseEvent = 0;
    int touchValue = 0;
};

} // namespace mc
