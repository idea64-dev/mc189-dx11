#include "core/JavaString.h"

#include "core/StringUtil.h"

#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace mc::java {

namespace {

template <class T>
std::string floatingToString(T value)
{
    if (std::isnan(value))
        return "NaN";
    if (std::isinf(value))
        return value > 0 ? "Infinity" : "-Infinity";
    if (value == 0)
        return std::signbit(value) ? "-0.0" : "0.0";

    // Shortest round-trip digits and the decimal exponent, from scientific form "d.ddde±x". Java's
    // FloatingDecimal keeps at least two significant digits for subnormals (Float.MIN_VALUE is "1.4E-45").
    std::array<char, 64> buffer {};
    const bool subnormal = std::abs(value) < std::numeric_limits<T>::min();
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::scientific);
    if (subnormal && std::string_view(buffer.data(), static_cast<size_t>(result.ptr - buffer.data())).find('.') == std::string_view::npos)
        result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::scientific, 1);
    const std::string_view text(buffer.data(), static_cast<size_t>(result.ptr - buffer.data()));

    const bool negative = text.front() == '-';
    const size_t ePos = text.find('e');
    std::string digits;
    for (char c : text.substr(negative ? 1 : 0, ePos - (negative ? 1 : 0))) {
        if (c != '.')
            digits += c;
    }
    int exponent = 0;
    std::from_chars(text.data() + ePos + (text[ePos + 1] == '+' ? 2 : 1), text.data() + text.size(), exponent);
    while (digits.size() > 1 && digits.back() == '0')
        digits.pop_back();

    std::string out = negative ? "-" : "";
    const T magnitude = std::abs(value);
    if (magnitude >= static_cast<T>(1e-3) && magnitude < static_cast<T>(1e7)) {
        if (exponent >= 0) {
            const size_t integerDigits = static_cast<size_t>(exponent) + 1;
            std::string integerPart = digits.substr(0, std::min(integerDigits, digits.size()));
            while (integerPart.size() < integerDigits)
                integerPart += '0';
            const std::string fraction = digits.size() > integerDigits ? digits.substr(integerDigits) : "0";
            out += integerPart + "." + fraction;
        } else {
            out += "0." + std::string(static_cast<size_t>(-exponent - 1), '0') + digits;
        }
    } else {
        out += digits.substr(0, 1) + "." + (digits.size() > 1 ? digits.substr(1) : "0") + "E" + std::to_string(exponent);
    }
    return out;
}

std::string_view trimJava(std::string_view text)
{
    // String.trim(): strips code points <= ' '.
    while (!text.empty() && static_cast<unsigned char>(text.front()) <= ' ')
        text.remove_prefix(1);
    while (!text.empty() && static_cast<unsigned char>(text.back()) <= ' ')
        text.remove_suffix(1);
    return text;
}

} // namespace

std::string toString(float value)
{
    return floatingToString(value);
}

std::string toString(double value)
{
    return floatingToString(value);
}

std::string toString(char16_t value)
{
    return toUtf8(std::wstring(1, static_cast<wchar_t>(value)));
}

int32_t parseInt(std::string_view text)
{
    std::string_view digits = text;
    if (!digits.empty() && digits.front() == '+')
        digits.remove_prefix(1);
    if (digits.empty() || digits.front() == '+' || (digits.front() == '-' && digits.size() == 1))
        throw std::invalid_argument("For input string: \"" + std::string(text) + "\"");

    int32_t value = 0;
    const auto result = std::from_chars(digits.data(), digits.data() + digits.size(), value, 10);
    if (result.ec != std::errc() || result.ptr != digits.data() + digits.size())
        throw std::invalid_argument("For input string: \"" + std::string(text) + "\"");
    return value;
}

double parseDouble(std::string_view text)
{
    std::string_view s = trimJava(text);
    bool negative = false;
    if (!s.empty() && (s.front() == '+' || s.front() == '-')) {
        negative = s.front() == '-';
        s.remove_prefix(1);
    }
    if (s == "Infinity")
        return negative ? -INFINITY : INFINITY;
    if (s == "NaN")
        return NAN;
    if (!s.empty() && (s.back() == 'f' || s.back() == 'F' || s.back() == 'd' || s.back() == 'D'))
        s.remove_suffix(1);
    if (s.empty() || !(std::isdigit(static_cast<unsigned char>(s.front())) || s.front() == '.'))
        throw std::invalid_argument("For input string: \"" + std::string(text) + "\"");

    double value = 0.0;
    const auto result = std::from_chars(s.data(), s.data() + s.size(), value, std::chars_format::general);
    if (result.ptr != s.data() + s.size() || (result.ec != std::errc() && result.ec != std::errc::result_out_of_range))
        throw std::invalid_argument("For input string: \"" + std::string(text) + "\"");
    if (result.ec == std::errc::result_out_of_range)
        value = std::abs(value) < 1.0 ? 0.0 : INFINITY;
    return negative ? -value : value;
}

float parseFloat(std::string_view text)
{
    // Float.parseFloat rounds the decimal directly to float; going through double can double-round in rare
    // halfway cases, so parse as float when possible.
    std::string_view s = trimJava(text);
    bool negative = false;
    if (!s.empty() && (s.front() == '+' || s.front() == '-')) {
        negative = s.front() == '-';
        s.remove_prefix(1);
    }
    if (s == "Infinity" || s == "NaN")
        return static_cast<float>(parseDouble(text));
    if (!s.empty() && (s.back() == 'f' || s.back() == 'F' || s.back() == 'd' || s.back() == 'D'))
        s.remove_suffix(1);
    if (s.empty() || !(std::isdigit(static_cast<unsigned char>(s.front())) || s.front() == '.'))
        throw std::invalid_argument("For input string: \"" + std::string(text) + "\"");

    float value = 0.0f;
    const auto result = std::from_chars(s.data(), s.data() + s.size(), value, std::chars_format::general);
    if (result.ptr != s.data() + s.size() || (result.ec != std::errc() && result.ec != std::errc::result_out_of_range))
        throw std::invalid_argument("For input string: \"" + std::string(text) + "\"");
    if (result.ec == std::errc::result_out_of_range)
        value = std::abs(value) < 1.0f ? 0.0f : INFINITY;
    return negative ? -value : value;
}

size_t utf16Length(std::string_view utf8)
{
    size_t units = 0;
    for (size_t i = 0; i < utf8.size();) {
        const auto lead = static_cast<unsigned char>(utf8[i]);
        if (lead < 0x80) {
            i += 1;
            units += 1;
        } else if ((lead >> 5) == 0x6) {
            i += 2;
            units += 1;
        } else if ((lead >> 4) == 0xE) {
            i += 3;
            units += 1;
        } else if ((lead >> 3) == 0x1E) {
            i += 4;
            units += 2;
        } else {
            i += 1;
            units += 1;
        }
    }
    return units;
}

std::optional<std::string> format(std::string_view pattern, std::span<const std::string> arguments)
{
    std::string out;
    size_t nextArgument = 0;

    for (size_t i = 0; i < pattern.size(); ++i) {
        const char c = pattern[i];
        if (c != '%') {
            out += c;
            continue;
        }

        // %[argument_index$][flags][width]conversion
        size_t j = i + 1;
        if (j >= pattern.size())
            return std::nullopt; // UnknownFormatConversionException

        int explicitIndex = -1;
        size_t k = j;
        while (k < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[k])))
            ++k;
        if (k > j && k < pattern.size() && pattern[k] == '$') {
            explicitIndex = 0;
            std::from_chars(pattern.data() + j, pattern.data() + k, explicitIndex);
            j = k + 1;
        }

        bool leftJustify = false;
        while (j < pattern.size() && (pattern[j] == '-' || pattern[j] == '#' || pattern[j] == '+' || pattern[j] == ' ' || pattern[j] == '0' || pattern[j] == ',' || pattern[j] == '(')) {
            if (pattern[j] == '-')
                leftJustify = true;
            ++j;
        }
        int width = 0;
        while (j < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[j])))
            width = width * 10 + (pattern[j++] - '0');
        int precision = -1;
        if (j < pattern.size() && pattern[j] == '.') {
            precision = 0;
            ++j;
            while (j < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[j])))
                precision = precision * 10 + (pattern[j++] - '0');
        }
        if (j >= pattern.size())
            return std::nullopt;

        const char conversion = pattern[j];
        i = j;
        std::string piece;
        if (conversion == '%') {
            piece = "%";
        } else if (conversion == 'n') {
            piece = "\r\n"; // line.separator on Windows
        } else if (conversion == 's' || conversion == 'S') {
            size_t index;
            if (explicitIndex > 0)
                index = static_cast<size_t>(explicitIndex - 1);
            else
                index = nextArgument++; // includes "%0$s", which Java 8 treats as an ordinary index
            if (index >= arguments.size())
                return std::nullopt; // MissingFormatArgumentException
            piece = arguments[index];
            if (precision >= 0 && java::utf16Length(piece) > static_cast<size_t>(precision))
                piece = piece.substr(0, static_cast<size_t>(precision)); // ASCII-only truncation is enough here
            if (conversion == 'S') {
                for (char& ch : piece)
                    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
        } else {
            // Translations only carry %s after Locale's rewrite; anything else needs typed arguments.
            return std::nullopt;
        }

        const size_t length = java::utf16Length(piece);
        if (width > 0 && length < static_cast<size_t>(width)) {
            const std::string padding(static_cast<size_t>(width) - length, ' ');
            piece = leftJustify ? piece + padding : padding + piece;
        }
        out += piece;
    }
    return out;
}

} // namespace mc::java
