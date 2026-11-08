#pragma once

// Java's string conversions and String.format, where the exact text matters (options.txt, translations,
// debug screens). Strings are UTF-8.

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace mc::java {

// Float.toString / Double.toString: shortest round-trip digits, "1.0" style, scientific ("1.0E-5") outside
// [1e-3, 1e7), "NaN", "Infinity".
std::string toString(float value);
std::string toString(double value);
inline std::string toString(bool value) { return value ? "true" : "false"; }
inline std::string toString(const std::string& value) { return value; }
inline std::string toString(std::string_view value) { return std::string(value); }
inline std::string toString(const char* value) { return value ? value : "null"; }
std::string toString(char16_t value);

template <class T>
    requires std::is_integral_v<T> && (!std::is_same_v<T, bool>) && (!std::is_same_v<T, char16_t>)
std::string toString(T value)
{
    return std::to_string(value);
}

// Integer.parseInt / Float.parseFloat: throw std::invalid_argument (NumberFormatException) on bad input.
int32_t parseInt(std::string_view text);
int64_t parseLong(std::string_view text);
float parseFloat(std::string_view text);
double parseDouble(std::string_view text);

// String.format with arguments already converted to text (String.valueOf). Supports %s %S %% %n with
// argument indices (%2$s) and width/'-' flags; returns nullopt where Java throws IllegalFormatException.
std::optional<std::string> format(std::string_view pattern, std::span<const std::string> arguments);

template <class... Args>
std::optional<std::string> formatArgs(std::string_view pattern, const Args&... args)
{
    const std::vector<std::string> converted { toString(args)... };
    return format(pattern, converted);
}

// String.length() and charAt() counting UTF-16 code units.
size_t utf16Length(std::string_view utf8);

// String.compareTo: lexicographic over UTF-16 code units (differs from UTF-8 byte order only around surrogates).
int compareTo(std::string_view a, std::string_view b);

// Comparator for maps ordered like Java's TreeMap<String, ...>.
struct StringLess {
    bool operator()(std::string_view a, std::string_view b) const { return compareTo(a, b) < 0; }
};

// Guava's Floats.tryParse: Float.parseFloat for strings matching Java's float literal syntax (no surrounding
// whitespace), nullopt otherwise.
std::optional<float> tryParseFloat(std::string_view text);

// String.trim(): strips characters <= ' ' from both ends (UTF-8 bytes of non-ASCII characters are all above).
std::string trim(std::string_view text);

} // namespace mc::java
