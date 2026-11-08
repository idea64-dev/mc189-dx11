#include "nbt/JsonToNBT.h"

#include "core/JavaString.h"
#include "core/StringUtil.h"

#include <vector>

namespace mc {

namespace {

using U16 = std::u16string;

// String.trim(): strips chars <= ' ' at both ends.
U16 trim(const U16& s)
{
    size_t start = 0;
    size_t end = s.size();
    while (start < end && s[start] <= u' ')
        ++start;
    while (end > start && s[end - 1] <= u' ')
        --end;
    return s.substr(start, end - start);
}

bool startsWith(const U16& s, char16_t c)
{
    return !s.empty() && s.front() == c;
}

// Character.isWhitespace for UTF-16 units (Unicode 6.2, as in Java 8).
bool isWhitespace(char16_t c)
{
    if ((c >= 0x09 && c <= 0x0D) || (c >= 0x1C && c <= 0x20))
        return true;
    switch (c) {
    case 0x1680: case 0x180E: case 0x2028: case 0x2029: case 0x205F: case 0x3000:
        return true;
    default:
        return c >= 0x2000 && c <= 0x200A && c != 0x2007;
    }
}

std::string utf8(const U16& s)
{
    return fromUtf16(s);
}

[[noreturn]] void fail(const std::string& message)
{
    throw NBTException(message);
}

bool isEscaped(const U16& s, size_t index)
{
    return index > 0 && s[index - 1] == u'\\' && !isEscaped(s, index - 1);
}

int countTopTags(const U16& s)
{
    int i = 0;
    bool flag = false;
    std::vector<char16_t> stack;
    for (size_t j = 0; j < s.size(); ++j) {
        const char16_t c0 = s[j];
        if (c0 == u'"') {
            if (isEscaped(s, j)) {
                if (!flag)
                    fail("Illegal use of \\\": " + utf8(s));
            } else {
                flag = !flag;
            }
        } else if (!flag) {
            if (c0 != u'{' && c0 != u'[') {
                if (c0 == u'}' && (stack.empty() || (stack.back() != u'{' ? (stack.pop_back(), true) : (stack.pop_back(), false))))
                    fail("Unbalanced curly brackets {}: " + utf8(s));
                if (c0 == u']' && (stack.empty() || (stack.back() != u'[' ? (stack.pop_back(), true) : (stack.pop_back(), false))))
                    fail("Unbalanced square brackets []: " + utf8(s));
            } else {
                if (stack.empty())
                    ++i;
                stack.push_back(c0);
            }
        }
    }
    if (flag)
        fail("Unbalanced quotation: " + utf8(s));
    if (!stack.empty())
        fail("Unbalanced brackets: " + utf8(s));
    if (i == 0 && !s.empty())
        i = 1;
    return i;
}

// func_150312_a: index of `c` outside quotes, -1 at the first bracket.
int indexOutsideQuotes(const U16& s, char16_t c)
{
    bool flag = true;
    for (size_t i = 0; i < s.size(); ++i) {
        const char16_t c0 = s[i];
        if (c0 == u'"') {
            if (!isEscaped(s, i))
                flag = !flag;
        } else if (flag) {
            if (c0 == c)
                return static_cast<int>(i);
            if (c0 == u'{' || c0 == u'[')
                return -1;
        }
    }
    return -1;
}

// func_179269_a
U16 splitAt(const U16& s, int separator)
{
    std::vector<char16_t> stack;
    size_t i = static_cast<size_t>(separator + 1);
    bool flag = false;
    bool flag1 = false;
    bool flag2 = false;
    size_t j = 0;
    for (; i < s.size(); ++i) {
        const char16_t c0 = s[i];
        if (c0 == u'"') {
            if (isEscaped(s, i)) {
                if (!flag)
                    fail("Illegal use of \\\": " + utf8(s));
            } else {
                flag = !flag;
                if (flag && !flag2)
                    flag1 = true;
                if (!flag)
                    j = i;
            }
        } else if (!flag) {
            if (c0 != u'{' && c0 != u'[') {
                if (c0 == u'}' && (stack.empty() || (stack.back() != u'{' ? (stack.pop_back(), true) : (stack.pop_back(), false))))
                    fail("Unbalanced curly brackets {}: " + utf8(s));
                if (c0 == u']' && (stack.empty() || (stack.back() != u'[' ? (stack.pop_back(), true) : (stack.pop_back(), false))))
                    fail("Unbalanced square brackets []: " + utf8(s));
                if (c0 == u',' && stack.empty())
                    return s.substr(0, i);
            } else {
                stack.push_back(c0);
            }
        }

        if (!isWhitespace(c0)) {
            if (!flag && flag1 && j != i)
                return s.substr(0, j + 1);
            flag2 = true;
        }
    }
    return s.substr(0, i);
}

// func_150314_a
U16 nextElement(const U16& s, bool named)
{
    int i = indexOutsideQuotes(s, u':');
    const int j = indexOutsideQuotes(s, u',');
    if (named) {
        if (i == -1)
            fail("Unable to locate name/value separator for string: " + utf8(s));
        if (j != -1 && j < i)
            fail("Name error at: " + utf8(s));
    } else if (i == -1 || i > j) {
        i = -1;
    }
    return splitAt(s, i);
}

// func_150313_b
U16 elementName(U16 s, bool inList)
{
    if (inList) {
        s = trim(s);
        if (startsWith(s, u'{') || startsWith(s, u'['))
            return U16();
    }
    const int i = indexOutsideQuotes(s, u':');
    if (i == -1) {
        if (inList)
            return U16();
        fail("Unable to locate name/value separator for string: " + utf8(s));
    }
    return trim(s.substr(0, static_cast<size_t>(i)));
}

// func_150311_c
U16 elementValue(U16 s, bool inList)
{
    if (inList) {
        s = trim(s);
        if (startsWith(s, u'{') || startsWith(s, u'['))
            return s;
    }
    const int i = indexOutsideQuotes(s, u':');
    if (i == -1) {
        if (inList)
            return s;
        fail("Unable to locate name/value separator for string: " + utf8(s));
    }
    return trim(s.substr(static_cast<size_t>(i) + 1));
}

bool isDigit(char16_t c)
{
    return c >= u'0' && c <= u'9';
}

// [-+]?[0-9]+
bool matchesInteger(const U16& s)
{
    size_t i = !s.empty() && (s[0] == u'-' || s[0] == u'+') ? 1 : 0;
    if (i == s.size())
        return false;
    for (; i < s.size(); ++i) {
        if (!isDigit(s[i]))
            return false;
    }
    return true;
}

// [-+]?[0-9]*\.?[0-9]+
bool matchesDecimal(const U16& s)
{
    size_t i = !s.empty() && (s[0] == u'-' || s[0] == u'+') ? 1 : 0;
    const U16 rest = s.substr(i);
    const size_t dot = rest.find(u'.');
    const auto allDigits = [](const U16& part) {
        for (const char16_t c : part) {
            if (!isDigit(c))
                return false;
        }
        return true;
    };
    if (dot == U16::npos)
        return !rest.empty() && allDigits(rest);
    const U16 before = rest.substr(0, dot);
    const U16 after = rest.substr(dot + 1);
    return allDigits(before) && !after.empty() && allDigits(after);
}

// The typed patterns: the body plus one suffix char from a class like [d|D] (the '|' included).
bool matchesSuffixed(const U16& s, bool decimal, char16_t lower, char16_t upper)
{
    if (s.empty())
        return false;
    const char16_t last = s.back();
    if (last != lower && last != upper && last != u'|')
        return false;
    const U16 body = s.substr(0, s.size() - 1);
    return decimal ? matchesDecimal(body) : matchesInteger(body);
}

// replaceAll("\\\\\"", "\"")
U16 unescapeQuotes(const U16& s)
{
    U16 out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == u'\\' && i + 1 < s.size() && s[i + 1] == u'"') {
            out += u'"';
            ++i;
        } else {
            out += s[i];
        }
    }
    return out;
}

bool equalsIgnoreCase(const U16& s, std::u16string_view word)
{
    if (s.size() != word.size())
        return false;
    for (size_t i = 0; i < s.size(); ++i) {
        const char16_t a = s[i] >= u'A' && s[i] <= u'Z' ? static_cast<char16_t>(s[i] + 32) : s[i];
        if (a != word[i])
            return false;
    }
    return true;
}

std::shared_ptr<NBTBase> parsePrimitive(U16 jsonValue)
{
    const std::string text = utf8(jsonValue);
    const std::string body = text.empty() ? text : text.substr(0, text.size() - 1);
    try {
        if (matchesSuffixed(jsonValue, true, u'd', u'D'))
            return std::make_shared<NBTTagDouble>(java::parseDouble(body));
        if (matchesSuffixed(jsonValue, true, u'f', u'F'))
            return std::make_shared<NBTTagFloat>(java::parseFloat(body));
        if (matchesSuffixed(jsonValue, false, u'b', u'B')) {
            const int32_t v = java::parseInt(body);
            if (v < -128 || v > 127)
                throw std::invalid_argument("Value out of range");
            return std::make_shared<NBTTagByte>(static_cast<int8_t>(v));
        }
        if (matchesSuffixed(jsonValue, false, u'l', u'L'))
            return std::make_shared<NBTTagLong>(java::parseLong(body));
        if (matchesSuffixed(jsonValue, false, u's', u'S')) {
            const int32_t v = java::parseInt(body);
            if (v < -32768 || v > 32767)
                throw std::invalid_argument("Value out of range");
            return std::make_shared<NBTTagShort>(static_cast<int16_t>(v));
        }
        if (matchesInteger(jsonValue))
            return std::make_shared<NBTTagInt>(java::parseInt(text));
        if (matchesDecimal(jsonValue))
            return std::make_shared<NBTTagDouble>(java::parseDouble(text));
        if (equalsIgnoreCase(jsonValue, u"true") || equalsIgnoreCase(jsonValue, u"false"))
            return std::make_shared<NBTTagByte>(static_cast<int8_t>(equalsIgnoreCase(jsonValue, u"true") ? 1 : 0));
    } catch (const std::invalid_argument&) {
        return std::make_shared<NBTTagString>(utf8(unescapeQuotes(jsonValue)));
    }

    if (startsWith(jsonValue, u'[') && !jsonValue.empty() && jsonValue.back() == u']') {
        const U16 s = jsonValue.substr(1, jsonValue.size() - 2);
        std::vector<int32_t> values;
        try {
            // Splitter.on(',').omitEmptyStrings()
            size_t start = 0;
            while (start <= s.size()) {
                const size_t comma = s.find(u',', start);
                const size_t end = comma == U16::npos ? s.size() : comma;
                if (end > start)
                    values.push_back(java::parseInt(utf8(trim(s.substr(start, end - start)))));
                if (comma == U16::npos)
                    break;
                start = comma + 1;
            }
            return std::make_shared<NBTTagIntArray>(std::move(values));
        } catch (const std::invalid_argument&) {
            return std::make_shared<NBTTagString>(utf8(jsonValue));
        }
    }

    if (jsonValue.size() >= 2 && startsWith(jsonValue, u'"') && jsonValue.back() == u'"')
        jsonValue = jsonValue.substr(1, jsonValue.size() - 2);
    else if (jsonValue == u"\"")
        jsonValue.clear(); // "\"".substring(1, 0) throws in Java; a lone quote can't get here past the balance check
    jsonValue = unescapeQuotes(jsonValue);
    U16 result;
    for (size_t i = 0; i < jsonValue.size(); ++i) {
        if (i < jsonValue.size() - 1 && jsonValue[i] == u'\\' && jsonValue[i + 1] == u'\\') {
            result += u'\\';
            ++i;
        } else {
            result += jsonValue[i];
        }
    }
    return std::make_shared<NBTTagString>(utf8(result));
}

// \[[-+\d|,\s]+\] -- a literal int array, which is parsed as a primitive.
bool isIntArrayLiteral(const U16& s)
{
    if (s.size() < 3 || s.front() != u'[' || s.back() != u']')
        return false;
    for (size_t i = 1; i + 1 < s.size(); ++i) {
        const char16_t c = s[i];
        const bool whitespace = c == u' ' || c == u'\t' || c == u'\n' || c == 0x0B || c == u'\f' || c == u'\r';
        if (!(isDigit(c) || c == u'-' || c == u'+' || c == u'|' || c == u',' || whitespace))
            return false;
    }
    return true;
}

std::shared_ptr<NBTBase> parseAny(U16 value);

std::shared_ptr<NBTBase> parseElement(const U16& element, bool inList)
{
    // Java builds the tree first and parses it afterwards; the name only matters to the caller.
    return parseAny(elementValue(element, inList));
}

std::shared_ptr<NBTBase> parseAny(U16 value)
{
    value = trim(value);
    if (startsWith(value, u'{')) {
        value = value.substr(1, value.size() - 2);
        auto compound = std::make_shared<NBTTagCompound>();
        std::vector<std::pair<std::string, std::shared_ptr<NBTBase>>> children;
        while (!value.empty()) {
            const U16 s1 = nextElement(value, true);
            if (!s1.empty()) {
                std::string name = utf8(elementName(s1, false)); // before the value, like Java
                children.emplace_back(std::move(name), parseElement(s1, false));
            }
            if (value.size() < s1.size() + 1)
                break;
            const char16_t c1 = value[s1.size()];
            if (c1 != u',' && c1 != u'{' && c1 != u'}' && c1 != u'[' && c1 != u']')
                fail("Unexpected token '" + utf8(U16(1, c1)) + "' at: " + utf8(value.substr(s1.size())));
            value = value.substr(s1.size() + 1);
        }
        for (auto& [name, tag] : children)
            compound->setTag(name, std::move(tag));
        return compound;
    }

    if (startsWith(value, u'[') && !isIntArrayLiteral(value)) {
        value = value.substr(1, value.size() - 2);
        auto list = std::make_shared<NBTTagList>();
        std::vector<std::shared_ptr<NBTBase>> children;
        while (!value.empty()) {
            const U16 s = nextElement(value, false);
            if (!s.empty())
                children.push_back(parseElement(s, true));
            if (value.size() < s.size() + 1)
                break;
            const char16_t c0 = value[s.size()];
            if (c0 != u',' && c0 != u'{' && c0 != u'}' && c0 != u'[' && c0 != u']')
                fail("Unexpected token '" + utf8(U16(1, c0)) + "' at: " + utf8(value.substr(s.size())));
            value = value.substr(s.size() + 1);
        }
        for (auto& tag : children)
            list->appendTag(std::move(tag));
        return list;
    }

    return parsePrimitive(value);
}

} // namespace

std::shared_ptr<NBTTagCompound> JsonToNBT::getTagFromJson(std::string_view jsonString)
{
    const U16 s = trim(toUtf16(jsonString));
    if (!startsWith(s, u'{'))
        fail("Invalid tag encountered, expected '{' as first char.");
    if (countTopTags(s) != 1)
        fail("Encountered multiple top tags, only one expected");
    return std::static_pointer_cast<NBTTagCompound>(parseAny(s));
}

} // namespace mc
