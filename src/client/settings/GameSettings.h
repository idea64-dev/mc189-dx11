#pragma once

// Original: net/minecraft/client/settings/GameSettings.java
// options.txt is read and written in the same format and order, so it can be shared with the original game.
// Side effects on systems that don't exist yet (chat, world renderer, sound, Twitch, server) are noted inline.

#include "client/audio/SoundCategory.h"
#include "client/settings/KeyBinding.h"
#include "entity/player/PlayerEnums.h"
#include "world/EnumDifficulty.h"

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mc {

class Minecraft;

class GameSettings {
public:
    class Options {
    public:
        static Options INVERT_MOUSE, SENSITIVITY, FOV, GAMMA, SATURATION, RENDER_DISTANCE, VIEW_BOBBING, ANAGLYPH,
            FRAMERATE_LIMIT, FBO_ENABLE, RENDER_CLOUDS, GRAPHICS, AMBIENT_OCCLUSION, GUI_SCALE, PARTICLES,
            CHAT_VISIBILITY, CHAT_COLOR, CHAT_LINKS, CHAT_OPACITY, CHAT_LINKS_PROMPT, SNOOPER_ENABLED, USE_FULLSCREEN,
            ENABLE_VSYNC, USE_VBO, TOUCHSCREEN, CHAT_SCALE, CHAT_WIDTH, CHAT_HEIGHT_FOCUSED, CHAT_HEIGHT_UNFOCUSED,
            MIPMAP_LEVELS, FORCE_UNICODE_FONT, STREAM_BYTES_PER_PIXEL, STREAM_VOLUME_MIC, STREAM_VOLUME_SYSTEM,
            STREAM_KBPS, STREAM_FPS, STREAM_COMPRESSION, STREAM_SEND_METADATA, STREAM_CHAT_ENABLED,
            STREAM_CHAT_USER_FILTER, STREAM_MIC_TOGGLE_BEHAVIOR, BLOCK_ALTERNATIVES, REDUCED_DEBUG_INFO,
            ENTITY_SHADOWS, REALMS_NOTIFICATIONS;

        static const std::vector<Options*>& values();
        static Options* getEnumOptions(int ordinal);

        bool getEnumFloat() const { return enumFloat; }
        bool getEnumBoolean() const { return enumBoolean; }
        int returnEnumOrdinal() const { return ordinal; }
        const std::string& getEnumString() const { return enumString; }
        float getValueMin() const { return valueMin; }
        float getValueMax() const { return valueMax; }
        void setValueMax(float value) { valueMax = value; }
        float normalizeValue(float value) const;
        float denormalizeValue(float value) const;
        float snapToStepClamp(float value) const;

        bool operator==(const Options& other) const { return this == &other; }

    private:
        Options(int ordinal, std::string enumString, bool isFloat, bool isBoolean, float valueMin = 0.0f, float valueMax = 1.0f, float valueStep = 0.0f);
        float snapToStep(float value) const;

        int ordinal;
        std::string enumString;
        bool enumFloat;
        bool enumBoolean;
        float valueStep;
        float valueMin;
        float valueMax;
    };

    GameSettings(Minecraft* mc, const std::filesystem::path& optionsDir);
    GameSettings();

    GameSettings(const GameSettings&) = delete;
    GameSettings& operator=(const GameSettings&) = delete;

    static std::string getKeyDisplayString(int key);
    static bool isKeyDown(const KeyBinding& key);

    void setOptionKeyBinding(KeyBinding& key, int keyCode);
    void setOptionFloatValue(const Options& option, float value);
    void setOptionValue(const Options& option, int value);
    float getOptionFloatValue(const Options& option) const;
    bool getOptionOrdinalValue(const Options& option) const;
    std::string getKeyBinding(const Options& option) const;
    void loadOptions();
    void saveOptions();
    float getSoundLevel(SoundCategory category) const;
    void setSoundLevel(SoundCategory category, float soundLevel);
    void sendSettingsToServer();
    std::set<EnumPlayerModelParts> getModelParts() const { return setModelParts; }
    void setModelPartEnabled(EnumPlayerModelParts modelPart, bool enable);
    void switchModelPartEnabled(EnumPlayerModelParts modelPart);
    int shouldRenderClouds() const { return renderDistanceChunks >= 4 ? clouds : 0; }
    bool isUsingNativeTransport() const { return useNativeTransport; }

    float mouseSensitivity = 0.5f;
    bool invertMouse = false;
    int renderDistanceChunks = -1;
    bool viewBobbing = true;
    bool anaglyph = false;
    bool fboEnable = true;
    int limitFramerate = 120;
    int clouds = 2;
    bool fancyGraphics = true;
    int ambientOcclusion = 2;
    std::vector<std::string> resourcePacks;
    std::vector<std::string> incompatibleResourcePacks;
    EnumChatVisibility chatVisibility = EnumChatVisibility::FULL;
    bool chatColours = true;
    bool chatLinks = true;
    bool chatLinksPrompt = true;
    float chatOpacity = 1.0f;
    bool snooperEnabled = true;
    bool fullScreen = false;
    bool enableVsync = true;
    bool useVbo = false;
    bool allowBlockAlternatives = true;
    bool reducedDebugInfo = false;
    bool hideServerAddress = false;
    bool advancedItemTooltips = false;
    bool pauseOnLostFocus = true;
    bool touchscreen = false;
    int overrideWidth = 0;
    int overrideHeight = 0;
    bool heldItemTooltips = true;
    float chatScale = 1.0f;
    float chatWidth = 1.0f;
    float chatHeightUnfocused = 0.44366196f;
    float chatHeightFocused = 1.0f;
    bool showInventoryAchievementHint = true;
    int mipmapLevels = 4;
    float streamBytesPerPixel = 0.5f;
    float streamMicVolume = 1.0f;
    float streamGameVolume = 1.0f;
    float streamKbps = 0.5412844f;
    float streamFps = 0.31690142f;
    int streamCompression = 1;
    bool streamSendMetadata = true;
    std::string streamPreferredServer;
    int streamChatEnabled = 0;
    int streamChatUserFilter = 0;
    int streamMicToggleBehavior = 0;
    bool useNativeTransport = true;
    bool entityShadows = true;
    bool realmsNotifications = true;

    KeyBinding keyBindForward { "key.forward", 17, "key.categories.movement" };
    KeyBinding keyBindLeft { "key.left", 30, "key.categories.movement" };
    KeyBinding keyBindBack { "key.back", 31, "key.categories.movement" };
    KeyBinding keyBindRight { "key.right", 32, "key.categories.movement" };
    KeyBinding keyBindJump { "key.jump", 57, "key.categories.movement" };
    KeyBinding keyBindSneak { "key.sneak", 42, "key.categories.movement" };
    KeyBinding keyBindSprint { "key.sprint", 29, "key.categories.movement" };
    KeyBinding keyBindInventory { "key.inventory", 18, "key.categories.inventory" };
    KeyBinding keyBindUseItem { "key.use", -99, "key.categories.gameplay" };
    KeyBinding keyBindDrop { "key.drop", 16, "key.categories.gameplay" };
    KeyBinding keyBindAttack { "key.attack", -100, "key.categories.gameplay" };
    KeyBinding keyBindPickBlock { "key.pickItem", -98, "key.categories.gameplay" };
    KeyBinding keyBindChat { "key.chat", 20, "key.categories.multiplayer" };
    KeyBinding keyBindPlayerList { "key.playerlist", 15, "key.categories.multiplayer" };
    KeyBinding keyBindCommand { "key.command", 53, "key.categories.multiplayer" };
    KeyBinding keyBindScreenshot { "key.screenshot", 60, "key.categories.misc" };
    KeyBinding keyBindTogglePerspective { "key.togglePerspective", 63, "key.categories.misc" };
    KeyBinding keyBindSmoothCamera { "key.smoothCamera", 0, "key.categories.misc" };
    KeyBinding keyBindFullscreen { "key.fullscreen", 87, "key.categories.misc" };
    KeyBinding keyBindSpectatorOutlines { "key.spectatorOutlines", 0, "key.categories.misc" };
    KeyBinding keyBindStreamStartStop { "key.streamStartStop", 64, "key.categories.stream" };
    KeyBinding keyBindStreamPauseUnpause { "key.streamPauseUnpause", 65, "key.categories.stream" };
    KeyBinding keyBindStreamCommercials { "key.streamCommercial", 0, "key.categories.stream" };
    KeyBinding keyBindStreamToggleMic { "key.streamToggleMic", 0, "key.categories.stream" };
    KeyBinding keyBindsHotbar[9] {
        { "key.hotbar.1", 2, "key.categories.inventory" }, { "key.hotbar.2", 3, "key.categories.inventory" },
        { "key.hotbar.3", 4, "key.categories.inventory" }, { "key.hotbar.4", 5, "key.categories.inventory" },
        { "key.hotbar.5", 6, "key.categories.inventory" }, { "key.hotbar.6", 7, "key.categories.inventory" },
        { "key.hotbar.7", 8, "key.categories.inventory" }, { "key.hotbar.8", 9, "key.categories.inventory" },
        { "key.hotbar.9", 10, "key.categories.inventory" }
    };
    std::vector<KeyBinding*> keyBindings;

    EnumDifficulty difficulty = EnumDifficulty::NORMAL;
    bool hideGUI = false;
    int thirdPersonView = 0;
    bool showDebugInfo = false;
    bool showDebugProfilerChart = false;
    bool showLagometer = false;
    std::string lastServer;
    bool smoothCamera = false;
    bool debugCamEnable = false;
    float fovSetting = 70.0f;
    float gammaSetting = 0.0f;
    float saturation = 0.0f;
    int guiScale = 0;
    int particleSetting = 0;
    std::string language = "en_US";
    bool forceUnicodeFont = false;

private:
    void initKeyBindings();
    static float parseFloat(const std::string& text);

    Minecraft* mc = nullptr;
    std::filesystem::path optionsFile;
    std::set<EnumPlayerModelParts> setModelParts { kPlayerModelParts.begin(), kPlayerModelParts.end() };
    std::map<SoundCategory, float> mapSoundLevels;
};

} // namespace mc
