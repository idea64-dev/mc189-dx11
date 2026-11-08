#pragma once

// The JVM's default format locale and what depends on it: new SimpleDateFormat().format(date) and the %f / %d
// conversions of String.format. Patterns, AM/PM strings, digits, decimal separators and the Thai Buddhist
// calendar come from Java 8's locale data (JavaLocaleData.inc).

#include <cstdint>
#include <string>
#include <string_view>

namespace mc::java {

// Java's name of the default format locale ("ru_RU", "en_US"...), as the JVM derives it from Windows (the user's
// regional format).
std::string defaultFormatLocale();

// new SimpleDateFormat().format(new Date(millis)) in the local time zone.
std::string formatDefaultDateTime(int64_t millis);
// The same for an explicit locale and broken-down local time (for tests).
std::string formatDateTime(std::string_view locale, int year, int month, int day, int hour, int minute);

// String.format("%<width>.<precision>f", value) in the default locale: Java rounds the shortest decimal digits of
// the value half up (not the exact binary value), then localizes digits and the decimal separator.
std::string formatFixed(double value, int width, int precision);
std::string formatFixed(std::string_view locale, double value, int width, int precision);
// String.format("%d", value) in the default locale.
std::string formatInt(int64_t value);

} // namespace mc::java
