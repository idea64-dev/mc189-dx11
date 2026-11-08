#pragma once

// Gson's JsonParser in lenient mode (Gson.fromJson): comments (// /* */ #), unquoted and single-quoted names and
// strings, '=' / '=>' after names, ';' between values. Numbers keep their text, as Gson's LazilyParsedNumber does,
// so getAsFloat / getAsInt convert exactly like Java.

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mc::gson {

class JsonSyntaxException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct JsonElement {
    enum class Type { JSON_NULL, BOOLEAN, NUMBER, STRING, ARRAY, OBJECT };

    Type type = Type::JSON_NULL;
    bool boolean = false;
    std::string text; // number literal or string value
    std::vector<std::shared_ptr<JsonElement>> array;
    std::map<std::string, std::shared_ptr<JsonElement>> object; // last duplicate wins, like JsonObject

    bool isJsonPrimitive() const { return type == Type::BOOLEAN || type == Type::NUMBER || type == Type::STRING; }
    bool isNumber() const { return type == Type::NUMBER; }
    bool isJsonObject() const { return type == Type::OBJECT; }
    bool has(const std::string& name) const { return object.contains(name); }
    const JsonElement& get(const std::string& name) const { return *object.at(name); }
    // JsonPrimitive.getAsFloat / getAsInt / getAsBoolean. Throw like Java where Java would (NumberFormatException
    // as std::invalid_argument).
    float getAsFloat() const;
    int getAsInt() const;
    bool getAsBoolean() const;
};

// Gson.fromJson(String, JsonElement.class): the whole input must be one value (an empty input is null).
// Throws JsonSyntaxException.
std::shared_ptr<JsonElement> parse(std::string_view json);

} // namespace mc::gson
