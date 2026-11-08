#pragma once

// Original: net/minecraft/util/JsonUtils.java, over the Gson tree of core/GsonLenient.h. Failures throw
// gson::JsonSyntaxException with Java's messages.
// Java overloads each getter for a JsonElement (with a name for messages) and for a JsonObject member; the
// element forms are called asXxx here, since both take (element, string).

#include "core/GsonLenient.h"

#include <string>

namespace mc::JsonUtils {

bool isString(const gson::JsonElement& object, const std::string& key);
bool isBoolean(const gson::JsonElement& object, const std::string& key);
bool isJsonArray(const gson::JsonElement& object, const std::string& key);
bool isJsonPrimitive(const gson::JsonElement& object, const std::string& key);
bool hasField(const gson::JsonElement& object, const std::string& key);

// JsonPrimitive.getAsString: string values as they are, numbers as written, booleans as true / false.
std::string getAsString(const gson::JsonElement& element);

std::string asString(const gson::JsonElement& element, const std::string& name);
bool asBoolean(const gson::JsonElement& element, const std::string& name);
float asFloat(const gson::JsonElement& element, const std::string& name);
int asInt(const gson::JsonElement& element, const std::string& name);
const gson::JsonElement& asJsonObject(const gson::JsonElement& element, const std::string& name);
const gson::JsonElement& asJsonArray(const gson::JsonElement& element, const std::string& name);

std::string getString(const gson::JsonElement& object, const std::string& key);
std::string getString(const gson::JsonElement& object, const std::string& key, const std::string& fallback);
bool getBoolean(const gson::JsonElement& object, const std::string& key);
bool getBoolean(const gson::JsonElement& object, const std::string& key, bool fallback);
float getFloat(const gson::JsonElement& object, const std::string& key);
float getFloat(const gson::JsonElement& object, const std::string& key, float fallback);
int getInt(const gson::JsonElement& object, const std::string& key);
int getInt(const gson::JsonElement& object, const std::string& key, int fallback);
const gson::JsonElement& getJsonObject(const gson::JsonElement& object, const std::string& key);
const gson::JsonElement& getJsonArray(const gson::JsonElement& object, const std::string& key);

// Gson's JsonElement.toString, for error messages.
std::string toString(const gson::JsonElement& element);

} // namespace mc::JsonUtils
