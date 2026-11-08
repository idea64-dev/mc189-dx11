#include "core/GsonLenient.h"

#include "core/JavaString.h"
#include "core/StringUtil.h"

#include <cmath>
#include <limits>

namespace mc::gson {

namespace {

class Reader {
public:
    explicit Reader(std::string_view in)
        : s(in)
    {
    }

    std::shared_ptr<JsonElement> readValue()
    {
        const int c = nextNonWhitespace();
        if (c < 0)
            throw JsonSyntaxException("End of input");
        auto e = std::make_shared<JsonElement>();
        if (c == '{') {
            ++pos;
            e->type = JsonElement::Type::OBJECT;
            readObject(*e);
        } else if (c == '[') {
            ++pos;
            e->type = JsonElement::Type::ARRAY;
            readArray(*e);
        } else if (c == '"' || c == '\'') {
            ++pos;
            e->type = JsonElement::Type::STRING;
            e->text = readQuoted(static_cast<char>(c));
        } else {
            const std::string literal = readLiteral();
            if (literal.empty())
                throw JsonSyntaxException("Expected value at " + std::to_string(pos));
            if (keyword(literal, "true")) {
                e->type = JsonElement::Type::BOOLEAN;
                e->boolean = true;
            } else if (keyword(literal, "false")) {
                e->type = JsonElement::Type::BOOLEAN;
            } else if (keyword(literal, "null")) {
                e->type = JsonElement::Type::JSON_NULL;
            } else if (isJsonNumber(literal)) {
                e->type = JsonElement::Type::NUMBER;
                e->text = literal;
            } else {
                e->type = JsonElement::Type::STRING; // an unquoted string
                e->text = literal;
            }
        }
        return e;
    }

    bool atEnd() { return nextNonWhitespace() < 0; }

private:
    // JsonReader.peekKeyword: each character may be the keyword's lower- or upper-case form.
    static bool keyword(const std::string& literal, std::string_view word)
    {
        if (literal.size() != word.size())
            return false;
        for (size_t i = 0; i < word.size(); ++i) {
            if (literal[i] != word[i] && literal[i] != static_cast<char>(std::toupper(static_cast<unsigned char>(word[i]))))
                return false;
        }
        return true;
    }

    // JsonReader.peekNumber's grammar (no leading zeros before other digits).
    static bool isJsonNumber(const std::string& t)
    {
        size_t i = 0;
        if (i < t.size() && t[i] == '-')
            ++i;
        if (i >= t.size() || !std::isdigit(static_cast<unsigned char>(t[i])))
            return false;
        if (t[i] == '0' && i + 1 < t.size() && std::isdigit(static_cast<unsigned char>(t[i + 1])))
            return false;
        while (i < t.size() && std::isdigit(static_cast<unsigned char>(t[i])))
            ++i;
        if (i < t.size() && t[i] == '.') {
            ++i;
            if (i >= t.size() || !std::isdigit(static_cast<unsigned char>(t[i])))
                return false;
            while (i < t.size() && std::isdigit(static_cast<unsigned char>(t[i])))
                ++i;
        }
        if (i < t.size() && (t[i] == 'e' || t[i] == 'E')) {
            ++i;
            if (i < t.size() && (t[i] == '+' || t[i] == '-'))
                ++i;
            if (i >= t.size() || !std::isdigit(static_cast<unsigned char>(t[i])))
                return false;
            while (i < t.size() && std::isdigit(static_cast<unsigned char>(t[i])))
                ++i;
        }
        return i == t.size();
    }

    int nextNonWhitespace()
    {
        while (pos < s.size()) {
            const char c = s[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos;
            } else if (c == '/' && pos + 1 < s.size() && s[pos + 1] == '*') {
                const size_t end = s.find("*/", pos + 2);
                if (end == std::string_view::npos)
                    throw JsonSyntaxException("Unterminated comment");
                pos = end + 2;
            } else if (c == '/' && pos + 1 < s.size() && s[pos + 1] == '/') {
                skipToEndOfLine();
            } else if (c == '#') {
                skipToEndOfLine();
            } else {
                return static_cast<unsigned char>(c);
            }
        }
        return -1;
    }

    void skipToEndOfLine()
    {
        while (pos < s.size() && s[pos] != '\n' && s[pos] != '\r')
            ++pos;
    }

    // JsonReader.nextUnquotedValue: up to a structural character, whitespace or a comment start.
    std::string readLiteral()
    {
        const size_t start = pos;
        while (pos < s.size()) {
            const char c = s[pos];
            if (c == '/' || c == '\\' || c == ';' || c == '#' || c == '=' || c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ','
                || c == ' ' || c == '\t' || c == '\f' || c == '\r' || c == '\n')
                break;
            ++pos;
        }
        return std::string(s.substr(start, pos - start));
    }

    std::string readQuoted(char quote)
    {
        std::u16string out;
        while (true) {
            if (pos >= s.size())
                throw JsonSyntaxException("Unterminated string");
            const char c = s[pos++];
            if (c == quote)
                return fromUtf16(out);
            if (c != '\\') {
                // Copy one UTF-8 sequence.
                size_t len = 1;
                const auto lead = static_cast<unsigned char>(c);
                if (lead >= 0xF0)
                    len = 4;
                else if (lead >= 0xE0)
                    len = 3;
                else if (lead >= 0xC0)
                    len = 2;
                out += toUtf16(s.substr(pos - 1, len));
                pos += len - 1;
                continue;
            }
            if (pos >= s.size())
                throw JsonSyntaxException("Unterminated escape sequence");
            const char e = s[pos++];
            switch (e) {
            case 'u': {
                if (pos + 4 > s.size())
                    throw JsonSyntaxException("Unterminated escape sequence");
                char16_t value = 0;
                for (int i = 0; i < 4; ++i) {
                    const char h = s[pos++];
                    value = static_cast<char16_t>(value << 4);
                    if (h >= '0' && h <= '9')
                        value = static_cast<char16_t>(value + (h - '0'));
                    else if (h >= 'a' && h <= 'f')
                        value = static_cast<char16_t>(value + (h - 'a' + 10));
                    else if (h >= 'A' && h <= 'F')
                        value = static_cast<char16_t>(value + (h - 'A' + 10));
                    else
                        throw JsonSyntaxException("\\u escape");
                }
                out += value;
                break;
            }
            case 't': out += u'\t'; break;
            case 'b': out += u'\b'; break;
            case 'n': out += u'\n'; break;
            case 'r': out += u'\r'; break;
            case 'f': out += u'\f'; break;
            default: out += static_cast<char16_t>(static_cast<unsigned char>(e)); break; // \' \" \\ \/ and anything else
            }
        }
    }

    std::string readName()
    {
        const int c = nextNonWhitespace();
        if (c == '"' || c == '\'') {
            ++pos;
            return readQuoted(static_cast<char>(c));
        }
        const std::string literal = readLiteral();
        if (literal.empty())
            throw JsonSyntaxException("Expected name at " + std::to_string(pos));
        return literal;
    }

    void readObject(JsonElement& e)
    {
        if (nextNonWhitespace() == '}') {
            ++pos;
            return;
        }
        while (true) {
            const std::string name = readName();
            const int sep = nextNonWhitespace();
            if (sep == ':') {
                ++pos;
            } else if (sep == '=') {
                ++pos;
                if (pos < s.size() && s[pos] == '>')
                    ++pos;
            } else {
                throw JsonSyntaxException("Expected ':' at " + std::to_string(pos));
            }
            e.object[name] = readValue();
            const int next = nextNonWhitespace();
            if (next == '}') {
                ++pos;
                return;
            }
            if (next != ',' && next != ';')
                throw JsonSyntaxException("Unterminated object at " + std::to_string(pos));
            ++pos;
        }
    }

    void readArray(JsonElement& e)
    {
        if (nextNonWhitespace() == ']') {
            ++pos;
            return;
        }
        while (true) {
            const int c = nextNonWhitespace();
            if (c == ',' || c == ';') {
                e.array.push_back(std::make_shared<JsonElement>()); // lenient: an empty element is null
            } else {
                e.array.push_back(readValue());
            }
            const int next = nextNonWhitespace();
            if (next == ']') {
                ++pos;
                return;
            }
            if (next != ',' && next != ';')
                throw JsonSyntaxException("Unterminated array at " + std::to_string(pos));
            ++pos;
        }
    }

    std::string_view s;
    size_t pos = 0;
};

} // namespace

std::shared_ptr<JsonElement> parse(std::string_view json)
{
    Reader reader(json);
    if (reader.atEnd())
        return std::make_shared<JsonElement>();
    auto value = reader.readValue();
    if (!reader.atEnd())
        throw JsonSyntaxException("JSON document was not fully consumed.");
    return value;
}

float JsonElement::getAsFloat() const
{
    if (type == Type::BOOLEAN)
        throw std::invalid_argument("boolean"); // Java: NumberFormatException from parseFloat("true")
    return java::parseFloat(text);
}

int JsonElement::getAsInt() const
{
    if (type == Type::NUMBER) {
        // LazilyParsedNumber.intValue: Integer.parseInt, else (int) Long.parseLong, else new BigDecimal(text).intValue().
        try {
            return java::parseInt(text);
        } catch (const std::invalid_argument&) {
        }
        try {
            return static_cast<int32_t>(java::parseLong(text));
        } catch (const std::invalid_argument&) {
        }
        const double d = std::trunc(java::parseDouble(text));
        if (!std::isfinite(d) || std::fabs(d) >= 9.2e18)
            return 0;
        return static_cast<int32_t>(static_cast<int64_t>(d));
    }
    if (type == Type::BOOLEAN)
        throw std::invalid_argument("boolean");
    return java::parseInt(text); // a string: new BigInteger-free path, Integer.parseInt
}

bool JsonElement::getAsBoolean() const
{
    if (type == Type::BOOLEAN)
        return boolean;
    // Boolean.parseBoolean(getAsString())
    if (text.size() != 4)
        return false;
    for (size_t i = 0; i < 4; ++i) {
        if (std::tolower(static_cast<unsigned char>(text[i])) != "true"[i])
            return false;
    }
    return true;
}

} // namespace mc::gson
