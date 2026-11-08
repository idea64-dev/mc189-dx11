#pragma once

// Original: net/minecraft/util/IChatComponent.java (+ Serializer), ChatComponentStyle.java, ChatComponentText.java,
// ChatComponentTranslation.java, ChatComponentScore.java, ChatComponentSelector.java, ChatStyle.java (+ Serializer),
// EnumChatFormatting.java (names and codes).
// Enough for formatted/unformatted text and JSON parsing (pack descriptions, translated GUI strings). Click and hover
// events, serialization and scoreboard lookups arrive with the chat (stage 8).

#include <nlohmann/json_fwd.hpp>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace mc {

// Java: com.google.gson.JsonParseException
class JsonParseException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// EnumChatFormatting, identified by its formatting code character ('0'-'9', 'a'-'f', 'k'-'o', 'r').
namespace EnumChatFormatting {
// "§" + code, e.g. RED = "§c"
std::string toString(char code);
// Lower-case enum name ("dark_red", "bold"...) as Gson's enum adapter reads it; nullopt when unknown.
std::optional<char> getValueByName(std::string_view name);
} // namespace EnumChatFormatting

class ChatStyle {
public:
    std::optional<char> getColor() const;
    bool getBold() const;
    bool getItalic() const;
    bool getStrikethrough() const;
    bool getUnderlined() const;
    bool getObfuscated() const;
    bool isEmpty() const;

    ChatStyle& setColor(std::optional<char> colorIn) { color = colorIn; return *this; }
    ChatStyle& setBold(std::optional<bool> value) { bold = value; return *this; }
    ChatStyle& setItalic(std::optional<bool> value) { italic = value; return *this; }
    ChatStyle& setStrikethrough(std::optional<bool> value) { strikethrough = value; return *this; }
    ChatStyle& setUnderlined(std::optional<bool> value) { underlined = value; return *this; }
    ChatStyle& setObfuscated(std::optional<bool> value) { obfuscated = value; return *this; }
    ChatStyle& setInsertion(std::optional<std::string> value) { insertion = std::move(value); return *this; }
    ChatStyle& setParentStyle(std::shared_ptr<ChatStyle> parent) { parentStyle = std::move(parent); return *this; }

    std::string getFormattingCode() const;
    std::shared_ptr<ChatStyle> createShallowCopy() const;

    // Java: ChatStyle.Serializer.deserialize; null for non-objects.
    static std::shared_ptr<ChatStyle> deserialize(const nlohmann::json& element);

private:
    std::shared_ptr<ChatStyle> parentStyle;
    std::optional<char> color;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<bool> underlined;
    std::optional<bool> strikethrough;
    std::optional<bool> obfuscated;
    std::optional<std::string> insertion;
    // Stand-ins for ClickEvent/HoverEvent until the chat exists: they only matter for isEmpty().
    bool hasClickEvent = false;
    bool hasHoverEvent = false;
};

class IChatComponent : public std::enable_shared_from_this<IChatComponent> {
public:
    virtual ~IChatComponent() = default;

    virtual IChatComponent& setChatStyle(std::shared_ptr<ChatStyle> style);
    std::shared_ptr<ChatStyle> getChatStyle();
    IChatComponent& appendText(const std::string& text);
    IChatComponent& appendSibling(std::shared_ptr<IChatComponent> component);
    const std::vector<std::shared_ptr<IChatComponent>>& getSiblings() const { return siblings; }

    virtual std::string getUnformattedTextForChat() = 0;
    std::string getUnformattedText();
    // Every component in iteration order: its formatting codes, its text and a reset code.
    std::string getFormattedText();

    // Java: IChatComponent.Serializer.deserialize; throws JsonParseException.
    static std::shared_ptr<IChatComponent> deserialize(const nlohmann::json& element);
    static std::shared_ptr<IChatComponent> jsonToComponent(std::string_view json);

    // Java's iterator: the component itself, then everything its siblings iterate over. ChatComponentTranslation
    // yields its translated parts instead of itself.
    virtual void collect(std::vector<IChatComponent*>& out);

protected:
    void collectSiblings(std::vector<IChatComponent*>& out);

    std::vector<std::shared_ptr<IChatComponent>> siblings;
    std::shared_ptr<ChatStyle> style;
};

class ChatComponentText : public IChatComponent {
public:
    explicit ChatComponentText(std::string msg)
        : text(std::move(msg))
    {
    }

    const std::string& getChatComponentText_TextValue() const { return text; }
    std::string getUnformattedTextForChat() override { return text; }

private:
    std::string text;
};

class ChatComponentTranslation : public IChatComponent {
public:
    using Argument = std::variant<std::string, std::shared_ptr<IChatComponent>>;

    ChatComponentTranslation(std::string translationKey, std::vector<Argument> args = {});

    IChatComponent& setChatStyle(std::shared_ptr<ChatStyle> style) override;
    std::string getUnformattedTextForChat() override;
    const std::string& getKey() const { return key; }
    void collect(std::vector<IChatComponent*>& out) override;

private:
    void ensureInitialized();
    // Throws std::runtime_error (ChatComponentTranslationFormatException) on a bad format.
    void initializeFromFormat(const std::string& format);
    std::shared_ptr<IChatComponent> getFormatArgumentAsComponent(int index);

    std::string key;
    std::vector<Argument> formatArgs;
    std::vector<std::shared_ptr<IChatComponent>> children;
};

class ChatComponentScore : public IChatComponent {
public:
    ChatComponentScore(std::string name, std::string objective)
        : name(std::move(name))
        , objective(std::move(objective))
    {
    }

    void setValue(std::string valueIn) { value = std::move(valueIn); }
    // Java looks the score up when a server with a loaded world runs (stage 9); otherwise the stored value.
    std::string getUnformattedTextForChat() override { return value; }

private:
    std::string name;
    std::string objective;
    std::string value;
};

class ChatComponentSelector : public IChatComponent {
public:
    explicit ChatComponentSelector(std::string selector)
        : selector(std::move(selector))
    {
    }

    std::string getUnformattedTextForChat() override { return selector; }

private:
    std::string selector;
};

} // namespace mc
