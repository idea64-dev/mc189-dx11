#include "util/ChatComponent.h"

#include "client/resources/Locale.h"
#include "core/JavaString.h"

#include <nlohmann/json.hpp>

#include <array>
#include <utility>

namespace mc {

namespace EnumChatFormatting {

namespace {

constexpr std::array<std::pair<std::string_view, char>, 22> kFormats { {
    { "black", '0' }, { "dark_blue", '1' }, { "dark_green", '2' }, { "dark_aqua", '3' }, { "dark_red", '4' },
    { "dark_purple", '5' }, { "gold", '6' }, { "gray", '7' }, { "dark_gray", '8' }, { "blue", '9' }, { "green", 'a' },
    { "aqua", 'b' }, { "red", 'c' }, { "light_purple", 'd' }, { "yellow", 'e' }, { "white", 'f' }, { "obfuscated", 'k' },
    { "bold", 'l' }, { "strikethrough", 'm' }, { "underline", 'n' }, { "italic", 'o' }, { "reset", 'r' },
} };

} // namespace

std::string toString(char code)
{
    return std::string("§") + code;
}

std::optional<char> getValueByName(std::string_view name)
{
    for (const auto& [formatName, code] : kFormats) {
        if (formatName == name)
            return code;
    }
    return std::nullopt;
}

} // namespace EnumChatFormatting

namespace {

// Gson's JsonElement.getAsString: primitives only.
std::string getAsString(const nlohmann::json& element)
{
    if (element.is_string())
        return element.get<std::string>();
    if (element.is_boolean())
        return element.get<bool>() ? "true" : "false";
    if (element.is_number_integer() || element.is_number_unsigned())
        return element.dump();
    if (element.is_number_float())
        return java::toString(element.get<double>());
    throw JsonParseException("Not a JSON primitive: " + element.dump());
}

// Gson's JsonPrimitive.getAsBoolean: Boolean.parseBoolean of anything that isn't a boolean.
bool getAsBoolean(const nlohmann::json& element)
{
    if (element.is_boolean())
        return element.get<bool>();
    std::string text = getAsString(element);
    for (char& c : text)
        c = static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    return text == "true";
}

const nlohmann::json* member(const nlohmann::json& object, const char* name)
{
    const auto it = object.find(name);
    return it == object.end() ? nullptr : &*it;
}

// The formatting code of a style with every property resolved through its parents. Java builds it from a
// deep copy of the style, which holds exactly those resolved values.
std::string resolvedFormattingCode(const ChatStyle& style)
{
    std::string code;
    if (const auto color = style.getColor())
        code += EnumChatFormatting::toString(*color);
    if (style.getBold())
        code += EnumChatFormatting::toString('l');
    if (style.getItalic())
        code += EnumChatFormatting::toString('o');
    if (style.getUnderlined())
        code += EnumChatFormatting::toString('n');
    if (style.getObfuscated())
        code += EnumChatFormatting::toString('k');
    if (style.getStrikethrough())
        code += EnumChatFormatting::toString('m');
    return code;
}

} // namespace

// --- ChatStyle ---

std::optional<char> ChatStyle::getColor() const
{
    return color ? color : (parentStyle ? parentStyle->getColor() : std::nullopt);
}

bool ChatStyle::getBold() const
{
    return bold ? *bold : (parentStyle && parentStyle->getBold());
}

bool ChatStyle::getItalic() const
{
    return italic ? *italic : (parentStyle && parentStyle->getItalic());
}

bool ChatStyle::getStrikethrough() const
{
    return strikethrough ? *strikethrough : (parentStyle && parentStyle->getStrikethrough());
}

bool ChatStyle::getUnderlined() const
{
    return underlined ? *underlined : (parentStyle && parentStyle->getUnderlined());
}

bool ChatStyle::getObfuscated() const
{
    return obfuscated ? *obfuscated : (parentStyle && parentStyle->getObfuscated());
}

bool ChatStyle::isEmpty() const
{
    return !bold && !italic && !strikethrough && !underlined && !obfuscated && !color && !hasClickEvent && !hasHoverEvent;
}

std::string ChatStyle::getFormattingCode() const
{
    if (isEmpty())
        return parentStyle ? parentStyle->getFormattingCode() : "";
    return resolvedFormattingCode(*this);
}

std::shared_ptr<ChatStyle> ChatStyle::createShallowCopy() const
{
    auto copy = std::make_shared<ChatStyle>();
    copy->bold = bold;
    copy->italic = italic;
    copy->strikethrough = strikethrough;
    copy->underlined = underlined;
    copy->obfuscated = obfuscated;
    copy->color = color;
    copy->hasClickEvent = hasClickEvent;
    copy->hasHoverEvent = hasHoverEvent;
    copy->parentStyle = parentStyle;
    copy->insertion = insertion;
    return copy;
}

std::shared_ptr<ChatStyle> ChatStyle::deserialize(const nlohmann::json& element)
{
    if (!element.is_object())
        return nullptr;

    auto chatstyle = std::make_shared<ChatStyle>();
    if (const auto* value = member(element, "bold"))
        chatstyle->bold = getAsBoolean(*value);
    if (const auto* value = member(element, "italic"))
        chatstyle->italic = getAsBoolean(*value);
    if (const auto* value = member(element, "underlined"))
        chatstyle->underlined = getAsBoolean(*value);
    if (const auto* value = member(element, "strikethrough"))
        chatstyle->strikethrough = getAsBoolean(*value);
    if (const auto* value = member(element, "obfuscated"))
        chatstyle->obfuscated = getAsBoolean(*value);
    if (const auto* value = member(element, "color")) {
        // Gson's enum adapter: a string (or number) name, null when unknown.
        if (value->is_string() || value->is_number())
            chatstyle->color = EnumChatFormatting::getValueByName(getAsString(*value));
        else if (!value->is_null())
            throw JsonParseException("Expected a string for color");
    }
    if (const auto* value = member(element, "insertion"))
        chatstyle->insertion = getAsString(*value);
    // Java: clickEvent / hoverEvent with an allowed action (stage 8). Their presence is kept for isEmpty().
    if (const auto* value = member(element, "clickEvent"); value != nullptr && value->is_object()) {
        const auto* action = member(*value, "action");
        const auto* text = member(*value, "value");
        if (action != nullptr && text != nullptr && action->is_string()) {
            const std::string name = action->get<std::string>();
            // ClickEvent.Action.shouldAllowInChat: open_file and twitch_user_info are refused.
            chatstyle->hasClickEvent = name == "open_url" || name == "run_command" || name == "suggest_command" || name == "change_page";
        }
    }
    if (const auto* value = member(element, "hoverEvent"); value != nullptr && value->is_object()) {
        const auto* action = member(*value, "action");
        const auto* text = member(*value, "value");
        if (action != nullptr && text != nullptr && action->is_string()) {
            const std::string name = action->get<std::string>();
            const bool allowed = name == "show_text" || name == "show_achievement" || name == "show_item" || name == "show_entity";
            chatstyle->hasHoverEvent = allowed && IChatComponent::deserialize(*text) != nullptr;
        }
    }
    return chatstyle;
}

// --- IChatComponent (ChatComponentStyle) ---

IChatComponent& IChatComponent::setChatStyle(std::shared_ptr<ChatStyle> styleIn)
{
    style = std::move(styleIn);
    for (const auto& sibling : siblings)
        sibling->getChatStyle()->setParentStyle(getChatStyle());
    return *this;
}

std::shared_ptr<ChatStyle> IChatComponent::getChatStyle()
{
    if (style == nullptr) {
        style = std::make_shared<ChatStyle>();
        for (const auto& sibling : siblings)
            sibling->getChatStyle()->setParentStyle(style);
    }
    return style;
}

IChatComponent& IChatComponent::appendSibling(std::shared_ptr<IChatComponent> component)
{
    component->getChatStyle()->setParentStyle(getChatStyle());
    siblings.push_back(std::move(component));
    return *this;
}

IChatComponent& IChatComponent::appendText(const std::string& text)
{
    return appendSibling(std::make_shared<ChatComponentText>(text));
}

void IChatComponent::collect(std::vector<IChatComponent*>& out)
{
    out.push_back(this);
    collectSiblings(out);
}

void IChatComponent::collectSiblings(std::vector<IChatComponent*>& out)
{
    for (const auto& sibling : siblings)
        sibling->collect(out);
}

std::string IChatComponent::getUnformattedText()
{
    std::vector<IChatComponent*> components;
    collect(components);
    std::string result;
    for (IChatComponent* component : components)
        result += component->getUnformattedTextForChat();
    return result;
}

std::string IChatComponent::getFormattedText()
{
    std::vector<IChatComponent*> components;
    collect(components);
    std::string result;
    for (IChatComponent* component : components) {
        result += resolvedFormattingCode(*component->getChatStyle());
        result += component->getUnformattedTextForChat();
        result += EnumChatFormatting::toString('r');
    }
    return result;
}

std::shared_ptr<IChatComponent> IChatComponent::jsonToComponent(std::string_view json)
{
    nlohmann::json element;
    try {
        element = nlohmann::json::parse(json, nullptr, true, true);
    } catch (const nlohmann::json::exception& e) {
        throw JsonParseException(e.what());
    }
    return deserialize(element);
}

std::shared_ptr<IChatComponent> IChatComponent::deserialize(const nlohmann::json& element)
{
    if (element.is_null())
        return nullptr; // Gson hands JSON null to no adapter
    if (element.is_primitive())
        return std::make_shared<ChatComponentText>(getAsString(element));

    if (element.is_array()) {
        std::shared_ptr<IChatComponent> component;
        for (const nlohmann::json& item : element) {
            auto next = deserialize(item);
            if (component == nullptr)
                component = std::move(next);
            else
                component->appendSibling(std::move(next));
        }
        return component;
    }

    if (!element.is_object())
        throw JsonParseException("Don't know how to turn " + element.dump() + " into a Component");

    std::shared_ptr<IChatComponent> component;
    if (const auto* text = member(element, "text")) {
        component = std::make_shared<ChatComponentText>(getAsString(*text));
    } else if (const auto* translate = member(element, "translate")) {
        const std::string s = getAsString(*translate);
        std::vector<ChatComponentTranslation::Argument> args;
        if (const auto* with = member(element, "with")) {
            if (!with->is_array())
                throw JsonParseException("Expected 'with' to be an array");
            for (const nlohmann::json& item : *with) {
                auto arg = deserialize(item);
                // Plain unstyled text arguments become strings.
                if (auto* textArg = dynamic_cast<ChatComponentText*>(arg.get()); textArg != nullptr && textArg->getChatStyle()->isEmpty() && textArg->getSiblings().empty())
                    args.emplace_back(textArg->getChatComponentText_TextValue());
                else
                    args.emplace_back(std::move(arg));
            }
        }
        component = std::make_shared<ChatComponentTranslation>(s, std::move(args));
    } else if (const auto* score = member(element, "score")) {
        if (!score->is_object() || !score->contains("name") || !score->contains("objective"))
            throw JsonParseException("A score component needs a least a name and an objective");
        auto scoreComponent = std::make_shared<ChatComponentScore>(getAsString(score->at("name")), getAsString(score->at("objective")));
        if (const auto* value = member(*score, "value"))
            scoreComponent->setValue(getAsString(*value));
        component = std::move(scoreComponent);
    } else if (const auto* selector = member(element, "selector")) {
        component = std::make_shared<ChatComponentSelector>(getAsString(*selector));
    } else {
        throw JsonParseException("Don't know how to turn " + element.dump() + " into a Component");
    }

    if (const auto* extra = member(element, "extra")) {
        if (!extra->is_array())
            throw JsonParseException("Expected 'extra' to be an array");
        if (extra->empty())
            throw JsonParseException("Unexpected empty array of components");
        for (const nlohmann::json& item : *extra)
            component->appendSibling(deserialize(item));
    }

    component->setChatStyle(ChatStyle::deserialize(element));
    return component;
}

// --- ChatComponentTranslation ---

ChatComponentTranslation::ChatComponentTranslation(std::string translationKey, std::vector<Argument> args)
    : key(std::move(translationKey))
    , formatArgs(std::move(args))
{
    for (const Argument& arg : formatArgs) {
        if (const auto* component = std::get_if<std::shared_ptr<IChatComponent>>(&arg); component != nullptr && *component)
            (*component)->getChatStyle()->setParentStyle(getChatStyle());
    }
}

IChatComponent& ChatComponentTranslation::setChatStyle(std::shared_ptr<ChatStyle> styleIn)
{
    IChatComponent::setChatStyle(std::move(styleIn));
    for (const Argument& arg : formatArgs) {
        if (const auto* component = std::get_if<std::shared_ptr<IChatComponent>>(&arg); component != nullptr && *component)
            (*component)->getChatStyle()->setParentStyle(getChatStyle());
    }
    return *this;
}

void ChatComponentTranslation::ensureInitialized()
{
    // Java caches the parts until the language changes; rebuilding them each time gives the same text.
    children.clear();
    try {
        initializeFromFormat(StatCollector::translateToLocal(key));
    } catch (const std::runtime_error&) {
        children.clear();
        // Java retries with the en_US table bundled in the jar (StatCollector.translateToFallback); without it
        // the key itself is the fallback, which is what that table returns for unknown keys.
        initializeFromFormat(key);
    }
}

void ChatComponentTranslation::initializeFromFormat(const std::string& format)
{
    // Pattern: %(?:(\d+)\$)?([A-Za-z%]|$)
    struct Match {
        size_t start;
        size_t end;
        std::optional<std::string> index;
        std::string conversion;
    };
    const auto isConversion = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '%'; };
    const auto find = [&](size_t from) -> std::optional<Match> {
        for (size_t p = format.find('%', from); p != std::string::npos; p = format.find('%', p + 1)) {
            size_t q = p + 1;
            while (q < format.size() && format[q] >= '0' && format[q] <= '9')
                ++q;
            if (q > p + 1 && q < format.size() && format[q] == '$') {
                const size_t c = q + 1;
                if (c == format.size())
                    return Match { p, c, format.substr(p + 1, q - p - 1), "" };
                if (isConversion(format[c]))
                    return Match { p, c + 1, format.substr(p + 1, q - p - 1), std::string(1, format[c]) };
            }
            if (p + 1 == format.size())
                return Match { p, p + 1, std::nullopt, "" };
            if (isConversion(format[p + 1]))
                return Match { p, p + 2, std::nullopt, std::string(1, format[p + 1]) };
        }
        return std::nullopt;
    };
    const auto literal = [&](std::string_view text) {
        // String.format(text) of the plain parts: rejects stray '%' like Java's IllegalFormatException.
        auto formatted = java::format(text, {});
        if (!formatted)
            throw std::runtime_error("Error parsing: " + key);
        auto part = std::make_shared<ChatComponentText>(std::move(*formatted));
        part->getChatStyle()->setParentStyle(getChatStyle());
        children.push_back(std::move(part));
    };

    int i = 0;
    size_t j = 0;
    while (const auto match = find(j)) {
        const size_t k = match->start;
        const size_t l = match->end;
        if (k > j)
            literal(std::string_view(format).substr(j, k - j));

        if (match->conversion == "%" && l - k == 2) {
            auto percent = std::make_shared<ChatComponentText>("%");
            percent->getChatStyle()->setParentStyle(getChatStyle());
            children.push_back(std::move(percent));
        } else {
            if (match->conversion != "s")
                throw std::runtime_error("Unsupported format: '" + format.substr(k, l - k) + "'");
            const int i1 = match->index ? java::parseInt(*match->index) - 1 : i++;
            if (i1 < static_cast<int>(formatArgs.size()))
                children.push_back(getFormatArgumentAsComponent(i1));
        }
        j = l;
    }

    if (j < format.size())
        literal(std::string_view(format).substr(j));
}

std::shared_ptr<IChatComponent> ChatComponentTranslation::getFormatArgumentAsComponent(int index)
{
    if (index < 0 || index >= static_cast<int>(formatArgs.size()))
        throw std::out_of_range("translation argument " + std::to_string(index)); // Java: ArrayIndexOutOfBounds
    const Argument& object = formatArgs[static_cast<size_t>(index)];
    if (const auto* component = std::get_if<std::shared_ptr<IChatComponent>>(&object))
        return *component;
    auto text = std::make_shared<ChatComponentText>(std::get<std::string>(object));
    text->getChatStyle()->setParentStyle(getChatStyle());
    return text;
}

std::string ChatComponentTranslation::getUnformattedTextForChat()
{
    ensureInitialized();
    std::string result;
    for (const auto& child : children)
        result += child->getUnformattedTextForChat();
    return result;
}

void ChatComponentTranslation::collect(std::vector<IChatComponent*>& out)
{
    ensureInitialized();
    for (const auto& child : children)
        child->collect(out);
    collectSiblings(out);
}

} // namespace mc
