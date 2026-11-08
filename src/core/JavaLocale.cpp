#include "core/JavaLocale.h"

#include "core/JavaLocaleData.inc"
#include "core/JavaString.h"
#include "core/StringUtil.h"

#include <windows.h>

#include <cctype>
#include <charconv>
#include <cmath>
#include <ctime>

namespace mc::java {

namespace {

const DateFormatLocale& lookup(std::string_view locale)
{
    // ResourceBundle fallback: language_COUNTRY, then language, then the root.
    std::string name(locale);
    while (true) {
        for (const DateFormatLocale& entry : kDateFormatLocales) {
            if (name == entry.locale)
                return entry;
        }
        if (name.empty())
            return kDateFormatLocales[0];
        const size_t underscore = name.rfind('_');
        name = underscore == std::string::npos ? "" : name.substr(0, underscore);
    }
}

void appendNumber(std::u16string& out, int value, int minDigits, int zeroDigit)
{
    std::string digits = std::to_string(value);
    while (static_cast<int>(digits.size()) < minDigits)
        digits.insert(digits.begin(), '0');
    for (const char c : digits)
        out += static_cast<char16_t>(zeroDigit + (c - '0'));
}

} // namespace

std::string defaultFormatLocale()
{
    wchar_t name[LOCALE_NAME_MAX_LENGTH] {};
    if (GetUserDefaultLocaleName(name, LOCALE_NAME_MAX_LENGTH) == 0)
        return "en_US";
    // "ru-RU" -> "ru_RU"; a script subtag ("sr-Latn-RS") is dropped, Java 8 keeps the language and country.
    std::string tag = toUtf8(name);
    std::string language;
    std::string country;
    size_t start = 0;
    int index = 0;
    while (start <= tag.size()) {
        const size_t dash = tag.find('-', start);
        const std::string part = tag.substr(start, dash == std::string::npos ? std::string::npos : dash - start);
        if (index == 0)
            language = part;
        else if (part.size() == 2 || (part.size() == 3 && std::isdigit(static_cast<unsigned char>(part[0]))))
            country = part;
        ++index;
        if (dash == std::string::npos)
            break;
        start = dash + 1;
    }
    // Java's old ISO codes and its Windows mapping of Norwegian.
    if (language == "he")
        language = "iw";
    else if (language == "id")
        language = "in";
    else if (language == "yi")
        language = "ji";
    else if (language == "nb")
        language = "no";
    else if (language == "nn")
        return "no_NO_NY";
    return country.empty() ? language : language + "_" + country;
}

std::string formatDateTime(std::string_view locale, int year, int month, int day, int hour, int minute)
{
    const DateFormatLocale& data = lookup(locale);
    if (data.buddhist)
        year += 543;
    std::u16string out;
    const std::u16string_view pattern = data.pattern;
    for (size_t i = 0; i < pattern.size();) {
        const char16_t c = pattern[i];
        if (c == u'\'') {
            // Quoted literal; '' is a quote.
            size_t j = i + 1;
            if (j < pattern.size() && pattern[j] == u'\'') {
                out += u'\'';
                i = j + 1;
                continue;
            }
            while (j < pattern.size()) {
                if (pattern[j] == u'\'') {
                    if (j + 1 < pattern.size() && pattern[j + 1] == u'\'') {
                        out += u'\'';
                        j += 2;
                        continue;
                    }
                    break;
                }
                out += pattern[j++];
            }
            i = j + 1;
            continue;
        }
        if ((c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z')) {
            size_t count = 1;
            while (i + count < pattern.size() && pattern[i + count] == c)
                ++count;
            const int n = static_cast<int>(count);
            switch (c) {
            case u'y':
                if (n == 2)
                    appendNumber(out, year % 100, 2, data.zeroDigit);
                else
                    appendNumber(out, year, n, data.zeroDigit);
                break;
            case u'M': appendNumber(out, month, n, data.zeroDigit); break;
            case u'd': appendNumber(out, day, n, data.zeroDigit); break;
            case u'H': appendNumber(out, hour, n, data.zeroDigit); break;
            case u'h': appendNumber(out, hour % 12 == 0 ? 12 : hour % 12, n, data.zeroDigit); break;
            case u'm': appendNumber(out, minute, n, data.zeroDigit); break;
            case u'a': out += hour < 12 ? data.am : data.pm; break;
            default: break; // G (the Japanese imperial era) never occurs for a Windows locale
            }
            i += count;
            continue;
        }
        out += c;
        ++i;
    }
    return fromUtf16(out);
}

std::string formatDefaultDateTime(int64_t millis)
{
    const std::time_t seconds = static_cast<std::time_t>(millis >= 0 ? millis / 1000 : (millis - 999) / 1000);
    std::tm local {};
    localtime_s(&local, &seconds);
    return formatDateTime(defaultFormatLocale(), local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min);
}

namespace {

std::string localize(std::string_view ascii, const DateFormatLocale& data)
{
    std::u16string out;
    for (const char c : ascii) {
        if (c >= '0' && c <= '9')
            out += static_cast<char16_t>(data.zeroDigit + (c - '0'));
        else if (c == '.')
            out += data.decimalSeparator;
        else
            out += static_cast<char16_t>(c);
    }
    return fromUtf16(out);
}

const std::string& defaultLocaleCached()
{
    static const std::string locale = defaultFormatLocale();
    return locale;
}

} // namespace

std::string formatFixed(std::string_view locale, double value, int width, int precision)
{
    std::string text;
    if (std::isnan(value)) {
        text = "NaN";
    } else if (std::isinf(value)) {
        text = value > 0 ? "Infinity" : "-Infinity";
    } else {
        // The shortest digits that round-trip (Java's FloatingDecimal), as d.ddd...e<exp>.
        char buf[64];
        const auto result = std::to_chars(buf, buf + sizeof(buf), std::fabs(value), std::chars_format::scientific);
        const std::string sci(buf, result.ptr);
        const size_t e = sci.find('e');
        std::string digits;
        for (size_t i = 0; i < e; ++i) {
            if (sci[i] != '.')
                digits += sci[i];
        }
        int exponent = std::stoi(sci.substr(e + 1)); // value = 0.d1d2... * 10^(exponent + 1)
        int decExp = exponent + 1;
        if (digits == "0")
            decExp = 1;
        // Round half up to `precision` fraction digits (FormattedFloatingDecimal.applyPrecision).
        const int keep = decExp + precision;
        if (keep < 0) {
            digits = "0";
            decExp = 1;
        } else if (keep < static_cast<int>(digits.size())) {
            const bool up = digits[static_cast<size_t>(keep)] >= '5';
            digits.resize(static_cast<size_t>(keep));
            if (up) {
                int i = keep - 1;
                while (i >= 0 && digits[static_cast<size_t>(i)] == '9') {
                    digits[static_cast<size_t>(i)] = '0';
                    --i;
                }
                if (i < 0) {
                    digits.insert(digits.begin(), '1');
                    ++decExp;
                } else {
                    ++digits[static_cast<size_t>(i)];
                }
            }
            if (digits.empty()) {
                digits = "0";
                decExp = 1;
            }
        }
        std::string integerPart;
        std::string fraction;
        for (int i = 0; i < decExp; ++i)
            integerPart += i < static_cast<int>(digits.size()) ? digits[static_cast<size_t>(i)] : '0';
        if (integerPart.empty())
            integerPart = "0";
        for (int i = 0; i < precision; ++i) {
            const int index = decExp + i;
            fraction += index >= 0 && index < static_cast<int>(digits.size()) ? digits[static_cast<size_t>(index)] : '0';
        }
        // Java keeps the sign of a value that rounds to zero ("-0.000").
        text = (std::signbit(value) ? "-" : "") + integerPart + (precision > 0 ? "." + fraction : "");
    }
    std::string localized = localize(text, lookup(locale));
    const size_t length = java::utf16Length(localized);
    if (static_cast<int>(length) < width)
        localized.insert(0, static_cast<size_t>(width) - length, ' ');
    return localized;
}

std::string formatFixed(double value, int width, int precision)
{
    return formatFixed(defaultLocaleCached(), value, width, precision);
}

std::string formatInt(int64_t value)
{
    return localize(std::to_string(value), lookup(defaultLocaleCached()));
}

} // namespace mc::java
