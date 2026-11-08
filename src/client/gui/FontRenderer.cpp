#include "client/gui/FontRenderer.h"

#include "client/Minecraft.h"
#include "client/gui/FontCharacters.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/settings/GameSettings.h"
#include "core/Java.h"
#include "core/StringUtil.h"
#include "render/gl/GL.h"

#include <icu.h>

#include <format>
#include <stdexcept>

namespace mc {

std::array<std::optional<ResourceLocation>, 256> FontRenderer::unicodePageLocations;

namespace {

constexpr char16_t kSection = u'§';
constexpr std::u16string_view kFormatCodes = u"0123456789abcdefklmnor";

int indexOf(std::u16string_view text, char16_t c)
{
    const size_t i = text.find(c);
    return i == std::u16string_view::npos ? -1 : static_cast<int>(i);
}

// Character.toLowerCase for the format code character; only letters that can map into "0-9a-fk-or" matter.
char16_t formatCodeLower(char16_t c)
{
    if (c >= u'A' && c <= u'Z')
        return static_cast<char16_t>(c - u'A' + u'a');
    if (c == u'K') // KELVIN SIGN lowercases to 'k'
        return u'k';
    return c;
}

bool isFormatColor(char16_t c)
{
    return (c >= 48 && c <= 57) || (c >= 97 && c <= 102) || (c >= 65 && c <= 70);
}

bool isFormatSpecial(char16_t c)
{
    return (c >= 107 && c <= 111) || (c >= 75 && c <= 79) || c == 114 || c == 82;
}

// String.split("\n"): trailing empty strings are dropped, but "" yields [""].
std::vector<std::u16string> splitNewlines(std::u16string_view text)
{
    std::vector<std::u16string> parts;
    size_t start = 0;
    while (true) {
        const size_t newline = text.find(u'\n', start);
        parts.emplace_back(text.substr(start, newline == std::u16string_view::npos ? std::u16string_view::npos : newline - start));
        if (newline == std::u16string_view::npos)
            break;
        start = newline + 1;
    }
    while (parts.size() > 1 && parts.back().empty())
        parts.pop_back();
    if (parts.size() == 1 && parts[0].empty() && !text.empty())
        parts.clear(); // "\n\n".split("\n") is []
    return parts;
}

} // namespace

FontRenderer::FontRenderer(const GameSettings& gameSettings, ResourceLocation location, TextureManager& textureManager, bool unicode)
    : locationFontTexture(std::move(location))
    , renderEngine(&textureManager)
    , unicodeFlag(unicode)
{
    textureManager.bindTexture(locationFontTexture);

    for (int i = 0; i < 32; ++i) {
        const int j = ((i >> 3) & 1) * 85;
        int k = ((i >> 2) & 1) * 170 + j;
        int l = ((i >> 1) & 1) * 170 + j;
        int i1 = (i & 1) * 170 + j;
        if (i == 6)
            k += 85;
        if (gameSettings.anaglyph) {
            const int j1 = (k * 30 + l * 59 + i1 * 11) / 100;
            const int k1 = (k * 30 + l * 70) / 100;
            const int l1 = (k * 30 + i1 * 70) / 100;
            k = j1;
            l = k1;
            i1 = l1;
        }
        if (i >= 16) {
            k /= 4;
            l /= 4;
            i1 /= 4;
        }
        colorCode[static_cast<size_t>(i)] = ((k & 255) << 16) | ((l & 255) << 8) | (i1 & 255);
    }

    readGlyphSizes(Minecraft::getMinecraft()->getResourceManager());
}

void FontRenderer::onResourceManagerReload(const IResourceManager& resourceManager)
{
    readFontTexture(resourceManager);
}

void FontRenderer::readFontTexture(const IResourceManager& resourceManager)
{
    const BufferedImage image = TextureUtil::readBufferedImage(resourceManager.getResource(locationFontTexture)->getInputStream());
    const int i = image.width;
    const int j = image.height;
    const int k = j / 16;
    const int l = i / 16;
    const int i1 = 1;
    const float f = 8.0f / static_cast<float>(l);

    for (int j1 = 0; j1 < 256; ++j1) {
        const int k1 = j1 % 16;
        const int l1 = j1 / 16;
        if (j1 == 32)
            charWidth[static_cast<size_t>(j1)] = 3 + i1;

        int i2;
        for (i2 = l - 1; i2 >= 0; --i2) {
            const int j2 = k1 * l + i2;
            bool flag = true;
            for (int k2 = 0; k2 < k && flag; ++k2) {
                const int l2 = (l1 * l + k2) * i;
                if (((image.pixels[static_cast<size_t>(j2 + l2)] >> 24) & 255) != 0)
                    flag = false;
            }
            if (!flag)
                break;
        }
        ++i2;
        charWidth[static_cast<size_t>(j1)] = java::d2i(0.5 + static_cast<double>(static_cast<float>(i2) * f)) + i1;
    }
}

void FontRenderer::readGlyphSizes(const IResourceManager& resourceManager)
{
    const std::shared_ptr<IResource> resource = resourceManager.getResource(ResourceLocation("font/glyph_sizes.bin"));
    const std::vector<uint8_t>& data = resource->getInputStream();
    // InputStream.read(byte[]) fills what it can.
    const size_t count = std::min(data.size(), glyphWidth.size());
    for (size_t i = 0; i < count; ++i)
        glyphWidth[i] = static_cast<int8_t>(data[i]);
}

int FontRenderer::glyphStart(char16_t ch) const
{
    return java::ushr(static_cast<int32_t>(glyphWidth[ch]), 4);
}

float FontRenderer::renderChar(char16_t ch, bool italic)
{
    if (ch == 32)
        return 4.0f;
    const int i = indexOf(font::kAsciiChars, ch);
    return i != -1 && !unicodeFlag ? renderDefaultChar(i, italic) : renderUnicodeChar(ch, italic);
}

float FontRenderer::renderDefaultChar(int ch, bool italic)
{
    const int i = ch % 16 * 8;
    const int j = ch / 16 * 8;
    const int k = italic ? 1 : 0;
    renderEngine->bindTexture(locationFontTexture);
    const int l = charWidth[static_cast<size_t>(ch)];
    const float f = static_cast<float>(l) - 0.01f;
    GL::glBegin(GL::GL_TRIANGLE_STRIP);
    GL::glTexCoord2f(static_cast<float>(i) / 128.0f, static_cast<float>(j) / 128.0f);
    GL::glVertex3f(posX + static_cast<float>(k), posY, 0.0f);
    GL::glTexCoord2f(static_cast<float>(i) / 128.0f, (static_cast<float>(j) + 7.99f) / 128.0f);
    GL::glVertex3f(posX - static_cast<float>(k), posY + 7.99f, 0.0f);
    GL::glTexCoord2f((static_cast<float>(i) + f - 1.0f) / 128.0f, static_cast<float>(j) / 128.0f);
    GL::glVertex3f(posX + f - 1.0f + static_cast<float>(k), posY, 0.0f);
    GL::glTexCoord2f((static_cast<float>(i) + f - 1.0f) / 128.0f, (static_cast<float>(j) + 7.99f) / 128.0f);
    GL::glVertex3f(posX + f - 1.0f - static_cast<float>(k), posY + 7.99f, 0.0f);
    GL::glEnd();
    return static_cast<float>(l);
}

ResourceLocation FontRenderer::getUnicodePageLocation(int page)
{
    auto& location = unicodePageLocations[static_cast<size_t>(page)];
    if (!location)
        location = ResourceLocation(std::format("textures/font/unicode_page_{:02x}.png", page));
    return *location;
}

float FontRenderer::renderUnicodeChar(char16_t ch, bool italic)
{
    if (glyphWidth[ch] == 0)
        return 0.0f;

    const int i = ch / 256;
    renderEngine->bindTexture(getUnicodePageLocation(i));
    const int j = glyphStart(ch);
    const int k = glyphEnd(ch);
    const float f = static_cast<float>(j);
    const float f1 = static_cast<float>(k + 1);
    const float f2 = static_cast<float>(ch % 16 * 16) + f;
    const float f3 = static_cast<float>((ch & 255) / 16 * 16);
    const float f4 = f1 - f - 0.02f;
    const float f5 = italic ? 1.0f : 0.0f;
    GL::glBegin(GL::GL_TRIANGLE_STRIP);
    GL::glTexCoord2f(f2 / 256.0f, f3 / 256.0f);
    GL::glVertex3f(posX + f5, posY, 0.0f);
    GL::glTexCoord2f(f2 / 256.0f, (f3 + 15.98f) / 256.0f);
    GL::glVertex3f(posX - f5, posY + 7.99f, 0.0f);
    GL::glTexCoord2f((f2 + f4) / 256.0f, f3 / 256.0f);
    GL::glVertex3f(posX + f4 / 2.0f + f5, posY, 0.0f);
    GL::glTexCoord2f((f2 + f4) / 256.0f, (f3 + 15.98f) / 256.0f);
    GL::glVertex3f(posX + f4 / 2.0f - f5, posY + 7.99f, 0.0f);
    GL::glEnd();
    return (f1 - f) / 2.0f + 1.0f;
}

int FontRenderer::drawStringWithShadow(std::string_view text, float x, float y, int color)
{
    return drawString(text, x, y, color, true);
}

int FontRenderer::drawString(std::string_view text, int x, int y, int color)
{
    return drawString(text, static_cast<float>(x), static_cast<float>(y), color, false);
}

int FontRenderer::drawStringWithShadow16(std::u16string_view utf16, float x, float y, int color)
{
    DxStateManager::enableAlpha();
    resetStyles();
    const int i = renderString(utf16, x + 1.0f, y + 1.0f, color, true);
    return std::max(i, renderString(utf16, x, y, color, false));
}

int FontRenderer::drawString(std::string_view text, float x, float y, int color, bool dropShadow)
{
    DxStateManager::enableAlpha();
    resetStyles();
    const std::u16string utf16 = toUtf16(text);
    int i;
    if (dropShadow) {
        i = renderString(utf16, x + 1.0f, y + 1.0f, color, true);
        i = std::max(i, renderString(utf16, x, y, color, false));
    } else {
        i = renderString(utf16, x, y, color, false);
    }
    return i;
}

void FontRenderer::resetStyles()
{
    randomStyle = false;
    boldStyle = false;
    italicStyle = false;
    underlineStyle = false;
    strikethroughStyle = false;
}

void FontRenderer::renderStringAtPos(std::u16string_view text, bool shadow)
{
    for (size_t i = 0; i < text.size(); ++i) {
        char16_t c0 = text[i];

        if (c0 == kSection && i + 1 < text.size()) {
            int i1 = indexOf(kFormatCodes, formatCodeLower(text[i + 1]));

            if (i1 < 16) {
                randomStyle = false;
                boldStyle = false;
                strikethroughStyle = false;
                underlineStyle = false;
                italicStyle = false;
                if (i1 < 0 || i1 > 15)
                    i1 = 15;
                if (shadow)
                    i1 += 16;
                const int j1 = colorCode[static_cast<size_t>(i1)];
                textColor = j1;
                DxStateManager::color(static_cast<float>(j1 >> 16) / 255.0f, static_cast<float>((j1 >> 8) & 255) / 255.0f,
                    static_cast<float>(j1 & 255) / 255.0f, alpha);
            } else if (i1 == 16) {
                randomStyle = true;
            } else if (i1 == 17) {
                boldStyle = true;
            } else if (i1 == 18) {
                strikethroughStyle = true;
            } else if (i1 == 19) {
                underlineStyle = true;
            } else if (i1 == 20) {
                italicStyle = true;
            } else if (i1 == 21) {
                randomStyle = false;
                boldStyle = false;
                strikethroughStyle = false;
                underlineStyle = false;
                italicStyle = false;
                DxStateManager::color(red, blue, green, alpha);
            }
            ++i;
            continue;
        }

        int j = indexOf(font::kAsciiChars, c0);
        if (randomStyle && j != -1) {
            const int k = getCharWidth(c0);
            char16_t c1;
            while (true) {
                j = fontRandom.nextInt(static_cast<int32_t>(font::kAsciiChars.size()));
                c1 = font::kAsciiChars[static_cast<size_t>(j)];
                if (k == getCharWidth(c1))
                    break;
            }
            c0 = c1;
        }

        const float f1 = unicodeFlag ? 0.5f : 1.0f;
        const bool flag = (c0 == 0 || j == -1 || unicodeFlag) && shadow;
        if (flag) {
            posX -= f1;
            posY -= f1;
        }
        float f = renderChar(c0, italicStyle);
        if (flag) {
            posX += f1;
            posY += f1;
        }

        if (boldStyle) {
            posX += f1;
            if (flag) {
                posX -= f1;
                posY -= f1;
            }
            renderChar(c0, italicStyle);
            posX -= f1;
            if (flag) {
                posX += f1;
                posY += f1;
            }
            ++f;
        }

        if (strikethroughStyle) {
            Tessellator& tessellator = Tessellator::getInstance();
            WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
            DxStateManager::disableTexture2D();
            worldrenderer.begin(7, DefaultVertexFormats::POSITION);
            worldrenderer.pos(posX, posY + static_cast<float>(FONT_HEIGHT / 2), 0.0).endVertex();
            worldrenderer.pos(posX + f, posY + static_cast<float>(FONT_HEIGHT / 2), 0.0).endVertex();
            worldrenderer.pos(posX + f, posY + static_cast<float>(FONT_HEIGHT / 2) - 1.0f, 0.0).endVertex();
            worldrenderer.pos(posX, posY + static_cast<float>(FONT_HEIGHT / 2) - 1.0f, 0.0).endVertex();
            tessellator.draw();
            DxStateManager::enableTexture2D();
        }

        if (underlineStyle) {
            Tessellator& tessellator = Tessellator::getInstance();
            WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
            DxStateManager::disableTexture2D();
            worldrenderer.begin(7, DefaultVertexFormats::POSITION);
            const int l = underlineStyle ? -1 : 0;
            worldrenderer.pos(posX + static_cast<float>(l), posY + static_cast<float>(FONT_HEIGHT), 0.0).endVertex();
            worldrenderer.pos(posX + f, posY + static_cast<float>(FONT_HEIGHT), 0.0).endVertex();
            worldrenderer.pos(posX + f, posY + static_cast<float>(FONT_HEIGHT) - 1.0f, 0.0).endVertex();
            worldrenderer.pos(posX + static_cast<float>(l), posY + static_cast<float>(FONT_HEIGHT) - 1.0f, 0.0).endVertex();
            tessellator.draw();
            DxStateManager::enableTexture2D();
        }

        posX += static_cast<float>(java::f2i(f));
    }
}

std::u16string FontRenderer::bidiReorder(std::u16string_view text)
{
    // Java (ICU4J): new Bidi(new ArabicShaping(LETTERS_SHAPE).shape(text), DIRECTION_DEFAULT_RIGHT_TO_LEFT),
    // setReorderingMode(REORDER_DEFAULT), writeReordered(DO_MIRRORING). Windows ships the same ICU as icu.dll.
    // A shaping failure returns the text unchanged, like Java's ArabicShapingException handler.
    const auto* source = reinterpret_cast<const UChar*>(text.data());
    const auto length = static_cast<int32_t>(text.size());
    if (length == 0)
        return {};

    UErrorCode status = U_ZERO_ERROR;
    const int32_t shapedLength = u_shapeArabic(source, length, nullptr, 0, U_SHAPE_LETTERS_SHAPE, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
        return std::u16string(text);
    std::u16string shaped(static_cast<size_t>(shapedLength), u'\0');
    status = U_ZERO_ERROR;
    u_shapeArabic(source, length, reinterpret_cast<UChar*>(shaped.data()), shapedLength, U_SHAPE_LETTERS_SHAPE, &status);
    if (U_FAILURE(status))
        return std::u16string(text);

    UBiDi* bidi = ubidi_open();
    if (bidi == nullptr)
        return shaped;
    std::u16string result;
    status = U_ZERO_ERROR;
    ubidi_setPara(bidi, reinterpret_cast<const UChar*>(shaped.data()), shapedLength, UBIDI_DEFAULT_RTL, nullptr, &status);
    ubidi_setReorderingMode(bidi, UBIDI_REORDER_DEFAULT);
    if (U_SUCCESS(status)) {
        const int32_t outLength = ubidi_writeReordered(bidi, nullptr, 0, UBIDI_DO_MIRRORING, &status);
        if (status == U_BUFFER_OVERFLOW_ERROR || U_SUCCESS(status)) {
            result.assign(static_cast<size_t>(outLength), u'\0');
            status = U_ZERO_ERROR;
            ubidi_writeReordered(bidi, reinterpret_cast<UChar*>(result.data()), outLength, UBIDI_DO_MIRRORING, &status);
        }
    }
    ubidi_close(bidi);
    return U_SUCCESS(status) ? result : shaped;
}

int FontRenderer::renderStringAligned(std::u16string_view text, int x, int y, int width, int color, bool dropShadow)
{
    if (bidiFlag) {
        const int i = getStringWidth16(bidiReorder(text));
        x = x + width - i;
    }
    return renderString(text, static_cast<float>(x), static_cast<float>(y), color, dropShadow);
}

int FontRenderer::renderString(std::u16string_view textIn, float x, float y, int color, bool dropShadow)
{
    std::u16string reordered;
    std::u16string_view text = textIn;
    if (bidiFlag) {
        reordered = bidiReorder(textIn);
        text = reordered;
    }

    if ((color & -67108864) == 0)
        color |= -16777216;
    if (dropShadow)
        color = ((color & 16579836) >> 2) | (color & -16777216);

    red = static_cast<float>((color >> 16) & 255) / 255.0f;
    blue = static_cast<float>((color >> 8) & 255) / 255.0f;
    green = static_cast<float>(color & 255) / 255.0f;
    alpha = static_cast<float>((color >> 24) & 255) / 255.0f;
    DxStateManager::color(red, blue, green, alpha);
    posX = x;
    posY = y;
    renderStringAtPos(text, dropShadow);
    return java::f2i(posX);
}

int FontRenderer::getStringWidth(std::string_view text) const
{
    return getStringWidth16(toUtf16(text));
}

int FontRenderer::getStringWidth16(std::u16string_view text) const
{
    int i = 0;
    bool flag = false;
    for (size_t j = 0; j < text.size(); ++j) {
        char16_t c0 = text[j];
        int k = getCharWidth(c0);
        if (k < 0 && j < text.size() - 1) {
            ++j;
            c0 = text[j];
            if (c0 != 108 && c0 != 76) {
                if (c0 == 114 || c0 == 82)
                    flag = false;
            } else {
                flag = true;
            }
            k = 0;
        }
        i += k;
        if (flag && k > 0)
            ++i;
    }
    return i;
}

int FontRenderer::getCharWidth(char16_t character) const
{
    if (character == kSection)
        return -1;
    if (character == 32)
        return 4;

    const int i = indexOf(font::kAsciiChars, character);
    if (character > 0 && i != -1 && !unicodeFlag)
        return charWidth[static_cast<size_t>(i)];
    if (glyphWidth[character] != 0) {
        int j = glyphStart(character);
        int k = glyphEnd(character);
        if (k > 7) {
            k = 15;
            j = 0;
        }
        ++k;
        return java::sub(k, j) / 2 + 1;
    }
    return 0;
}

std::string FontRenderer::trimStringToWidth(std::string_view text, int width) const
{
    return trimStringToWidth(text, width, false);
}

std::string FontRenderer::trimStringToWidth(std::string_view text, int width, bool reverse) const
{
    return fromUtf16(trimStringToWidth16(toUtf16(text), width, reverse));
}

std::u16string FontRenderer::trimStringToWidth16(std::u16string_view text, int width, bool reverse) const
{
    std::u16string builder;
    int i = 0;
    const int start = reverse ? static_cast<int>(text.size()) - 1 : 0;
    const int step = reverse ? -1 : 1;
    bool flag = false;
    bool flag1 = false;

    for (int l = start; l >= 0 && l < static_cast<int>(text.size()) && i < width; l += step) {
        const char16_t c0 = text[static_cast<size_t>(l)];
        const int i1 = getCharWidth(c0);
        if (flag) {
            flag = false;
            if (c0 != 108 && c0 != 76) {
                if (c0 == 114 || c0 == 82)
                    flag1 = false;
            } else {
                flag1 = true;
            }
        } else if (i1 < 0) {
            flag = true;
        } else {
            i += i1;
            if (flag1)
                ++i;
        }
        if (i > width)
            break;
        if (reverse)
            builder.insert(builder.begin(), c0);
        else
            builder.push_back(c0);
    }
    return builder;
}

void FontRenderer::drawSplitString(std::string_view str, int x, int y, int wrapWidth, int textColorIn)
{
    resetStyles();
    textColor = textColorIn;
    std::u16string text = toUtf16(str);
    while (!text.empty() && text.back() == u'\n') // trimStringNewline
        text.pop_back();
    renderSplitString(text, x, y, wrapWidth, false);
}

void FontRenderer::renderSplitString(std::u16string_view str, int x, int y, int wrapWidth, bool addShadow)
{
    for (const std::u16string& line : listFormattedStringToWidth16(str, wrapWidth)) {
        renderStringAligned(line, x, y, wrapWidth, textColor, addShadow);
        y += FONT_HEIGHT;
    }
}

int FontRenderer::splitStringWidth(std::string_view str, int maxLength) const
{
    return FONT_HEIGHT * static_cast<int>(listFormattedStringToWidth16(toUtf16(str), maxLength).size());
}

std::vector<std::string> FontRenderer::listFormattedStringToWidth(std::string_view str, int wrapWidth) const
{
    std::vector<std::string> lines;
    for (const std::u16string& line : listFormattedStringToWidth16(toUtf16(str), wrapWidth))
        lines.push_back(fromUtf16(line));
    return lines;
}

std::vector<std::u16string> FontRenderer::listFormattedStringToWidth16(std::u16string_view str, int wrapWidth) const
{
    return splitNewlines(wrapFormattedStringToWidth16(str, wrapWidth));
}

std::string FontRenderer::wrapFormattedStringToWidth(std::string_view str, int wrapWidth) const
{
    return fromUtf16(wrapFormattedStringToWidth16(toUtf16(str), wrapWidth));
}

std::u16string FontRenderer::wrapFormattedStringToWidth16(std::u16string_view str, int wrapWidth) const
{
    // Java recurses once per line; iterate instead.
    std::u16string result;
    std::u16string remaining(str);
    while (true) {
        const int i = sizeStringToWidth(remaining, wrapWidth);
        if (static_cast<int>(remaining.size()) <= i) {
            result += remaining;
            return result;
        }
        const std::u16string s = remaining.substr(0, static_cast<size_t>(i));
        const char16_t c0 = remaining[static_cast<size_t>(i)];
        const bool flag = c0 == 32 || c0 == 10;
        std::u16string s1 = getFormatFromString16(s) + remaining.substr(static_cast<size_t>(i) + (flag ? 1 : 0));
        result += s + u"\n";
        remaining = std::move(s1);
    }
}

int FontRenderer::sizeStringToWidth(std::u16string_view str, int wrapWidth) const
{
    const int i = static_cast<int>(str.size());
    int j = 0;
    int k = 0;
    int l = -1;

    for (bool flag = false; k < i; ++k) {
        const char16_t c0 = str[static_cast<size_t>(k)];
        switch (c0) {
        case u'\n':
            --k;
            break;
        case u' ':
            l = k;
            [[fallthrough]];
        default:
            j += getCharWidth(c0);
            if (flag)
                ++j;
            break;
        case kSection:
            if (k < i - 1) {
                ++k;
                const char16_t c1 = str[static_cast<size_t>(k)];
                if (c1 != 108 && c1 != 76) {
                    if (c1 == 114 || c1 == 82 || isFormatColor(c1))
                        flag = false;
                } else {
                    flag = true;
                }
            }
            break;
        }

        if (c0 == 10) {
            ++k;
            l = k;
            break;
        }
        if (j > wrapWidth)
            break;
    }
    return k != i && l != -1 && l < k ? l : k;
}

std::string FontRenderer::getFormatFromString(std::string_view text)
{
    return fromUtf16(getFormatFromString16(toUtf16(text)));
}

std::u16string FontRenderer::getFormatFromString16(std::u16string_view text)
{
    std::u16string s;
    const size_t j = text.size();
    for (size_t i = text.find(kSection); i != std::u16string_view::npos; i = text.find(kSection, i + 1)) {
        if (i < j - 1) {
            const char16_t c0 = text[i + 1];
            if (isFormatColor(c0))
                s = std::u16string { kSection, c0 };
            else if (isFormatSpecial(c0))
                s += std::u16string { kSection, c0 };
        }
    }
    return s;
}

int FontRenderer::getColorCode(char16_t character) const
{
    const int i = indexOf(u"0123456789abcdef", character);
    if (i < 0)
        throw std::out_of_range("color code"); // Java: ArrayIndexOutOfBoundsException
    return colorCode[static_cast<size_t>(i)];
}

} // namespace mc
