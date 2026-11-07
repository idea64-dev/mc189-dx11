// Java string semantics (Float.toString, String.format, Locale's rewrite) against JVM output, and options.txt.

#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"
#include "core/Java.h"
#include "core/JavaString.h"

#include "reference/JavaReference.inc"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace mc;

namespace {

std::string view(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

} // namespace

TEST_CASE("Float.toString and Double.toString match the JVM")
{
    for (const auto& entry : javaref::floatToStrings) {
        const float value = java::intBitsToFloat(entry.bits);
        CAPTURE(view(entry.text));
        CHECK(java::toString(value) == view(entry.text));
    }
    for (const auto& entry : javaref::doubleToStrings) {
        const double value = java::longBitsToDouble(entry.bits);
        CAPTURE(view(entry.text));
        CHECK(java::toString(value) == view(entry.text));
    }
}

TEST_CASE("Locale rewrite and String.format match the JVM")
{
    const std::string arguments[] = { "A", "B", "C" };
    for (const auto& entry : javaref::formatCases) {
        CAPTURE(view(entry.pattern));
        const std::string rewritten = Locale::rewriteFormatSpecifiers(view(entry.pattern));
        CHECK(rewritten == view(entry.rewritten));
        const auto formatted = java::format(rewritten, arguments);
        CHECK(formatted.has_value() == entry.ok);
        if (formatted && entry.ok)
            CHECK(*formatted == view(entry.result));
    }
}

TEST_CASE("Integer.parseInt and Float.parseFloat reject what Java rejects")
{
    CHECK(java::parseInt("42") == 42);
    CHECK(java::parseInt("+7") == 7);
    CHECK(java::parseInt("-2147483648") == INT32_MIN);
    CHECK_THROWS(java::parseInt(""));
    CHECK_THROWS(java::parseInt(" 1"));
    CHECK_THROWS(java::parseInt("2147483648"));
    CHECK_THROWS(java::parseInt("1.5"));
    CHECK(java::parseFloat(" 0.5 ") == 0.5f);
    CHECK(java::parseFloat("1e3f") == 1000.0f);
    CHECK_THROWS(java::parseFloat("abc"));
    CHECK_THROWS(java::parseFloat("inf"));
}

TEST_CASE("options.txt round-trips and tolerates bad lines")
{
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "mcdx11-options-test";
    std::filesystem::create_directories(dir);
    {
        std::ofstream file(dir / "options.txt", std::ios::binary | std::ios::trunc);
        file << "guiScale:3\r\n"
                "fov:0.5\r\n"
                "lang:ru_RU\r\n"
                "lastServer:example.com:25565\r\n"
                "resourcePacks:[\"Faithful.zip\"]\r\n"
                "maxFps:abc\r\n"          // bad: skipped
                "key_key.jump:\r\n"       // bad: no value
                "difficulty:-1\r\n"       // bad: negative index throws in Java
                "renderClouds:fast\r\n"
                "key_key.screenshot:61\r\n"
                "soundCategory_music:0.25\r\n"
                "modelPart_hat:false\r\n";
    }

    {
        GameSettings settings(nullptr, dir);
        CHECK(settings.guiScale == 3);
        CHECK(settings.fovSetting == 90.0f);
        CHECK(settings.language == "ru_RU");
        CHECK(settings.lastServer == "example.com:25565");
        REQUIRE(settings.resourcePacks.size() == 1);
        CHECK(settings.resourcePacks[0] == "Faithful.zip");
        CHECK(settings.limitFramerate == 120);
        CHECK(settings.keyBindJump.getKeyCode() == 57);
        CHECK(settings.difficulty == EnumDifficulty::NORMAL);
        CHECK(settings.clouds == 1);
        CHECK(settings.keyBindScreenshot.getKeyCode() == 61);
        CHECK(settings.getSoundLevel(SoundCategory::MUSIC) == 0.25f);
        CHECK_FALSE(settings.getModelParts().contains(EnumPlayerModelParts::HAT));
        settings.saveOptions();
    }

    const std::string text = [&] {
        std::ifstream file(dir / "options.txt", std::ios::binary);
        std::stringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }();
    CHECK(text.starts_with("invertYMouse:false\r\nmouseSensitivity:0.5\r\nfov:0.5\r\n"));
    CHECK(text.find("\r\nresourcePacks:[\"Faithful.zip\"]\r\n") != std::string::npos);
    CHECK(text.find("\r\nchatHeightUnfocused:0.44366196\r\n") != std::string::npos);
    CHECK(text.find("\r\nrenderClouds:fast\r\n") != std::string::npos);
    CHECK(text.find("\r\nkey_key.screenshot:61\r\n") != std::string::npos);
    CHECK(text.find("\r\nsoundCategory_music:0.25\r\n") != std::string::npos);
    CHECK(text.find("\r\nmodelPart_hat:false\r\n") != std::string::npos);

    GameSettings reloaded(nullptr, dir);
    CHECK(reloaded.guiScale == 3);
    CHECK(reloaded.keyBindScreenshot.getKeyCode() == 61);

    std::filesystem::remove_all(dir);
}

TEST_CASE("option labels follow the original formatting")
{
    GameSettings settings;
    CHECK(GameSettings::Options::FRAMERATE_LIMIT.normalizeValue(125.0f) == doctest::Approx((130.0f - 10.0f) / 250.0f));
    CHECK(GameSettings::Options::FOV.snapToStepClamp(70.4f) == 70.0f);
    CHECK(GameSettings::getKeyDisplayString(-100) == "key.mouseButton"); // no locale loaded: the key itself
    CHECK(GameSettings::getKeyDisplayString(57) == "SPACE");
    CHECK(GameSettings::getKeyDisplayString(256 + 'a') == "A");
}
