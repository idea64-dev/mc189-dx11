#include "util/JsonUtils.h"

namespace mc::JsonUtils {

using gson::JsonElement;
using gson::JsonSyntaxException;
using Type = JsonElement::Type;

namespace {

const JsonElement* member(const JsonElement& object, const std::string& key)
{
    if (!object.isJsonObject())
        return nullptr;
    const auto it = object.object.find(key);
    return it == object.object.end() ? nullptr : it->second.get();
}

std::string escape(const std::string& text)
{
    std::string out = "\"";
    for (const char c : text) {
        if (c == '"' || c == '\\')
            out += '\\';
        out += c;
    }
    return out + "\"";
}

} // namespace

bool hasField(const JsonElement& object, const std::string& key)
{
    return member(object, key) != nullptr;
}

bool isJsonPrimitive(const JsonElement& object, const std::string& key)
{
    const JsonElement* e = member(object, key);
    return e && e->isJsonPrimitive();
}

bool isString(const JsonElement& object, const std::string& key)
{
    const JsonElement* e = member(object, key);
    return e && e->type == Type::STRING;
}

bool isBoolean(const JsonElement& object, const std::string& key)
{
    const JsonElement* e = member(object, key);
    return e && e->type == Type::BOOLEAN;
}

bool isJsonArray(const JsonElement& object, const std::string& key)
{
    const JsonElement* e = member(object, key);
    return e && e->type == Type::ARRAY;
}

std::string getAsString(const JsonElement& element)
{
    if (element.type == Type::BOOLEAN)
        return element.boolean ? "true" : "false";
    return element.text;
}

std::string toString(const JsonElement& element)
{
    switch (element.type) {
    case Type::JSON_NULL:
        return "null (json)";
    case Type::ARRAY:
        return "an array";
    case Type::OBJECT:
        return "an object";
    case Type::NUMBER:
        return "a number (" + element.text + ")";
    case Type::BOOLEAN:
        return std::string("a boolean (") + (element.boolean ? "true" : "false") + ")";
    case Type::STRING:
        return escape(element.text);
    }
    return "";
}

std::string asString(const JsonElement& element, const std::string& name)
{
    if (element.isJsonPrimitive())
        return getAsString(element);
    throw JsonSyntaxException("Expected " + name + " to be a string, was " + toString(element));
}

bool asBoolean(const JsonElement& element, const std::string& name)
{
    if (element.isJsonPrimitive())
        return element.getAsBoolean();
    throw JsonSyntaxException("Expected " + name + " to be a Boolean, was " + toString(element));
}

float asFloat(const JsonElement& element, const std::string& name)
{
    if (element.isNumber())
        return element.getAsFloat();
    throw JsonSyntaxException("Expected " + name + " to be a Float, was " + toString(element));
}

int asInt(const JsonElement& element, const std::string& name)
{
    if (element.isNumber())
        return element.getAsInt();
    throw JsonSyntaxException("Expected " + name + " to be a Int, was " + toString(element));
}

const JsonElement& asJsonObject(const JsonElement& element, const std::string& name)
{
    if (element.isJsonObject())
        return element;
    throw JsonSyntaxException("Expected " + name + " to be a JsonObject, was " + toString(element));
}

const JsonElement& asJsonArray(const JsonElement& element, const std::string& name)
{
    if (element.type == Type::ARRAY)
        return element;
    throw JsonSyntaxException("Expected " + name + " to be a JsonArray, was " + toString(element));
}

std::string getString(const JsonElement& object, const std::string& key)
{
    if (const JsonElement* e = member(object, key))
        return asString(*e, key);
    throw JsonSyntaxException("Missing " + key + ", expected to find a string");
}

std::string getString(const JsonElement& object, const std::string& key, const std::string& fallback)
{
    const JsonElement* e = member(object, key);
    return e ? asString(*e, key) : fallback;
}

bool getBoolean(const JsonElement& object, const std::string& key)
{
    if (const JsonElement* e = member(object, key))
        return asBoolean(*e, key);
    throw JsonSyntaxException("Missing " + key + ", expected to find a Boolean");
}

bool getBoolean(const JsonElement& object, const std::string& key, bool fallback)
{
    const JsonElement* e = member(object, key);
    return e ? asBoolean(*e, key) : fallback;
}

float getFloat(const JsonElement& object, const std::string& key)
{
    if (const JsonElement* e = member(object, key))
        return asFloat(*e, key);
    throw JsonSyntaxException("Missing " + key + ", expected to find a Float");
}

float getFloat(const JsonElement& object, const std::string& key, float fallback)
{
    const JsonElement* e = member(object, key);
    return e ? asFloat(*e, key) : fallback;
}

int getInt(const JsonElement& object, const std::string& key)
{
    if (const JsonElement* e = member(object, key))
        return asInt(*e, key);
    throw JsonSyntaxException("Missing " + key + ", expected to find a Int");
}

int getInt(const JsonElement& object, const std::string& key, int fallback)
{
    const JsonElement* e = member(object, key);
    return e ? asInt(*e, key) : fallback;
}

const JsonElement& getJsonObject(const JsonElement& object, const std::string& key)
{
    if (const JsonElement* e = member(object, key))
        return asJsonObject(*e, key);
    throw JsonSyntaxException("Missing " + key + ", expected to find a JsonObject");
}

const JsonElement& getJsonArray(const JsonElement& object, const std::string& key)
{
    if (const JsonElement* e = member(object, key))
        return asJsonArray(*e, key);
    throw JsonSyntaxException("Missing " + key + ", expected to find a JsonArray");
}

} // namespace mc::JsonUtils
