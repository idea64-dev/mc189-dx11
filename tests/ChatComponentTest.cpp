// Chat components: JSON parsing and formatted text, with the expectations worked out from the Java sources
// (ChatComponentStyle.getFormattedText, ChatStyle inheritance, ChatComponentTranslation's format parsing). No
// locale is loaded here, so translation keys translate to themselves.

#include "util/ChatComponent.h"

#include <doctest/doctest.h>

using namespace mc;

TEST_CASE("Formatted text carries inherited styles and a reset per component")
{
    const auto component = IChatComponent::jsonToComponent(
        R"({"text":"Test ","color":"gold","extra":[{"text":"pack","bold":true},{"translate":"gui.done","color":"aqua"}]})");
    CHECK(component->getUnformattedText() == "Test packgui.done");
    CHECK(component->getFormattedText() == "§6Test §r§6§lpack§r§bgui.done§r");
}

TEST_CASE("Primitive and array components")
{
    CHECK(IChatComponent::jsonToComponent(R"("plain")")->getFormattedText() == "plain§r");
    CHECK(IChatComponent::jsonToComponent("42")->getUnformattedText() == "42");
    CHECK(IChatComponent::jsonToComponent("true")->getUnformattedText() == "true");
    // The first element is the parent of the others.
    const auto array = IChatComponent::jsonToComponent(R"([{"text":"a","italic":true},"b",{"text":"c","italic":false}])");
    CHECK(array->getFormattedText() == "§oa§r§ob§rc§r");
}

TEST_CASE("Style codes come in Java's order and unknown colours are dropped")
{
    const auto all = IChatComponent::jsonToComponent(
        R"({"text":"x","color":"red","bold":true,"italic":true,"underlined":true,"obfuscated":true,"strikethrough":true})");
    CHECK(all->getFormattedText() == "§c§l§o§n§k§mx§r");
    CHECK(IChatComponent::jsonToComponent(R"({"text":"x","color":"no_such_colour"})")->getFormattedText() == "x§r");
    // Gson's enum adapter accepts any formatting name as a colour.
    CHECK(IChatComponent::jsonToComponent(R"({"text":"x","color":"bold"})")->getFormattedText() == "§lx§r");
    // Booleans given as strings go through Boolean.parseBoolean.
    CHECK(IChatComponent::jsonToComponent(R"({"text":"x","bold":"TRUE"})")->getFormattedText() == "§lx§r");
}

TEST_CASE("Translation arguments, positions and percent signs")
{
    const auto translation = IChatComponent::jsonToComponent(R"({"translate":"%2$s and %s, 100%%","with":["one",{"text":"two","color":"green"}]})");
    CHECK(translation->getUnformattedText() == "two and one, 100%");
    CHECK(translation->getFormattedText() == "§atwo§r and §rone§r, 100§r%§r");
    // Arguments past the end are skipped.
    CHECK(IChatComponent::jsonToComponent(R"({"translate":"a%sb%sc","with":["1"]})")->getUnformattedText() == "a1bc");
}

TEST_CASE("Malformed components throw JsonParseException")
{
    CHECK_THROWS_AS(IChatComponent::jsonToComponent(R"({"color":"red"})"), JsonParseException);
    CHECK_THROWS_AS(IChatComponent::jsonToComponent(R"({"text":"x","extra":[]})"), JsonParseException);
    CHECK_THROWS_AS(IChatComponent::jsonToComponent(R"({"score":{"name":"x"}})"), JsonParseException);
    CHECK(IChatComponent::jsonToComponent(R"({"selector":"@p"})")->getUnformattedText() == "@p");
    CHECK(IChatComponent::jsonToComponent(R"({"score":{"name":"x","objective":"o","value":"7"}})")->getUnformattedText() == "7");
}
