#pragma once

// Original: net/minecraft/client/gui/FontRenderer.java
// Strings are UTF-8 at the API and UTF-16 inside, because the original measures and indexes Java chars
// (surrogate halves included). Right-to-left text goes through ICU like the original (Windows' icu.dll).

#include "client/resources/IResourceManager.h"
#include "util/Random.h"
#include "util/ResourceLocation.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace mc {

class GameSettings;
class TextureManager;

class FontRenderer : public IResourceManagerReloadListener {
public:
    FontRenderer(const GameSettings& gameSettings, ResourceLocation location, TextureManager& textureManager, bool unicode);

    void onResourceManagerReload(const IResourceManager& resourceManager) override;

    int drawStringWithShadow(std::string_view text, float x, float y, int color);
    int drawString(std::string_view text, int x, int y, int color);
    int drawString(std::string_view text, float x, float y, int color, bool dropShadow);
    int getStringWidth(std::string_view text) const;
    int getCharWidth(char16_t character) const;
    std::string trimStringToWidth(std::string_view text, int width) const;
    std::string trimStringToWidth(std::string_view text, int width, bool reverse) const;
    void drawSplitString(std::string_view str, int x, int y, int wrapWidth, int textColor);
    int splitStringWidth(std::string_view str, int maxLength) const;
    void setUnicodeFlag(bool unicodeFlagIn) { unicodeFlag = unicodeFlagIn; }
    bool getUnicodeFlag() const { return unicodeFlag; }
    void setBidiFlag(bool bidiFlagIn) { bidiFlag = bidiFlagIn; }
    bool getBidiFlag() const { return bidiFlag; }
    std::vector<std::string> listFormattedStringToWidth(std::string_view str, int wrapWidth) const;
    std::string wrapFormattedStringToWidth(std::string_view str, int wrapWidth) const;
    static std::string getFormatFromString(std::string_view text);
    int getColorCode(char16_t character) const;

    int FONT_HEIGHT = 9;
    Random fontRandom;

private:
    void readFontTexture(const IResourceManager& resourceManager);
    void readGlyphSizes(const IResourceManager& resourceManager);
    float renderChar(char16_t ch, bool italic);
    float renderDefaultChar(int ch, bool italic);
    ResourceLocation getUnicodePageLocation(int page);
    float renderUnicodeChar(char16_t ch, bool italic);
    void resetStyles();
    void renderStringAtPos(std::u16string_view text, bool shadow);
    static std::u16string bidiReorder(std::u16string_view text);
    int renderStringAligned(std::u16string_view text, int x, int y, int width, int color, bool dropShadow);
    int renderString(std::u16string_view text, float x, float y, int color, bool dropShadow);
    void renderSplitString(std::u16string_view str, int x, int y, int wrapWidth, bool addShadow);

    int getStringWidth16(std::u16string_view text) const;
    std::u16string trimStringToWidth16(std::u16string_view text, int width, bool reverse) const;
    std::u16string wrapFormattedStringToWidth16(std::u16string_view str, int wrapWidth) const;
    std::vector<std::u16string> listFormattedStringToWidth16(std::u16string_view str, int wrapWidth) const;
    int sizeStringToWidth(std::u16string_view str, int wrapWidth) const;
    static std::u16string getFormatFromString16(std::u16string_view text);
    // Java's unsigned shift of a sign-extended byte: glyph_sizes.bin entries >= 0x80 give huge start columns.
    int glyphStart(char16_t ch) const;
    int glyphEnd(char16_t ch) const { return glyphWidth[ch] & 15; }

    static std::array<std::optional<ResourceLocation>, 256> unicodePageLocations;

    std::array<int, 256> charWidth {};
    std::array<int8_t, 65536> glyphWidth {};
    std::array<int, 32> colorCode {};
    ResourceLocation locationFontTexture;
    TextureManager* renderEngine;
    float posX = 0.0f;
    float posY = 0.0f;
    bool unicodeFlag;
    bool bidiFlag = false;
    float red = 0.0f;
    float blue = 0.0f;  // Java's names: holds the green component
    float green = 0.0f; // Java's names: holds the blue component
    float alpha = 0.0f;
    int textColor = 0;
    bool randomStyle = false;
    bool boldStyle = false;
    bool italicStyle = false;
    bool underlineStyle = false;
    bool strikethroughStyle = false;
};

} // namespace mc
