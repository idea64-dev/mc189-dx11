#include "client/settings/GameSettings.h"

#include "client/Minecraft.h"
#include "client/resources/Locale.h"
#include "core/Java.h"
#include "core/JavaString.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "platform/Display.h"
#include "platform/Keyboard.h"
#include "platform/Mouse.h"
#include "util/MathHelper.h"

#include <Windows.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <format>
#include <fstream>
#include <sstream>

namespace mc {

namespace {

const char* const GUISCALES[] = { "options.guiScale.auto", "options.guiScale.small", "options.guiScale.normal", "options.guiScale.large" };
const char* const PARTICLES[] = { "options.particles.all", "options.particles.decreased", "options.particles.minimal" };
const char* const AMBIENT_OCCLUSIONS[] = { "options.ao.off", "options.ao.min", "options.ao.max" };
const char* const STREAM_COMPRESSIONS[] = { "options.stream.compression.low", "options.stream.compression.medium", "options.stream.compression.high" };
const char* const STREAM_CHAT_MODES[] = { "options.stream.chat.enabled.streaming", "options.stream.chat.enabled.always", "options.stream.chat.enabled.never" };
const char* const STREAM_CHAT_FILTER_MODES[] = { "options.stream.chat.userFilter.all", "options.stream.chat.userFilter.subs", "options.stream.chat.userFilter.mods" };
const char* const STREAM_MIC_MODES[] = { "options.stream.mic_toggle.mute", "options.stream.mic_toggle.talk" };
const char* const CLOUDS_TYPES[] = { "options.off", "options.graphics.fast", "options.graphics.fancy" };

template <size_t N>
std::string getTranslation(const char* const (&strings)[N], int index)
{
    if (index < 0 || index >= static_cast<int>(N))
        index = 0;
    return I18n::format(strings[index]);
}

// Math.round(float): round half up, computed exactly.
int javaRound(float value)
{
    return java::d2i(std::floor(static_cast<double>(value) + 0.5));
}

// String.split(":") -- trailing empty strings are removed.
std::vector<std::string> splitColon(const std::string& s)
{
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t colon = s.find(':', start);
        parts.push_back(s.substr(start, colon == std::string::npos ? std::string::npos : colon - start));
        if (colon == std::string::npos)
            break;
        start = colon + 1;
    }
    while (!parts.empty() && parts.back().empty())
        parts.pop_back();
    if (parts.empty())
        parts.push_back(""); // "".split(":") is [""]
    return parts;
}

// Gson.toJson(List<String>): compact, with Gson's HTML-safe escaping.
std::string gsonStringList(const std::vector<std::string>& list)
{
    std::string out = "[";
    for (size_t i = 0; i < list.size(); ++i) {
        if (i > 0)
            out += ",";
        out += "\"";
        for (const unsigned char c : list[i]) {
            switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\f': out += "\\f"; break;
            case '<': case '>': case '&': case '=': case '\'':
                out += std::format("\\u{:04x}", c);
                break;
            default:
                if (c < 0x20)
                    out += std::format("\\u{:04x}", c);
                else
                    out += static_cast<char>(c);
            }
        }
        out += "\"";
    }
    return out + "]";
}

std::vector<std::string> gsonParseStringList(const std::string& json)
{
    const nlohmann::json parsed = nlohmann::json::parse(json);
    std::vector<std::string> list;
    if (parsed.is_null())
        return list;
    for (const auto& element : parsed)
        list.push_back(element.is_null() ? std::string() : element.get<std::string>());
    return list;
}

// Java's FileReader/FileWriter use the platform charset (the ANSI code page on Windows).
std::string fromAnsi(const std::string& text)
{
    if (text.empty())
        return text;
    const int length = MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
    return toUtf8(wide);
}

std::string toAnsi(const std::string& utf8)
{
    const std::wstring wide = toWide(utf8);
    if (wide.empty())
        return {};
    const int length = WideCharToMultiByte(CP_ACP, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, "?", nullptr);
    std::string out(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_ACP, 0, wide.data(), static_cast<int>(wide.size()), out.data(), length, "?", nullptr);
    return out;
}

} // namespace

// --- Options ---

GameSettings::Options GameSettings::Options::INVERT_MOUSE { 0, "options.invertMouse", false, true };
GameSettings::Options GameSettings::Options::SENSITIVITY { 1, "options.sensitivity", true, false };
GameSettings::Options GameSettings::Options::FOV { 2, "options.fov", true, false, 30.0f, 110.0f, 1.0f };
GameSettings::Options GameSettings::Options::GAMMA { 3, "options.gamma", true, false };
GameSettings::Options GameSettings::Options::SATURATION { 4, "options.saturation", true, false };
GameSettings::Options GameSettings::Options::RENDER_DISTANCE { 5, "options.renderDistance", true, false, 2.0f, 16.0f, 1.0f };
GameSettings::Options GameSettings::Options::VIEW_BOBBING { 6, "options.viewBobbing", false, true };
GameSettings::Options GameSettings::Options::ANAGLYPH { 7, "options.anaglyph", false, true };
GameSettings::Options GameSettings::Options::FRAMERATE_LIMIT { 8, "options.framerateLimit", true, false, 10.0f, 260.0f, 10.0f };
GameSettings::Options GameSettings::Options::FBO_ENABLE { 9, "options.fboEnable", false, true };
GameSettings::Options GameSettings::Options::RENDER_CLOUDS { 10, "options.renderClouds", false, false };
GameSettings::Options GameSettings::Options::GRAPHICS { 11, "options.graphics", false, false };
GameSettings::Options GameSettings::Options::AMBIENT_OCCLUSION { 12, "options.ao", false, false };
GameSettings::Options GameSettings::Options::GUI_SCALE { 13, "options.guiScale", false, false };
GameSettings::Options GameSettings::Options::PARTICLES { 14, "options.particles", false, false };
GameSettings::Options GameSettings::Options::CHAT_VISIBILITY { 15, "options.chat.visibility", false, false };
GameSettings::Options GameSettings::Options::CHAT_COLOR { 16, "options.chat.color", false, true };
GameSettings::Options GameSettings::Options::CHAT_LINKS { 17, "options.chat.links", false, true };
GameSettings::Options GameSettings::Options::CHAT_OPACITY { 18, "options.chat.opacity", true, false };
GameSettings::Options GameSettings::Options::CHAT_LINKS_PROMPT { 19, "options.chat.links.prompt", false, true };
GameSettings::Options GameSettings::Options::SNOOPER_ENABLED { 20, "options.snooper", false, true };
GameSettings::Options GameSettings::Options::USE_FULLSCREEN { 21, "options.fullscreen", false, true };
GameSettings::Options GameSettings::Options::ENABLE_VSYNC { 22, "options.vsync", false, true };
GameSettings::Options GameSettings::Options::USE_VBO { 23, "options.vbo", false, true };
GameSettings::Options GameSettings::Options::TOUCHSCREEN { 24, "options.touchscreen", false, true };
GameSettings::Options GameSettings::Options::CHAT_SCALE { 25, "options.chat.scale", true, false };
GameSettings::Options GameSettings::Options::CHAT_WIDTH { 26, "options.chat.width", true, false };
GameSettings::Options GameSettings::Options::CHAT_HEIGHT_FOCUSED { 27, "options.chat.height.focused", true, false };
GameSettings::Options GameSettings::Options::CHAT_HEIGHT_UNFOCUSED { 28, "options.chat.height.unfocused", true, false };
GameSettings::Options GameSettings::Options::MIPMAP_LEVELS { 29, "options.mipmapLevels", true, false, 0.0f, 4.0f, 1.0f };
GameSettings::Options GameSettings::Options::FORCE_UNICODE_FONT { 30, "options.forceUnicodeFont", false, true };
GameSettings::Options GameSettings::Options::STREAM_BYTES_PER_PIXEL { 31, "options.stream.bytesPerPixel", true, false };
GameSettings::Options GameSettings::Options::STREAM_VOLUME_MIC { 32, "options.stream.micVolumne", true, false };
GameSettings::Options GameSettings::Options::STREAM_VOLUME_SYSTEM { 33, "options.stream.systemVolume", true, false };
GameSettings::Options GameSettings::Options::STREAM_KBPS { 34, "options.stream.kbps", true, false };
GameSettings::Options GameSettings::Options::STREAM_FPS { 35, "options.stream.fps", true, false };
GameSettings::Options GameSettings::Options::STREAM_COMPRESSION { 36, "options.stream.compression", false, false };
GameSettings::Options GameSettings::Options::STREAM_SEND_METADATA { 37, "options.stream.sendMetadata", false, true };
GameSettings::Options GameSettings::Options::STREAM_CHAT_ENABLED { 38, "options.stream.chat.enabled", false, false };
GameSettings::Options GameSettings::Options::STREAM_CHAT_USER_FILTER { 39, "options.stream.chat.userFilter", false, false };
GameSettings::Options GameSettings::Options::STREAM_MIC_TOGGLE_BEHAVIOR { 40, "options.stream.micToggleBehavior", false, false };
GameSettings::Options GameSettings::Options::BLOCK_ALTERNATIVES { 41, "options.blockAlternatives", false, true };
GameSettings::Options GameSettings::Options::REDUCED_DEBUG_INFO { 42, "options.reducedDebugInfo", false, true };
GameSettings::Options GameSettings::Options::ENTITY_SHADOWS { 43, "options.entityShadows", false, true };
GameSettings::Options GameSettings::Options::REALMS_NOTIFICATIONS { 44, "options.realmsNotifications", false, true };

GameSettings::Options::Options(int ordinalIn, std::string str, bool isFloat, bool isBoolean, float valMin, float valMax, float valStep)
    : ordinal(ordinalIn)
    , enumString(std::move(str))
    , enumFloat(isFloat)
    , enumBoolean(isBoolean)
    , valueStep(valStep)
    , valueMin(valMin)
    , valueMax(valMax)
{
}

const std::vector<GameSettings::Options*>& GameSettings::Options::values()
{
    static const std::vector<Options*> all { &INVERT_MOUSE, &SENSITIVITY, &FOV, &GAMMA, &SATURATION, &RENDER_DISTANCE,
        &VIEW_BOBBING, &ANAGLYPH, &FRAMERATE_LIMIT, &FBO_ENABLE, &RENDER_CLOUDS, &GRAPHICS, &AMBIENT_OCCLUSION, &GUI_SCALE,
        &PARTICLES, &CHAT_VISIBILITY, &CHAT_COLOR, &CHAT_LINKS, &CHAT_OPACITY, &CHAT_LINKS_PROMPT, &SNOOPER_ENABLED,
        &USE_FULLSCREEN, &ENABLE_VSYNC, &USE_VBO, &TOUCHSCREEN, &CHAT_SCALE, &CHAT_WIDTH, &CHAT_HEIGHT_FOCUSED,
        &CHAT_HEIGHT_UNFOCUSED, &MIPMAP_LEVELS, &FORCE_UNICODE_FONT, &STREAM_BYTES_PER_PIXEL, &STREAM_VOLUME_MIC,
        &STREAM_VOLUME_SYSTEM, &STREAM_KBPS, &STREAM_FPS, &STREAM_COMPRESSION, &STREAM_SEND_METADATA,
        &STREAM_CHAT_ENABLED, &STREAM_CHAT_USER_FILTER, &STREAM_MIC_TOGGLE_BEHAVIOR, &BLOCK_ALTERNATIVES,
        &REDUCED_DEBUG_INFO, &ENTITY_SHADOWS, &REALMS_NOTIFICATIONS };
    return all;
}

GameSettings::Options* GameSettings::Options::getEnumOptions(int ordinal)
{
    for (Options* option : values()) {
        if (option->returnEnumOrdinal() == ordinal)
            return option;
    }
    return nullptr;
}

float GameSettings::Options::normalizeValue(float value) const
{
    return MathHelper::clamp_float((snapToStepClamp(value) - valueMin) / (valueMax - valueMin), 0.0f, 1.0f);
}

float GameSettings::Options::denormalizeValue(float value) const
{
    return snapToStepClamp(valueMin + (valueMax - valueMin) * MathHelper::clamp_float(value, 0.0f, 1.0f));
}

float GameSettings::Options::snapToStepClamp(float value) const
{
    value = snapToStep(value);
    return MathHelper::clamp_float(value, valueMin, valueMax);
}

float GameSettings::Options::snapToStep(float value) const
{
    if (valueStep > 0.0f)
        value = valueStep * static_cast<float>(javaRound(value / valueStep));
    return value;
}

// --- GameSettings ---

GameSettings::GameSettings(Minecraft* mcIn, const std::filesystem::path& optionsDir)
    : mc(mcIn)
    , optionsFile(optionsDir / "options.txt")
{
    initKeyBindings();
    // Java: 32 chunks when running on a 64-bit JVM with at least 1 GB of heap, which is always true here.
    Options::RENDER_DISTANCE.setValueMax(32.0f);
    renderDistanceChunks = 12;
    loadOptions();
}

GameSettings::GameSettings()
{
    initKeyBindings();
}

void GameSettings::initKeyBindings()
{
    keyBindings = { &keyBindAttack, &keyBindUseItem, &keyBindForward, &keyBindLeft, &keyBindBack, &keyBindRight,
        &keyBindJump, &keyBindSneak, &keyBindSprint, &keyBindDrop, &keyBindInventory, &keyBindChat, &keyBindPlayerList,
        &keyBindPickBlock, &keyBindCommand, &keyBindScreenshot, &keyBindTogglePerspective, &keyBindSmoothCamera,
        &keyBindStreamStartStop, &keyBindStreamPauseUnpause, &keyBindStreamCommercials, &keyBindStreamToggleMic,
        &keyBindFullscreen, &keyBindSpectatorOutlines };
    for (KeyBinding& hotbar : keyBindsHotbar)
        keyBindings.push_back(&hotbar);
}

std::string GameSettings::getKeyDisplayString(int key)
{
    if (key < 0)
        return I18n::format("key.mouseButton", key + 101);
    if (key < 256)
        return std::string(Keyboard::getKeyName(key));

    // String.format("%c", (char)(key - 256)).toUpperCase()
    wchar_t c[2] = { static_cast<wchar_t>(static_cast<char16_t>(key - 256)), 0 };
    CharUpperBuffW(c, 1);
    return toUtf8(std::wstring_view(c, 1));
}

bool GameSettings::isKeyDown(const KeyBinding& key)
{
    const int code = key.getKeyCode();
    return code == 0 ? false : (code < 0 ? Mouse::isButtonDown(code + 100) : Keyboard::isKeyDown(code));
}

void GameSettings::setOptionKeyBinding(KeyBinding& key, int keyCode)
{
    key.setKeyCode(keyCode);
    saveOptions();
}

void GameSettings::setOptionFloatValue(const Options& option, float value)
{
    if (option == Options::SENSITIVITY)
        mouseSensitivity = value;
    if (option == Options::FOV)
        fovSetting = value;
    if (option == Options::GAMMA)
        gammaSetting = value;
    if (option == Options::FRAMERATE_LIMIT)
        limitFramerate = java::f2i(value);
    // Chat options also refresh the chat (stage 8).
    if (option == Options::CHAT_OPACITY)
        chatOpacity = value;
    if (option == Options::CHAT_HEIGHT_FOCUSED)
        chatHeightFocused = value;
    if (option == Options::CHAT_HEIGHT_UNFOCUSED)
        chatHeightUnfocused = value;
    if (option == Options::CHAT_WIDTH)
        chatWidth = value;
    if (option == Options::CHAT_SCALE)
        chatScale = value;
    if (option == Options::MIPMAP_LEVELS) {
        // Java also rebuilds the block atlas mipmaps (stage 7).
        mipmapLevels = java::f2i(value);
    }
    if (option == Options::BLOCK_ALTERNATIVES)
        allowBlockAlternatives = !allowBlockAlternatives; // + renderGlobal.loadRenderers() (stage 7)
    if (option == Options::RENDER_DISTANCE)
        renderDistanceChunks = java::f2i(value); // + renderGlobal.setDisplayListEntitiesDirty() (stage 7)
    if (option == Options::STREAM_BYTES_PER_PIXEL)
        streamBytesPerPixel = value;
    if (option == Options::STREAM_VOLUME_MIC)
        streamMicVolume = value;
    if (option == Options::STREAM_VOLUME_SYSTEM)
        streamGameVolume = value;
    if (option == Options::STREAM_KBPS)
        streamKbps = value;
    if (option == Options::STREAM_FPS)
        streamFps = value;
}

void GameSettings::setOptionValue(const Options& option, int value)
{
    if (option == Options::INVERT_MOUSE)
        invertMouse = !invertMouse;
    if (option == Options::GUI_SCALE)
        guiScale = (guiScale + value) & 3;
    if (option == Options::PARTICLES)
        particleSetting = (particleSetting + value) % 3;
    if (option == Options::VIEW_BOBBING)
        viewBobbing = !viewBobbing;
    if (option == Options::RENDER_CLOUDS)
        clouds = (clouds + value) % 3;
    if (option == Options::FORCE_UNICODE_FONT) {
        forceUnicodeFont = !forceUnicodeFont;
        if (mc)
            mc->updateFontUnicodeFlag();
    }
    if (option == Options::FBO_ENABLE)
        fboEnable = !fboEnable;
    if (option == Options::ANAGLYPH) {
        anaglyph = !anaglyph;
        if (mc)
            mc->refreshResources();
    }
    if (option == Options::GRAPHICS)
        fancyGraphics = !fancyGraphics; // + renderGlobal.loadRenderers() (stage 7)
    if (option == Options::AMBIENT_OCCLUSION)
        ambientOcclusion = (ambientOcclusion + value) % 3; // + renderGlobal.loadRenderers() (stage 7)
    if (option == Options::CHAT_VISIBILITY)
        chatVisibility = getEnumChatVisibility((getChatVisibility(chatVisibility) + value) % 3);
    if (option == Options::STREAM_COMPRESSION)
        streamCompression = (streamCompression + value) % 3;
    if (option == Options::STREAM_SEND_METADATA)
        streamSendMetadata = !streamSendMetadata;
    if (option == Options::STREAM_CHAT_ENABLED)
        streamChatEnabled = (streamChatEnabled + value) % 3;
    if (option == Options::STREAM_CHAT_USER_FILTER)
        streamChatUserFilter = (streamChatUserFilter + value) % 3;
    if (option == Options::STREAM_MIC_TOGGLE_BEHAVIOR)
        streamMicToggleBehavior = (streamMicToggleBehavior + value) % 2;
    if (option == Options::CHAT_COLOR)
        chatColours = !chatColours;
    if (option == Options::CHAT_LINKS)
        chatLinks = !chatLinks;
    if (option == Options::CHAT_LINKS_PROMPT)
        chatLinksPrompt = !chatLinksPrompt;
    if (option == Options::SNOOPER_ENABLED)
        snooperEnabled = !snooperEnabled;
    if (option == Options::TOUCHSCREEN)
        touchscreen = !touchscreen;
    if (option == Options::USE_FULLSCREEN) {
        fullScreen = !fullScreen;
        if (mc && mc->isFullScreen() != fullScreen)
            mc->toggleFullscreen();
    }
    if (option == Options::ENABLE_VSYNC) {
        enableVsync = !enableVsync;
        Display::setVSyncEnabled(enableVsync);
    }
    if (option == Options::USE_VBO)
        useVbo = !useVbo; // + renderGlobal.loadRenderers() (stage 7)
    if (option == Options::BLOCK_ALTERNATIVES)
        allowBlockAlternatives = !allowBlockAlternatives; // + renderGlobal.loadRenderers() (stage 7)
    if (option == Options::REDUCED_DEBUG_INFO)
        reducedDebugInfo = !reducedDebugInfo;
    if (option == Options::ENTITY_SHADOWS)
        entityShadows = !entityShadows;
    if (option == Options::REALMS_NOTIFICATIONS)
        realmsNotifications = !realmsNotifications;

    saveOptions();
}

float GameSettings::getOptionFloatValue(const Options& option) const
{
    if (option == Options::FOV) return fovSetting;
    if (option == Options::GAMMA) return gammaSetting;
    if (option == Options::SATURATION) return saturation;
    if (option == Options::SENSITIVITY) return mouseSensitivity;
    if (option == Options::CHAT_OPACITY) return chatOpacity;
    if (option == Options::CHAT_HEIGHT_FOCUSED) return chatHeightFocused;
    if (option == Options::CHAT_HEIGHT_UNFOCUSED) return chatHeightUnfocused;
    if (option == Options::CHAT_SCALE) return chatScale;
    if (option == Options::CHAT_WIDTH) return chatWidth;
    if (option == Options::FRAMERATE_LIMIT) return static_cast<float>(limitFramerate);
    if (option == Options::MIPMAP_LEVELS) return static_cast<float>(mipmapLevels);
    if (option == Options::RENDER_DISTANCE) return static_cast<float>(renderDistanceChunks);
    if (option == Options::STREAM_BYTES_PER_PIXEL) return streamBytesPerPixel;
    if (option == Options::STREAM_VOLUME_MIC) return streamMicVolume;
    if (option == Options::STREAM_VOLUME_SYSTEM) return streamGameVolume;
    if (option == Options::STREAM_KBPS) return streamKbps;
    if (option == Options::STREAM_FPS) return streamFps;
    return 0.0f;
}

bool GameSettings::getOptionOrdinalValue(const Options& option) const
{
    if (option == Options::INVERT_MOUSE) return invertMouse;
    if (option == Options::VIEW_BOBBING) return viewBobbing;
    if (option == Options::ANAGLYPH) return anaglyph;
    if (option == Options::FBO_ENABLE) return fboEnable;
    if (option == Options::CHAT_COLOR) return chatColours;
    if (option == Options::CHAT_LINKS) return chatLinks;
    if (option == Options::CHAT_LINKS_PROMPT) return chatLinksPrompt;
    if (option == Options::SNOOPER_ENABLED) return snooperEnabled;
    if (option == Options::USE_FULLSCREEN) return fullScreen;
    if (option == Options::ENABLE_VSYNC) return enableVsync;
    if (option == Options::USE_VBO) return useVbo;
    if (option == Options::TOUCHSCREEN) return touchscreen;
    if (option == Options::STREAM_SEND_METADATA) return streamSendMetadata;
    if (option == Options::FORCE_UNICODE_FONT) return forceUnicodeFont;
    if (option == Options::BLOCK_ALTERNATIVES) return allowBlockAlternatives;
    if (option == Options::REDUCED_DEBUG_INFO) return reducedDebugInfo;
    if (option == Options::ENTITY_SHADOWS) return entityShadows;
    if (option == Options::REALMS_NOTIFICATIONS) return realmsNotifications;
    return false;
}

std::string GameSettings::getKeyBinding(const Options& option) const
{
    const std::string s = I18n::format(option.getEnumString()) + ": ";

    if (option.getEnumFloat()) {
        const float f1 = getOptionFloatValue(option);
        const float f = option.normalizeValue(f1);
        const auto i = [](float v) { return java::toString(java::f2i(v)); };

        if (option == Options::SENSITIVITY)
            return f == 0.0f ? s + I18n::format("options.sensitivity.min") : (f == 1.0f ? s + I18n::format("options.sensitivity.max") : s + i(f * 200.0f) + "%");
        if (option == Options::FOV)
            return f1 == 70.0f ? s + I18n::format("options.fov.min") : (f1 == 110.0f ? s + I18n::format("options.fov.max") : s + i(f1));
        if (option == Options::FRAMERATE_LIMIT)
            return f1 == option.getValueMax() ? s + I18n::format("options.framerateLimit.max") : s + i(f1) + " fps";
        if (option == Options::RENDER_CLOUDS)
            return f1 == option.getValueMin() ? s + I18n::format("options.cloudHeight.min") : s + java::toString(java::f2i(f1) + 128);
        if (option == Options::GAMMA)
            return f == 0.0f ? s + I18n::format("options.gamma.min") : (f == 1.0f ? s + I18n::format("options.gamma.max") : s + "+" + i(f * 100.0f) + "%");
        if (option == Options::SATURATION)
            return s + i(f * 400.0f) + "%";
        if (option == Options::CHAT_OPACITY)
            return s + i(f * 90.0f + 10.0f) + "%";
        if (option == Options::CHAT_HEIGHT_UNFOCUSED || option == Options::CHAT_HEIGHT_FOCUSED)
            return s + java::toString(MathHelper::floor_float(f * 160.0f + 20.0f)) + "px"; // GuiNewChat.calculateChatboxHeight
        if (option == Options::CHAT_WIDTH)
            return s + java::toString(MathHelper::floor_float(f * 280.0f + 40.0f)) + "px"; // GuiNewChat.calculateChatboxWidth
        if (option == Options::RENDER_DISTANCE)
            return s + i(f1) + " chunks";
        if (option == Options::MIPMAP_LEVELS)
            return f1 == 0.0f ? s + I18n::format("options.off") : s + i(f1);
        if (option == Options::STREAM_FPS || option == Options::STREAM_KBPS || option == Options::STREAM_BYTES_PER_PIXEL)
            return s; // Twitch streaming is not ported
        return f == 0.0f ? s + I18n::format("options.off") : s + i(f * 100.0f) + "%";
    }
    if (option.getEnumBoolean())
        return getOptionOrdinalValue(option) ? s + I18n::format("options.on") : s + I18n::format("options.off");
    if (option == Options::GUI_SCALE)
        return s + getTranslation(GUISCALES, guiScale);
    if (option == Options::CHAT_VISIBILITY)
        return s + I18n::format(std::string(getResourceKey(chatVisibility)));
    if (option == Options::PARTICLES)
        return s + getTranslation(PARTICLES, particleSetting);
    if (option == Options::AMBIENT_OCCLUSION)
        return s + getTranslation(AMBIENT_OCCLUSIONS, ambientOcclusion);
    if (option == Options::STREAM_COMPRESSION)
        return s + getTranslation(STREAM_COMPRESSIONS, streamCompression);
    if (option == Options::STREAM_CHAT_ENABLED)
        return s + getTranslation(STREAM_CHAT_MODES, streamChatEnabled);
    if (option == Options::STREAM_CHAT_USER_FILTER)
        return s + getTranslation(STREAM_CHAT_FILTER_MODES, streamChatUserFilter);
    if (option == Options::STREAM_MIC_TOGGLE_BEHAVIOR)
        return s + getTranslation(STREAM_MIC_MODES, streamMicToggleBehavior);
    if (option == Options::RENDER_CLOUDS)
        return s + getTranslation(CLOUDS_TYPES, clouds);
    if (option == Options::GRAPHICS)
        return s + I18n::format(fancyGraphics ? "options.graphics.fancy" : "options.graphics.fast");
    return s;
}

float GameSettings::parseFloat(const std::string& text)
{
    return text == "true" ? 1.0f : (text == "false" ? 0.0f : java::parseFloat(text));
}

void GameSettings::loadOptions()
{
    try {
        std::error_code ec;
        if (!std::filesystem::exists(optionsFile, ec))
            return;

        std::ifstream stream(optionsFile, std::ios::binary);
        if (!stream)
            throw std::runtime_error("can't open options.txt");
        std::stringstream buffer;
        buffer << stream.rdbuf();
        const std::string contents = fromAnsi(buffer.str());

        mapSoundLevels.clear();

        // BufferedReader.readLine: lines end with \n, \r\n or \r.
        size_t start = 0;
        while (start < contents.size()) {
            size_t end = contents.find_first_of("\r\n", start);
            const std::string s = contents.substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (end == std::string::npos)
                start = contents.size();
            else
                start = end + ((contents[end] == '\r' && end + 1 < contents.size() && contents[end + 1] == '\n') ? 2 : 1);

            try {
                const std::vector<std::string> astring = splitColon(s);
                const std::string& key = astring[0];
                const auto value = [&]() -> const std::string& {
                    if (astring.size() < 2)
                        throw std::out_of_range("missing value"); // Java: ArrayIndexOutOfBoundsException
                    return astring[1];
                };
                const auto afterColon = [&] { return s.substr(s.find(':') + 1); };

                if (key == "mouseSensitivity") mouseSensitivity = parseFloat(value());
                if (key == "fov") fovSetting = parseFloat(value()) * 40.0f + 70.0f;
                if (key == "gamma") gammaSetting = parseFloat(value());
                if (key == "saturation") saturation = parseFloat(value());
                if (key == "invertYMouse") invertMouse = value() == "true";
                if (key == "renderDistance") renderDistanceChunks = java::parseInt(value());
                if (key == "guiScale") guiScale = java::parseInt(value());
                if (key == "particles") particleSetting = java::parseInt(value());
                if (key == "bobView") viewBobbing = value() == "true";
                if (key == "anaglyph3d") anaglyph = value() == "true";
                if (key == "maxFps") limitFramerate = java::parseInt(value());
                if (key == "fboEnable") fboEnable = value() == "true";
                if (key == "difficulty") difficulty = getDifficultyEnum(java::parseInt(value()));
                if (key == "fancyGraphics") fancyGraphics = value() == "true";
                if (key == "ao") {
                    if (value() == "true")
                        ambientOcclusion = 2;
                    else if (value() == "false")
                        ambientOcclusion = 0;
                    else
                        ambientOcclusion = java::parseInt(value());
                }
                if (key == "renderClouds") {
                    if (value() == "true")
                        clouds = 2;
                    else if (value() == "false")
                        clouds = 0;
                    else if (value() == "fast")
                        clouds = 1;
                }
                if (key == "resourcePacks") resourcePacks = gsonParseStringList(afterColon());
                if (key == "incompatibleResourcePacks") incompatibleResourcePacks = gsonParseStringList(afterColon());
                if (key == "lastServer" && astring.size() >= 2) lastServer = afterColon();
                if (key == "lang" && astring.size() >= 2) language = astring[1];
                if (key == "chatVisibility") chatVisibility = getEnumChatVisibility(java::parseInt(value()));
                if (key == "chatColors") chatColours = value() == "true";
                if (key == "chatLinks") chatLinks = value() == "true";
                if (key == "chatLinksPrompt") chatLinksPrompt = value() == "true";
                if (key == "chatOpacity") chatOpacity = parseFloat(value());
                if (key == "snooperEnabled") snooperEnabled = value() == "true";
                if (key == "fullscreen") fullScreen = value() == "true";
                if (key == "enableVsync") enableVsync = value() == "true";
                if (key == "useVbo") useVbo = value() == "true";
                if (key == "hideServerAddress") hideServerAddress = value() == "true";
                if (key == "advancedItemTooltips") advancedItemTooltips = value() == "true";
                if (key == "pauseOnLostFocus") pauseOnLostFocus = value() == "true";
                if (key == "touchscreen") touchscreen = value() == "true";
                if (key == "overrideHeight") overrideHeight = java::parseInt(value());
                if (key == "overrideWidth") overrideWidth = java::parseInt(value());
                if (key == "heldItemTooltips") heldItemTooltips = value() == "true";
                if (key == "chatHeightFocused") chatHeightFocused = parseFloat(value());
                if (key == "chatHeightUnfocused") chatHeightUnfocused = parseFloat(value());
                if (key == "chatScale") chatScale = parseFloat(value());
                if (key == "chatWidth") chatWidth = parseFloat(value());
                if (key == "showInventoryAchievementHint") showInventoryAchievementHint = value() == "true";
                if (key == "mipmapLevels") mipmapLevels = java::parseInt(value());
                if (key == "streamBytesPerPixel") streamBytesPerPixel = parseFloat(value());
                if (key == "streamMicVolume") streamMicVolume = parseFloat(value());
                if (key == "streamSystemVolume") streamGameVolume = parseFloat(value());
                if (key == "streamKbps") streamKbps = parseFloat(value());
                if (key == "streamFps") streamFps = parseFloat(value());
                if (key == "streamCompression") streamCompression = java::parseInt(value());
                if (key == "streamSendMetadata") streamSendMetadata = value() == "true";
                if (key == "streamPreferredServer" && astring.size() >= 2) streamPreferredServer = afterColon();
                if (key == "streamChatEnabled") streamChatEnabled = java::parseInt(value());
                if (key == "streamChatUserFilter") streamChatUserFilter = java::parseInt(value());
                if (key == "streamMicToggleBehavior") streamMicToggleBehavior = java::parseInt(value());
                if (key == "forceUnicodeFont") forceUnicodeFont = value() == "true";
                if (key == "allowBlockAlternatives") allowBlockAlternatives = value() == "true";
                if (key == "reducedDebugInfo") reducedDebugInfo = value() == "true";
                if (key == "useNativeTransport") useNativeTransport = value() == "true";
                if (key == "entityShadows") entityShadows = value() == "true";
                if (key == "realmsNotifications") realmsNotifications = value() == "true";

                for (KeyBinding* keybinding : keyBindings) {
                    if (key == "key_" + keybinding->getKeyDescription())
                        keybinding->setKeyCode(java::parseInt(value()));
                }
                for (const SoundCategory category : kSoundCategories) {
                    if (key == "soundCategory_" + std::string(getCategoryName(category)))
                        mapSoundLevels[category] = parseFloat(value());
                }
                for (const EnumPlayerModelParts part : kPlayerModelParts) {
                    if (key == "modelPart_" + std::string(getPartName(part)))
                        setModelPartEnabled(part, value() == "true");
                }
            } catch (const std::exception&) {
                Log::warn("Skipping bad option: {}", s);
            }
        }
        KeyBinding::resetKeyBindingArrayAndHash();
    } catch (const std::exception& e) {
        Log::error("Failed to load options ({})", e.what());
    }
}

void GameSettings::saveOptions()
{
    if (optionsFile.empty())
        return;

    try {
        std::string out;
        const auto println = [&](const std::string& line) { out += line + "\r\n"; }; // PrintWriter on Windows
        using java::toString;

        println("invertYMouse:" + toString(invertMouse));
        println("mouseSensitivity:" + toString(mouseSensitivity));
        println("fov:" + toString((fovSetting - 70.0f) / 40.0f));
        println("gamma:" + toString(gammaSetting));
        println("saturation:" + toString(saturation));
        println("renderDistance:" + toString(renderDistanceChunks));
        println("guiScale:" + toString(guiScale));
        println("particles:" + toString(particleSetting));
        println("bobView:" + toString(viewBobbing));
        println("anaglyph3d:" + toString(anaglyph));
        println("maxFps:" + toString(limitFramerate));
        println("fboEnable:" + toString(fboEnable));
        println("difficulty:" + toString(getDifficultyId(difficulty)));
        println("fancyGraphics:" + toString(fancyGraphics));
        println("ao:" + toString(ambientOcclusion));
        switch (clouds) {
        case 0: println("renderClouds:false"); break;
        case 1: println("renderClouds:fast"); break;
        case 2: println("renderClouds:true"); break;
        default: break;
        }
        println("resourcePacks:" + gsonStringList(resourcePacks));
        println("incompatibleResourcePacks:" + gsonStringList(incompatibleResourcePacks));
        println("lastServer:" + lastServer);
        println("lang:" + language);
        println("chatVisibility:" + toString(getChatVisibility(chatVisibility)));
        println("chatColors:" + toString(chatColours));
        println("chatLinks:" + toString(chatLinks));
        println("chatLinksPrompt:" + toString(chatLinksPrompt));
        println("chatOpacity:" + toString(chatOpacity));
        println("snooperEnabled:" + toString(snooperEnabled));
        println("fullscreen:" + toString(fullScreen));
        println("enableVsync:" + toString(enableVsync));
        println("useVbo:" + toString(useVbo));
        println("hideServerAddress:" + toString(hideServerAddress));
        println("advancedItemTooltips:" + toString(advancedItemTooltips));
        println("pauseOnLostFocus:" + toString(pauseOnLostFocus));
        println("touchscreen:" + toString(touchscreen));
        println("overrideWidth:" + toString(overrideWidth));
        println("overrideHeight:" + toString(overrideHeight));
        println("heldItemTooltips:" + toString(heldItemTooltips));
        println("chatHeightFocused:" + toString(chatHeightFocused));
        println("chatHeightUnfocused:" + toString(chatHeightUnfocused));
        println("chatScale:" + toString(chatScale));
        println("chatWidth:" + toString(chatWidth));
        println("showInventoryAchievementHint:" + toString(showInventoryAchievementHint));
        println("mipmapLevels:" + toString(mipmapLevels));
        println("streamBytesPerPixel:" + toString(streamBytesPerPixel));
        println("streamMicVolume:" + toString(streamMicVolume));
        println("streamSystemVolume:" + toString(streamGameVolume));
        println("streamKbps:" + toString(streamKbps));
        println("streamFps:" + toString(streamFps));
        println("streamCompression:" + toString(streamCompression));
        println("streamSendMetadata:" + toString(streamSendMetadata));
        println("streamPreferredServer:" + streamPreferredServer);
        println("streamChatEnabled:" + toString(streamChatEnabled));
        println("streamChatUserFilter:" + toString(streamChatUserFilter));
        println("streamMicToggleBehavior:" + toString(streamMicToggleBehavior));
        println("forceUnicodeFont:" + toString(forceUnicodeFont));
        println("allowBlockAlternatives:" + toString(allowBlockAlternatives));
        println("reducedDebugInfo:" + toString(reducedDebugInfo));
        println("useNativeTransport:" + toString(useNativeTransport));
        println("entityShadows:" + toString(entityShadows));
        println("realmsNotifications:" + toString(realmsNotifications));
        for (const KeyBinding* keybinding : keyBindings)
            println("key_" + keybinding->getKeyDescription() + ":" + toString(keybinding->getKeyCode()));
        for (const SoundCategory category : kSoundCategories)
            println("soundCategory_" + std::string(getCategoryName(category)) + ":" + toString(getSoundLevel(category)));
        for (const EnumPlayerModelParts part : kPlayerModelParts)
            println("modelPart_" + std::string(getPartName(part)) + ":" + toString(setModelParts.contains(part)));

        std::ofstream stream(optionsFile, std::ios::binary | std::ios::trunc);
        const std::string encoded = toAnsi(out);
        stream.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
        if (!stream)
            throw std::runtime_error("write failed");
    } catch (const std::exception& e) {
        Log::error("Failed to save options ({})", e.what());
    }

    sendSettingsToServer();
}

float GameSettings::getSoundLevel(SoundCategory category) const
{
    const auto it = mapSoundLevels.find(category);
    return it == mapSoundLevels.end() ? 1.0f : it->second;
}

void GameSettings::setSoundLevel(SoundCategory category, float soundLevel)
{
    // Java: mc.getSoundHandler().setSoundLevel(category, level) (stage 3)
    mapSoundLevels[category] = soundLevel;
}

void GameSettings::sendSettingsToServer()
{
    // Java: C15PacketClientSettings when a player exists (stage 14).
}

void GameSettings::setModelPartEnabled(EnumPlayerModelParts modelPart, bool enable)
{
    if (enable)
        setModelParts.insert(modelPart);
    else
        setModelParts.erase(modelPart);
    sendSettingsToServer();
}

void GameSettings::switchModelPartEnabled(EnumPlayerModelParts modelPart)
{
    if (!setModelParts.contains(modelPart))
        setModelParts.insert(modelPart);
    else
        setModelParts.erase(modelPart);
    sendSettingsToServer();
}

} // namespace mc
